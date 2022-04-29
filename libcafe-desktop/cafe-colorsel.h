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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CAFE_COLOR_SELECTION_H__
#define __CAFE_COLOR_SELECTION_H__

#include <glib.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CAFE_TYPE_COLOR_SELECTION			(cafe_color_selection_get_type ())
#define CAFE_COLOR_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_COLOR_SELECTION, CafeColorSelection))
#define CAFE_COLOR_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_COLOR_SELECTION, CafeColorSelectionClass))
#define CAFE_IS_COLOR_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_COLOR_SELECTION))
#define CAFE_IS_COLOR_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_COLOR_SELECTION))
#define CAFE_COLOR_SELECTION_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_COLOR_SELECTION, CafeColorSelectionClass))


typedef struct _CafeColorSelection       CafeColorSelection;
typedef struct _CafeColorSelectionClass  CafeColorSelectionClass;
typedef struct _CafeColorSelectionPrivate    CafeColorSelectionPrivate;


typedef void (* CafeColorSelectionChangePaletteFunc) (const CdkColor    *colors,
                                                     gint               n_colors);
typedef void (* CafeColorSelectionChangePaletteWithScreenFunc) (CdkScreen         *screen,
							       const CdkColor    *colors,
							       gint               n_colors);

struct _CafeColorSelection
{
  CtkBox parent_instance;

  /* < private_data > */
  CafeColorSelectionPrivate *private_data;
};

struct _CafeColorSelectionClass
{
  CtkBoxClass parent_class;

  void (*color_changed)	(CafeColorSelection *color_selection);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/* ColorSelection */

GType      cafe_color_selection_get_type                (void) G_GNUC_CONST;
CtkWidget *cafe_color_selection_new                     (void);
gboolean   cafe_color_selection_get_has_opacity_control (CafeColorSelection *colorsel);
void       cafe_color_selection_set_has_opacity_control (CafeColorSelection *colorsel,
							gboolean           has_opacity);
gboolean   cafe_color_selection_get_has_palette         (CafeColorSelection *colorsel);
void       cafe_color_selection_set_has_palette         (CafeColorSelection *colorsel,
							gboolean           has_palette);


void     cafe_color_selection_set_current_color   (CafeColorSelection *colorsel,
						  const CdkColor    *color);
void     cafe_color_selection_set_current_alpha   (CafeColorSelection *colorsel,
						  guint16            alpha);
void     cafe_color_selection_get_current_color   (CafeColorSelection *colorsel,
						  CdkColor          *color);
guint16  cafe_color_selection_get_current_alpha   (CafeColorSelection *colorsel);
void     cafe_color_selection_set_previous_color  (CafeColorSelection *colorsel,
						  const CdkColor    *color);
void     cafe_color_selection_set_previous_alpha  (CafeColorSelection *colorsel,
						  guint16            alpha);
void     cafe_color_selection_get_previous_color  (CafeColorSelection *colorsel,
						  CdkColor          *color);
guint16  cafe_color_selection_get_previous_alpha  (CafeColorSelection *colorsel);

gboolean cafe_color_selection_is_adjusting        (CafeColorSelection *colorsel);

gboolean cafe_color_selection_palette_from_string (const gchar       *str,
                                                  CdkColor         **colors,
                                                  gint              *n_colors);
gchar*   cafe_color_selection_palette_to_string   (const CdkColor    *colors,
                                                  gint               n_colors);

#ifndef CTK_DISABLE_DEPRECATED
#ifndef CDK_MULTIHEAD_SAFE
CafeColorSelectionChangePaletteFunc           cafe_color_selection_set_change_palette_hook             (CafeColorSelectionChangePaletteFunc           func);
#endif
#endif

CafeColorSelectionChangePaletteWithScreenFunc cafe_color_selection_set_change_palette_with_screen_hook (CafeColorSelectionChangePaletteWithScreenFunc func);

#ifndef CTK_DISABLE_DEPRECATED
/* Deprecated calls: */
void cafe_color_selection_set_color         (CafeColorSelection *colorsel,
					    gdouble           *color);
void cafe_color_selection_get_color         (CafeColorSelection *colorsel,
					    gdouble           *color);
#endif /* CTK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __CAFE_COLOR_SELECTION_H__ */
