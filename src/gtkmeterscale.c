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
 *  $Id: gtkmeterscale.c,v 1.1.1.1 2004/02/17 23:33:50 swh Exp $
 */

#include <math.h>
#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "gtkmeterscale.h"

#define METERSCALE_MAX_FONT_SIZE 8

/* Forward declarations */

static void gtk_meterscale_class_init          (GtkMeterScaleClass    *klass);
static void gtk_meterscale_init                (GtkMeterScale         *meterscale);
static void gtk_meterscale_destroy             (GtkObject        *object);
static void gtk_meterscale_realize             (GtkWidget        *widget);
static void gtk_meterscale_size_request        (GtkWidget      *widget,
					        GtkRequisition *requisition);
static void gtk_meterscale_size_allocate       (GtkWidget     *widget,
				 	        GtkAllocation *allocation);
static gint gtk_meterscale_expose              (GtkWidget        *widget,
						GdkEventExpose   *event);
static float iec_scale(float db);
static void meterscale_draw_notch_label(GtkMeterScale *meterscale, float db,
		int mark, PangoRectangle *last_label_rect);
static void meterscale_draw_notch(GtkMeterScale *meterscale, float db, int
		mark);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

GtkType
gtk_meterscale_get_type ()
{
  static GtkType meterscale_type = 0;

  if (!meterscale_type)
    {
      GtkTypeInfo meterscale_info =
      {
	"GtkMeterScale",
	sizeof (GtkMeterScale),
	sizeof (GtkMeterScaleClass),
	(GtkClassInitFunc) gtk_meterscale_class_init,
	(GtkObjectInitFunc) gtk_meterscale_init,
	/*(GtkArgSetFunc)*/ NULL,
	/*(GtkArgGetFunc)*/ NULL,
      };

      meterscale_type = gtk_type_unique (gtk_widget_get_type (),
		      &meterscale_info);
    }

  return meterscale_type;
}

static void
gtk_meterscale_class_init (GtkMeterScaleClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  object_class->destroy = gtk_meterscale_destroy;

  widget_class->realize = gtk_meterscale_realize;
  widget_class->expose_event = gtk_meterscale_expose;
  widget_class->size_request = gtk_meterscale_size_request;
  widget_class->size_allocate = gtk_meterscale_size_allocate;
}

static void
gtk_meterscale_init (GtkMeterScale *meterscale)
{
  meterscale->direction = 0;
  meterscale->iec_lower = 0.0f;
  meterscale->iec_upper = 0.0f;
  meterscale->min_width = -1;
  meterscale->min_height = -1;
}

GtkWidget*
gtk_meterscale_new (gint direction, float min, float max)
{
  GtkMeterScale *meterscale;

  meterscale = gtk_type_new (gtk_meterscale_get_type ());

  meterscale->direction = direction;
  meterscale->lower = min;
  meterscale->upper = max;
  meterscale->iec_lower = iec_scale(min);
  meterscale->iec_upper = iec_scale(max);

  gtk_object_ref(GTK_OBJECT(meterscale));

  return GTK_WIDGET(meterscale);
}

