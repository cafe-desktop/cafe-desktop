/* cafe-bg-crossfade.h - fade window background between two surfaces
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Author: Ray Strode <rstrode@redhat.com>
*/
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include <gio/gio.h>

#include <cdk/cdk.h>
#include <cdk/cdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <ctk/ctk.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define CAFE_DESKTOP_USE_UNSTABLE_API
#include <cafe-bg.h>
#include "cafe-bg-crossfade.h"

struct _CafeBGCrossfadePrivate
{
    CdkWindow       *window;
    CtkWidget       *widget;
    int              width;
    int              height;
    cairo_surface_t *fading_surface;
    cairo_surface_t *start_surface;
    cairo_surface_t *end_surface;
    gdouble          start_time;
    gdouble          total_duration;
    guint            timeout_id;
    guint            is_first_frame : 1;
};

enum {
    PROP_0,
    PROP_WIDTH,
    PROP_HEIGHT,
};

enum {
    FINISHED,
    NUMBER_OF_SIGNALS
};

static guint signals[NUMBER_OF_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CafeBGCrossfade, cafe_bg_crossfade, G_TYPE_OBJECT)

static void
cafe_bg_crossfade_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    CafeBGCrossfade *fade;

    g_assert (CAFE_IS_BG_CROSSFADE (object));

    fade = CAFE_BG_CROSSFADE (object);

    switch (property_id)
    {
    case PROP_WIDTH:
        fade->priv->width = g_value_get_int (value);
        break;
    case PROP_HEIGHT:
        fade->priv->height = g_value_get_int (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
cafe_bg_crossfade_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    CafeBGCrossfade *fade;

    g_assert (CAFE_IS_BG_CROSSFADE (object));

    fade = CAFE_BG_CROSSFADE (object);

    switch (property_id)
    {
    case PROP_WIDTH:
        g_value_set_int (value, fade->priv->width);
        break;
    case PROP_HEIGHT:
        g_value_set_int (value, fade->priv->height);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
cafe_bg_crossfade_finalize (GObject *object)
{
    CafeBGCrossfade *fade;

    fade = CAFE_BG_CROSSFADE (object);

    cafe_bg_crossfade_stop (fade);

    if (fade->priv->fading_surface != NULL) {
        cairo_surface_destroy (fade->priv->fading_surface);
        fade->priv->fading_surface = NULL;
    }

    if (fade->priv->start_surface != NULL) {
        cairo_surface_destroy (fade->priv->start_surface);
        fade->priv->start_surface = NULL;
    }

    if (fade->priv->end_surface != NULL) {
        cairo_surface_destroy (fade->priv->end_surface);
        fade->priv->end_surface = NULL;
    }
}

static void
cafe_bg_crossfade_class_init (CafeBGCrossfadeClass *fade_class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (fade_class);

    gobject_class->get_property = cafe_bg_crossfade_get_property;
    gobject_class->set_property = cafe_bg_crossfade_set_property;
    gobject_class->finalize = cafe_bg_crossfade_finalize;

    /**
     * CafeBGCrossfade:width:
     *
     * When a crossfade is running, this is width of the fading
     * surface.
     */
    g_object_class_install_property (gobject_class,
                                     PROP_WIDTH,
                                     g_param_spec_int ("width",
                                     "Window Width",
                                     "Width of window to fade",
                                     0, G_MAXINT, 0,
                                     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

    /**
     * CafeBGCrossfade:height:
     *
     * When a crossfade is running, this is height of the fading
     * surface.
     */
    g_object_class_install_property (gobject_class,
                                     PROP_HEIGHT,
                                     g_param_spec_int ("height", "Window Height",
                                     "Height of window to fade on",
                                     0, G_MAXINT, 0,
                                     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

    /**
     * CafeBGCrossfade::finished:
     * @fade: the #CafeBGCrossfade that received the signal
     * @window: the #CdkWindow the crossfade happend on.
     *
     * When a crossfade finishes, @window will have a copy
     * of the end surface as its background, and this signal will
     * get emitted.
     */
    signals[FINISHED] = g_signal_new ("finished",
                                      G_OBJECT_CLASS_TYPE (gobject_class),
                                      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                      g_cclosure_marshal_VOID__OBJECT,
                                      G_TYPE_NONE, 1, G_TYPE_OBJECT);
}

static void
cafe_bg_crossfade_init (CafeBGCrossfade *fade)
{
    fade->priv = cafe_bg_crossfade_get_instance_private (fade);

    fade->priv->window = NULL;
    fade->priv->widget = NULL;
    fade->priv->fading_surface = NULL;
    fade->priv->start_surface = NULL;
    fade->priv->end_surface = NULL;
    fade->priv->timeout_id = 0;
}

/**
 * cafe_bg_crossfade_new:
 * @width: The width of the crossfading window
 * @height: The height of the crossfading window
 *
 * Creates a new object to manage crossfading a
 * window background between two #cairo_surface_ts.
 *
 * Return value: the new #CafeBGCrossfade
 **/
CafeBGCrossfade* cafe_bg_crossfade_new (int width, int height)
{
    GObject* object;

    object = g_object_new(CAFE_TYPE_BG_CROSSFADE,
                          "width", width,
                          "height", height,
                          NULL);

    return (CafeBGCrossfade*) object;
}

static cairo_surface_t *
tile_surface (cairo_surface_t *surface,
              int              width,
              int              height)
{
    cairo_surface_t *copy;
    cairo_t *cr;

    if (surface == NULL)
    {
        copy = cdk_window_create_similar_surface (cdk_get_default_root_window (),
                                                  CAIRO_CONTENT_COLOR,
                                                  width, height);
    }
    else
    {
        copy = cairo_surface_create_similar (surface,
                                             cairo_surface_get_content (surface),
                                             width, height);
    }

    cr = cairo_create (copy);

    if (surface != NULL)
    {
        cairo_pattern_t *pattern;
        cairo_set_source_surface (cr, surface, 0.0, 0.0);
        pattern = cairo_get_source (cr);
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
    }
    else
    {
        static CtkCssProvider *provider = NULL;
        CtkStyleContext *context;
        CdkRGBA bg;

        if (provider == NULL)
              provider = ctk_css_provider_new ();

        context = ctk_style_context_new ();
        ctk_style_context_add_provider (context,
                                        CTK_STYLE_PROVIDER (provider),
                                        CTK_STYLE_PROVIDER_PRIORITY_THEME);
        ctk_style_context_get_background_color (context, CTK_STATE_FLAG_NORMAL, &bg);
        cdk_cairo_set_source_rgba(cr, &bg);
        g_object_unref (G_OBJECT (context));
    }

    cairo_paint (cr);

    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy (copy);
        copy = NULL;
    }

    cairo_destroy(cr);

    return copy;
}

/**
 * cafe_bg_crossfade_set_start_surface:
 * @fade: a #CafeBGCrossfade
 * @surface: The cairo surface to fade from
 *
 * Before initiating a crossfade with cafe_bg_crossfade_start()
 * a start and end surface have to be set.  This function sets
 * the surface shown at the beginning of the crossfade effect.
 *
 * Return value: %TRUE if successful, or %FALSE if the surface
 * could not be copied.
 **/
gboolean
cafe_bg_crossfade_set_start_surface (CafeBGCrossfade* fade, cairo_surface_t *surface)
{
    g_return_val_if_fail (CAFE_IS_BG_CROSSFADE (fade), FALSE);

    if (fade->priv->start_surface != NULL)
    {
        cairo_surface_destroy (fade->priv->start_surface);
        fade->priv->start_surface = NULL;
    }

    fade->priv->start_surface = tile_surface (surface,
                                              fade->priv->width,
                                              fade->priv->height);

    return fade->priv->start_surface != NULL;
}

static gdouble
get_current_time (void)
{
    const double microseconds_per_second = (double) G_USEC_PER_SEC;
    gint64 tv;

    tv = g_get_real_time ();

    return (double) (tv / microseconds_per_second);
}

/**
 * cafe_bg_crossfade_set_end_surface:
 * @fade: a #CafeBGCrossfade
 * @surface: The cairo surface to fade to
 *
 * Before initiating a crossfade with cafe_bg_crossfade_start()
 * a start and end surface have to be set.  This function sets
 * the surface shown at the end of the crossfade effect.
 *
 * Return value: %TRUE if successful, or %FALSE if the surface
 * could not be copied.
 **/
gboolean
cafe_bg_crossfade_set_end_surface (CafeBGCrossfade* fade, cairo_surface_t *surface)
{
    g_return_val_if_fail (CAFE_IS_BG_CROSSFADE (fade), FALSE);

    if (fade->priv->end_surface != NULL) {
        cairo_surface_destroy (fade->priv->end_surface);
        fade->priv->end_surface = NULL;
    }

    fade->priv->end_surface = tile_surface (surface,
                                            fade->priv->width,
                                            fade->priv->height);

    /* Reset timer in case we're called while animating
     */
    fade->priv->start_time = get_current_time ();
    return fade->priv->end_surface != NULL;
}

static gboolean
animations_are_disabled (CafeBGCrossfade *fade)
{
    CtkSettings *settings;
    CdkScreen *screen;
    gboolean are_enabled;

    g_assert (fade->priv->window != NULL);

    screen = cdk_window_get_screen(fade->priv->window);

    settings = ctk_settings_get_for_screen (screen);

    g_object_get (settings, "ctk-enable-animations", &are_enabled, NULL);

    return !are_enabled;
}

static void
send_root_property_change_notification (CafeBGCrossfade *fade)
{
        long zero_length_pixmap = 0;

        /* We do a zero length append to force a change notification,
         * without changing the value */
        XChangeProperty (CDK_WINDOW_XDISPLAY (fade->priv->window),
                         CDK_WINDOW_XID (fade->priv->window),
                         cdk_x11_get_xatom_by_name ("_XROOTPMAP_ID"),
                         XA_PIXMAP, 32, PropModeAppend,
                         (unsigned char *) &zero_length_pixmap, 0);
}

static void
draw_background (CafeBGCrossfade *fade)
{
    if (fade->priv->widget != NULL) {
        ctk_widget_queue_draw (fade->priv->widget);
    } else if (cdk_window_get_window_type (fade->priv->window) != CDK_WINDOW_ROOT) {
        cairo_t           *cr;
        cairo_region_t    *region;
        CdkDrawingContext *draw_context;

        region = cdk_window_get_visible_region (fade->priv->window);
        draw_context = cdk_window_begin_draw_frame (fade->priv->window,
                                                    region);
        cr = cdk_drawing_context_get_cairo_context (draw_context);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface (cr, fade->priv->fading_surface, 0, 0);
        cairo_paint (cr);
        cdk_window_end_draw_frame (fade->priv->window, draw_context);
        cairo_region_destroy (region);
    } else {
        Display *xdisplay = CDK_WINDOW_XDISPLAY (fade->priv->window);
        CdkDisplay *display;
        display = cdk_display_get_default ();
        cdk_x11_display_error_trap_push (display);
        XGrabServer (xdisplay);
        XClearWindow (xdisplay, CDK_WINDOW_XID (fade->priv->window));
        send_root_property_change_notification (fade);
        XFlush (xdisplay);
        XUngrabServer (xdisplay);
        cdk_x11_display_error_trap_pop_ignored (display);
    }
}

static gboolean
on_widget_draw (CtkWidget       *widget G_GNUC_UNUSED,
                cairo_t         *cr,
                CafeBGCrossfade *fade)
{
    g_assert (fade->priv->fading_surface != NULL);

    cairo_set_source_surface (cr, fade->priv->fading_surface, 0, 0);
    cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
    cairo_paint (cr);

    return FALSE;
}

static gboolean
on_tick (CafeBGCrossfade *fade)
{
    gdouble now, percent_done;
    cairo_t *cr;
    cairo_status_t status;

    g_return_val_if_fail (CAFE_IS_BG_CROSSFADE (fade), FALSE);

    now = get_current_time ();

    percent_done = (now - fade->priv->start_time) / fade->priv->total_duration;
    percent_done = CLAMP (percent_done, 0.0, 1.0);

    /* If it's taking a long time to get to the first frame,
     * then lengthen the duration, so the user will get to see
     * the effect.
     */
    if (fade->priv->is_first_frame && percent_done > .33) {
        fade->priv->is_first_frame = FALSE;
        fade->priv->total_duration *= 1.5;
        return on_tick (fade);
    }

    if (fade->priv->fading_surface == NULL ||
        fade->priv->end_surface == NULL) {
        return FALSE;
    }

    if (animations_are_disabled (fade)) {
        return FALSE;
    }

    /* We accumulate the results in place for performance reasons.
     *
     * This means 1) The fade is exponential, not linear (looks good!)
     * 2) The rate of fade is not independent of frame rate. Slower machines
     * will get a slower fade (but never longer than .75 seconds), and
     * even the fastest machines will get *some* fade because the framerate
     * is capped.
     */
    cr = cairo_create (fade->priv->fading_surface);

    cairo_set_source_surface (cr, fade->priv->end_surface,
                              0.0, 0.0);
    cairo_paint_with_alpha (cr, percent_done);

    status = cairo_status (cr);
    cairo_destroy (cr);

    if (status == CAIRO_STATUS_SUCCESS) {
        draw_background (fade);
    }
    return percent_done <= .99;
}

static void
on_finished (CafeBGCrossfade *fade)
{
    cairo_t *cr;

    if (fade->priv->timeout_id == 0)
        return;

    g_assert (fade->priv->fading_surface != NULL);
    g_assert (fade->priv->end_surface != NULL);

    cr = cairo_create (fade->priv->fading_surface);
    cairo_set_source_surface (cr, fade->priv->end_surface, 0, 0);
    cairo_paint (cr);
    cairo_destroy (cr);
    draw_background (fade);

    cairo_surface_destroy (fade->priv->fading_surface);
    fade->priv->fading_surface = NULL;

    cairo_surface_destroy (fade->priv->end_surface);
    fade->priv->end_surface = NULL;

    g_assert (fade->priv->start_surface != NULL);

    cairo_surface_destroy (fade->priv->start_surface);
    fade->priv->start_surface = NULL;

    if (fade->priv->widget != NULL) {
        g_signal_handlers_disconnect_by_func (fade->priv->widget,
                                              (GCallback) on_widget_draw,
                                              fade);
    }
    fade->priv->widget = NULL;

    fade->priv->timeout_id = 0;
    g_signal_emit (fade, signals[FINISHED], 0, fade->priv->window);
}

/* This function queries the _XROOTPMAP_ID property from the root window
 * to determine the current root window background pixmap and returns a
 * surface to draw directly to it.
 * If _XROOTPMAP_ID is not set, then NULL returned.
 */
static cairo_surface_t *
get_root_pixmap_id_surface (CdkDisplay *display)
{
    CdkScreen       *screen;
    Display         *xdisplay;
    Visual          *xvisual;
    Window           xroot;
    Atom             type;
    int              format, result;
    unsigned long    nitems, bytes_after;
    unsigned char   *data;
    cairo_surface_t *surface = NULL;

    g_return_val_if_fail (display != NULL, NULL);

    screen   = cdk_display_get_default_screen (display);
    xdisplay = CDK_DISPLAY_XDISPLAY (display);
    xvisual  = CDK_VISUAL_XVISUAL (cdk_screen_get_system_visual (screen));
    xroot    = RootWindow (xdisplay, CDK_SCREEN_XNUMBER (screen));

    result = XGetWindowProperty (xdisplay, xroot,
                                 cdk_x11_get_xatom_by_name ("_XROOTPMAP_ID"),
                                 0L, 1L, False, XA_PIXMAP,
                                 &type, &format, &nitems, &bytes_after,
                                 &data);

    if (result != Success || type != XA_PIXMAP ||
        format != 32 || nitems != 1) {
        XFree (data);
        data = NULL;
    }

    if (data != NULL) {
        Pixmap pixmap = *(Pixmap *) data;
        Window root_ret;
        int x_ret, y_ret;
        unsigned int w_ret, h_ret, bw_ret, depth_ret;

        cdk_x11_display_error_trap_push (display);
        if (XGetGeometry (xdisplay, pixmap, &root_ret,
                          &x_ret, &y_ret, &w_ret, &h_ret,
                          &bw_ret, &depth_ret))
        {
            surface = cairo_xlib_surface_create (xdisplay,
                                                 pixmap, xvisual,
                                                 w_ret, h_ret);
        }

        cdk_x11_display_error_trap_pop_ignored (display);
        XFree (data);
    }

    cdk_display_flush (display);
    return surface;
}

/**
 * cafe_bg_crossfade_start:
 * @fade: a #CafeBGCrossfade
 * @window: The #CdkWindow to draw crossfade on
 *
 * This function initiates a quick crossfade between two surfaces on
 * the background of @window. Before initiating the crossfade both
 * cafe_bg_crossfade_set_start_surface() and
 * cafe_bg_crossfade_set_end_surface() need to be called. If animations
 * are disabled, the crossfade is skipped, and the window background is
 * set immediately to the end surface.
 **/
void
cafe_bg_crossfade_start (CafeBGCrossfade *fade,
                         CdkWindow       *window)
{
    GSource *source;
    GMainContext *context;

    g_return_if_fail (CAFE_IS_BG_CROSSFADE (fade));
    g_return_if_fail (window != NULL);
    g_return_if_fail (fade->priv->start_surface != NULL);
    g_return_if_fail (fade->priv->end_surface != NULL);
    g_return_if_fail (!cafe_bg_crossfade_is_started (fade));
    g_return_if_fail (cdk_window_get_window_type (window) != CDK_WINDOW_FOREIGN);

    /* If drawing is done on the root window,
     * it is essential to have the root pixmap.
     */
    if (cdk_window_get_window_type (window) == CDK_WINDOW_ROOT) {
        CdkDisplay *display = cdk_window_get_display (window);
        cairo_surface_t *surface = get_root_pixmap_id_surface (display);

        g_return_if_fail (surface != NULL);
        cairo_surface_destroy (surface);
    }

    if (fade->priv->fading_surface != NULL) {
        cairo_surface_destroy (fade->priv->fading_surface);
        fade->priv->fading_surface = NULL;
    }

    fade->priv->window = window;
    if (cdk_window_get_window_type (fade->priv->window) != CDK_WINDOW_ROOT) {
        fade->priv->fading_surface = tile_surface (fade->priv->start_surface,
                                                   fade->priv->width,
                                                   fade->priv->height);
        if (fade->priv->widget != NULL) {
            g_signal_connect (fade->priv->widget, "draw",
                              (GCallback) on_widget_draw, fade);
        }
    } else {
        cairo_t   *cr;
        CdkDisplay *display = cdk_window_get_display (fade->priv->window);

        fade->priv->fading_surface = get_root_pixmap_id_surface (display);
        cr = cairo_create (fade->priv->fading_surface);
        cairo_set_source_surface (cr, fade->priv->start_surface, 0, 0);
        cairo_paint (cr);
        cairo_destroy (cr);
    }
    draw_background (fade);

    source = g_timeout_source_new (1000 / 60.0);
    g_source_set_callback (source,
                           (GSourceFunc) on_tick,
                           fade,
                           (GDestroyNotify) on_finished);
    context = g_main_context_default ();
    fade->priv->timeout_id = g_source_attach (source, context);
    g_source_unref (source);

    fade->priv->is_first_frame = TRUE;
    fade->priv->total_duration = .75;
    fade->priv->start_time = get_current_time ();
}

/**
 * cafe_bg_crossfade_start_widget:
 * @fade: a #CafeBGCrossfade
 * @widget: The #CtkWidget to draw crossfade on
 *
 * This function initiates a quick crossfade between two surfaces on
 * the background of @widget. Before initiating the crossfade both
 * cafe_bg_crossfade_set_start_surface() and
 * cafe_bg_crossfade_set_end_surface() need to be called. If animations
 * are disabled, the crossfade is skipped, and the window background is
 * set immediately to the end surface.
 **/
void
cafe_bg_crossfade_start_widget (CafeBGCrossfade *fade,
                                CtkWidget       *widget)
{
    CdkWindow *window;

    g_return_if_fail (CAFE_IS_BG_CROSSFADE (fade));
    g_return_if_fail (widget != NULL);

    fade->priv->widget = widget;
    ctk_widget_realize (fade->priv->widget);
    window = ctk_widget_get_window (fade->priv->widget);

    cafe_bg_crossfade_start (fade, window);
}

/**
 * cafe_bg_crossfade_is_started:
 * @fade: a #CafeBGCrossfade
 *
 * This function reveals whether or not @fade is currently
 * running on a window.  See cafe_bg_crossfade_start() for
 * information on how to initiate a crossfade.
 *
 * Return value: %TRUE if fading, or %FALSE if not fading
 **/
gboolean
cafe_bg_crossfade_is_started (CafeBGCrossfade *fade)
{
    g_return_val_if_fail (CAFE_IS_BG_CROSSFADE (fade), FALSE);

    return fade->priv->timeout_id != 0;
}

/**
 * cafe_bg_crossfade_stop:
 * @fade: a #CafeBGCrossfade
 *
 * This function stops any in progress crossfades that may be
 * happening.  It's harmless to call this function if @fade is
 * already stopped.
 **/
void
cafe_bg_crossfade_stop (CafeBGCrossfade *fade)
{
    g_return_if_fail (CAFE_IS_BG_CROSSFADE (fade));

    if (!cafe_bg_crossfade_is_started (fade))
        return;

    g_assert (fade->priv->timeout_id != 0);
    g_source_remove (fade->priv->timeout_id);
    fade->priv->timeout_id = 0;
}
