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
 *  $Id: main.c,v 1.7 2005/01/22 17:17:39 swh Exp $
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <jack/jack.h>
#include <getopt.h>
#include <sndfile.h>
#include <gtk/gtk.h>

#ifdef HAVE_LASH
#include <lash/lash.h>

lash_client_t *lash_client;
#endif

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef HAVE_LIBLO
#include <lo/lo.h>
#endif

#include "threads.h"
#include "interface.h"
#include "meters.h"
#include "support.h"
#include "main.h"

#define DEFAULT_BUF_LENGTH 10 /* in seconds */

#define DEBUG(lvl, txt...) \
    if (verbosity >= lvl) fprintf(stderr, PACKAGE ": " txt)

const static int verbosity = 0;

gboolean idle_cb(gpointer data);
void cleanup();

GtkWidget *main_window;

int num_ports = DEFAULT_NUM_PORTS;
unsigned int buf_length = DEFAULT_BUF_LENGTH; /* in seconds */

char *client_name = DEFAULT_CLIENT_NAME;
char *prefix = DEFAULT_PREFIX;
char *format_name = DEFAULT_FORMAT;
int format_sf = 0;
int safe_filename = 0;

jack_port_t *ports[MAX_PORTS];
jack_client_t *client;

GdkPixbuf *img_on, *img_off, *img_busy;
GdkPixbuf *icon_on, *icon_off;

#ifdef HAVE_LIBLO
int osc_handler(const char *path, const char *types, lo_arg **argv, int argc,
		lo_message msg, void *user_data);
int osc_handler_nox(const char *path, const char *types, lo_arg **argv,
		int argc, lo_message msg, void *user_data);
#endif

