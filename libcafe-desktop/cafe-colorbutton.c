/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include "private.h"

#include "cafe-colorbutton.h"
#include "cafe-colorsel.h"
#include "cafe-colorseldialog.h"
#include <ctk/ctk.h>
#include <cdk/cdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n-lib.h>

/* Size of checks and gray levels for alpha compositing checkerboard */
#define CHECK_SIZE  4
#define CHECK_DARK  (1.0 / 3.0)
#define CHECK_LIGHT (2.0 / 3.0)

struct _CafeColorButtonPrivate
{
  CtkWidget *draw_area; /* Widget where we draw the color sample */
  CtkWidget *cs_dialog; /* Color selection dialog */

  gchar *title;         /* Title for the color selection window */

  CdkColor color;
  guint16 alpha;

  guint use_alpha : 1;  /* Use alpha or not */
};

/* Properties */
enum
{
  PROP_0,
  PROP_USE_ALPHA,
  PROP_TITLE,
  PROP_COLOR,
  PROP_ALPHA
};

/* Signals */
enum
{
  COLOR_SET,
  LAST_SIGNAL
};

/* gobject signals */
static void cafe_color_button_finalize      (GObject             *object);
static void cafe_color_button_set_property  (GObject        *object,
					    guint           param_id,
					    const GValue   *value,
					    GParamSpec     *pspec);
static void cafe_color_button_get_property  (GObject        *object,
					    guint           param_id,
					    GValue         *value,
					    GParamSpec     *pspec);

/* ctkwidget signals */
static void cafe_color_button_state_changed (CtkWidget           *widget,
					    CtkStateType         previous_state);

/* ctkbutton signals */
static void cafe_color_button_clicked       (CtkButton           *button);

/* source side drag signals */
static void cafe_color_button_drag_begin (CtkWidget        *widget,
					 CdkDragContext   *context,
					 gpointer          data);
static void cafe_color_button_drag_data_get (CtkWidget        *widget,
                                            CdkDragContext   *context,
                                            CtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time,
                                            CafeColorButton   *color_button);

/* target side drag signals */
static void cafe_color_button_drag_data_received (CtkWidget        *widget,
						 CdkDragContext   *context,
						 gint              x,
						 gint              y,
						 CtkSelectionData *selection_data,
						 guint             info,
						 guint32           time,
						 CafeColorButton   *color_button);


static guint color_button_signals[LAST_SIGNAL] = { 0 };

static const CtkTargetEntry drop_types[] = { { "application/x-color", 0, 0 } };

G_DEFINE_TYPE_WITH_PRIVATE (CafeColorButton, cafe_color_button, CTK_TYPE_BUTTON)

