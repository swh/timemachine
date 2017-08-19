/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: threads.c,v 1.5 2005/01/22 17:17:39 swh Exp $
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sndfile.h>
#include <jack/jack.h>
#include <gtk/gtk.h>

#include "config.h"
#include "support.h"
#include "main.h"
#include "threads.h"
#include "meters.h"

#define BUF_SIZE 4096

float *pre_buffer[MAX_PORTS];
float *disk_buffer[MAX_PORTS];

static int recording = 0;	    /* recording yes/no */
static int user_recording = 0;	    /* recording initiated by user yes/no */
static int quiting = 0;		    /* quit pending yes/no */
volatile int recording_done = 0;    /* recording completed after quit */
volatile int need_ui_sync = 0;      /* need to update record button */

static unsigned int pre_time = 0;
static unsigned int pre_size = 0;
static unsigned int pre_pos = 0;
static unsigned int disk_read_pos = 0;
static unsigned int disk_write_pos = 0;

/* Peak data for meters */
static volatile float peak[MAX_PORTS];

static unsigned int silent_count = 0;

int process(jack_nframes_t nframes, void *arg)
{
    unsigned int i, port, pos = 0;
    const unsigned int rec = recording;
    const jack_nframes_t sample_rate = jack_get_sample_rate(client);

    for (port = 0; port < num_ports; port++) {
	jack_default_audio_sample_t *in;

	/* just incase the port isn't registered yet */
	if (ports[port] == NULL) {
	    break;
	}

	in = (jack_default_audio_sample_t *)
	    jack_port_get_buffer(ports[port], nframes);

	if (!in) {
	    fprintf(stderr, "bad buffer!\n");
	    break;
	}

	if (auto_record) {
	    if (rec && !user_recording) {
		for (i = 0; i < nframes; i++) {
		    if (fabsf(in[i]) <= auto_end_threshold) {
			silent_count++;
		    } else {
			silent_count = 0;
		    }
		}
		if (silent_count > (auto_end_time * sample_rate * num_ports)) {
		    recording = 0;
		}
	    } else {
		for (i = 0; i < nframes; i++) {
		    if (fabsf(in[i]) > auto_begin_threshold) {
			recording = 1;
			silent_count = 0;
			break;
		    }
		}
	    }
	}

	for (i = 0; i < nframes; i++) {
	    if (fabsf(in[i]) > peak[port]) {
		peak[port] = fabsf(in[i]);
	    }
	}

	if (rec) {
	    pos = disk_write_pos;
	    for (i = 0; i < nframes; i++) {
		disk_buffer[port][pos] = in[i];
		pos = (pos + 1) & (DISK_SIZE - 1);
	    }
	} else {
	    pos = pre_pos;
	    for (i = 0; i < nframes; i++) {
		pre_buffer[port][pos++] = in[i];
		if (pos > pre_size) {
		    pos = 0;
		}
	    }
	}
    }
    if (rec) {
	disk_write_pos = (disk_write_pos + nframes) & (DISK_SIZE - 1);
    } else {
	pre_pos = pos;
    }

    return 0;
}

float clamp_sample(float sample)
{
	// some audio formats (FLAC) will generate faulty files if data out of range, so clamp the sample
	if(sample < -0.999)
		sample = -0.999;
	else if(sample > 0.999)
		sample = 0.999;
	return sample;
}