int main(int argc, char *argv[])
{
    unsigned int i;
    int opt;
    int help = 0;
    int console = 0;
    char port_name[32];
    pthread_t dt;
#ifdef HAVE_LASH
    lash_args_t *lash_args = lash_extract_args(&argc, &argv);
     lash_event_t *event;
#endif

    while ((opt = getopt(argc, argv, "hic:t:n:p:f:s")) != -1) {
	switch (opt) {
	case 'h':
	    help = 1;
	    break;
	case 'i':
	    console = 1;
	    break;
	case 'c':
	    num_ports = atoi(optarg);
	    DEBUG(1, "ports: %d\n", num_ports);
	    break;
	case 't':
	    buf_length = atoi(optarg);
	    DEBUG(1, "buffer: %ds\n", buf_length);
	    break;
	case 'n':
	    client_name = optarg;
	    DEBUG(1, "client name: %s\n", client_name);
	    break;
	case 'p':
	    prefix = optarg;
	    DEBUG(1, "prefix: %s\n", prefix);
	    break;
	case 'f':
	    format_name = optarg;
	    break;
	case 's':
	    safe_filename = 1;
	    break;
	default:
	    num_ports = 0;
	    break;
	}
    }

    if (optind != argc) {
	num_ports = argc - optind;
    }

    if (num_ports < 1 || num_ports > MAX_PORTS || help) {
	fprintf(stderr, "Usage %s: [-h] [-i] [-c channels] [-n jack-name]\n\t"
			"[-t buffer-length] [-p file prefix] [-f format] "
			"[port-name ...]\n\n", argv[0]);
	fprintf(stderr, "\t-h\tshow this help\n");
	fprintf(stderr, "\t-i\tinteractive mode (console instead of X11) also enabled\n\t\tif DISPLAY is unset\n");
	fprintf(stderr, "\t-c\tspecify number of recording channels\n");
	fprintf(stderr, "\t-n\tspecify the JACK name timemachine will use\n");
	fprintf(stderr, "\t-t\tspecify the pre-recording buffer length\n");
	fprintf(stderr, "\t-p\tspecify the saved file prefix, may include path\n");
	fprintf(stderr, "\t-s\tuse safer characters in filename (windows compatibility)\n");
	fprintf(stderr, "\t-f\tspecify the saved file format\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\tchannels must be in the range 1-8, default %d\n",
			DEFAULT_NUM_PORTS);
	fprintf(stderr, "\tjack-name, default \"%s\"\n", DEFAULT_CLIENT_NAME);
	fprintf(stderr, "\tfile-prefix, default \"%s\"\n", DEFAULT_PREFIX);
	fprintf(stderr, "\tbuffer-length, default %d secs\n", DEFAULT_BUF_LENGTH);
	fprintf(stderr, "\tformat, default '%s', options: wav, w64\n", DEFAULT_FORMAT);
	fprintf(stderr, "\n");
	fprintf(stderr, "specifying port names to connect to on the command line overrides -c\n\n");
	exit(1);
    }

    if (!strcasecmp(format_name, "wav")) {
	format_sf = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    }
#ifdef HAVE_W64
    if (!strcasecmp(format_name, "w64")) {
	format_sf = SF_FORMAT_W64 | SF_FORMAT_FLOAT;
    }
#endif

    if (format_sf == 0) {
	fprintf(stderr, "Unkown format '%s'\n", format_name);
    }

    /* Register with jack */
    if ((client = jack_client_open(client_name, 0, NULL)) == 0) {
	DEBUG(0, "jack server not running?\n");
	exit(1);
    }
    DEBUG(1, "registering as %s\n", client_name);

    process_init(buf_length);

#ifdef HAVE_LASH
    lash_client = lash_init (lash_args, "TimeMachine",
                     0, /* would be LASH_Config_Data_Set etc. */
                     LASH_PROTOCOL (2,0));
    if (!lash_client) {
	DEBUG(1, "could not initialise LASH\n");
    }
    event = lash_event_new_with_type(LASH_Client_Name);
    lash_event_set_string(event, client_name);
    lash_send_event(lash_client, event);
#endif

    jack_set_process_callback(client, process, 0);

    if (jack_activate(client)) {
	DEBUG(0, "cannot activate JACK client");
	exit(1);
    }
#ifdef HAVE_LASH
    lash_jack_client_name(lash_client, client_name);
#endif

    /* Create the jack ports */
    for (i = 0; i < num_ports; i++) {
	jack_port_t *port;

	snprintf(port_name, 31, "in_%d", i + 1);
	ports[i] = jack_port_register(client, port_name,
				      JACK_DEFAULT_AUDIO_TYPE,
				      JackPortIsInput, 0);
	if (optind != argc) {
	    port = jack_port_by_name(client, argv[optind+i]);
	    if (port == NULL) {
		fprintf(stderr, "Can't find port '%s'\n", port_name);
		continue;
	    }
	    if (jack_connect(client, argv[optind+i], jack_port_name(ports[i]))) {
		fprintf(stderr, "Cannot connect port '%s' to '%s'\n",
			argv[optind+i], jack_port_name(ports[i]));
	    }
	}
    }

    /* Start the disk thread */
    pthread_create(&dt, NULL, (void *)&writer_thread, NULL);

#ifdef HAVE_LIBREADLINE
    if (console || !getenv("DISPLAY") || getenv("DISPLAY")[0] == '\0') {
#ifdef HAVE_LIBLO
      lo_server_thread st = lo_server_thread_new(OSC_PORT, NULL);
      if (st) {
	  lo_server_thread_add_method(st, "/start", "", osc_handler_nox, (void *)1);
	  lo_server_thread_add_method(st, "/stop", "", osc_handler_nox, (void *)0);
	  lo_server_thread_start(st);
	  printf("Listening for OSC requests on osc.udp://localhost:%s\n",
	    OSC_PORT);
      }
#endif

      int done = 0;
      while (!done) {
	char *line = readline("TimeMachine> ");
	if (!line) {
	  printf("EOF\n");
	  break;
	}
	if (line && *line) {
	  add_history(line);
	  if (strncmp(line, "q", 1) == 0) done = 1;
	  else if (strncmp(line, "start", 3) == 0) recording_start();
	  else if (strncmp(line, "stop", 3) == 0) recording_stop();
	  else if (strncmp(line, "help", 3) == 0) {
	    printf("Commands: start stop\n");
	  } else {
	    printf("Unknown command\n");
          }
	}
	free(line);
      }
    } else
#endif
    {
      gtk_init(&argc, &argv);

      add_pixmap_directory(PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
      add_pixmap_directory("pixmaps");
      add_pixmap_directory("../pixmaps");

      img_on = create_pixbuf("on.png");
      img_off = create_pixbuf("off.png");
      img_busy = create_pixbuf("busy.png");
      icon_on = create_pixbuf("on-icon.png");
      icon_off = create_pixbuf("off-icon.png");

      main_window = create_window(client_name);
      gtk_window_set_icon(GTK_WINDOW(main_window), icon_off);
      gtk_widget_show(main_window);

      bind_meters();
      g_timeout_add(100, meter_tick, NULL);

#ifdef HAVE_LIBLO
    lo_server_thread st = lo_server_thread_new(OSC_PORT, NULL);
    if (st) {
	lo_server_thread_add_method(st, "/start", "", osc_handler, (void *)1);
	lo_server_thread_add_method(st, "/stop", "", osc_handler, (void *)0);
	lo_server_thread_start(st);
	printf("Listening for OSC requests on osc.udp://localhost:%s\n",
	       OSC_PORT);
    }
#endif

#ifdef HAVE_LASH
      gtk_idle_add(idle_cb, lash_client);
#endif

      gtk_main();
    }

    cleanup();

    /* We can't ever get here, but it keeps gcc quiet */
    return 0;
}

void cleanup()
{
    /* Leave the jack graph */
    jack_client_close(client);

    recording_quit();

    while(!recording_done) {
	usleep(1000);
    }

    DEBUG(0, "exiting\n");
    fflush(stderr);

    /* And were done */
    exit(0);
}

#ifdef HAVE_LASH
gboolean idle_cb(gpointer data)
{
    lash_client_t *lash_client = (lash_client_t *)data;
    lash_event_t *event;
    lash_config_t *config;

    while ((event = lash_get_event(lash_client))) {
	if (lash_event_get_type(event) == LASH_Save_Data_Set) {
	    /* we can ignore this as timemachine has no state thats not on the
             * command line */
	} else if (lash_event_get_type(event) == LASH_Quit) {
	    cleanup();
	} else {
	    DEBUG(0, "unhandled LASH event: type %d, '%s''\n",
		   lash_event_get_type(event),
		   lash_event_get_string(event));
	}
    }

    while ((config = lash_get_config(lash_client))) {
	DEBUG(0, "got unexpected LASH config: %s\n",
	       lash_config_get_key(config));
    }

    usleep(10000);

    return TRUE;
}
#endif

/* vi:set ts=8 sts=4 sw=4: */
