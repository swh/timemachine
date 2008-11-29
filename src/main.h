#ifndef MAIN_H
#define MAIN_H

#include <jack/jack.h>
#include <gtk/gtk.h>

#include "config.h"

#define MAX_PORTS 		8
#define MAX_TIME		3600

#define DEFAULT_NUM_PORTS 	2
#define DEFAULT_CLIENT_NAME 	"TimeMachine"
#define DEFAULT_PREFIX 		"tm-"

#ifdef HAVE_W64
#define DEFAULT_FORMAT 	"w64"
#else
#define DEFAULT_FORMAT 	"wav"
#endif

#define OSC_PORT "7133"

extern GtkWidget *main_window;

extern GdkPixbuf *img_on, *img_off, *img_busy;
extern GdkPixbuf *icon_on, *icon_off;

extern int num_ports;
extern char *prefix;
extern char *format_name;
extern int format_sf;
extern int safe_filename;
extern jack_client_t *client;
extern jack_port_t *ports[MAX_PORTS];

#endif