static void
cafe_color_button_class_init (CafeColorButtonClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkButtonClass *button_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = CTK_WIDGET_CLASS (klass);
  button_class = CTK_BUTTON_CLASS (klass);

  gobject_class->get_property = cafe_color_button_get_property;
  gobject_class->set_property = cafe_color_button_set_property;
  gobject_class->finalize = cafe_color_button_finalize;
  widget_class->state_changed = cafe_color_button_state_changed;
  button_class->clicked = cafe_color_button_clicked;
  klass->color_set = NULL;

  /**
   * CafeColorButton:use-alpha:
   *
   * If this property is set to %TRUE, the color swatch on the button is rendered against a
   * checkerboard background to show its opacity and the opacity slider is displayed in the
   * color selection dialog.
   *
   * Since: 1.9.1
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_ALPHA,
                                   g_param_spec_boolean ("use-alpha", _("Use alpha"),
                                                         _("Whether or not to give the color an alpha value"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * CafeColorButton:title:
   *
   * The title of the color selection dialog
   *
   * Since: 1.9.1
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
							_("Title"),
                                                        _("The title of the color selection dialog"),
                                                        _("Pick a Color"),
                                                        G_PARAM_READWRITE));

  /**
   * CafeColorButton:color:
   *
   * The selected color.
   *
   * Since: 1.9.1
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       _("Current Color"),
                                                       _("The selected color"),
                                                       CDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * CafeColorButton:alpha:
   *
   * The selected opacity value (0 fully transparent, 65535 fully opaque).
   *
   * Since: 1.9.1
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ALPHA,
                                   g_param_spec_uint ("alpha",
                                                      _("Current Alpha"),
                                                      _("The selected opacity value (0 fully transparent, 65535 fully opaque)"),
                                                      0, 65535, 65535,
                                                      G_PARAM_READWRITE));

  /**
   * CafeColorButton::color-set:
   * @widget: the object which received the signal.
   *
   * The ::color-set signal is emitted when the user selects a color.
   * When handling this signal, use cafe_color_button_get_color() and
   * cafe_color_button_get_alpha() to find out which color was just selected.
   *
   * Note that this signal is only emitted when the <emphasis>user</emphasis>
   * changes the color. If you need to react to programmatic color changes
   * as well, use the notify::color signal.
   *
   * Since: 1.9.1
   */
  color_button_signals[COLOR_SET] = g_signal_new ("color-set",
						  G_TYPE_FROM_CLASS (gobject_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (CafeColorButtonClass, color_set),
						  NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE, 0);
}

static gboolean
cafe_color_button_has_alpha (CafeColorButton *color_button)
{
  return color_button->priv->use_alpha &&
      color_button->priv->alpha < 65535;
}

static cairo_pattern_t *
cafe_color_button_get_checkered (void)
{
  static cairo_surface_t *checkered = NULL;
  cairo_pattern_t *pattern;

  if (checkered == NULL)
    {
      /* need to respect pixman's stride being a multiple of 4 */
      static unsigned char data[8] = { 0xFF, 0x00, 0x00, 0x00,
                                       0x00, 0xFF, 0x00, 0x00 };

      checkered = cairo_image_surface_create_for_data (data,
                                                       CAIRO_FORMAT_A8,
                                                       2, 2, 4);
    }

  pattern = cairo_pattern_create_for_surface (checkered);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_pattern_set_filter (pattern, CAIRO_FILTER_NEAREST);

  return pattern;
}

/* Handle exposure events for the color picker's drawing area */
static gboolean
draw (CtkWidget *widget G_GNUC_UNUSED,
      cairo_t   *cr,
      gpointer   data)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (data);
  cairo_pattern_t *checkered;

  if (cafe_color_button_has_alpha (color_button))
    {
      cairo_save (cr);

      cairo_set_source_rgb (cr, CHECK_DARK, CHECK_DARK, CHECK_DARK);
      cairo_paint (cr);

      cairo_set_source_rgb (cr, CHECK_LIGHT, CHECK_LIGHT, CHECK_LIGHT);
      cairo_scale (cr, CHECK_SIZE, CHECK_SIZE);

      checkered = cafe_color_button_get_checkered ();
      cairo_mask (cr, checkered);
      cairo_pattern_destroy (checkered);

      cairo_restore (cr);

      cairo_set_source_rgba (cr,
                             color_button->priv->color.red / 65535.,
                             color_button->priv->color.green / 65535.,
                             color_button->priv->color.blue / 65535.,
                             color_button->priv->alpha / 65535.);
    }
  else
    {
      cdk_cairo_set_source_color (cr, &color_button->priv->color);
    }

  cairo_paint (cr);

  if (!ctk_widget_is_sensitive (CTK_WIDGET (color_button)))
    {
      cdk_cairo_set_source_color (cr, &ctk_widget_get_style (CTK_WIDGET(color_button))->bg[CTK_STATE_INSENSITIVE]);
      checkered = cafe_color_button_get_checkered ();
      cairo_mask (cr, checkered);
      cairo_pattern_destroy (checkered);
    }

  return FALSE;
}

static void
cafe_color_button_state_changed (CtkWidget   *widget,
				 CtkStateType previous_state G_GNUC_UNUSED)
{
  ctk_widget_queue_draw (widget);
}

