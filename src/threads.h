#ifndef PROCESS_H
#define PROCESS_H

#include <gtk/gtk.h>
#include <jack/jack.h>

/* size of disk buffer, must be power of two */
#define DISK_SIZE 131072

int process(jack_nframes_t nframes, void *arg);

void process_init(unsigned int time);

int writer_thread(void *d);

void recording_start();
void recording_stop();
void recording_quit();

gboolean meter_tick(gpointer data);

extern volatile int recording_done;

#endif
