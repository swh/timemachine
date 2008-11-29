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
 *  $Id: meters.c,v 1.1.1.1 2004/02/17 23:33:50 swh Exp $
 */

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "main.h"
#include "gtkmeter.h"
#include "threads.h"
#include "support.h"

#define lin2db(lin) (20.0f * log10(lin))
static GtkAdjustment *meter_adj[MAX_PORTS];

void bind_meters()
{
    unsigned int i;

    for (i=0; i<num_ports; i++) {
	char name[32];
	GtkMeter *meter;

	snprintf(name, 31, "meter%d", i);
	meter = GTK_METER(lookup_widget(main_window, name));
	if (!meter) {
	    fprintf(stderr, "Cant find meter %d at %s:%d\n", i, __FILE__, __LINE__);
	    exit(1);
	}
	meter_adj[i] = gtk_meter_get_adjustment(meter);
	if (!meter_adj[i]) {
	    fprintf(stderr, "Cant get adjustment %d at %s:%d\n", i, __FILE__, __LINE__);
	    exit(1);
	}
    }
}

void update_meters(float amp[])
{
    unsigned int i;

    for (i=0; i<num_ports; i++) {
	gtk_adjustment_set_value(meter_adj[i], lin2db(amp[i]));
    }
}

/* vi:set ts=8 sts=4 sw=4: */