static void
cafe_color_button_drag_data_received (CtkWidget        *widget G_GNUC_UNUSED,
				     CdkDragContext   *context G_GNUC_UNUSED,
				     gint              x G_GNUC_UNUSED,
				     gint              y G_GNUC_UNUSED,
				     CtkSelectionData *selection_data,
				     guint             info G_GNUC_UNUSED,
				     guint32           time G_GNUC_UNUSED,
				     CafeColorButton   *color_button)
{
  guint16 *dropped;

  if (ctk_selection_data_get_length (selection_data) < 0)
    return;

  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (ctk_selection_data_get_length (selection_data) != 8)
    {
      g_warning (_("Received invalid color data\n"));
      return;
    }


  dropped = (guint16 *)ctk_selection_data_get_data (selection_data);

  color_button->priv->color.red = dropped[0];
  color_button->priv->color.green = dropped[1];
  color_button->priv->color.blue = dropped[2];
  color_button->priv->alpha = dropped[3];

  ctk_widget_queue_draw (color_button->priv->draw_area);

  g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);

  g_object_freeze_notify (G_OBJECT (color_button));
  g_object_notify (G_OBJECT (color_button), "color");
  g_object_notify (G_OBJECT (color_button), "alpha");
  g_object_thaw_notify (G_OBJECT (color_button));
}

static void
set_color_icon (CdkDragContext *context,
		CdkColor       *color)
{
  GdkPixbuf *pixbuf;
  guint32 pixel;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
			   8, 48, 32);

  pixel = ((color->red & 0xff00) << 16) |
          ((color->green & 0xff00) << 8) |
           (color->blue & 0xff00);

  gdk_pixbuf_fill (pixbuf, pixel);

  ctk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

static void
cafe_color_button_drag_begin (CtkWidget      *widget G_GNUC_UNUSED,
			      CdkDragContext *context,
			      gpointer        data)
{
  CafeColorButton *color_button = data;

  set_color_icon (context, &color_button->priv->color);
}

static void
cafe_color_button_drag_data_get (CtkWidget       *widget G_GNUC_UNUSED,
				CdkDragContext   *context G_GNUC_UNUSED,
				CtkSelectionData *selection_data,
				guint             info G_GNUC_UNUSED,
				guint             time G_GNUC_UNUSED,
				CafeColorButton   *color_button)
{
  guint16 dropped[4];

  dropped[0] = color_button->priv->color.red;
  dropped[1] = color_button->priv->color.green;
  dropped[2] = color_button->priv->color.blue;
  dropped[3] = color_button->priv->alpha;

  ctk_selection_data_set (selection_data, ctk_selection_data_get_target (selection_data),
			  16, (guchar *)dropped, 8);
}

static void
cafe_color_button_init (CafeColorButton *color_button)
{
  CtkWidget *alignment;
  CtkWidget *frame;
  PangoLayout *layout;
  PangoRectangle rect;

  _cafe_desktop_init_i18n ();

  /* Create the widgets */
  color_button->priv = cafe_color_button_get_instance_private (color_button);

  alignment = ctk_alignment_new (0.5, 0.5, 0.5, 1.0);
  ctk_container_set_border_width (CTK_CONTAINER (alignment), 1);
  ctk_container_add (CTK_CONTAINER (color_button), alignment);
  ctk_widget_show (alignment);

  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_ETCHED_OUT);
  ctk_container_add (CTK_CONTAINER (alignment), frame);
  ctk_widget_show (frame);

  /* Just some widget we can hook to expose-event on */
  color_button->priv->draw_area = ctk_alignment_new (0.5, 0.5, 0.0, 0.0);

  layout = ctk_widget_create_pango_layout (CTK_WIDGET (color_button), "Black");
  pango_layout_get_pixel_extents (layout, NULL, &rect);
  g_object_unref (layout);

  ctk_widget_set_size_request (color_button->priv->draw_area, rect.width - 2, rect.height - 2);
  g_signal_connect (color_button->priv->draw_area, "draw",
                    G_CALLBACK (draw), color_button);
  ctk_container_add (CTK_CONTAINER (frame), color_button->priv->draw_area);
  ctk_widget_show (color_button->priv->draw_area);

  color_button->priv->title = g_strdup (_("Pick a Color")); /* default title */

  /* Start with opaque black, alpha disabled */

  color_button->priv->color.red = 0;
  color_button->priv->color.green = 0;
  color_button->priv->color.blue = 0;
  color_button->priv->alpha = 65535;
  color_button->priv->use_alpha = FALSE;

  ctk_drag_dest_set (CTK_WIDGET (color_button),
                     CTK_DEST_DEFAULT_MOTION |
                     CTK_DEST_DEFAULT_HIGHLIGHT |
                     CTK_DEST_DEFAULT_DROP,
                     drop_types, 1, CDK_ACTION_COPY);
  ctk_drag_source_set (CTK_WIDGET(color_button),
                       CDK_BUTTON1_MASK|CDK_BUTTON3_MASK,
                       drop_types, 1,
                       CDK_ACTION_COPY);
  g_signal_connect (color_button, "drag-begin",
		    G_CALLBACK (cafe_color_button_drag_begin), color_button);
  g_signal_connect (color_button, "drag-data-received",
                    G_CALLBACK (cafe_color_button_drag_data_received), color_button);
  g_signal_connect (color_button, "drag-data-get",
                    G_CALLBACK (cafe_color_button_drag_data_get), color_button);
}

