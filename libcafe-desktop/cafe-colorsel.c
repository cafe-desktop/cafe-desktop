/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the CTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include "private.h"
#include <math.h>
#include <string.h>

#include <cdk/cdkx.h>
#include <cdk/cdkkeysyms.h>
#include <ctk/ctk.h>
#include <glib/gi18n-lib.h>
#include "cafe-colorsel.h"

#define DEFAULT_COLOR_PALETTE "#ef2929:#fcaf3e:#fce94f:#8ae234:#729fcf:#ad7fa8:#e9b96e:#888a85:#eeeeec:#cc0000:#f57900:#edd400:#73d216:#3465a4:#75507b:#c17d11:#555753:#d3d7cf:#a40000:#ce5c00:#c4a000:#4e9a06:#204a87:#5c3566:#8f5902:#2e3436:#babdb6:#000000:#2e3436:#555753:#888a85:#babdb6:#d3d7cf:#eeeeec:#f3f3f3:#ffffff"

/* Number of elements in the custom palatte */
#define CTK_CUSTOM_PALETTE_WIDTH 9
#define CTK_CUSTOM_PALETTE_HEIGHT 4

#define CUSTOM_PALETTE_ENTRY_WIDTH   20
#define CUSTOM_PALETTE_ENTRY_HEIGHT  20

/* The cursor for the dropper */
#define DROPPER_WIDTH 16
#define DROPPER_HEIGHT 16
#define DROPPER_STRIDE 64
#define DROPPER_X_HOT 2
#define DROPPER_Y_HOT 16

#define SAMPLE_WIDTH  64
#define SAMPLE_HEIGHT 28
#define CHECK_SIZE 16
#define BIG_STEP 20

/* Conversion between 0->1 double and and guint16. See
 * scale_round() below for more general conversions
 */
#define SCALE(i) (i / 65535.)
#define UNSCALE(d) ((guint16)(d * 65535 + 0.5))
#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

enum {
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_HAS_PALETTE,
  PROP_HAS_OPACITY_CONTROL,
  PROP_CURRENT_COLOR,
  PROP_CURRENT_ALPHA,
  PROP_HEX_STRING
};

enum {
  COLORSEL_RED = 0,
  COLORSEL_GREEN = 1,
  COLORSEL_BLUE = 2,
  COLORSEL_OPACITY = 3,
  COLORSEL_HUE,
  COLORSEL_SATURATION,
  COLORSEL_VALUE,
  COLORSEL_NUM_CHANNELS
};

struct _CafeColorSelectionPrivate
{
  guint has_opacity : 1;
  guint has_palette : 1;
  guint changing : 1;
  guint default_set : 1;
  guint default_alpha_set : 1;
  guint has_grab : 1;

  gdouble color[COLORSEL_NUM_CHANNELS];
  gdouble old_color[COLORSEL_NUM_CHANNELS];

  CtkWidget *triangle_colorsel;
  CtkWidget *hue_spinbutton;
  CtkWidget *sat_spinbutton;
  CtkWidget *val_spinbutton;
  CtkWidget *red_spinbutton;
  CtkWidget *green_spinbutton;
  CtkWidget *blue_spinbutton;
  CtkWidget *opacity_slider;
  CtkWidget *opacity_label;
  CtkWidget *opacity_entry;
  CtkWidget *palette_frame;
  CtkWidget *hex_entry;

  /* The Palette code */
  CtkWidget *custom_palette [CTK_CUSTOM_PALETTE_WIDTH][CTK_CUSTOM_PALETTE_HEIGHT];

  /* The color_sample stuff */
  CtkWidget *sample_area;
  CtkWidget *old_sample;
  CtkWidget *cur_sample;
  CtkWidget *colorsel;

  /* Window for grabbing on */
  CtkWidget *dropper_grab_widget;
  guint32    grab_time;

  /* Connection to settings */
  gulong settings_connection;
};


static void cafe_color_selection_dispose		(GObject		 *object);
static void cafe_color_selection_finalize        (GObject		 *object);
static void update_color			(CafeColorSelection	 *colorsel);
static void cafe_color_selection_set_property    (GObject                 *object,
					         guint                    prop_id,
					         const GValue            *value,
					         GParamSpec              *pspec);
static void cafe_color_selection_get_property    (GObject                 *object,
					         guint                    prop_id,
					         GValue                  *value,
					         GParamSpec              *pspec);

static void cafe_color_selection_realize         (CtkWidget               *widget);
static void cafe_color_selection_unrealize       (CtkWidget               *widget);
static void cafe_color_selection_show_all        (CtkWidget               *widget);
static gboolean cafe_color_selection_grab_broken (CtkWidget               *widget,
						 CdkEventGrabBroken      *event);

static void     cafe_color_selection_set_palette_color   (CafeColorSelection *colorsel,
                                                         gint               index,
                                                         CdkColor          *color);
static void     default_noscreen_change_palette_func    (const CdkColor    *colors,
							 gint               n_colors);
static void     default_change_palette_func             (CdkScreen	   *screen,
							 const CdkColor    *colors,
							 gint               n_colors);
static void     make_control_relations                  (AtkObject         *atk_obj,
                                                         CtkWidget         *widget);
static void     make_all_relations                      (AtkObject         *atk_obj,
                                                         CafeColorSelectionPrivate *priv);

static void 	hsv_changed                             (CtkWidget         *hsv,
							 gpointer           data);
static void 	get_screen_color                        (CtkWidget         *button);
static void 	adjustment_changed                      (CtkAdjustment     *adjustment,
							 gpointer           data);
static void 	opacity_entry_changed                   (CtkWidget 	   *opacity_entry,
							 gpointer  	    data);
static void 	hex_changed                             (CtkWidget 	   *hex_entry,
							 gpointer  	    data);
static gboolean hex_focus_out                           (CtkWidget     	   *hex_entry,
							 CdkEventFocus 	   *event,
							 gpointer      	    data);
static void 	color_sample_new                        (CafeColorSelection *colorsel);
static void 	make_label_spinbutton     		(CafeColorSelection *colorsel,
	    				  		 CtkWidget        **spinbutton,
	    				  		 gchar             *text,
	    				  		 CtkWidget         *grid,
	    				  		 gint               i,
	    				  		 gint               j,
	    				  		 gint               channel_type,
	    				  		 const gchar       *tooltip);
static void 	make_palette_frame                      (CafeColorSelection *colorsel,
							 CtkWidget         *grid,
							 gint               i,
							 gint               j);
static void 	set_selected_palette                    (CafeColorSelection *colorsel,
							 int                x,
							 int                y);
static void 	set_focus_line_attributes               (CtkWidget 	   *drawing_area,
							 cairo_t   	   *cr,
							 gint      	   *focus_width);
static gboolean mouse_press 		     	       	(CtkWidget         *invisible,
                            		     	       	 CdkEventButton    *event,
                            		     	       	 gpointer           data);
static void  palette_change_notify_instance (GObject    *object,
					     GParamSpec *pspec,
					     gpointer    data);
static void update_palette (CafeColorSelection *colorsel);
static void shutdown_eyedropper (CtkWidget *widget);

static guint color_selection_signals[LAST_SIGNAL] = { 0 };

static CafeColorSelectionChangePaletteFunc noscreen_change_palette_hook = default_noscreen_change_palette_func;
static CafeColorSelectionChangePaletteWithScreenFunc change_palette_hook = default_change_palette_func;

static const guchar dropper_bits[] = {
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0U\35\35\35\373\40\40\40\350\0\0\0""6\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\"\0\0\0\265"
  "\0\0\0\373\233\233\233\377\233\233\233\377\0\0\0\351\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0y\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\375\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0U555\374\0\0\0\377\0\0\0\377\0\0\0\366\0\0\0t\0\0\0\15\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0=EEE\377\362"
  "\362\362\377HHH\377\0\0\0\377\0\0\0\332\0\0\0.\0\0\0\6\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0=ddd\370\352\352\352"
  "\377xxx\377\5\5\5\305\0\0\0\256\0\0\0q\0\0\0\15\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0=EEE\377\353\353\353\377\206"
  "\206\206\377\2\2\2\311\1\1\1=\0\0\0@\0\0\0-\0\0\0\4\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0=ddd\370\354\354\354\377xxx\377\2"
  "\2\2\311\0\0\0\77\0\0\0\21\0\0\0\14\0\0\0\10\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0<EEE\377\354\354\354\377\206\206\206\377"
  "\2\2\2\311\2\2\2""6\0\0\0\20\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\316\374\374\374\377yyy\377\2\2"
  "\2\311\0\0\0\77\0\0\0\20\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\32)))\351111\362\0\0\0\306\2\2\2""6"
  "\0\0\0\20\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\21\0\0\0j\0\0\0g\0\0\0<\0\0\0\20\0\0\0\1\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\11\0\0\0(\0\0\0\40\0\0\0\14\0\0\0\1\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\3\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"};

G_DEFINE_TYPE_WITH_PRIVATE (CafeColorSelection, cafe_color_selection, CTK_TYPE_BOX)

