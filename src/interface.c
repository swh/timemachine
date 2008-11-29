#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "main.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "gtkmeter.h"
#include "gtkmeterscale.h"

#define HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget *make_meter(float lower, float upper)
{
    GtkAdjustment *adjustment = (GtkAdjustment *)
        gtk_adjustment_new(-60.0f, lower, upper, 0.0, 0.0, 0.0);

    return gtk_meter_new(adjustment, GTK_METER_RIGHT);
}

GtkWidget *create_window(const char *title)
{
    GtkWidget *window;
    GtkWidget *vbox1;
    GtkWidget *togglebutton1;
    GtkWidget *image1;
    GtkWidget *meter[MAX_PORTS];
    GtkWidget *scale;
    int i;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(window, "window");
    gtk_window_set_title(GTK_WINDOW(window), title);

    vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_widget_set_name(vbox1, "vbox1");
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(window), vbox1);

    togglebutton1 = gtk_button_new();
    gtk_widget_set_name(togglebutton1, "togglebutton1");
    gtk_widget_show(togglebutton1);
    gtk_button_set_relief(GTK_BUTTON(togglebutton1), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(vbox1), togglebutton1, FALSE, FALSE, 0);

    image1 = create_pixmap(window, "off.png");
    gtk_widget_set_name(image1, "toggle_image");
    gtk_widget_show(image1);
    gtk_container_add(GTK_CONTAINER(togglebutton1), image1);

    scale = gtk_meterscale_new(GTK_METERSCALE_BOTTOM, -60.0f, 6.0f);
    gtk_widget_set_name(scale, "scale_top");
    gtk_widget_show(scale);
    gtk_box_pack_start(GTK_BOX(vbox1), scale, FALSE, TRUE, 0);
    gtk_widget_set_size_request(scale, -1, 10);
    GTK_WIDGET_UNSET_FLAGS(scale, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS(scale, GTK_CAN_DEFAULT);

    for (i=0; i<num_ports; i++) {
	char name[32];

	snprintf(name, 31, "meter%d", i);
	meter[i] = make_meter(-60, 6);
	gtk_widget_set_name(meter[i], name);
	gtk_widget_show(meter[i]);
	gtk_meter_set_warn_point(GTK_METER(meter[i]), 0.0);
	gtk_box_pack_start(GTK_BOX(vbox1), meter[i], FALSE, TRUE, 0);
	gtk_widget_set_size_request(meter[i], -1, 10);
	GTK_WIDGET_UNSET_FLAGS(meter[i], GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS(meter[i], GTK_CAN_DEFAULT);
    }

    if (num_ports > 1) {
	scale = gtk_meterscale_new(GTK_METERSCALE_TOP, -60.0f, 6.0f);
	gtk_widget_set_name(scale, "scale_bottom");
	gtk_widget_show(scale);
	gtk_box_pack_start(GTK_BOX(vbox1), scale, FALSE, TRUE, 0);
	gtk_widget_set_size_request(scale, -1, 10);
	GTK_WIDGET_UNSET_FLAGS(scale, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS(scale, GTK_CAN_DEFAULT);
    }

    g_signal_connect((gpointer) window, "delete_event",
		     G_CALLBACK(on_window_delete_event), NULL);
    g_signal_connect((gpointer) togglebutton1, "clicked",
		     G_CALLBACK(on_togglebutton1_clicked), NULL);

    /* Store pointers to all widgets, for use by lookup_widget(). */
    HOOKUP_OBJECT_NO_REF(window, window, "window");
    HOOKUP_OBJECT(window, vbox1, "vbox1");
    HOOKUP_OBJECT(window, togglebutton1, "togglebutton1");
    HOOKUP_OBJECT(window, image1, "toggle_image");
    for (i=0; i<num_ports; i++) {
	char name[32];

	snprintf(name, 31, "meter%d", i);
	HOOKUP_OBJECT(window, meter[i], name);
    }

    return window;
}

/* vi:set ts=8 sts=4 sw=4: */