static void
cafe_color_button_finalize (GObject *object)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (object);

  if (color_button->priv->cs_dialog != NULL)
    ctk_widget_destroy (color_button->priv->cs_dialog);
  color_button->priv->cs_dialog = NULL;

  g_free (color_button->priv->title);
  color_button->priv->title = NULL;

  G_OBJECT_CLASS (cafe_color_button_parent_class)->finalize (object);
}


/**
 * cafe_color_button_new:
 *
 * Creates a new color button. This returns a widget in the form of
 * a small button containing a swatch representing the current selected
 * color. When the button is clicked, a color-selection dialog will open,
 * allowing the user to select a color. The swatch will be updated to reflect
 * the new color when the user finishes.
 *
 * Returns: a new color button.
 *
 * Since: 1.9.1
 */
CtkWidget *
cafe_color_button_new (void)
{
  return g_object_new (CAFE_TYPE_COLOR_BUTTON, NULL);
}

/**
 * cafe_color_button_new_with_color:
 * @color: A #CdkColor to set the current color with.
 *
 * Creates a new color button.
 *
 * Returns: a new color button.
 *
 * Since: 1.9.1
 */
CtkWidget *
cafe_color_button_new_with_color (const CdkColor *color)
{
  return g_object_new (CAFE_TYPE_COLOR_BUTTON, "color", color, NULL);
}

static void
dialog_ok_clicked (CtkWidget *widget G_GNUC_UNUSED,
		   gpointer   data)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (data);
  CafeColorSelection *color_selection;

  color_selection = CAFE_COLOR_SELECTION (CAFE_COLOR_SELECTION_DIALOG (color_button->priv->cs_dialog)->colorsel);

  cafe_color_selection_get_current_color (color_selection, &color_button->priv->color);
  color_button->priv->alpha = cafe_color_selection_get_current_alpha (color_selection);

  ctk_widget_hide (color_button->priv->cs_dialog);

  ctk_widget_queue_draw (color_button->priv->draw_area);

  g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);

  g_object_freeze_notify (G_OBJECT (color_button));
  g_object_notify (G_OBJECT (color_button), "color");
  g_object_notify (G_OBJECT (color_button), "alpha");
  g_object_thaw_notify (G_OBJECT (color_button));
}

static gboolean
dialog_destroy (CtkWidget *widget G_GNUC_UNUSED,
		gpointer   data)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (data);

  color_button->priv->cs_dialog = NULL;

  return FALSE;
}

static void
dialog_cancel_clicked (CtkWidget *widget G_GNUC_UNUSED,
		       gpointer   data)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (data);

  ctk_widget_hide (color_button->priv->cs_dialog);
}