static void
cafe_color_selection_class_init (CafeColorSelectionClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = cafe_color_selection_finalize;
  gobject_class->set_property = cafe_color_selection_set_property;
  gobject_class->get_property = cafe_color_selection_get_property;

  gobject_class->dispose = cafe_color_selection_dispose;

  widget_class = CTK_WIDGET_CLASS (klass);
  widget_class->realize = cafe_color_selection_realize;
  widget_class->unrealize = cafe_color_selection_unrealize;
  widget_class->show_all = cafe_color_selection_show_all;
  widget_class->grab_broken_event = cafe_color_selection_grab_broken;

  g_object_class_install_property (gobject_class,
                                   PROP_HAS_OPACITY_CONTROL,
                                   g_param_spec_boolean ("has-opacity-control",
							 _("Has Opacity Control"),
							 _("Whether the color selector should allow setting opacity"),
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_HAS_PALETTE,
                                   g_param_spec_boolean ("has-palette",
							 _("Has palette"),
							 _("Whether a palette should be used"),
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_CURRENT_COLOR,
                                   g_param_spec_boxed ("current-color",
                                                       _("Current Color"),
                                                       _("The current color"),
                                                       CDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_CURRENT_ALPHA,
                                   g_param_spec_uint ("current-alpha",
						      _("Current Alpha"),
						      _("The current opacity value (0 fully transparent, 65535 fully opaque)"),
						      0, 65535, 65535,
						      G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_HEX_STRING,
                                   g_param_spec_string ("hex-string",
                                                       _("HEX String"),
                                                       _("The hexadecimal string of current color"),
                                                       "",
                                                       G_PARAM_READABLE));


  color_selection_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CafeColorSelectionClass, color_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
cafe_color_selection_init (CafeColorSelection *colorsel)
{
  CtkWidget *top_hbox;
  CtkWidget *top_right_vbox;
  CtkWidget *grid, *label, *hbox, *frame, *vbox, *button;
  CtkAdjustment *adjust;
  CtkWidget *picker_image;
  gint i, j;
  CafeColorSelectionPrivate *priv;
  AtkObject *atk_obj;
  GList *focus_chain = NULL;

  _cafe_desktop_init_i18n ();

  priv = colorsel->private_data = cafe_color_selection_get_instance_private (colorsel);
  priv->changing = FALSE;
  priv->default_set = FALSE;
  priv->default_alpha_set = FALSE;

  top_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_box_pack_start (CTK_BOX (colorsel), top_hbox, FALSE, FALSE, 0);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  priv->triangle_colorsel = ctk_hsv_new ();
  g_signal_connect (priv->triangle_colorsel, "changed",
                    G_CALLBACK (hsv_changed), colorsel);
  ctk_hsv_set_metrics (CTK_HSV (priv->triangle_colorsel), 174, 15);
  ctk_box_pack_start (CTK_BOX (top_hbox), vbox, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (vbox), priv->triangle_colorsel, FALSE, FALSE, 0);
  ctk_widget_set_tooltip_text (priv->triangle_colorsel,
                        _("Select the color you want from the outer ring. Select the darkness or lightness of that color using the inner triangle."));

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_box_pack_end (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  frame = ctk_frame_new (NULL);
  ctk_widget_set_size_request (frame, -1, 30);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
  color_sample_new (colorsel);
  ctk_container_add (CTK_CONTAINER (frame), priv->sample_area);
  ctk_box_pack_start (CTK_BOX (hbox), frame, TRUE, TRUE, 0);

  button = ctk_button_new ();

  ctk_widget_set_events (button, CDK_POINTER_MOTION_MASK | CDK_POINTER_MOTION_HINT_MASK);
  g_object_set_data (G_OBJECT (button), "COLORSEL", colorsel);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (get_screen_color), NULL);
  picker_image = ctk_image_new_from_icon_name ("ctk-color-picker", CTK_ICON_SIZE_BUTTON);
  ctk_container_add (CTK_CONTAINER (button), picker_image);
  ctk_widget_show (CTK_WIDGET (picker_image));
  ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  ctk_widget_set_tooltip_text (button,
                        _("Click the eyedropper, then click a color anywhere on your screen to select that color."));

  top_right_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_box_pack_start (CTK_BOX (top_hbox), top_right_vbox, FALSE, FALSE, 0);
  grid = ctk_grid_new ();
  ctk_box_pack_start (CTK_BOX (top_right_vbox), grid, FALSE, FALSE, 0);
  ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 12);

  make_label_spinbutton (colorsel, &priv->hue_spinbutton, _("_Hue:"), grid, 0, 0, COLORSEL_HUE,
                         _("Position on the color wheel."));
  ctk_spin_button_set_wrap (CTK_SPIN_BUTTON (priv->hue_spinbutton), TRUE);
  make_label_spinbutton (colorsel, &priv->sat_spinbutton, _("_Saturation:"), grid, 0, 1, COLORSEL_SATURATION,
                         _("\"Deepness\" of the color."));
  make_label_spinbutton (colorsel, &priv->val_spinbutton, _("_Value:"), grid, 0, 2, COLORSEL_VALUE,
                         _("Brightness of the color."));
  make_label_spinbutton (colorsel, &priv->red_spinbutton, _("_Red:"), grid, 6, 0, COLORSEL_RED,
                         _("Amount of red light in the color."));
  make_label_spinbutton (colorsel, &priv->green_spinbutton, _("_Green:"), grid, 6, 1, COLORSEL_GREEN,
                         _("Amount of green light in the color."));
  make_label_spinbutton (colorsel, &priv->blue_spinbutton, _("_Blue:"), grid, 6, 2, COLORSEL_BLUE,
                         _("Amount of blue light in the color."));
  ctk_grid_attach (CTK_GRID (grid), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL), 0, 3, 8, 1);

  priv->opacity_label = ctk_label_new_with_mnemonic (_("Op_acity:"));
  ctk_label_set_xalign (CTK_LABEL (priv->opacity_label), 0.0);
  ctk_grid_attach (CTK_GRID (grid), priv->opacity_label, 0, 4, 1, 1);
  adjust = CTK_ADJUSTMENT (ctk_adjustment_new (0.0, 0.0, 255.0, 1.0, 1.0, 0.0));
  g_object_set_data (G_OBJECT (adjust), "COLORSEL", colorsel);
  priv->opacity_slider = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, adjust);
  ctk_widget_set_tooltip_text (priv->opacity_slider,
                        _("Transparency of the color."));
  ctk_label_set_mnemonic_widget (CTK_LABEL (priv->opacity_label),
                                 priv->opacity_slider);
  ctk_scale_set_draw_value (CTK_SCALE (priv->opacity_slider), FALSE);
  g_signal_connect (adjust, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_OPACITY));
  ctk_grid_attach (CTK_GRID (grid), priv->opacity_slider, 1, 4, 6, 1);
  priv->opacity_entry = ctk_entry_new ();
  ctk_widget_set_tooltip_text (priv->opacity_entry,
                        _("Transparency of the color."));
  ctk_widget_set_size_request (priv->opacity_entry, 40, -1);

  g_signal_connect (priv->opacity_entry, "activate",
                    G_CALLBACK (opacity_entry_changed), colorsel);
  ctk_grid_attach (CTK_GRID (grid), priv->opacity_entry, 7, 4, 1, 1);

  label = ctk_label_new_with_mnemonic (_("Color _name:"));
  ctk_grid_attach (CTK_GRID (grid), label, 0, 5, 1, 1);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  priv->hex_entry = ctk_entry_new ();

  ctk_label_set_mnemonic_widget (CTK_LABEL (label), priv->hex_entry);

  g_signal_connect (priv->hex_entry, "activate",
                    G_CALLBACK (hex_changed), colorsel);

  g_signal_connect (priv->hex_entry, "focus-out-event",
                    G_CALLBACK (hex_focus_out), colorsel);

  ctk_widget_set_tooltip_text (priv->hex_entry,
                        _("You can enter an HTML-style hexadecimal color value, or simply a color name such as 'orange' in this entry."));

  ctk_entry_set_width_chars (CTK_ENTRY (priv->hex_entry), 7);
  ctk_grid_attach (CTK_GRID (grid), priv->hex_entry, 1, 5, 4, 1);

  focus_chain = g_list_append (focus_chain, priv->hue_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->sat_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->val_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->red_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->green_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->blue_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->opacity_slider);
  focus_chain = g_list_append (focus_chain, priv->opacity_entry);
  focus_chain = g_list_append (focus_chain, priv->hex_entry);
  ctk_container_set_focus_chain (CTK_CONTAINER (grid), focus_chain);
  g_list_free (focus_chain);

  /* Set up the palette */
  grid = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (grid), 1);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 1);
  for (i = 0; i < CTK_CUSTOM_PALETTE_WIDTH; i++)
    {
      for (j = 0; j < CTK_CUSTOM_PALETTE_HEIGHT; j++)
	{
	  make_palette_frame (colorsel, grid, i, j);
	}
    }
  set_selected_palette (colorsel, 0, 0);
  priv->palette_frame = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  label = ctk_label_new_with_mnemonic (_("_Palette:"));

  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_box_pack_start (CTK_BOX (priv->palette_frame), label, FALSE, FALSE, 0);

  ctk_label_set_mnemonic_widget (CTK_LABEL (label),
                                 priv->custom_palette[0][0]);

  ctk_box_pack_end (CTK_BOX (top_right_vbox), priv->palette_frame, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (priv->palette_frame), grid, FALSE, FALSE, 0);

  ctk_widget_show_all (top_hbox);

  /* hide unused stuff */

  if (priv->has_opacity == FALSE)
    {
      ctk_widget_hide (priv->opacity_label);
      ctk_widget_hide (priv->opacity_slider);
      ctk_widget_hide (priv->opacity_entry);
    }

  if (priv->has_palette == FALSE)
    {
      ctk_widget_hide (priv->palette_frame);
    }

  atk_obj = ctk_widget_get_accessible (priv->triangle_colorsel);
  if (CTK_IS_ACCESSIBLE (atk_obj))
    {
      atk_object_set_name (atk_obj, _("Color Wheel"));
      atk_object_set_role (ctk_widget_get_accessible (CTK_WIDGET (colorsel)), ATK_ROLE_COLOR_CHOOSER);
      make_all_relations (atk_obj, priv);
    }
}

/* GObject methods */
static void
cafe_color_selection_finalize (GObject *object)
{
  G_OBJECT_CLASS (cafe_color_selection_parent_class)->finalize (object);
}

