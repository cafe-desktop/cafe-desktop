/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * cafe-rr-labeler.c - Utility to label monitors to identify them
 * while they are being configured.
 *
 * Copyright 2008, Novell, Inc.
 *
 * This file is part of the Cafe Library.
 *
 * The Cafe Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Cafe Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Cafe Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Federico Mena-Quintero <federico@novell.com>
 */

#define CAFE_DESKTOP_USE_UNSTABLE_API

#include <config.h>
#include <glib/gi18n-lib.h>
#include <ctk/ctk.h>

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cdk/cdkx.h>

#include "cafe-rr-labeler.h"

struct _CafeRRLabelerPrivate {
	CafeRRConfig *config;

	int num_outputs;

	CdkRGBA *palette;
	CtkWidget **windows;

	CdkScreen  *screen;
	Atom        workarea_atom;
};

enum {
	PROP_0,
	PROP_CONFIG,
	PROP_LAST
};

G_DEFINE_TYPE_WITH_PRIVATE (CafeRRLabeler, cafe_rr_labeler, G_TYPE_OBJECT);

static void cafe_rr_labeler_finalize (GObject *object);
static void create_label_windows (CafeRRLabeler *labeler);
static void setup_from_config (CafeRRLabeler *labeler);

static int
get_current_desktop (CdkScreen *screen)
{
        Display *display;
        Window win;
        Atom current_desktop, type;
        int format;
        unsigned long n_items, bytes_after;
        unsigned char *data_return = NULL;
        int workspace = 0;

        display = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (screen));
        win = XRootWindow (display, CDK_SCREEN_XNUMBER (screen));

        current_desktop = XInternAtom (display, "_NET_CURRENT_DESKTOP", True);

        XGetWindowProperty (display,
                            win,
                            current_desktop,
                            0, G_MAXLONG,
                            False, XA_CARDINAL,
                            &type, &format, &n_items, &bytes_after,
                            &data_return);

        if (type == XA_CARDINAL && format == 32 && n_items > 0)
                workspace = (int) data_return[0];
        if (data_return)
                XFree (data_return);

        return workspace;
}

static gboolean
get_work_area (CafeRRLabeler *labeler,
	       CdkRectangle   *rect)
{
	Atom            workarea;
	Atom            type;
	Window          win;
	int             format;
	gulong          num;
	gulong          leftovers;
	gulong          max_len = 4 * 32;
	guchar         *ret_workarea;
	long           *workareas;
	int             result;
	int             disp_screen;
	int             desktop;
	Display        *display;

	display = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (labeler->priv->screen));
	workarea = XInternAtom (display, "_NET_WORKAREA", True);

	disp_screen = CDK_SCREEN_XNUMBER (labeler->priv->screen);

	/* Defaults in case of error */
	rect->x = 0;
	rect->y = 0;
	rect->width = WidthOfScreen (cdk_x11_screen_get_xscreen (labeler->priv->screen));
	rect->height = HeightOfScreen (cdk_x11_screen_get_xscreen (labeler->priv->screen));

	if (workarea == None)
		return FALSE;

	win = XRootWindow (display, disp_screen);
	result = XGetWindowProperty (display,
				     win,
				     workarea,
				     0,
				     max_len,
				     False,
				     AnyPropertyType,
				     &type,
				     &format,
				     &num,
				     &leftovers,
				     &ret_workarea);

	if (result != Success
	    || type == None
	    || format == 0
	    || leftovers
	    || num % 4) {
		return FALSE;
	}

	desktop = get_current_desktop (labeler->priv->screen);

	workareas = (long *) ret_workarea;
	rect->x = workareas[desktop * 4];
	rect->y = workareas[desktop * 4 + 1];
	rect->width = workareas[desktop * 4 + 2];
	rect->height = workareas[desktop * 4 + 3];

	XFree (ret_workarea);

	return TRUE;
}

static CdkFilterReturn
screen_xevent_filter (CdkXEvent     *xevent,
		      CdkEvent      *event G_GNUC_UNUSED,
		      CafeRRLabeler *labeler)
{
	XEvent *xev;

	xev = (XEvent *) xevent;

	if (xev->type == PropertyNotify &&
	    xev->xproperty.atom == labeler->priv->workarea_atom) {
		/* update label positions */
		cafe_rr_labeler_hide (labeler);
		create_label_windows (labeler);
	}

	return CDK_FILTER_CONTINUE;
}

