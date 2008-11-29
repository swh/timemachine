/*
 *  Copyright (C) 2003 Steve Harris
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
 *  $Id: gtkmeter.h,v 1.1.1.1 2004/02/17 23:33:50 swh Exp $
 */

#ifndef __GTK_METER_H__
#define __GTK_METER_H__


#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_METER(obj)          GTK_CHECK_CAST (obj, gtk_meter_get_type (), GtkMeter)
#define GTK_METER_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_meter_get_type (), GtkMeterClass)
#define GTK_IS_METER(obj)       GTK_CHECK_TYPE (obj, gtk_meter_get_type ())

#define GTK_METER_UP    0
#define GTK_METER_DOWN  1
#define GTK_METER_LEFT  2
#define GTK_METER_RIGHT 3

typedef struct _GtkMeter        GtkMeter;
typedef struct _GtkMeterClass   GtkMeterClass;

struct _GtkMeter
{
  GtkWidget widget;

  /* update policy (GTK_UPDATE_[CONTINUOUS/DELAYED/DISCONTINUOUS]) */
  guint direction : 2;

  /* Button currently pressed or 0 if none */
  guint8 button;

  /* Amber dB and deflection points */
  gfloat amber_level;
  gfloat amber_frac;

  /* Deflection limits */
  gfloat iec_lower;
  gfloat iec_upper;

  /* Peak deflection */
  gfloat peak;

  /* ID of update timer, or 0 if none */
  guint32 timer;

  /* Old values from adjustment stored so we know when something changes */
  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;

  GdkGC *green_gc;
  GdkGC *amber_gc;
  GdkGC *red_gc;
  GdkGC *peak_gc;

  /* The adjustment object that stores the data for this meter */
  GtkAdjustment *adjustment;
};

struct _GtkMeterClass
{
  GtkWidgetClass parent_class;
};


GtkWidget*     gtk_meter_new                    (GtkAdjustment *adjustment,
						 gint direction);

guint          gtk_meter_get_type               (void);
GtkAdjustment* gtk_meter_get_adjustment         (GtkMeter     *meter);

void           gtk_meter_set_adjustment         (GtkMeter     *meter,
						 GtkAdjustment *adjustment);

void	       gtk_meter_reset_peak		(GtkMeter     *meter);

void           gtk_meter_set_warn_point         (GtkMeter *meter,
						 gfloat pt);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_METER */