static void
cafe_color_selection_set_property (GObject         *object,
				  guint            prop_id,
				  const GValue    *value,
				  GParamSpec      *pspec)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (object);

  switch (prop_id)
    {
    case PROP_HAS_OPACITY_CONTROL:
      cafe_color_selection_set_has_opacity_control (colorsel,
						   g_value_get_boolean (value));
      break;
    case PROP_HAS_PALETTE:
      cafe_color_selection_set_has_palette (colorsel,
					   g_value_get_boolean (value));
      break;
    case PROP_CURRENT_COLOR:
      cafe_color_selection_set_current_color (colorsel, g_value_get_boxed (value));
      break;
    case PROP_CURRENT_ALPHA:
      cafe_color_selection_set_current_alpha (colorsel, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
cafe_color_selection_get_property (GObject     *object,
				  guint        prop_id,
				  GValue      *value,
				  GParamSpec  *pspec)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (object);
  CafeColorSelectionPrivate *priv = colorsel->private_data;
  CdkColor color;

  switch (prop_id)
    {
    case PROP_HAS_OPACITY_CONTROL:
      g_value_set_boolean (value, cafe_color_selection_get_has_opacity_control (colorsel));
      break;
    case PROP_HAS_PALETTE:
      g_value_set_boolean (value, cafe_color_selection_get_has_palette (colorsel));
      break;
    case PROP_CURRENT_COLOR:
      cafe_color_selection_get_current_color (colorsel, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_CURRENT_ALPHA:
      g_value_set_uint (value, cafe_color_selection_get_current_alpha (colorsel));
      break;
    case PROP_HEX_STRING:
      g_value_set_string (value, ctk_editable_get_chars (CTK_EDITABLE (priv->hex_entry), 0, -1));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cafe_color_selection_dispose (GObject *object)
{
  CafeColorSelection *cselection = CAFE_COLOR_SELECTION (object);
  CafeColorSelectionPrivate *priv = cselection->private_data;

  if (priv->dropper_grab_widget)
    {
      ctk_widget_destroy (priv->dropper_grab_widget);
      priv->dropper_grab_widget = NULL;
    }

  G_OBJECT_CLASS (cafe_color_selection_parent_class)->dispose (object);
}

/* CtkWidget methods */

static void
cafe_color_selection_realize (CtkWidget *widget)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (widget);
  CafeColorSelectionPrivate *priv = colorsel->private_data;
  CtkSettings *settings = ctk_widget_get_settings (widget);

  priv->settings_connection =  g_signal_connect (settings,
						 "notify::ctk-color-palette",
						 G_CALLBACK (palette_change_notify_instance),
						 widget);
  update_palette (colorsel);

  CTK_WIDGET_CLASS (cafe_color_selection_parent_class)->realize (widget);
}

static void
cafe_color_selection_unrealize (CtkWidget *widget)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (widget);
  CafeColorSelectionPrivate *priv = colorsel->private_data;
  CtkSettings *settings = ctk_widget_get_settings (widget);

  g_signal_handler_disconnect (settings, priv->settings_connection);

  CTK_WIDGET_CLASS (cafe_color_selection_parent_class)->unrealize (widget);
}

/* We override show-all since we have internal widgets that
 * shouldn't be shown when you call show_all(), like the
 * palette and opacity sliders.
 */
static void
cafe_color_selection_show_all (CtkWidget *widget)
{
  ctk_widget_show (widget);
}

static gboolean
cafe_color_selection_grab_broken (CtkWidget          *widget,
				 CdkEventGrabBroken *event)
{
  shutdown_eyedropper (widget);

  return TRUE;
}

/*
 *
 * The Sample Color
 *
 */

static void color_sample_draw_sample (CafeColorSelection *colorsel, cairo_t *cr, int which);
static void color_sample_update_samples (CafeColorSelection *colorsel);

static void
set_color_internal (CafeColorSelection *colorsel,
		    gdouble           *color)
{
  CafeColorSelectionPrivate *priv;
  gint i;

  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->color[COLORSEL_RED] = color[0];
  priv->color[COLORSEL_GREEN] = color[1];
  priv->color[COLORSEL_BLUE] = color[2];
  priv->color[COLORSEL_OPACITY] = color[3];
  ctk_rgb_to_hsv (priv->color[COLORSEL_RED],
		  priv->color[COLORSEL_GREEN],
		  priv->color[COLORSEL_BLUE],
		  &priv->color[COLORSEL_HUE],
		  &priv->color[COLORSEL_SATURATION],
		  &priv->color[COLORSEL_VALUE]);
  if (priv->default_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
	priv->old_color[i] = priv->color[i];
    }
  priv->default_set = TRUE;
  priv->default_alpha_set = TRUE;
  update_color (colorsel);
}

static void
set_color_icon (CdkDragContext *context,
		gdouble        *colors)
{
  GdkPixbuf *pixbuf;
  guint32 pixel;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE,
			   8, 48, 32);

  pixel = (((UNSCALE (colors[COLORSEL_RED])   & 0xff00) << 16) |
	   ((UNSCALE (colors[COLORSEL_GREEN]) & 0xff00) << 8) |
	   ((UNSCALE (colors[COLORSEL_BLUE])  & 0xff00)));

  gdk_pixbuf_fill (pixbuf, pixel);

  ctk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

static void
color_sample_drag_begin (CtkWidget      *widget,
			 CdkDragContext *context,
			 gpointer        data)
{
  CafeColorSelection *colorsel = data;
  CafeColorSelectionPrivate *priv;
  gdouble *colsrc;

  priv = colorsel->private_data;

  if (widget == priv->old_sample)
    colsrc = priv->old_color;
  else
    colsrc = priv->color;

  set_color_icon (context, colsrc);
}

static void
color_sample_drag_end (CtkWidget      *widget,
		       CdkDragContext *context,
		       gpointer        data)
{
  g_object_set_data (G_OBJECT (widget), "ctk-color-selection-drag-window", NULL);
}

static void
color_sample_drop_handle (CtkWidget        *widget,
			  CdkDragContext   *context,
			  gint              x,
			  gint              y,
			  CtkSelectionData *selection_data,
			  guint             info,
			  guint             time,
			  gpointer          data)
{
  CafeColorSelection *colorsel = data;
  CafeColorSelectionPrivate *priv;
  guint16 *vals;
  gdouble color[4];
  priv = colorsel->private_data;

  /* This is currently a guint16 array of the format:
   * R
   * G
   * B
   * opacity
   */

  if (ctk_selection_data_get_length (selection_data) < 0)
    return;

  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (ctk_selection_data_get_length (selection_data) != 8)
    {
      g_warning ("Received invalid color data\n");
      return;
    }

  vals = (guint16 *) ctk_selection_data_get_data (selection_data);

  if (widget == priv->cur_sample)
    {
      color[0] = (gdouble)vals[0] / 0xffff;
      color[1] = (gdouble)vals[1] / 0xffff;
      color[2] = (gdouble)vals[2] / 0xffff;
      color[3] = (gdouble)vals[3] / 0xffff;

      set_color_internal (colorsel, color);
    }
}

static void
color_sample_drag_handle (CtkWidget        *widget,
			  CdkDragContext   *context,
			  CtkSelectionData *selection_data,
			  guint             info,
			  guint             time,
			  gpointer          data)
{
  CafeColorSelection *colorsel = data;
  CafeColorSelectionPrivate *priv;
  guint16 vals[4];
  gdouble *colsrc;

  priv = colorsel->private_data;

  if (widget == priv->old_sample)
    colsrc = priv->old_color;
  else
    colsrc = priv->color;

  vals[0] = colsrc[COLORSEL_RED] * 0xffff;
  vals[1] = colsrc[COLORSEL_GREEN] * 0xffff;
  vals[2] = colsrc[COLORSEL_BLUE] * 0xffff;
  vals[3] = priv->has_opacity ? colsrc[COLORSEL_OPACITY] * 0xffff : 0xffff;

  ctk_selection_data_set (selection_data,
			  cdk_atom_intern_static_string ("application/x-color"),
			  16, (guchar *)vals, 8);
}

/* which = 0 means draw old sample, which = 1 means draw new */
static void
color_sample_draw_sample (CafeColorSelection *colorsel, cairo_t *cr, int which)
{
  CtkWidget *da;
  gint x, y, wid, heig, goff;
  CafeColorSelectionPrivate *priv;
  CtkAllocation allocation;

  g_return_if_fail (colorsel != NULL);
  priv = colorsel->private_data;

  g_return_if_fail (priv->sample_area != NULL);
  if (!ctk_widget_is_drawable (priv->sample_area))
    return;

  if (which == 0)
    {
      da = priv->old_sample;
      goff = 0;
    }
  else
    {
      da = priv->cur_sample;
      ctk_widget_get_allocation (priv->old_sample, &allocation);
      goff =  allocation.width % 32;
    }

  ctk_widget_get_allocation (da, &allocation);
  wid = allocation.width;
  heig = allocation.height;

  /* Below needs tweaking for non-power-of-two */

  if (priv->has_opacity)
    {
      /* Draw checks in background */

      cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
      cairo_rectangle (cr, 0, 0, wid, heig);
      cairo_fill (cr);

      cairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
      for (x = goff & -CHECK_SIZE; x < goff + wid; x += CHECK_SIZE)
	for (y = 0; y < heig; y += CHECK_SIZE)
	  if ((x / CHECK_SIZE + y / CHECK_SIZE) % 2 == 0)
	    cairo_rectangle (cr, x - goff, y, CHECK_SIZE, CHECK_SIZE);
      cairo_fill (cr);
    }

  if (which == 0)
    {
      cairo_set_source_rgba (cr,
			     priv->old_color[COLORSEL_RED],
			     priv->old_color[COLORSEL_GREEN],
			     priv->old_color[COLORSEL_BLUE],
			     priv->has_opacity ?
			        priv->old_color[COLORSEL_OPACITY] : 1.0);
    }
  else
    {
      cairo_set_source_rgba (cr,
			     priv->color[COLORSEL_RED],
			     priv->color[COLORSEL_GREEN],
			     priv->color[COLORSEL_BLUE],
			     priv->has_opacity ?
			       priv->color[COLORSEL_OPACITY] : 1.0);
    }

  cairo_rectangle (cr, 0, 0, wid, heig);
  cairo_fill (cr);
}


static void
color_sample_update_samples (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv = colorsel->private_data;
  ctk_widget_queue_draw (priv->old_sample);
  ctk_widget_queue_draw (priv->cur_sample);
}

static gboolean
color_old_sample_draw (CtkWidget          *da,
                       cairo_t            *cr,
                       CafeColorSelection *colorsel)
{
  color_sample_draw_sample (colorsel, cr, 0);
  return FALSE;
}


static gboolean
color_cur_sample_draw (CtkWidget          *da,
                       cairo_t            *cr,
                       CafeColorSelection *colorsel)
{
  color_sample_draw_sample (colorsel, cr, 1);
  return FALSE;
}

static void
color_sample_setup_dnd (CafeColorSelection *colorsel, CtkWidget *sample)
{
  static const CtkTargetEntry targets[] = {
    { "application/x-color", 0 }
  };
  CafeColorSelectionPrivate *priv;
  priv = colorsel->private_data;

  ctk_drag_source_set (sample,
		       CDK_BUTTON1_MASK | CDK_BUTTON3_MASK,
		       targets, 1,
		       CDK_ACTION_COPY | CDK_ACTION_MOVE);

  g_signal_connect (sample, "drag-begin",
		    G_CALLBACK (color_sample_drag_begin),
		    colorsel);
  if (sample == priv->cur_sample)
    {

      ctk_drag_dest_set (sample,
			 CTK_DEST_DEFAULT_HIGHLIGHT |
			 CTK_DEST_DEFAULT_MOTION |
			 CTK_DEST_DEFAULT_DROP,
			 targets, 1,
			 CDK_ACTION_COPY);

      g_signal_connect (sample, "drag-end",
			G_CALLBACK (color_sample_drag_end),
			colorsel);
    }

  g_signal_connect (sample, "drag-data-get",
		    G_CALLBACK (color_sample_drag_handle),
		    colorsel);
  g_signal_connect (sample, "drag-data-received",
		    G_CALLBACK (color_sample_drop_handle),
		    colorsel);

}

static void
update_tooltips (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  priv = colorsel->private_data;

  if (priv->has_palette == TRUE)
    {
      ctk_widget_set_tooltip_text (priv->old_sample,
                            _("The previously-selected color, for comparison to the color you're selecting now. You can drag this color to a palette entry, or select this color as current by dragging it to the other color swatch alongside."));

      ctk_widget_set_tooltip_text (priv->cur_sample,
                            _("The color you've chosen. You can drag this color to a palette entry to save it for use in the future."));
    }
  else
    {
      ctk_widget_set_tooltip_text (priv->old_sample,
                            _("The previously-selected color, for comparison to the color you're selecting now."));

      ctk_widget_set_tooltip_text (priv->cur_sample,
                            _("The color you've chosen."));
    }
}

static void
color_sample_new (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  priv = colorsel->private_data;

  priv->sample_area = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  priv->old_sample = ctk_drawing_area_new ();
  priv->cur_sample = ctk_drawing_area_new ();

  ctk_box_pack_start (CTK_BOX (priv->sample_area), priv->old_sample,
		      TRUE, TRUE, 0);
  ctk_box_pack_start (CTK_BOX (priv->sample_area), priv->cur_sample,
		      TRUE, TRUE, 0);

  g_signal_connect (priv->old_sample, "draw",
		    G_CALLBACK (color_old_sample_draw),
		    colorsel);
  g_signal_connect (priv->cur_sample, "draw",
		    G_CALLBACK (color_cur_sample_draw),
		    colorsel);

  color_sample_setup_dnd (colorsel, priv->old_sample);
  color_sample_setup_dnd (colorsel, priv->cur_sample);

  update_tooltips (colorsel);

  ctk_widget_show_all (priv->sample_area);
}


/*
 *
 * The palette area code
 *
 */

static void
palette_get_color (CtkWidget *drawing_area, gdouble *color)
{
  gdouble *color_val;

  g_return_if_fail (color != NULL);

  color_val = g_object_get_data (G_OBJECT (drawing_area), "color_val");
  if (color_val == NULL)
    {
      /* Default to white for no good reason */
      color[0] = 1.0;
      color[1] = 1.0;
      color[2] = 1.0;
      color[3] = 1.0;
      return;
    }

  color[0] = color_val[0];
  color[1] = color_val[1];
  color[2] = color_val[2];
  color[3] = 1.0;
}

static void
palette_paint (CtkWidget    *drawing_area,
               cairo_t      *cr,
               gpointer      data)
{
  gint focus_width;
  CtkAllocation allocation;

  if (ctk_widget_get_window (drawing_area) == NULL)
    return;

  ctk_widget_get_allocation (drawing_area, &allocation);

  cdk_cairo_set_source_color (cr, &ctk_widget_get_style (drawing_area)->bg[CTK_STATE_NORMAL]);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  cairo_fill (cr);

  if (ctk_widget_has_focus (drawing_area))
    {
      set_focus_line_attributes (drawing_area, cr, &focus_width);

      cairo_rectangle (cr,
		       focus_width / 2., focus_width / 2.,
		       allocation.width - focus_width,
		       allocation.height - focus_width);
      cairo_stroke (cr);
    }
}

static void
set_focus_line_attributes (CtkWidget *drawing_area,
			   cairo_t   *cr,
			   gint      *focus_width)
{
  gdouble color[4];
  gint8 *dash_list;

  ctk_widget_style_get (drawing_area,
			"focus-line-width", focus_width,
			"focus-line-pattern", (gchar *)&dash_list,
			NULL);

  palette_get_color (drawing_area, color);

  if (INTENSITY (color[0], color[1], color[2]) > 0.5)
    cairo_set_source_rgb (cr, 0., 0., 0.);
  else
    cairo_set_source_rgb (cr, 1., 1., 1.);

  cairo_set_line_width (cr, *focus_width);

  if (dash_list[0])
    {
      gint n_dashes = strlen ((gchar *)dash_list);
      gdouble *dashes = g_new (gdouble, n_dashes);
      gdouble total_length = 0;
      gdouble dash_offset;
      gint i;

      for (i = 0; i < n_dashes; i++)
	{
	  dashes[i] = dash_list[i];
	  total_length += dash_list[i];
	}

      /* The dash offset here aligns the pattern to integer pixels
       * by starting the dash at the right side of the left border
       * Negative dash offsets in cairo don't work
       * (https://bugs.freedesktop.org/show_bug.cgi?id=2729)
       */
      dash_offset = - *focus_width / 2.;
      while (dash_offset < 0)
	dash_offset += total_length;

      cairo_set_dash (cr, dashes, n_dashes, dash_offset);
      g_free (dashes);
    }

  g_free (dash_list);
}

static void
palette_drag_begin (CtkWidget      *widget,
		    CdkDragContext *context,
		    gpointer        data)
{
  gdouble colors[4];

  palette_get_color (widget, colors);
  set_color_icon (context, colors);
}

static void
palette_drag_handle (CtkWidget        *widget,
		     CdkDragContext   *context,
		     CtkSelectionData *selection_data,
		     guint             info,
		     guint             time,
		     gpointer          data)
{
  guint16 vals[4];
  gdouble colsrc[4];

  palette_get_color (widget, colsrc);

  vals[0] = colsrc[COLORSEL_RED] * 0xffff;
  vals[1] = colsrc[COLORSEL_GREEN] * 0xffff;
  vals[2] = colsrc[COLORSEL_BLUE] * 0xffff;
  vals[3] = 0xffff;

  ctk_selection_data_set (selection_data,
			  cdk_atom_intern_static_string ("application/x-color"),
			  16, (guchar *)vals, 8);
}

static void
palette_drag_end (CtkWidget      *widget,
		  CdkDragContext *context,
		  gpointer        data)
{
  g_object_set_data (G_OBJECT (widget), "ctk-color-selection-drag-window", NULL);
}

static CdkColor *
get_current_colors (CafeColorSelection *colorsel)
{
  CdkColor *colors = NULL;
  gint n_colors = 0;

  cafe_color_selection_palette_from_string (DEFAULT_COLOR_PALETTE,
                                            &colors,
                                            &n_colors);

  /* make sure that we fill every slot */
  g_assert (n_colors == CTK_CUSTOM_PALETTE_WIDTH * CTK_CUSTOM_PALETTE_HEIGHT);

  return colors;
}

/* Changes the model color */
static void
palette_change_color (CtkWidget         *drawing_area,
                      CafeColorSelection *colorsel,
                      gdouble           *color)
{
  gint x, y;
  CafeColorSelectionPrivate *priv;
  CdkColor cdk_color;
  CdkColor *current_colors;
  CdkScreen *screen;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (CTK_IS_DRAWING_AREA (drawing_area));

  priv = colorsel->private_data;

  cdk_color.red = UNSCALE (color[0]);
  cdk_color.green = UNSCALE (color[1]);
  cdk_color.blue = UNSCALE (color[2]);
  cdk_color.pixel = 0;

  x = 0;
  y = 0;			/* Quiet GCC */
  while (x < CTK_CUSTOM_PALETTE_WIDTH)
    {
      y = 0;
      while (y < CTK_CUSTOM_PALETTE_HEIGHT)
        {
          if (priv->custom_palette[x][y] == drawing_area)
            goto out;

          ++y;
        }

      ++x;
    }

 out:

  g_assert (x < CTK_CUSTOM_PALETTE_WIDTH || y < CTK_CUSTOM_PALETTE_HEIGHT);

  current_colors = get_current_colors (colorsel);
  current_colors[y * CTK_CUSTOM_PALETTE_WIDTH + x] = cdk_color;

  screen = ctk_widget_get_screen (CTK_WIDGET (colorsel));
  if (change_palette_hook != default_change_palette_func)
    (* change_palette_hook) (screen, current_colors,
			     CTK_CUSTOM_PALETTE_WIDTH * CTK_CUSTOM_PALETTE_HEIGHT);
  else if (noscreen_change_palette_hook != default_noscreen_change_palette_func)
    {
      if (screen != cdk_screen_get_default ())
	g_warning ("cafe_color_selection_set_change_palette_hook used by widget is not on the default screen.");
      (* noscreen_change_palette_hook) (current_colors,
					CTK_CUSTOM_PALETTE_WIDTH * CTK_CUSTOM_PALETTE_HEIGHT);
    }
  else
    (* change_palette_hook) (screen, current_colors,
			     CTK_CUSTOM_PALETTE_WIDTH * CTK_CUSTOM_PALETTE_HEIGHT);

  g_free (current_colors);
}

static void
override_background_color (CtkWidget *widget,
                           CdkRGBA   *rgba)
{
  gchar          *css;
  CtkCssProvider *provider;

  provider = ctk_css_provider_new ();

  css = g_strdup_printf ("* { background-color: %s;}",
                         cdk_rgba_to_string (rgba));
  ctk_css_provider_load_from_data (provider, css, -1, NULL);
  g_free (css);

  ctk_style_context_add_provider (ctk_widget_get_style_context (widget),
                                  CTK_STYLE_PROVIDER (provider),
                                  CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
}

/* Changes the view color */
static void
palette_set_color (CtkWidget         *drawing_area,
		   CafeColorSelection *colorsel,
		   gdouble           *color)
{
  gdouble *new_color = g_new (double, 4);
  CdkRGBA  box_color;

  box_color.red = color[0];
  box_color.green = color[1];
  box_color.blue = color[2];
  box_color.alpha = 1;

  override_background_color (drawing_area, &box_color);

  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drawing_area), "color_set")) == 0)
    {
      static const CtkTargetEntry targets[] = {
	{ "application/x-color", 0 }
      };
      ctk_drag_source_set (drawing_area,
			   CDK_BUTTON1_MASK | CDK_BUTTON3_MASK,
			   targets, 1,
			   CDK_ACTION_COPY | CDK_ACTION_MOVE);

      g_signal_connect (drawing_area, "drag-begin",
			G_CALLBACK (palette_drag_begin),
			colorsel);
      g_signal_connect (drawing_area, "drag-data-get",
			G_CALLBACK (palette_drag_handle),
			colorsel);

      g_object_set_data (G_OBJECT (drawing_area), "color_set",
			 GINT_TO_POINTER (1));
    }

  new_color[0] = color[0];
  new_color[1] = color[1];
  new_color[2] = color[2];
  new_color[3] = 1.0;

  g_object_set_data_full (G_OBJECT (drawing_area), "color_val", new_color, (GDestroyNotify)g_free);
}