static void
gtk_meterscale_destroy (GtkObject *object)
{
  GtkMeterScale *meterscale;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_METERSCALE (object));

  meterscale = GTK_METERSCALE (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_meterscale_realize (GtkWidget *widget)
{
  GtkMeterScale *meterscale;
  GdkWindowAttr attributes;
  gint attributes_mask;
  PangoContext *pc = gtk_widget_get_pango_context(widget);
  PangoLayout *pl;
  PangoFontDescription *pfd;
  PangoRectangle rect;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_METERSCALE (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  meterscale = GTK_METERSCALE (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

  pfd = pango_font_description_new();
  pango_font_description_set_family(pfd, "sans");
  pango_font_description_set_size(pfd, METERSCALE_MAX_FONT_SIZE * PANGO_SCALE);
  pango_context_set_font_description(pc, pfd);
  pl = pango_layout_new(pc);
  pango_layout_set_text(pl, "99", -1);
  pango_layout_get_pixel_extents(pl, &rect, NULL);

  if (meterscale->direction & GTK_METERSCALE_LEFT ||
      meterscale->direction & GTK_METERSCALE_RIGHT) {
    meterscale->min_width = rect.width + METERSCALE_MAX_FONT_SIZE;
  }
  if (meterscale->direction & GTK_METERSCALE_TOP ||
      meterscale->direction & GTK_METERSCALE_BOTTOM) {
    meterscale->min_height = rect.height + METERSCALE_MAX_FONT_SIZE + 1;
  }
  gtk_widget_set_usize(widget, meterscale->min_width, meterscale->min_height);
}

static void 
gtk_meterscale_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  requisition->width = GTK_METERSCALE(widget)->min_width;
  requisition->height = GTK_METERSCALE(widget)->min_height;
}

static void
gtk_meterscale_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkMeterScale *meterscale;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_METERSCALE (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  meterscale = GTK_METERSCALE (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {

      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

    }
}

static gint
gtk_meterscale_expose (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  GtkMeterScale *meterscale;
  PangoRectangle lr;
  float val;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_METERSCALE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;
  
  meterscale = GTK_METERSCALE (widget);

  /* Draw the background */
  gtk_paint_box (widget->style, widget->window, GTK_STATE_NORMAL,
		  GTK_SHADOW_ETCHED_IN, NULL, widget, "trough", 0, 0,
		  widget->allocation.width, widget->allocation.height);
  gdk_window_clear_area (widget->window, 1, 1, widget->allocation.width - 2,
		  widget->allocation.height - 2);


  lr.x = 0;
  lr.y = 0;
  lr.width = 0;
  lr.height = 0;

  meterscale_draw_notch_label(meterscale, 0.0f, 3, &lr);

  for (val = 5.0f; val < meterscale->upper; val += 5.0f) {
    meterscale_draw_notch_label(meterscale, val, 2, &lr);
  }

  for (val = -5.0f; val > meterscale->lower; val -= 5.0f) {
    meterscale_draw_notch_label(meterscale, val, 2, &lr);
  }

  for (val = -10.0f; val < 10.0f; val += 1.0f) {
    meterscale_draw_notch(meterscale, val, 1);
  }
  
  return FALSE;
}

static void meterscale_draw_notch_label(GtkMeterScale *meterscale, float db,
		int mark, PangoRectangle *last_label_rect)
{
    GtkWidget *widget = GTK_WIDGET(meterscale);
    int length, width, pos;
    int vertical = 0;

    if (meterscale->direction & GTK_METERSCALE_LEFT ||
        meterscale->direction & GTK_METERSCALE_RIGHT) {
	    length = widget->allocation.height - 2;
	    width = widget->allocation.width - 2;
	    pos = length - length * (iec_scale(db) - meterscale->iec_lower) /
		    (meterscale->iec_upper - meterscale->iec_lower);
	    vertical = 1;
    } else {
	    length = widget->allocation.width - 2;
	    width = widget->allocation.height - 2;
	    pos = length * (iec_scale(db) - meterscale->iec_lower) /
		    (meterscale->iec_upper - meterscale->iec_lower);
    }


    if (last_label_rect) {
        PangoContext *pc = gtk_widget_get_pango_context(widget);
        PangoLayout *pl;
	PangoFontDescription *pfd;
	PangoRectangle rect;
	char text[128];
	int x, y, size;

	size = (6 + length / 150);
	if (size > METERSCALE_MAX_FONT_SIZE) {
	    size = METERSCALE_MAX_FONT_SIZE;
	}
	pfd = pango_font_description_new();
	pango_font_description_set_family(pfd, "sans");
	pango_font_description_set_size(pfd, size * PANGO_SCALE);
	pango_context_set_font_description(pc, pfd);

	pl = pango_layout_new(pc);

	snprintf(text, 127, "%.0f", fabs(db));
	pango_layout_set_text(pl, text, -1);
	pango_layout_get_pixel_extents(pl, NULL, &rect);

	if (vertical) {
	    x = width/2 - rect.width/2 + 1;
	    y = pos - rect.height/2;
	    if (y < 1) {
		y = 1;
	    }
	} else {
	    x = pos - rect.width/2 + 1;
	    y = width/2 - rect.height / 2 + 1;
	    if (x < 1) {
		x = 1;
	    } else if (x + rect.width > length) {
	        x = length - rect.width + 1;
	    }
	}

	if (vertical && last_label_rect->y < y + rect.height + 2 && last_label_rect->y + last_label_rect->height + 2 > y) {
	    return;
	}
	if (!vertical && last_label_rect->x < x + rect.width + 2 && last_label_rect->x + last_label_rect->width + 2 > y) {
	    return;
	}

	gdk_draw_layout(widget->window, widget->style->black_gc, x, y, pl);
	last_label_rect->width = rect.width;
	last_label_rect->height = rect.height;
	last_label_rect->x = x;
	last_label_rect->y = y;
    }

    meterscale_draw_notch(meterscale, db, mark);
}

static void meterscale_draw_notch(GtkMeterScale *meterscale, float db,
		int mark)
{
    GtkWidget *widget = GTK_WIDGET(meterscale);
    int pos, length, width;

    if (meterscale->direction & GTK_METERSCALE_LEFT ||
        meterscale->direction & GTK_METERSCALE_RIGHT) {
	    length = widget->allocation.height - 2;
	    width = widget->allocation.width - 2;
	    pos = length - length * (iec_scale(db) - meterscale->iec_lower) /
		    (meterscale->iec_upper - meterscale->iec_lower);
    } else {
	    length = widget->allocation.width - 2;
	    width = widget->allocation.height - 2;
	    pos = length * (iec_scale(db) - meterscale->iec_lower) /
		    (meterscale->iec_upper - meterscale->iec_lower);
    }

    if (meterscale->direction & GTK_METERSCALE_LEFT) {
      gdk_draw_rectangle(widget->window, widget->style->black_gc, TRUE,
	    0, pos, mark, 1);
    }
    if (meterscale->direction & GTK_METERSCALE_RIGHT) {
      gdk_draw_rectangle(widget->window, widget->style->black_gc, TRUE,
	    width - mark + 1, pos, mark, 1);
    }
    if (meterscale->direction & GTK_METERSCALE_TOP) {
      gdk_draw_rectangle(widget->window, widget->style->black_gc, TRUE,
	    pos+1, 1, 1, mark);
    }
    if (meterscale->direction & GTK_METERSCALE_BOTTOM) {
      gdk_draw_rectangle(widget->window, widget->style->black_gc, TRUE,
	    pos+1, width -mark + 1, 1, mark);
    }
}

static float iec_scale(float db)
{
    float def = 0.0f;		/* MeterScale deflection %age */

    if (db < -70.0f) {
	def = 0.0f;
    } else if (db < -60.0f) {
	def = (db + 70.0f) * 0.25f;
    } else if (db < -50.0f) {
	def = (db + 60.0f) * 0.5f + 5.0f;
    } else if (db < -40.0f) {
	def = (db + 50.0f) * 0.75f + 7.5;
    } else if (db < -30.0f) {
	def = (db + 40.0f) * 1.5f + 15.0f;
    } else if (db < -20.0f) {
	def = (db + 30.0f) * 2.0f + 30.0f;
    } else {
	def = (db + 20.0f) * 2.5f + 50.0f;
    }

    return def;
}
