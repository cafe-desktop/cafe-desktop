/*
 * cafe-thumbnail.h: Utilities for handling thumbnails
 *
 * Copyright (C) 2002 Red Hat, Inc.
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
 * Boston, MA  02110-1301, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef CAFE_DESKTOP_THUMBNAIL_H
#define CAFE_DESKTOP_THUMBNAIL_H

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error    CafeDesktopThumbnail is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including cafe-desktop-thumbnail.h
#endif

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

typedef enum {
  CAFE_DESKTOP_THUMBNAIL_SIZE_NORMAL,
  CAFE_DESKTOP_THUMBNAIL_SIZE_LARGE
} CafeDesktopThumbnailSize;

#define CAFE_DESKTOP_TYPE_THUMBNAIL_FACTORY    (cafe_desktop_thumbnail_factory_get_type ())
#define CAFE_DESKTOP_THUMBNAIL_FACTORY(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_DESKTOP_TYPE_THUMBNAIL_FACTORY, CafeDesktopThumbnailFactory))
#define CAFE_DESKTOP_THUMBNAIL_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_DESKTOP_TYPE_THUMBNAIL_FACTORY, CafeDesktopThumbnailFactoryClass))
#define CAFE_DESKTOP_IS_THUMBNAIL_FACTORY(obj)    (G_TYPE_INSTANCE_CHECK_TYPE ((obj), CAFE_DESKTOP_TYPE_THUMBNAIL_FACTORY))
#define CAFE_DESKTOP_IS_THUMBNAIL_FACTORY_CLASS(klass)    (G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), CAFE_DESKTOP_TYPE_THUMBNAIL_FACTORY))

typedef struct _CafeDesktopThumbnailFactory        CafeDesktopThumbnailFactory;
typedef struct _CafeDesktopThumbnailFactoryClass   CafeDesktopThumbnailFactoryClass;
typedef struct _CafeDesktopThumbnailFactoryPrivate CafeDesktopThumbnailFactoryPrivate;

struct _CafeDesktopThumbnailFactory {
    GObject parent;

    CafeDesktopThumbnailFactoryPrivate *priv;
};

struct _CafeDesktopThumbnailFactoryClass {
    GObjectClass parent;
};

GType      cafe_desktop_thumbnail_factory_get_type (void);
CafeDesktopThumbnailFactory *cafe_desktop_thumbnail_factory_new      (CafeDesktopThumbnailSize     size);

char *     cafe_desktop_thumbnail_factory_lookup   (CafeDesktopThumbnailFactory *factory,
                                                    const char                  *uri,
                                                    time_t                       mtime);

gboolean   cafe_desktop_thumbnail_factory_has_valid_failed_thumbnail (CafeDesktopThumbnailFactory *factory,
                                                                      const char                  *uri,
                                                                      time_t                       mtime);
gboolean   cafe_desktop_thumbnail_factory_can_thumbnail (CafeDesktopThumbnailFactory *factory,
                                                         const char                  *uri,
                                                         const char                  *mime_type,
                                                         time_t                       mtime);
CdkPixbuf *  cafe_desktop_thumbnail_factory_generate_thumbnail (CafeDesktopThumbnailFactory *factory,
                                                                const char                  *uri,
                                                                const char                  *mime_type);
void       cafe_desktop_thumbnail_factory_save_thumbnail (CafeDesktopThumbnailFactory *factory,
                                                          CdkPixbuf                   *thumbnail,
                                                          const char                  *uri,
                                                          time_t                       original_mtime);
void       cafe_desktop_thumbnail_factory_create_failed_thumbnail (CafeDesktopThumbnailFactory *factory,
                                                                   const char                  *uri,
                                                                   time_t                       mtime);


/* Thumbnailing utils: */
gboolean   cafe_desktop_thumbnail_has_uri           (CdkPixbuf          *pixbuf,
                                                     const char         *uri);
gboolean   cafe_desktop_thumbnail_is_valid          (CdkPixbuf          *pixbuf,
                                                     const char         *uri,
                                                     time_t              mtime);
char *     cafe_desktop_thumbnail_path_for_uri      (const char         *uri,
                                                     CafeDesktopThumbnailSize  size);

G_END_DECLS

#endif /* CAFE_DESKTOP_THUMBNAIL_H */