static gboolean
palette_draw (CtkWidget      *drawing_area,
		cairo_t        *cr,
		gpointer        data)
{
  if (ctk_widget_get_window (drawing_area) == NULL)
    return FALSE;

  palette_paint (drawing_area, cr, data);

  return FALSE;
}

static void
popup_position_func (CtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data)
{
  CtkWidget *widget;
  CtkRequisition req;
  gint root_x, root_y;
  CdkScreen *screen;
  CtkAllocation allocation;

  widget = CTK_WIDGET (user_data);

  g_return_if_fail (ctk_widget_get_realized (widget));

  cdk_window_get_origin (ctk_widget_get_window (widget), &root_x, &root_y);

  ctk_widget_get_preferred_size (CTK_WIDGET (menu), &req, NULL);

  /* Put corner of menu centered on color cell */
  ctk_widget_get_allocation (widget, &allocation);
  *x = root_x + allocation.width / 2;
  *y = root_y + allocation.height / 2;

  /* Ensure sanity */
  screen = ctk_widget_get_screen (widget);
  *x = CLAMP (*x, 0, MAX (0, WidthOfScreen (cdk_x11_screen_get_xscreen (screen)) - req.width));
  *y = CLAMP (*y, 0, MAX (0, HeightOfScreen (cdk_x11_screen_get_xscreen (screen)) - req.height));
}

static void
save_color_selected (CtkWidget *menuitem,
                     gpointer   data)
{
  CafeColorSelection *colorsel;
  CtkWidget *drawing_area;
  CafeColorSelectionPrivate *priv;

  drawing_area = CTK_WIDGET (data);

  colorsel = CAFE_COLOR_SELECTION (g_object_get_data (G_OBJECT (drawing_area),
                                                     "ctk-color-sel"));

  priv = colorsel->private_data;

  palette_change_color (drawing_area, colorsel, priv->color);
}

static void
do_popup (CafeColorSelection *colorsel,
          CtkWidget         *drawing_area,
          guint32            timestamp)
{
  CtkWidget *menu;
  CtkWidget *mi;

  g_object_set_data (G_OBJECT (drawing_area),
                     _("ctk-color-sel"),
                     colorsel);

  menu = ctk_menu_new ();

  mi = ctk_menu_item_new_with_mnemonic (_("_Save color here"));

  g_signal_connect (mi, "activate",
                    G_CALLBACK (save_color_selected),
                    drawing_area);

  ctk_menu_shell_append (CTK_MENU_SHELL (menu), mi);

  ctk_widget_show_all (mi);

  ctk_menu_popup (CTK_MENU (menu), NULL, NULL,
                  popup_position_func, drawing_area,
                  3, timestamp);
}


static gboolean
palette_enter (CtkWidget        *drawing_area,
	       CdkEventCrossing *event,
	       gpointer        data)
{
  g_object_set_data (G_OBJECT (drawing_area),
		     "ctk-colorsel-have-pointer",
		     GUINT_TO_POINTER (TRUE));

  return FALSE;
}

static gboolean
palette_leave (CtkWidget        *drawing_area,
	       CdkEventCrossing *event,
	       gpointer        data)
{
  g_object_set_data (G_OBJECT (drawing_area),
		     "ctk-colorsel-have-pointer",
		     NULL);

  return FALSE;
}