static void
cafe_rr_labeler_init (CafeRRLabeler *labeler)
{
	CdkWindow *cdkwindow;

	labeler->priv = cafe_rr_labeler_get_instance_private (labeler);

	labeler->priv->workarea_atom = XInternAtom (CDK_DISPLAY_XDISPLAY (cdk_display_get_default ()),
						    "_NET_WORKAREA",
						    True);

	labeler->priv->screen = cdk_screen_get_default ();
	/* code is not really designed to handle multiple screens so *shrug* */
	cdkwindow = cdk_screen_get_root_window (labeler->priv->screen);
	cdk_window_add_filter (cdkwindow, (CdkFilterFunc) screen_xevent_filter, labeler);
	cdk_window_set_events (cdkwindow, cdk_window_get_events (cdkwindow) | CDK_PROPERTY_CHANGE_MASK);
}

static void
cafe_rr_labeler_set_property (GObject *gobject, guint property_id, const GValue *value, GParamSpec *param_spec)
{
	CafeRRLabeler *self = CAFE_RR_LABELER (gobject);

	switch (property_id) {
	case PROP_CONFIG:
		self->priv->config = CAFE_RR_CONFIG (g_value_dup_object (value));
		return;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, param_spec);
	}
}

static GObject *
cafe_rr_labeler_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
	CafeRRLabeler *self = (CafeRRLabeler*) G_OBJECT_CLASS (cafe_rr_labeler_parent_class)->constructor (type, n_construct_properties, construct_properties);

	setup_from_config (self);

	return (GObject*) self;
}