int writer_thread(void *d)
{
    unsigned int i, j, k, opos;
    char *filename;
    SNDFILE *out;
    SF_INFO info;
    float buf[BUF_SIZE * MAX_PORTS];
    time_t t;
    struct tm *parts;

  again:
    while (!recording && !quiting) {
	usleep(100);

    }
    if (quiting) {
	recording_done = 1;

	return 0;
    }

    /* Find the ISO 8601 date string for the time of the start of the
     * buffer */
    time(&t);
    t -= pre_time;
    parts = localtime(&t);
    if (safe_filename) {
	filename = g_strdup_printf("%s%04d-%02d-%02dT%02d-%02d-%02d.%s",
	     prefix,
	     parts->tm_year + 1900, parts->tm_mon + 1, parts->tm_mday,
	     parts->tm_hour, parts->tm_min, parts->tm_sec, format_name);
    } else {
	filename = g_strdup_printf("%s%04d-%02d-%02dT%02d:%02d:%02d.%s",
	     prefix,
	     parts->tm_year + 1900, parts->tm_mon + 1, parts->tm_mday,
	     parts->tm_hour, parts->tm_min, parts->tm_sec, format_name);
    }

    /* Open the output file */
    info.samplerate = jack_get_sample_rate(client);
    info.channels = num_ports;

    info.format = format_sf;
    if (!sf_format_check(&info)) {
	fprintf(stderr, PACKAGE ": output file format error\n");
    }

    out = sf_open(filename, SFM_WRITE, &info);
    if (!out) {
	perror("cannot open file for writing");
	goto again;
    }

    printf("opened '%s'\n", filename);
    printf("writing buffer...\n");
    free(filename);

    /* Dump the pre buffer to disk as fast was we can */
    i = 0;
    while (i < pre_size) {
	for (j = 0; j < BUF_SIZE && i < pre_size; i++, j++) {
	    for (k = 0; k < num_ports; k++) {
		buf[j * num_ports + k] =
		    clamp_sample( pre_buffer[k][(i + pre_pos) % pre_size] );
	    }
	}
	sf_writef_float(out, buf, j);
    }

    /* Clear the pre buffer so we dont get stale data in it */
    for (i = 0; i < num_ports; i++) {
	memset(pre_buffer[i], 0, pre_size * sizeof(float));
    }

    /* This tells the UI that were ready to go again, it will reset it */
    need_ui_sync = 1;

    if (recording) printf("writing realtime data...\n");

    /* Start writing the RT ringbuffer to disk */
    opos = 0;
    while (recording) {
	for (i = disk_read_pos; i != disk_write_pos && opos < BUF_SIZE;
	     i = (i + 1) & (DISK_SIZE - 1), opos++) {
	    for (j = 0; j < num_ports; j++) {
			buf[opos * num_ports + j] = clamp_sample(disk_buffer[j][i]);
	    }
	}
	sf_writef_float(out, buf, opos);
	disk_read_pos = i;
	opos = 0;
	usleep(10);
    }
    sf_close(out);

    need_ui_sync = 1;

    printf("done writing...\n");

    /* Just make sure everythings reset */
    disk_read_pos = disk_write_pos;

    /* Ugh, I'm sorry */
    if (!quiting) goto again;

    recording_done = 1;

    return 0;
}

void process_init(unsigned int time)
{
    unsigned int port;

    if (time < 1) {
	fprintf(stderr, "timemachine: buffer time must be 1 second or "
			"greater\n");
	exit(1);
    }
    if (time > MAX_TIME) {
	fprintf(stderr, "timemachine: buffer time must be %d seconds or "
			"less\nthis is for your own good, it really will not"
			"work well with buffers that size\n", MAX_TIME);
	exit(1);
    }

    pre_size = time * jack_get_sample_rate(client);
    pre_time = time;

    for (port = 0; port < num_ports; port++) {
	pre_buffer[port] = calloc(pre_size, sizeof(float));
	disk_buffer[port] = calloc(DISK_SIZE, sizeof(float));
    }
    /* just make sure that if we try these ports we will SEGV */
    for (; port < MAX_PORTS; port++) {
	pre_buffer[port] = NULL;
	disk_buffer[port] = NULL;
    }
}

void recording_start()
{
    recording = 1;
    user_recording = 1;
}

void recording_stop()
{
    recording = 0;
    user_recording = 0;
}

void recording_quit()
{
    quiting = 1;
    recording_stop();
}

gboolean meter_tick(gpointer user_data)
{
    float data[MAX_PORTS];
    unsigned int i;

    if (need_ui_sync) {
	GtkWidget *img = lookup_widget(main_window, "toggle_image");

	if (recording) {
	    gtk_image_set_from_pixbuf(GTK_IMAGE(img), img_on);
	    gtk_window_set_icon(GTK_WINDOW(main_window), icon_on);
	} else {
	    gtk_image_set_from_pixbuf(GTK_IMAGE(img), img_off);
	    gtk_window_set_icon(GTK_WINDOW(main_window), icon_off);
	}
	gtk_widget_set_sensitive(img, TRUE);

	need_ui_sync = 0;
    }

    for (i=0; i<MAX_PORTS; i++) {
	data[i] = peak[i];
	peak[i] = peak[i] - 0.1f < 0.0f ? 0.0f : peak[i] - 0.1f;
    }
    update_meters(data);

    return TRUE;
}

/* vi:set ts=8 sts=4 sw=4: */