/* private function copied from ctk2/ctkmain.c */
static gboolean
_ctk_button_event_triggers_context_menu (CdkEventButton *event)
{
  if (event->type == CDK_BUTTON_PRESS)
    {
      if (event->button == 3 &&
          ! (event->state & (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK)))
        return TRUE;

#ifdef CDK_WINDOWING_QUARTZ
      if (event->button == 1 &&
          ! (event->state & (CDK_BUTTON2_MASK | CDK_BUTTON3_MASK)) &&
          (event->state & CDK_CONTROL_MASK))
        return TRUE;
#endif
    }

  return FALSE;
}

static gboolean
palette_press (CtkWidget      *drawing_area,
	       CdkEventButton *event,
	       gpointer        data)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (data);

  ctk_widget_grab_focus (drawing_area);

  if (_ctk_button_event_triggers_context_menu (event))
    {
      do_popup (colorsel, drawing_area, event->time);
      return TRUE;
    }

  return FALSE;
}

static gboolean
palette_release (CtkWidget      *drawing_area,
		 CdkEventButton *event,
		 gpointer        data)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (data);

  ctk_widget_grab_focus (drawing_area);

  if (event->button == 1 &&
      g_object_get_data (G_OBJECT (drawing_area),
			 "ctk-colorsel-have-pointer") != NULL)
    {
      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drawing_area), "color_set")) != 0)
        {
          gdouble color[4];
          palette_get_color (drawing_area, color);
          set_color_internal (colorsel, color);
        }
    }

  return FALSE;
}

static void
palette_drop_handle (CtkWidget        *widget,
		     CdkDragContext   *context,
		     gint              x,
		     gint              y,
		     CtkSelectionData *selection_data,
		     guint             info,
		     guint             time,
		     gpointer          data)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (data);
  guint16 *vals;
  gdouble color[4];

  if (ctk_selection_data_get_length (selection_data) < 0)
    return;

  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (ctk_selection_data_get_length (selection_data) != 8)
    {
      g_warning ("Received invalid color data\n");
      return;
    }

  vals = (guint16 *) ctk_selection_data_get_data (selection_data);

  color[0] = (gdouble)vals[0] / 0xffff;
  color[1] = (gdouble)vals[1] / 0xffff;
  color[2] = (gdouble)vals[2] / 0xffff;
  color[3] = (gdouble)vals[3] / 0xffff;
  palette_change_color (widget, colorsel, color);
  set_color_internal (colorsel, color);
}

static gint
palette_activate (CtkWidget   *widget,
		  CdkEventKey *event,
		  gpointer     data)
{
  /* should have a drawing area subclass with an activate signal */
  if ((event->keyval == CDK_KEY_space) ||
      (event->keyval == CDK_KEY_Return) ||
      (event->keyval == CDK_KEY_ISO_Enter) ||
      (event->keyval == CDK_KEY_KP_Enter) ||
      (event->keyval == CDK_KEY_KP_Space))
    {
      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "color_set")) != 0)
        {
          gdouble color[4];
          palette_get_color (widget, color);
          set_color_internal (CAFE_COLOR_SELECTION (data), color);
        }
      return TRUE;
    }

  return FALSE;
}

static gboolean
palette_popup (CtkWidget *widget,
               gpointer   data)
{
  CafeColorSelection *colorsel = CAFE_COLOR_SELECTION (data);

  do_popup (colorsel, widget, CDK_CURRENT_TIME);
  return TRUE;
}


static CtkWidget*
palette_new (CafeColorSelection *colorsel)
{
  static const CtkTargetEntry targets[] = {
    { "application/x-color", 0 }
  };

  CtkWidget *retval = ctk_drawing_area_new ();

  ctk_widget_set_can_focus (retval, TRUE);

  g_object_set_data (G_OBJECT (retval), "color_set", GINT_TO_POINTER (0));
  ctk_widget_set_events (retval, CDK_BUTTON_PRESS_MASK
                         | CDK_BUTTON_RELEASE_MASK
                         | CDK_EXPOSURE_MASK
                         | CDK_ENTER_NOTIFY_MASK
                         | CDK_LEAVE_NOTIFY_MASK);

  g_signal_connect (retval, "draw",
		    G_CALLBACK (palette_draw), colorsel);
  g_signal_connect (retval, "button-press-event",
		    G_CALLBACK (palette_press), colorsel);
  g_signal_connect (retval, "button-release-event",
		    G_CALLBACK (palette_release), colorsel);
  g_signal_connect (retval, "enter-notify-event",
		    G_CALLBACK (palette_enter), colorsel);
  g_signal_connect (retval, "leave-notify-event",
		    G_CALLBACK (palette_leave), colorsel);
  g_signal_connect (retval, "key-press-event",
		    G_CALLBACK (palette_activate), colorsel);
  g_signal_connect (retval, "popup-menu",
		    G_CALLBACK (palette_popup), colorsel);

  ctk_drag_dest_set (retval,
		     CTK_DEST_DEFAULT_HIGHLIGHT |
		     CTK_DEST_DEFAULT_MOTION |
		     CTK_DEST_DEFAULT_DROP,
		     targets, 1,
		     CDK_ACTION_COPY);

  g_signal_connect (retval, "drag-end",
                    G_CALLBACK (palette_drag_end), NULL);
  g_signal_connect (retval, "drag-data-received",
                    G_CALLBACK (palette_drop_handle), colorsel);

  ctk_widget_set_tooltip_text (retval,
                        _("Click this palette entry to make it the current color. To change this entry, drag a color swatch here or right-click it and select \"Save color here.\""));
  return retval;
}


/*
 *
 * The actual CafeColorSelection widget
 *
 */

static CdkCursor *
make_picker_cursor (CdkScreen *screen)
{
  CdkCursor *cursor;

  cursor = cdk_cursor_new_from_name (cdk_screen_get_display (screen),
				     "color-picker");

  if (!cursor)
    {
      GdkPixbuf *pixbuf;

      pixbuf = gdk_pixbuf_new_from_data (dropper_bits,
                                         GDK_COLORSPACE_RGB, TRUE, 8,
                                         DROPPER_WIDTH, DROPPER_HEIGHT,
                                         DROPPER_STRIDE,
                                         NULL, NULL);

      cursor = cdk_cursor_new_from_pixbuf (cdk_screen_get_display (screen),
                                           pixbuf,
                                           DROPPER_X_HOT, DROPPER_Y_HOT);
      g_object_unref (pixbuf);
    }

  return cursor;
}

static void
grab_color_at_mouse (CdkScreen *screen,
		     gint       x_root,
		     gint       y_root,
		     gpointer   data)
{
  GdkPixbuf *pixbuf;
  guchar *pixels;
  CafeColorSelection *colorsel = data;
  CafeColorSelectionPrivate *priv;
  CdkColor color;
  CdkWindow *root_window = cdk_screen_get_root_window (screen);

  priv = colorsel->private_data;

  pixbuf = gdk_pixbuf_get_from_window (root_window,
                                       x_root, y_root,
                                       1, 1);
  if (!pixbuf)
    {
      gint x, y;
      CdkDisplay *display = cdk_screen_get_display (screen);
      CdkWindow *window = cdk_display_get_window_at_pointer (display, &x, &y);
      if (!window)
	return;
      pixbuf = gdk_pixbuf_get_from_window (window,
                                           x, y,
                                           1, 1);
      if (!pixbuf)
	return;
    }
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  color.red = pixels[0] * 0x101;
  color.green = pixels[1] * 0x101;
  color.blue = pixels[2] * 0x101;
  g_object_unref (pixbuf);

  priv->color[COLORSEL_RED] = SCALE (color.red);
  priv->color[COLORSEL_GREEN] = SCALE (color.green);
  priv->color[COLORSEL_BLUE] = SCALE (color.blue);

  ctk_rgb_to_hsv (priv->color[COLORSEL_RED],
		  priv->color[COLORSEL_GREEN],
		  priv->color[COLORSEL_BLUE],
		  &priv->color[COLORSEL_HUE],
		  &priv->color[COLORSEL_SATURATION],
		  &priv->color[COLORSEL_VALUE]);

  update_color (colorsel);
}

static void
shutdown_eyedropper (CtkWidget *widget)
{
  CafeColorSelection *colorsel;
  CafeColorSelectionPrivate *priv;
  CdkDisplay *display = ctk_widget_get_display (widget);

  colorsel = CAFE_COLOR_SELECTION (widget);
  priv = colorsel->private_data;

  if (priv->has_grab)
    {
      cdk_display_keyboard_ungrab (display, priv->grab_time);
      cdk_display_pointer_ungrab (display, priv->grab_time);
      ctk_grab_remove (priv->dropper_grab_widget);

      priv->has_grab = FALSE;
    }
}

static void
mouse_motion (CtkWidget      *invisible,
	      CdkEventMotion *event,
	      gpointer        data)
{
  grab_color_at_mouse (cdk_event_get_screen ((CdkEvent *)event),
		       event->x_root, event->y_root, data);
}

static gboolean
mouse_release (CtkWidget      *invisible,
	       CdkEventButton *event,
	       gpointer        data)
{
  /* CafeColorSelection *colorsel = data; */

  if (event->button != 1)
    return FALSE;

  grab_color_at_mouse (cdk_event_get_screen ((CdkEvent *)event),
		       event->x_root, event->y_root, data);

  shutdown_eyedropper (CTK_WIDGET (data));

  g_signal_handlers_disconnect_by_func (invisible,
					mouse_motion,
					data);
  g_signal_handlers_disconnect_by_func (invisible,
					mouse_release,
					data);

  return TRUE;
}

/* Helper Functions */

static gboolean
key_press (CtkWidget   *invisible,
           CdkEventKey *event,
           gpointer     data)
{
  CdkDisplay *display = ctk_widget_get_display (invisible);
  CdkScreen *screen = cdk_event_get_screen ((CdkEvent *)event);
  guint state = event->state & ctk_accelerator_get_default_mod_mask ();
  gint x, y;
  gint dx, dy;

  cdk_display_get_pointer (display, NULL, &x, &y, NULL);

  dx = 0;
  dy = 0;

  switch (event->keyval)
    {
    case CDK_KEY_space:
    case CDK_KEY_Return:
    case CDK_KEY_ISO_Enter:
    case CDK_KEY_KP_Enter:
    case CDK_KEY_KP_Space:
      grab_color_at_mouse (screen, x, y, data);
      /* fall through */

    case CDK_KEY_Escape:
      shutdown_eyedropper (data);

      g_signal_handlers_disconnect_by_func (invisible,
					    mouse_press,
					    data);
      g_signal_handlers_disconnect_by_func (invisible,
					    key_press,
					    data);

      return TRUE;

#if defined CDK_WINDOWING_X11
    case CDK_KEY_Up:
    case CDK_KEY_KP_Up:
      dy = state == CDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case CDK_KEY_Down:
    case CDK_KEY_KP_Down:
      dy = state == CDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    case CDK_KEY_Left:
    case CDK_KEY_KP_Left:
      dx = state == CDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case CDK_KEY_Right:
    case CDK_KEY_KP_Right:
      dx = state == CDK_MOD1_MASK ? BIG_STEP : 1;
      break;
#endif

    default:
      return FALSE;
    }

  cdk_display_warp_pointer (display, screen, x + dx, y + dy);

  return TRUE;

}