static void
cafe_rr_labeler_class_init (CafeRRLabelerClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	object_class->set_property = cafe_rr_labeler_set_property;
	object_class->finalize = cafe_rr_labeler_finalize;
	object_class->constructor = cafe_rr_labeler_constructor;

	g_object_class_install_property (object_class, PROP_CONFIG, g_param_spec_object ("config",
											 "Configuration",
											 "RandR configuration to label",
											 CAFE_TYPE_RR_CONFIG,
											 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
											 G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void
cafe_rr_labeler_finalize (GObject *object)
{
	CafeRRLabeler *labeler;
	CdkWindow      *cdkwindow;

	labeler = CAFE_RR_LABELER (object);

	cdkwindow = cdk_screen_get_root_window (labeler->priv->screen);
	cdk_window_remove_filter (cdkwindow, (CdkFilterFunc) screen_xevent_filter, labeler);

	if (labeler->priv->config != NULL) {
		g_object_unref (labeler->priv->config);
	}

	if (labeler->priv->windows != NULL) {
		cafe_rr_labeler_hide (labeler);
		g_free (labeler->priv->windows);
	}

	g_free (labeler->priv->palette);

	G_OBJECT_CLASS (cafe_rr_labeler_parent_class)->finalize (object);
}

static int
count_outputs (CafeRRConfig *config)
{
	int i;
	CafeRROutputInfo **outputs = cafe_rr_config_get_outputs (config);

	for (i = 0; outputs[i] != NULL; i++)
		;

	return i;
}

static void
make_palette (CafeRRLabeler *labeler)
{
	/* The idea is that we go around an hue color wheel.  We want to start
	 * at red, go around to green/etc. and stop at blue --- because magenta
	 * is evil.  Eeeeek, no magenta, please!
	 *
	 * Purple would be nice, though.  Remember that we are watered down
	 * (i.e. low saturation), so that would be like Like berries with cream.
	 * Mmmmm, berries.
	 */
	double start_hue;
	double end_hue;
	int i;

	g_assert (labeler->priv->num_outputs > 0);

	labeler->priv->palette = g_new (CdkRGBA, labeler->priv->num_outputs);

	start_hue = 0.0; /* red */
	end_hue   = 2.0/3; /* blue */

	for (i = 0; i < labeler->priv->num_outputs; i++) {
		double h, s, v;
		double r, g, b;

		h = start_hue + (end_hue - start_hue) / labeler->priv->num_outputs * i;
		s = 1.0 / 3;
		v = 1.0;

		ctk_hsv_to_rgb (h, s, v, &r, &g, &b);

		labeler->priv->palette[i].red   = r;
		labeler->priv->palette[i].green = g;
		labeler->priv->palette[i].blue  = b;
		labeler->priv->palette[i].alpha  = 1.0;
	}
}

#define LABEL_WINDOW_EDGE_THICKNESS 2
#define LABEL_WINDOW_PADDING 12

static gboolean
label_window_draw_event_cb (CtkWidget *widget,
			    cairo_t   *cr,
			    gpointer   data G_GNUC_UNUSED)
{
	CdkRGBA *color;
	CtkAllocation allocation;

	color = g_object_get_data (G_OBJECT (widget), "color");
	ctk_widget_get_allocation (widget, &allocation);

	/* edge outline */

	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr,
			 LABEL_WINDOW_EDGE_THICKNESS / 2.0,
			 LABEL_WINDOW_EDGE_THICKNESS / 2.0,
			 allocation.width - LABEL_WINDOW_EDGE_THICKNESS,
			 allocation.height - LABEL_WINDOW_EDGE_THICKNESS);
	cairo_set_line_width (cr, LABEL_WINDOW_EDGE_THICKNESS);
	cairo_stroke (cr);

	/* fill */
	cdk_cairo_set_source_rgba (cr, color);
	cairo_rectangle (cr,
			 LABEL_WINDOW_EDGE_THICKNESS,
			 LABEL_WINDOW_EDGE_THICKNESS,
			 allocation.width - LABEL_WINDOW_EDGE_THICKNESS * 2,
			 allocation.height - LABEL_WINDOW_EDGE_THICKNESS * 2);
	cairo_fill (cr);

	return FALSE;
}

static void
position_window (CafeRRLabeler  *labeler,
		 CtkWidget       *window,
		 int              x,
		 int              y)
{
	CdkRectangle    workarea;
	CdkRectangle    monitor;
	CdkMonitor     *monitor_num;

	get_work_area (labeler, &workarea);
	monitor_num = cdk_display_get_monitor_at_point (cdk_screen_get_display (labeler->priv->screen), x, y);
	cdk_monitor_get_geometry (monitor_num, &monitor);
	cdk_rectangle_intersect (&monitor, &workarea, &workarea);

	ctk_window_move (CTK_WINDOW (window), workarea.x, workarea.y);
}

static CtkWidget *
create_label_window (CafeRRLabeler *labeler, CafeRROutputInfo *output, CdkRGBA *color)
{
	CtkWidget *window;
	CtkWidget *widget;
	char *str;
	char *display_name;
	CdkRGBA black = { 0, 0, 0, 1.0 };
	int x,y;

	window = ctk_window_new (CTK_WINDOW_POPUP);
	ctk_widget_set_app_paintable (window, TRUE);

	ctk_container_set_border_width (CTK_CONTAINER (window), LABEL_WINDOW_PADDING + LABEL_WINDOW_EDGE_THICKNESS);

	/* This is semi-dangerous.  The color is part of the labeler->palette
	 * array.  Note that in cafe_rr_labeler_finalize(), we are careful to
	 * free the palette only after we free the windows.
	 */
	g_object_set_data (G_OBJECT (window), "color", color);

	g_signal_connect (window, "draw",
			  G_CALLBACK (label_window_draw_event_cb), labeler);

	if (cafe_rr_config_get_clone (labeler->priv->config)) {
		/* Keep this string in sync with cafe-control-center/capplets/display/xrandr-capplet.c:get_display_name() */

		/* Translators:  this is the feature where what you see on your laptop's
		 * screen is the same as your external monitor.  Here, "Mirror" is being
		 * used as an adjective, not as a verb.  For example, the Spanish
		 * translation could be "Pantallas en Espejo", *not* "Espejar Pantallas".
		 */
		display_name = g_strdup_printf (_("Mirror Screens"));
		str = g_strdup_printf ("<b>%s</b>", display_name);
	} else {
		display_name = g_strdup_printf ("<b>%s</b>\n<small>%s</small>", cafe_rr_output_info_get_display_name (output), cafe_rr_output_info_get_name (output));
		str = g_strdup_printf ("%s", display_name);
	}
	g_free (display_name);

	widget = ctk_label_new (NULL);
	ctk_label_set_markup (CTK_LABEL (widget), str);
	g_free (str);

	/* Make the label explicitly black.  We don't want it to follow the
	 * theme's colors, since the label is always shown against a light
	 * pastel background.  See bgo#556050
	 */
	ctk_widget_override_color (widget, ctk_widget_get_state_flags (widget), &black);

	ctk_container_add (CTK_CONTAINER (window), widget);

	/* Should we center this at the top edge of the monitor, instead of using the upper-left corner? */
	cafe_rr_output_info_get_geometry (output, &x, &y, NULL, NULL);
	position_window (labeler, window, x, y);

	ctk_widget_show_all (window);

	return window;
}

static void
create_label_windows (CafeRRLabeler *labeler)
{
	int i;
	gboolean created_window_for_clone;
	CafeRROutputInfo **outputs;

	labeler->priv->windows = g_new (CtkWidget *, labeler->priv->num_outputs);

	created_window_for_clone = FALSE;

	outputs = cafe_rr_config_get_outputs (labeler->priv->config);

	for (i = 0; i < labeler->priv->num_outputs; i++) {
		if (!created_window_for_clone && cafe_rr_output_info_is_active (outputs[i])) {
			labeler->priv->windows[i] = create_label_window (labeler, outputs[i], labeler->priv->palette + i);

			if (cafe_rr_config_get_clone (labeler->priv->config))
				created_window_for_clone = TRUE;
		} else
			labeler->priv->windows[i] = NULL;
	}
}

static void
setup_from_config (CafeRRLabeler *labeler)
{
	labeler->priv->num_outputs = count_outputs (labeler->priv->config);

	make_palette (labeler);

	create_label_windows (labeler);
}

/**
 * cafe_rr_labeler_new:
 * @config: Configuration of the screens to label
 *
 * Create a GUI element that will display colored labels on each connected monitor.
 * This is useful when users are required to identify which monitor is which, e.g. for
 * for configuring multiple monitors.
 * The labels will be shown by default, use cafe_rr_labeler_hide to hide them.
 *
 * Returns: A new #CafeRRLabeler
 */
CafeRRLabeler *
cafe_rr_labeler_new (CafeRRConfig *config)
{
	g_return_val_if_fail (CAFE_IS_RR_CONFIG (config), NULL);

	return g_object_new (CAFE_TYPE_RR_LABELER, "config", config, NULL);
}

/**
 * cafe_rr_labeler_hide:
 * @labeler: A #CafeRRLabeler
 *
 * Hide ouput labels.
 */
void
cafe_rr_labeler_hide (CafeRRLabeler *labeler)
{
	int i;
	CafeRRLabelerPrivate *priv;

	g_return_if_fail (CAFE_IS_RR_LABELER (labeler));

	priv = labeler->priv;

	if (priv->windows == NULL)
		return;

	for (i = 0; i < priv->num_outputs; i++)
		if (priv->windows[i] != NULL) {
			ctk_widget_destroy (priv->windows[i]);
			priv->windows[i] = NULL;
	}
	g_free (priv->windows);
	priv->windows = NULL;
}

/**
 * cafe_rr_labeler_get_rgba_for_output:
 * @labeler: A #CafeRRLabeler
 * @output: Output device (i.e. monitor) to query
 * @color_out: (out): Color of selected monitor.
 *
 * Get the color used for the label on a given output (monitor).
 */
void
cafe_rr_labeler_get_rgba_for_output (CafeRRLabeler *labeler, CafeRROutputInfo *output, CdkRGBA *color_out)
{
	int i;
	CafeRROutputInfo **outputs;

	g_return_if_fail (CAFE_IS_RR_LABELER (labeler));
	g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (output));
	g_return_if_fail (color_out != NULL);

	outputs = cafe_rr_config_get_outputs (labeler->priv->config);

	for (i = 0; i < labeler->priv->num_outputs; i++)
		if (outputs[i] == output) {
			*color_out = labeler->priv->palette[i];
			return;
		}

	g_warning ("trying to get the color for unknown CafeOutputInfo %p; returning magenta!", output);

	color_out->red   = 1.0;
	color_out->green = 0.0;
	color_out->blue  = 1.0;
	color_out->alpha = 1.0;
}
