#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "gtkmeter.h"
#include "gtkmeterscale.h"
#include "threads.h"
#ifdef HAVE_LIBLO
#include <lo/lo.h>
#endif

static int button_pressed = 0;

void on_togglebutton1_clicked(GtkButton * button, gpointer user_data)
{
    GtkWidget *img = lookup_widget(main_window, "toggle_image");

    if (!GTK_WIDGET_IS_SENSITIVE(img)) {
	return;
    }

    button_pressed = !button_pressed;

    if (button_pressed) {
	recording_start();
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), img_on);
	gtk_window_set_icon(GTK_WINDOW(main_window), icon_on);
    } else {
	recording_stop();
	gtk_widget_set_sensitive(img, FALSE);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), img_busy);
	gtk_window_set_icon(GTK_WINDOW(main_window), icon_off);
    }
}

gboolean on_window_delete_event(GtkWidget * widget, GdkEvent * event,
				gpointer user_data)
{
    gtk_main_quit();

    return FALSE;
}

#ifdef HAVE_LIBLO
int osc_handler(const char *path, const char *types, lo_arg **argv, int argc,
		lo_message msg, void *user_data)
{
    GtkWidget *img = lookup_widget(main_window, "toggle_image");

    if (user_data) {
	recording_start();
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), img_on);
	gtk_window_set_icon(GTK_WINDOW(main_window), icon_on);
    } else {
	recording_stop();
	gtk_widget_set_sensitive(img, FALSE);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), img_busy);
	gtk_window_set_icon(GTK_WINDOW(main_window), icon_off);
    }

    return 0;
}

int osc_handler_nox(const char *path, const char *types, lo_arg **argv,
		int argc, lo_message msg, void *user_data)
{
    if (user_data) {
	recording_start();
    } else {
	recording_stop();
    }

    return 0;
}
#endif