static gboolean
mouse_press (CtkWidget      *invisible,
	     CdkEventButton *event,
	     gpointer        data)
{
  /* CafeColorSelection *colorsel = data; */

  if (event->type == CDK_BUTTON_PRESS &&
      event->button == 1)
    {
      g_signal_connect (invisible, "motion-notify-event",
                        G_CALLBACK (mouse_motion),
                        data);
      g_signal_connect (invisible, "button-release-event",
                        G_CALLBACK (mouse_release),
                        data);
      g_signal_handlers_disconnect_by_func (invisible,
					    mouse_press,
					    data);
      g_signal_handlers_disconnect_by_func (invisible,
					    key_press,
					    data);
      return TRUE;
    }

  return FALSE;
}

/* when the button is clicked */
static void
get_screen_color (CtkWidget *button)
{
  CafeColorSelection *colorsel = g_object_get_data (G_OBJECT (button), "COLORSEL");
  CafeColorSelectionPrivate *priv = colorsel->private_data;
  CdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (button));
  CdkCursor *picker_cursor;
  CdkGrabStatus grab_status;
  CtkWidget *grab_widget;

  guint32 time = ctk_get_current_event_time ();

  if (priv->dropper_grab_widget == NULL)
    {
      CtkWidget *toplevel;

      grab_widget = ctk_window_new (CTK_WINDOW_POPUP);
      ctk_window_set_screen (CTK_WINDOW (grab_widget), screen);
      ctk_window_resize (CTK_WINDOW (grab_widget), 1, 1);
      ctk_window_move (CTK_WINDOW (grab_widget), -100, -100);
      ctk_widget_show (grab_widget);

      ctk_widget_add_events (grab_widget,
                             CDK_BUTTON_RELEASE_MASK | CDK_BUTTON_PRESS_MASK | CDK_POINTER_MOTION_MASK);

      toplevel = ctk_widget_get_toplevel (CTK_WIDGET (colorsel));

      if (CTK_IS_WINDOW (toplevel))
	{
	  if (ctk_window_get_group (CTK_WINDOW (toplevel)))
	    ctk_window_group_add_window (ctk_window_get_group (CTK_WINDOW (toplevel)),
					 CTK_WINDOW (grab_widget));
	}

      priv->dropper_grab_widget = grab_widget;
    }

  if (cdk_keyboard_grab (ctk_widget_get_window (priv->dropper_grab_widget),
                         FALSE, time) != CDK_GRAB_SUCCESS)
    return;

  picker_cursor = make_picker_cursor (screen);
  grab_status = cdk_pointer_grab (ctk_widget_get_window (priv->dropper_grab_widget),
				  FALSE,
				  CDK_BUTTON_RELEASE_MASK | CDK_BUTTON_PRESS_MASK | CDK_POINTER_MOTION_MASK,
				  NULL,
				  picker_cursor,
				  time);
  g_object_unref (picker_cursor);

  if (grab_status != CDK_GRAB_SUCCESS)
    {
      cdk_display_keyboard_ungrab (ctk_widget_get_display (button), time);
      return;
    }

  ctk_grab_add (priv->dropper_grab_widget);
  priv->grab_time = time;
  priv->has_grab = TRUE;

  g_signal_connect (priv->dropper_grab_widget, "button-press-event",
                    G_CALLBACK (mouse_press), colorsel);
  g_signal_connect (priv->dropper_grab_widget, "key-press-event",
                    G_CALLBACK (key_press), colorsel);
}

static void
hex_changed (CtkWidget *hex_entry,
	     gpointer   data)
{
  CafeColorSelection *colorsel;
  CafeColorSelectionPrivate *priv;
  CdkColor color;
  gchar *text;

  colorsel = CAFE_COLOR_SELECTION (data);
  priv = colorsel->private_data;

  if (priv->changing)
    return;

  text = ctk_editable_get_chars (CTK_EDITABLE (priv->hex_entry), 0, -1);
  if (cdk_color_parse (text, &color))
    {
      priv->color[COLORSEL_RED] = CLAMP (color.red/65535.0, 0.0, 1.0);
      priv->color[COLORSEL_GREEN] = CLAMP (color.green/65535.0, 0.0, 1.0);
      priv->color[COLORSEL_BLUE] = CLAMP (color.blue/65535.0, 0.0, 1.0);
      ctk_rgb_to_hsv (priv->color[COLORSEL_RED],
		      priv->color[COLORSEL_GREEN],
		      priv->color[COLORSEL_BLUE],
		      &priv->color[COLORSEL_HUE],
		      &priv->color[COLORSEL_SATURATION],
		      &priv->color[COLORSEL_VALUE]);
      update_color (colorsel);
    }
  g_free (text);
}

static gboolean
hex_focus_out (CtkWidget     *hex_entry,
	       CdkEventFocus *event,
	       gpointer       data)
{
  hex_changed (hex_entry, data);

  return FALSE;
}

static void
hsv_changed (CtkWidget *hsv,
	     gpointer   data)
{
  CafeColorSelection *colorsel;
  CafeColorSelectionPrivate *priv;

  colorsel = CAFE_COLOR_SELECTION (data);
  priv = colorsel->private_data;

  if (priv->changing)
    return;

  ctk_hsv_get_color (CTK_HSV (hsv),
		     &priv->color[COLORSEL_HUE],
		     &priv->color[COLORSEL_SATURATION],
		     &priv->color[COLORSEL_VALUE]);
  ctk_hsv_to_rgb (priv->color[COLORSEL_HUE],
		  priv->color[COLORSEL_SATURATION],
		  priv->color[COLORSEL_VALUE],
		  &priv->color[COLORSEL_RED],
		  &priv->color[COLORSEL_GREEN],
		  &priv->color[COLORSEL_BLUE]);
  update_color (colorsel);
}

static void
adjustment_changed (CtkAdjustment *adjustment,
		    gpointer       data)
{
  CafeColorSelection *colorsel;
  CafeColorSelectionPrivate *priv;
  gdouble value;

  colorsel = CAFE_COLOR_SELECTION (g_object_get_data (G_OBJECT (adjustment), "COLORSEL"));
  priv = colorsel->private_data;
  value = ctk_adjustment_get_value (adjustment);

  if (priv->changing)
    return;

  switch (GPOINTER_TO_INT (data))
    {
    case COLORSEL_SATURATION:
    case COLORSEL_VALUE:
      priv->color[GPOINTER_TO_INT (data)] = value / 100;
      ctk_hsv_to_rgb (priv->color[COLORSEL_HUE],
		      priv->color[COLORSEL_SATURATION],
		      priv->color[COLORSEL_VALUE],
		      &priv->color[COLORSEL_RED],
		      &priv->color[COLORSEL_GREEN],
		      &priv->color[COLORSEL_BLUE]);
      break;
    case COLORSEL_HUE:
      priv->color[GPOINTER_TO_INT (data)] = value / 360;
      ctk_hsv_to_rgb (priv->color[COLORSEL_HUE],
		      priv->color[COLORSEL_SATURATION],
		      priv->color[COLORSEL_VALUE],
		      &priv->color[COLORSEL_RED],
		      &priv->color[COLORSEL_GREEN],
		      &priv->color[COLORSEL_BLUE]);
      break;
    case COLORSEL_RED:
    case COLORSEL_GREEN:
    case COLORSEL_BLUE:
      priv->color[GPOINTER_TO_INT (data)] = value / 255;

      ctk_rgb_to_hsv (priv->color[COLORSEL_RED],
		      priv->color[COLORSEL_GREEN],
		      priv->color[COLORSEL_BLUE],
		      &priv->color[COLORSEL_HUE],
		      &priv->color[COLORSEL_SATURATION],
		      &priv->color[COLORSEL_VALUE]);
      break;
    default:
      priv->color[GPOINTER_TO_INT (data)] = value / 255;
      break;
    }
  update_color (colorsel);
}

static void
opacity_entry_changed (CtkWidget *opacity_entry,
		       gpointer   data)
{
  CafeColorSelection *colorsel;
  CafeColorSelectionPrivate *priv;
  CtkAdjustment *adj;
  gchar *text;

  colorsel = CAFE_COLOR_SELECTION (data);
  priv = colorsel->private_data;

  if (priv->changing)
    return;

  text = ctk_editable_get_chars (CTK_EDITABLE (priv->opacity_entry), 0, -1);
  adj = ctk_range_get_adjustment (CTK_RANGE (priv->opacity_slider));
  ctk_adjustment_set_value (adj, g_strtod (text, NULL));

  update_color (colorsel);

  g_free (text);
}

static void
make_label_spinbutton (CafeColorSelection *colorsel,
		       CtkWidget        **spinbutton,
		       gchar             *text,
		       CtkWidget         *grid,
		       gint               i,
		       gint               j,
		       gint               channel_type,
                       const gchar       *tooltip)
{
  CtkWidget *label;
  CtkAdjustment *adjust;

  if (channel_type == COLORSEL_HUE)
    {
      adjust = CTK_ADJUSTMENT (ctk_adjustment_new (0.0, 0.0, 360.0, 1.0, 1.0, 0.0));
    }
  else if (channel_type == COLORSEL_SATURATION ||
	   channel_type == COLORSEL_VALUE)
    {
      adjust = CTK_ADJUSTMENT (ctk_adjustment_new (0.0, 0.0, 100.0, 1.0, 1.0, 0.0));
    }
  else
    {
      adjust = CTK_ADJUSTMENT (ctk_adjustment_new (0.0, 0.0, 255.0, 1.0, 1.0, 0.0));
    }
  g_object_set_data (G_OBJECT (adjust), "COLORSEL", colorsel);
  *spinbutton = ctk_spin_button_new (adjust, 10.0, 0);

  ctk_widget_set_tooltip_text (*spinbutton, tooltip);

  g_signal_connect (adjust, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (channel_type));
  label = ctk_label_new_with_mnemonic (text);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), *spinbutton);

  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_grid_attach (CTK_GRID (grid), label, i, j, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), *spinbutton, i+1, j, 1, 1);
}

static void
make_palette_frame (CafeColorSelection *colorsel,
		    CtkWidget         *grid,
		    gint               i,
		    gint               j)
{
  CtkWidget *frame;
  CafeColorSelectionPrivate *priv;

  priv = colorsel->private_data;
  frame = ctk_frame_new (NULL);
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_IN);
  priv->custom_palette[i][j] = palette_new (colorsel);
  ctk_widget_set_size_request (priv->custom_palette[i][j], CUSTOM_PALETTE_ENTRY_WIDTH, CUSTOM_PALETTE_ENTRY_HEIGHT);
  ctk_container_add (CTK_CONTAINER (frame), priv->custom_palette[i][j]);
  ctk_widget_set_hexpand (frame, TRUE);
  ctk_grid_attach (CTK_GRID (grid), frame, i, j, 1, 1);
}

/* Set the palette entry [x][y] to be the currently selected one. */
static void
set_selected_palette (CafeColorSelection *colorsel, int x, int y)
{
  CafeColorSelectionPrivate *priv = colorsel->private_data;

  ctk_widget_grab_focus (priv->custom_palette[x][y]);
}

static double
scale_round (double val, double factor)
{
  val = floor (val * factor + 0.5);
  val = MAX (val, 0);
  val = MIN (val, factor);
  return val;
}

