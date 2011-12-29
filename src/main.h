#ifndef MAIN_H
#define MAIN_H

#include <jack/jack.h>
#include <gtk/gtk.h>

#include "config.h"

#define MAX_PORTS 		8
#define MAX_TIME		3600

#define DEFAULT_BUF_LENGTH	10 /* in seconds */
#define DEFAULT_NUM_PORTS 	2
#define DEFAULT_CLIENT_NAME 	"TimeMachine"
#define DEFAULT_PREFIX 		"tm-"

#ifdef HAVE_W64
#define DEFAULT_FORMAT 	"w64"
#else
#define DEFAULT_FORMAT 	"wav"
#endif

#define DEFAULT_OSC_PORT "7133"

#define DEFAULT_AUTO_BEGIN_THRESHOLD	-35.0
#define DEFAULT_AUTO_END_THRESHOLD	DEFAULT_AUTO_BEGIN_THRESHOLD
#define DEFAULT_AUTO_END_TIME		DEFAULT_BUF_LENGTH

extern GtkWidget *main_window;

extern GdkPixbuf *img_on, *img_off, *img_busy;
extern GdkPixbuf *icon_on, *icon_off;

extern int num_ports;
extern char *prefix;
extern char *format_name;
extern int format_sf;
extern int safe_filename;
extern int auto_record;
extern float auto_begin_threshold;
extern float auto_end_threshold;
extern unsigned int auto_end_time;
extern jack_client_t *client;
extern jack_port_t *ports[MAX_PORTS];
extern char *osc_port;

#endif