static void
cafe_color_button_clicked (CtkButton *button)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (button);
  CafeColorSelectionDialog *color_dialog;

  /* if dialog already exists, make sure it's shown and raised */
  if (!color_button->priv->cs_dialog)
    {
      /* Create the dialog and connects its buttons */
      CtkWidget *parent;

      parent = ctk_widget_get_toplevel (CTK_WIDGET (color_button));

      color_button->priv->cs_dialog = cafe_color_selection_dialog_new (color_button->priv->title);

      color_dialog = CAFE_COLOR_SELECTION_DIALOG (color_button->priv->cs_dialog);

      if (ctk_widget_is_toplevel (parent) && CTK_IS_WINDOW (parent))
        {
          if (CTK_WINDOW (parent) != ctk_window_get_transient_for (CTK_WINDOW (color_dialog)))
 	    ctk_window_set_transient_for (CTK_WINDOW (color_dialog), CTK_WINDOW (parent));

	  ctk_window_set_modal (CTK_WINDOW (color_dialog),
				ctk_window_get_modal (CTK_WINDOW (parent)));
	}

      g_signal_connect (color_dialog->ok_button, "clicked",
                        G_CALLBACK (dialog_ok_clicked), color_button);
      g_signal_connect (color_dialog->cancel_button, "clicked",
			G_CALLBACK (dialog_cancel_clicked), color_button);
      g_signal_connect (color_dialog, "destroy",
                        G_CALLBACK (dialog_destroy), color_button);
    }

  color_dialog = CAFE_COLOR_SELECTION_DIALOG (color_button->priv->cs_dialog);

  cafe_color_selection_set_has_opacity_control (CAFE_COLOR_SELECTION (color_dialog->colorsel),
                                               color_button->priv->use_alpha);

  cafe_color_selection_set_has_palette (CAFE_COLOR_SELECTION (color_dialog->colorsel), TRUE);

  cafe_color_selection_set_previous_color (CAFE_COLOR_SELECTION (color_dialog->colorsel),
					  &color_button->priv->color);
  cafe_color_selection_set_previous_alpha (CAFE_COLOR_SELECTION (color_dialog->colorsel),
					  color_button->priv->alpha);

  cafe_color_selection_set_current_color (CAFE_COLOR_SELECTION (color_dialog->colorsel),
					 &color_button->priv->color);
  cafe_color_selection_set_current_alpha (CAFE_COLOR_SELECTION (color_dialog->colorsel),
					 color_button->priv->alpha);

  ctk_window_present (CTK_WINDOW (color_button->priv->cs_dialog));
}

/**
 * cafe_color_button_set_color:
 * @color_button: a #CafeColorButton.
 * @color: A #CdkColor to set the current color with.
 *
 * Sets the current color to be @color.
 *
 * Since: 1.9.1
 **/
void
cafe_color_button_set_color (CafeColorButton *color_button,
			    const CdkColor *color)
{
  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));
  g_return_if_fail (color != NULL);

  color_button->priv->color.red = color->red;
  color_button->priv->color.green = color->green;
  color_button->priv->color.blue = color->blue;

  ctk_widget_queue_draw (color_button->priv->draw_area);

  g_object_notify (G_OBJECT (color_button), "color");
}

/**
 * cafe_color_button_set_rgba:
 * @color_button: a #CafeColorButton.
 * @color: A #CdkRGBA to set the current color with.
 *
 * Sets the current color to be @color.
 *
 * Since: 1.9.1
 **/
void
cafe_color_button_set_rgba (CafeColorButton *color_button,
			    const CdkRGBA *color)
{
  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));
  g_return_if_fail (color != NULL);

  color_button->priv->color.red = color->red * 65535;
  color_button->priv->color.green = color->green * 65535;
  color_button->priv->color.blue = color->blue * 65535;
  color_button->priv->alpha = color->alpha * 65535;

  ctk_widget_queue_draw (color_button->priv->draw_area);

  g_object_notify (G_OBJECT (color_button), "color");
}

/**
 * cafe_color_button_set_alpha:
 * @color_button: a #CafeColorButton.
 * @alpha: an integer between 0 and 65535.
 *
 * Sets the current opacity to be @alpha.
 *
 * Since: 1.9.1
 **/
void
cafe_color_button_set_alpha (CafeColorButton *color_button,
			    guint16         alpha)
{
  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));

  color_button->priv->alpha = alpha;

  ctk_widget_queue_draw (color_button->priv->draw_area);

  g_object_notify (G_OBJECT (color_button), "alpha");
}

/**
 * cafe_color_button_get_color:
 * @color_button: a #CafeColorButton.
 * @color: a #CdkColor to fill in with the current color.
 *
 * Sets @color to be the current color in the #CafeColorButton widget.
 *
 * Since: 1.9.1
 **/
void
cafe_color_button_get_color (CafeColorButton *color_button,
			    CdkColor       *color)
{
  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));

  color->red = color_button->priv->color.red;
  color->green = color_button->priv->color.green;
  color->blue = color_button->priv->color.blue;
}

/**
 * cafe_color_button_get_rgba:
 * @color_button: a #CafeColorButton.
 * @color: a #CdkRGBA to fill in with the current color.
 *
 * Sets @color to be the current color in the #CafeColorButton widget.
 *
 * Since: 1.9.1
 **/