static void
update_color (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv = colorsel->private_data;
  gchar entryval[12];
  gchar opacity_text[32];
  gchar *ptr;

  priv->changing = TRUE;
  color_sample_update_samples (colorsel);

  ctk_hsv_set_color (CTK_HSV (priv->triangle_colorsel),
		     priv->color[COLORSEL_HUE],
		     priv->color[COLORSEL_SATURATION],
		     priv->color[COLORSEL_VALUE]);
  ctk_adjustment_set_value (ctk_spin_button_get_adjustment
			    (CTK_SPIN_BUTTON (priv->hue_spinbutton)),
			    scale_round (priv->color[COLORSEL_HUE], 360));
  ctk_adjustment_set_value (ctk_spin_button_get_adjustment
			    (CTK_SPIN_BUTTON (priv->sat_spinbutton)),
			    scale_round (priv->color[COLORSEL_SATURATION], 100));
  ctk_adjustment_set_value (ctk_spin_button_get_adjustment
			    (CTK_SPIN_BUTTON (priv->val_spinbutton)),
			    scale_round (priv->color[COLORSEL_VALUE], 100));
  ctk_adjustment_set_value (ctk_spin_button_get_adjustment
			    (CTK_SPIN_BUTTON (priv->red_spinbutton)),
			    scale_round (priv->color[COLORSEL_RED], 255));
  ctk_adjustment_set_value (ctk_spin_button_get_adjustment
			    (CTK_SPIN_BUTTON (priv->green_spinbutton)),
			    scale_round (priv->color[COLORSEL_GREEN], 255));
  ctk_adjustment_set_value (ctk_spin_button_get_adjustment
			    (CTK_SPIN_BUTTON (priv->blue_spinbutton)),
			    scale_round (priv->color[COLORSEL_BLUE], 255));
  ctk_adjustment_set_value (ctk_range_get_adjustment
			    (CTK_RANGE (priv->opacity_slider)),
			    scale_round (priv->color[COLORSEL_OPACITY], 255));

  g_snprintf (opacity_text, 32, "%.0f", scale_round (priv->color[COLORSEL_OPACITY], 255));
  ctk_entry_set_text (CTK_ENTRY (priv->opacity_entry), opacity_text);

  g_snprintf (entryval, 11, "#%2X%2X%2X",
	      (guint) (scale_round (priv->color[COLORSEL_RED], 255)),
	      (guint) (scale_round (priv->color[COLORSEL_GREEN], 255)),
	      (guint) (scale_round (priv->color[COLORSEL_BLUE], 255)));

  for (ptr = entryval; *ptr; ptr++)
    if (*ptr == ' ')
      *ptr = '0';
  ctk_entry_set_text (CTK_ENTRY (priv->hex_entry), entryval);
  priv->changing = FALSE;

  g_object_ref (colorsel);

  g_signal_emit (colorsel, color_selection_signals[COLOR_CHANGED], 0);

  g_object_freeze_notify (G_OBJECT (colorsel));
  g_object_notify (G_OBJECT (colorsel), "current-color");
  g_object_notify (G_OBJECT (colorsel), "current-alpha");
  g_object_thaw_notify (G_OBJECT (colorsel));

  g_object_unref (colorsel);
}

static void
update_palette (CafeColorSelection *colorsel)
{
  CdkColor *current_colors;
  gint i, j;

  current_colors = get_current_colors (colorsel);

  for (i = 0; i < CTK_CUSTOM_PALETTE_HEIGHT; i++)
    {
      for (j = 0; j < CTK_CUSTOM_PALETTE_WIDTH; j++)
	{
          gint index;

          index = i * CTK_CUSTOM_PALETTE_WIDTH + j;

          cafe_color_selection_set_palette_color (colorsel,
                                                 index,
                                                 &current_colors[index]);
	}
    }

  g_free (current_colors);
}

static void
palette_change_notify_instance (GObject    *object,
                                GParamSpec *pspec,
                                gpointer    data)
{
  update_palette (CAFE_COLOR_SELECTION (data));
}

static void
default_noscreen_change_palette_func (const CdkColor *colors,
				      gint            n_colors)
{
  default_change_palette_func (cdk_screen_get_default (), colors, n_colors);
}

static void
default_change_palette_func (CdkScreen	    *screen,
			     const CdkColor *colors,
                             gint            n_colors)
{
  gchar *str;

  str = cafe_color_selection_palette_to_string (colors, n_colors);

  g_object_set (G_OBJECT (ctk_settings_get_for_screen (screen)), "ctk-color-palette", str, NULL);

  g_free (str);
}

/**
 * cafe_color_selection_new:
 *
 * Creates a new CafeColorSelection.
 *
 * Return value: a new #CafeColorSelection
 **/
CtkWidget *
cafe_color_selection_new (void)
{
  CafeColorSelection *colorsel;
  CafeColorSelectionPrivate *priv;
  gdouble color[4];
  color[0] = 1.0;
  color[1] = 1.0;
  color[2] = 1.0;
  color[3] = 1.0;

  colorsel = g_object_new (CAFE_TYPE_COLOR_SELECTION, "orientation", CTK_ORIENTATION_VERTICAL, NULL);
  priv = colorsel->private_data;
  set_color_internal (colorsel, color);
  cafe_color_selection_set_has_opacity_control (colorsel, TRUE);

  /* We want to make sure that default_set is FALSE */
  /* This way the user can still set it */
  priv->default_set = FALSE;
  priv->default_alpha_set = FALSE;

  return CTK_WIDGET (colorsel);
}

/**
 * cafe_color_selection_get_has_opacity_control:
 * @colorsel: a #CafeColorSelection.
 *
 * Determines whether the colorsel has an opacity control.
 *
 * Return value: %TRUE if the @colorsel has an opacity control.  %FALSE if it does't.
 **/
gboolean
cafe_color_selection_get_has_opacity_control (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  g_return_val_if_fail (CAFE_IS_COLOR_SELECTION (colorsel), FALSE);

  priv = colorsel->private_data;

  return priv->has_opacity;
}

/**
 * cafe_color_selection_set_has_opacity_control:
 * @colorsel: a #CafeColorSelection.
 * @has_opacity: %TRUE if @colorsel can set the opacity, %FALSE otherwise.
 *
 * Sets the @colorsel to use or not use opacity.
 *
 **/
void
cafe_color_selection_set_has_opacity_control (CafeColorSelection *colorsel,
					     gboolean           has_opacity)
{
  CafeColorSelectionPrivate *priv;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));

  priv = colorsel->private_data;
  has_opacity = has_opacity != FALSE;

  if (priv->has_opacity != has_opacity)
    {
      priv->has_opacity = has_opacity;
      if (has_opacity)
	{
	  ctk_widget_show (priv->opacity_slider);
	  ctk_widget_show (priv->opacity_label);
	  ctk_widget_show (priv->opacity_entry);
	}
      else
	{
	  ctk_widget_hide (priv->opacity_slider);
	  ctk_widget_hide (priv->opacity_label);
	  ctk_widget_hide (priv->opacity_entry);
	}
      color_sample_update_samples (colorsel);

      g_object_notify (G_OBJECT (colorsel), "has-opacity-control");
    }
}

/**
 * cafe_color_selection_get_has_palette:
 * @colorsel: a #CafeColorSelection.
 *
 * Determines whether the color selector has a color palette.
 *
 * Return value: %TRUE if the selector has a palette.  %FALSE if it hasn't.
 **/
gboolean
cafe_color_selection_get_has_palette (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  g_return_val_if_fail (CAFE_IS_COLOR_SELECTION (colorsel), FALSE);

  priv = colorsel->private_data;

  return priv->has_palette;
}

/**
 * cafe_color_selection_set_has_palette:
 * @colorsel: a #CafeColorSelection.
 * @has_palette: %TRUE if palette is to be visible, %FALSE otherwise.
 *
 * Shows and hides the palette based upon the value of @has_palette.
 *
 **/
void
cafe_color_selection_set_has_palette (CafeColorSelection *colorsel,
				     gboolean           has_palette)
{
  CafeColorSelectionPrivate *priv;
  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));

  priv = colorsel->private_data;
  has_palette = has_palette != FALSE;

  if (priv->has_palette != has_palette)
    {
      priv->has_palette = has_palette;
      if (has_palette)
	ctk_widget_show (priv->palette_frame);
      else
	ctk_widget_hide (priv->palette_frame);

      update_tooltips (colorsel);

      g_object_notify (G_OBJECT (colorsel), "has-palette");
    }
}

/**
 * cafe_color_selection_set_current_color:
 * @colorsel: a #CafeColorSelection.
 * @color: A #CdkColor to set the current color with.
 *
 * Sets the current color to be @color.  The first time this is called, it will
 * also set the original color to be @color too.
 **/
void
cafe_color_selection_set_current_color (CafeColorSelection *colorsel,
				       const CdkColor    *color)
{
  CafeColorSelectionPrivate *priv;
  gint i;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);

  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->color[COLORSEL_RED] = SCALE (color->red);
  priv->color[COLORSEL_GREEN] = SCALE (color->green);
  priv->color[COLORSEL_BLUE] = SCALE (color->blue);
  ctk_rgb_to_hsv (priv->color[COLORSEL_RED],
		  priv->color[COLORSEL_GREEN],
		  priv->color[COLORSEL_BLUE],
		  &priv->color[COLORSEL_HUE],
		  &priv->color[COLORSEL_SATURATION],
		  &priv->color[COLORSEL_VALUE]);
  if (priv->default_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
	priv->old_color[i] = priv->color[i];
    }
  priv->default_set = TRUE;
  update_color (colorsel);
}

/**
 * cafe_color_selection_set_current_alpha:
 * @colorsel: a #CafeColorSelection.
 * @alpha: an integer between 0 and 65535.
 *
 * Sets the current opacity to be @alpha.  The first time this is called, it will
 * also set the original opacity to be @alpha too.
 **/
void
cafe_color_selection_set_current_alpha (CafeColorSelection *colorsel,
				       guint16            alpha)
{
  CafeColorSelectionPrivate *priv;
  gint i;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));

  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->color[COLORSEL_OPACITY] = SCALE (alpha);
  if (priv->default_alpha_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
	priv->old_color[i] = priv->color[i];
    }
  priv->default_alpha_set = TRUE;
  update_color (colorsel);
}

/**
 * cafe_color_selection_set_color:
 * @colorsel: a #CafeColorSelection.
 * @color: an array of 4 doubles specifying the red, green, blue and opacity
 *   to set the current color to.
 *
 * Sets the current color to be @color.  The first time this is called, it will
 * also set the original color to be @color too.
 *
 * Deprecated: 2.0: Use cafe_color_selection_set_current_color() instead.
 **/
void
cafe_color_selection_set_color (CafeColorSelection    *colorsel,
			       gdouble              *color)
{
  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));

  set_color_internal (colorsel, color);
}

/**
 * cafe_color_selection_get_current_color:
 * @colorsel: a #CafeColorSelection.
 * @color: (out): a #CdkColor to fill in with the current color.
 *
 * Sets @color to be the current color in the CafeColorSelection widget.
 **/
void
cafe_color_selection_get_current_color (CafeColorSelection *colorsel,
				       CdkColor          *color)
{
  CafeColorSelectionPrivate *priv;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);

  priv = colorsel->private_data;
  color->red = UNSCALE (priv->color[COLORSEL_RED]);
  color->green = UNSCALE (priv->color[COLORSEL_GREEN]);
  color->blue = UNSCALE (priv->color[COLORSEL_BLUE]);
}

/**
 * cafe_color_selection_get_current_alpha:
 * @colorsel: a #CafeColorSelection.
 *
 * Returns the current alpha value.
 *
 * Return value: an integer between 0 and 65535.
 **/
guint16
cafe_color_selection_get_current_alpha (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  g_return_val_if_fail (CAFE_IS_COLOR_SELECTION (colorsel), 0);

  priv = colorsel->private_data;
  return priv->has_opacity ? UNSCALE (priv->color[COLORSEL_OPACITY]) : 65535;
}

/**
 * cafe_color_selection_get_color:
 * @colorsel: a #CafeColorSelection.
 * @color: an array of 4 #gdouble to fill in with the current color.
 *
 * Sets @color to be the current color in the CafeColorSelection widget.
 *
 * Deprecated: 2.0: Use cafe_color_selection_get_current_color() instead.
 **/
void
cafe_color_selection_get_color (CafeColorSelection *colorsel,
			       gdouble           *color)
{
  CafeColorSelectionPrivate *priv;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));

  priv = colorsel->private_data;
  color[0] = priv->color[COLORSEL_RED];
  color[1] = priv->color[COLORSEL_GREEN];
  color[2] = priv->color[COLORSEL_BLUE];
  color[3] = priv->has_opacity ? priv->color[COLORSEL_OPACITY] : 65535;
}

/**
 * cafe_color_selection_set_previous_color:
 * @colorsel: a #CafeColorSelection.
 * @color: a #CdkColor to set the previous color with.
 *
 * Sets the 'previous' color to be @color.  This function should be called with
 * some hesitations, as it might seem confusing to have that color change.
 * Calling cafe_color_selection_set_current_color() will also set this color the first
 * time it is called.
 **/
void
cafe_color_selection_set_previous_color (CafeColorSelection *colorsel,
					const CdkColor    *color)
{
  CafeColorSelectionPrivate *priv;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);

  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->old_color[COLORSEL_RED] = SCALE (color->red);
  priv->old_color[COLORSEL_GREEN] = SCALE (color->green);
  priv->old_color[COLORSEL_BLUE] = SCALE (color->blue);
  ctk_rgb_to_hsv (priv->old_color[COLORSEL_RED],
		  priv->old_color[COLORSEL_GREEN],
		  priv->old_color[COLORSEL_BLUE],
		  &priv->old_color[COLORSEL_HUE],
		  &priv->old_color[COLORSEL_SATURATION],
		  &priv->old_color[COLORSEL_VALUE]);
  color_sample_update_samples (colorsel);
  priv->default_set = TRUE;
  priv->changing = FALSE;
}

/**
 * cafe_color_selection_set_previous_alpha:
 * @colorsel: a #CafeColorSelection.
 * @alpha: an integer between 0 and 65535.
 *
 * Sets the 'previous' alpha to be @alpha.  This function should be called with
 * some hesitations, as it might seem confusing to have that alpha change.
 **/
void
cafe_color_selection_set_previous_alpha (CafeColorSelection *colorsel,
					guint16            alpha)
{
  CafeColorSelectionPrivate *priv;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));

  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->old_color[COLORSEL_OPACITY] = SCALE (alpha);
  color_sample_update_samples (colorsel);
  priv->default_alpha_set = TRUE;
  priv->changing = FALSE;
}


/**
 * cafe_color_selection_get_previous_color:
 * @colorsel: a #CafeColorSelection.
 * @color: (out): a #CdkColor to fill in with the original color value.
 *
 * Fills @color in with the original color value.
 **/
void
cafe_color_selection_get_previous_color (CafeColorSelection *colorsel,
					CdkColor           *color)
{
  CafeColorSelectionPrivate *priv;

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);

  priv = colorsel->private_data;
  color->red = UNSCALE (priv->old_color[COLORSEL_RED]);
  color->green = UNSCALE (priv->old_color[COLORSEL_GREEN]);
  color->blue = UNSCALE (priv->old_color[COLORSEL_BLUE]);
}

/**
 * cafe_color_selection_get_previous_alpha:
 * @colorsel: a #CafeColorSelection.
 *
 * Returns the previous alpha value.
 *
 * Return value: an integer between 0 and 65535.
 **/
guint16
cafe_color_selection_get_previous_alpha (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  g_return_val_if_fail (CAFE_IS_COLOR_SELECTION (colorsel), 0);

  priv = colorsel->private_data;
  return priv->has_opacity ? UNSCALE (priv->old_color[COLORSEL_OPACITY]) : 65535;
}

/**
 * cafe_color_selection_set_palette_color:
 * @colorsel: a #CafeColorSelection.
 * @index: the color index of the palette.
 * @color: A #CdkColor to set the palette with.
 *
 * Sets the palette located at @index to have @color as its color.
 *
 **/
static void
cafe_color_selection_set_palette_color (CafeColorSelection   *colorsel,
				       gint                 index,
				       CdkColor            *color)
{
  CafeColorSelectionPrivate *priv;
  gint x, y;
  gdouble col[3];

  g_return_if_fail (CAFE_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (index >= 0  && index < CTK_CUSTOM_PALETTE_WIDTH*CTK_CUSTOM_PALETTE_HEIGHT);

  x = index % CTK_CUSTOM_PALETTE_WIDTH;
  y = index / CTK_CUSTOM_PALETTE_WIDTH;

  priv = colorsel->private_data;
  col[0] = SCALE (color->red);
  col[1] = SCALE (color->green);
  col[2] = SCALE (color->blue);

  palette_set_color (priv->custom_palette[x][y], colorsel, col);
}

/**
 * cafe_color_selection_is_adjusting:
 * @colorsel: a #CafeColorSelection.
 *
 * Gets the current state of the @colorsel.
 *
 * Return value: %TRUE if the user is currently dragging a color around, and %FALSE
 * if the selection has stopped.
 **/
gboolean
cafe_color_selection_is_adjusting (CafeColorSelection *colorsel)
{
  CafeColorSelectionPrivate *priv;

  g_return_val_if_fail (CAFE_IS_COLOR_SELECTION (colorsel), FALSE);

  priv = colorsel->private_data;

  return (ctk_hsv_is_adjusting (CTK_HSV (priv->triangle_colorsel)));
}


/**
 * cafe_color_selection_palette_from_string:
 * @str: a string encoding a color palette.
 * @colors: (out) (array length=n_colors): return location for allocated
 *          array of #CdkColor.
 * @n_colors: return location for length of array.
 *
 * Parses a color palette string; the string is a colon-separated
 * list of color names readable by cdk_color_parse().
 *
 * Return value: %TRUE if a palette was successfully parsed.
 **/
gboolean
cafe_color_selection_palette_from_string (const gchar *str,
                                         CdkColor   **colors,
                                         gint        *n_colors)
{
  CdkColor *retval;
  gint count;
  gchar *p;
  gchar *start;
  gchar *copy;

  count = 0;
  retval = NULL;
  copy = g_strdup (str);

  start = copy;
  p = copy;
  while (TRUE)
    {
      if (*p == ':' || *p == '\0')
        {
          gboolean done = TRUE;

          if (start == p)
            {
              goto failed; /* empty entry */
            }

          if (*p)
            {
              *p = '\0';
              done = FALSE;
            }

          retval = g_renew (CdkColor, retval, count + 1);
          if (!cdk_color_parse (start, retval + count))
            {
              goto failed;
            }

          ++count;

          if (done)
            break;
          else
            start = p + 1;
        }

      ++p;
    }

  g_free (copy);

  if (colors)
    *colors = retval;
  else
    g_free (retval);

  if (n_colors)
    *n_colors = count;

  return TRUE;

 failed:
  g_free (copy);
  g_free (retval);

  if (colors)
    *colors = NULL;
  if (n_colors)
    *n_colors = 0;

  return FALSE;
}

/**
 * cafe_color_selection_palette_to_string:
 * @colors: (array length=n_colors): an array of colors.
 * @n_colors: length of the array.
 *
 * Encodes a palette as a string, useful for persistent storage.
 *
 * Return value: allocated string encoding the palette.
 **/
gchar*
cafe_color_selection_palette_to_string (const CdkColor *colors,
                                       gint            n_colors)
{
  gint i;
  gchar **strs = NULL;
  gchar *retval;

  if (n_colors == 0)
    return g_strdup ("");

  strs = g_new0 (gchar*, n_colors + 1);

  i = 0;
  while (i < n_colors)
    {
      gchar *ptr;

      strs[i] =
        g_strdup_printf ("#%2X%2X%2X",
                         colors[i].red / 256,
                         colors[i].green / 256,
                         colors[i].blue / 256);

      for (ptr = strs[i]; *ptr; ptr++)
        if (*ptr == ' ')
          *ptr = '0';

      ++i;
    }

  retval = g_strjoinv (":", strs);

  g_strfreev (strs);

  return retval;
}

/**
 * cafe_color_selection_set_change_palette_hook:
 * @func: a function to call when the custom palette needs saving.
 *
 * Installs a global function to be called whenever the user tries to
 * modify the palette in a color selection. This function should save
 * the new palette contents, and update the CtkSettings property
 * "ctk-color-palette" so all CafeColorSelection widgets will be modified.
 *
 * Return value: the previous change palette hook (that was replaced).
 *
 * Deprecated: 2.4: This function does not work in multihead environments.
 *     Use cafe_color_selection_set_change_palette_with_screen_hook() instead.
 *
 **/
CafeColorSelectionChangePaletteFunc
cafe_color_selection_set_change_palette_hook (CafeColorSelectionChangePaletteFunc func)
{
  CafeColorSelectionChangePaletteFunc old;

  old = noscreen_change_palette_hook;

  noscreen_change_palette_hook = func;

  return old;
}

/**
 * cafe_color_selection_set_change_palette_with_screen_hook:
 * @func: a function to call when the custom palette needs saving.
 *
 * Installs a global function to be called whenever the user tries to
 * modify the palette in a color selection. This function should save
 * the new palette contents, and update the CtkSettings property
 * "ctk-color-palette" so all CafeColorSelection widgets will be modified.
 *
 * Return value: the previous change palette hook (that was replaced).
 *
 * Since: 1.9.1
 **/
CafeColorSelectionChangePaletteWithScreenFunc
cafe_color_selection_set_change_palette_with_screen_hook (CafeColorSelectionChangePaletteWithScreenFunc func)
{
  CafeColorSelectionChangePaletteWithScreenFunc old;

  old = change_palette_hook;

  change_palette_hook = func;

  return old;
}

static void
make_control_relations (AtkObject *atk_obj,
                        CtkWidget *widget)
{
  AtkObject *obj;

  obj = ctk_widget_get_accessible (widget);
  atk_object_add_relationship (atk_obj, ATK_RELATION_CONTROLLED_BY, obj);
  atk_object_add_relationship (obj, ATK_RELATION_CONTROLLER_FOR, atk_obj);
}

static void
make_all_relations (AtkObject *atk_obj,
                    CafeColorSelectionPrivate *priv)
{
  make_control_relations (atk_obj, priv->hue_spinbutton);
  make_control_relations (atk_obj, priv->sat_spinbutton);
  make_control_relations (atk_obj, priv->val_spinbutton);
  make_control_relations (atk_obj, priv->red_spinbutton);
  make_control_relations (atk_obj, priv->green_spinbutton);
  make_control_relations (atk_obj, priv->blue_spinbutton);
}