void
cafe_color_button_get_rgba (CafeColorButton *color_button,
			                      CdkRGBA         *color)
{
  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));

  color->red = color_button->priv->color.red / 65535.;
  color->green = color_button->priv->color.green / 65535.;
  color->blue = color_button->priv->color.blue / 65535.;
  color->alpha = color_button->priv->alpha / 65535.;
}

/**
 * cafe_color_button_get_alpha:
 * @color_button: a #CafeColorButton.
 *
 * Returns the current alpha value.
 *
 * Return value: an integer between 0 and 65535.
 *
 * Since: 1.9.1
 **/
guint16
cafe_color_button_get_alpha (CafeColorButton *color_button)
{
  g_return_val_if_fail (CAFE_IS_COLOR_BUTTON (color_button), 0);

  return color_button->priv->alpha;
}

/**
 * cafe_color_button_set_use_alpha:
 * @color_button: a #CafeColorButton.
 * @use_alpha: %TRUE if color button should use alpha channel, %FALSE if not.
 *
 * Sets whether or not the color button should use the alpha channel.
 *
 * Since: 1.9.1
 */
void
cafe_color_button_set_use_alpha (CafeColorButton *color_button,
				gboolean        use_alpha)
{
  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));

  use_alpha = (use_alpha != FALSE);

  if (color_button->priv->use_alpha != use_alpha)
    {
      color_button->priv->use_alpha = use_alpha;

      ctk_widget_queue_draw (color_button->priv->draw_area);

      g_object_notify (G_OBJECT (color_button), "use-alpha");
    }
}

/**
 * cafe_color_button_get_use_alpha:
 * @color_button: a #CafeColorButton.
 *
 * Does the color selection dialog use the alpha channel?
 *
 * Returns: %TRUE if the color sample uses alpha channel, %FALSE if not.
 *
 * Since: 1.9.1
 */
gboolean
cafe_color_button_get_use_alpha (CafeColorButton *color_button)
{
  g_return_val_if_fail (CAFE_IS_COLOR_BUTTON (color_button), FALSE);

  return color_button->priv->use_alpha;
}


/**
 * cafe_color_button_set_title:
 * @color_button: a #CafeColorButton
 * @title: String containing new window title.
 *
 * Sets the title for the color selection dialog.
 *
 * Since: 1.9.1
 */
void
cafe_color_button_set_title (CafeColorButton *color_button,
			    const gchar    *title)
{
  gchar *old_title;

  g_return_if_fail (CAFE_IS_COLOR_BUTTON (color_button));

  old_title = color_button->priv->title;
  color_button->priv->title = g_strdup (title);
  g_free (old_title);

  if (color_button->priv->cs_dialog)
    ctk_window_set_title (CTK_WINDOW (color_button->priv->cs_dialog),
			  color_button->priv->title);

  g_object_notify (G_OBJECT (color_button), "title");
}

/**
 * cafe_color_button_get_title:
 * @color_button: a #CafeColorButton
 *
 * Gets the title of the color selection dialog.
 *
 * Returns: An internal string, do not free the return value
 *
 * Since: 1.9.1
 */
const gchar *
cafe_color_button_get_title (CafeColorButton *color_button)
{
  g_return_val_if_fail (CAFE_IS_COLOR_BUTTON (color_button), NULL);

  return color_button->priv->title;
}

static void
cafe_color_button_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (object);

  switch (param_id)
    {
    case PROP_USE_ALPHA:
      cafe_color_button_set_use_alpha (color_button, g_value_get_boolean (value));
      break;
    case PROP_TITLE:
      cafe_color_button_set_title (color_button, g_value_get_string (value));
      break;
    case PROP_COLOR:
      cafe_color_button_set_color (color_button, g_value_get_boxed (value));
      break;
    case PROP_ALPHA:
      cafe_color_button_set_alpha (color_button, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
cafe_color_button_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  CafeColorButton *color_button = CAFE_COLOR_BUTTON (object);
  CdkColor color;

  switch (param_id)
    {
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, cafe_color_button_get_use_alpha (color_button));
      break;
    case PROP_TITLE:
      g_value_set_string (value, cafe_color_button_get_title (color_button));
      break;
    case PROP_COLOR:
      cafe_color_button_get_color (color_button, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_ALPHA:
      g_value_set_uint (value, cafe_color_button_get_alpha (color_button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}
