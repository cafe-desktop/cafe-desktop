/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* cafe-ditem.h - CAFE Desktop File Representation

   Copyright (C) 1999, 2000 Red Hat Inc.
   Copyright (C) 2001 Sid Vicious
   All rights reserved.

   This file is part of the Cafe Library.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA  02110-1301, USA.  */
/*
  @NOTATION@
 */

#ifndef CAFE_DITEM_H
#define CAFE_DITEM_H

#include <glib.h>
#include <glib-object.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	CAFE_DESKTOP_ITEM_TYPE_NULL = 0 /* This means its NULL, that is, not
					   * set */,
	CAFE_DESKTOP_ITEM_TYPE_OTHER /* This means it's not one of the below
					      strings types, and you must get the
					      Type attribute. */,

	/* These are the standard compliant types: */
	CAFE_DESKTOP_ITEM_TYPE_APPLICATION,
	CAFE_DESKTOP_ITEM_TYPE_LINK,
	CAFE_DESKTOP_ITEM_TYPE_FSDEVICE,
	CAFE_DESKTOP_ITEM_TYPE_MIME_TYPE,
	CAFE_DESKTOP_ITEM_TYPE_DIRECTORY,
	CAFE_DESKTOP_ITEM_TYPE_SERVICE,
	CAFE_DESKTOP_ITEM_TYPE_SERVICE_TYPE
} CafeDesktopItemType;

typedef enum {
        CAFE_DESKTOP_ITEM_UNCHANGED = 0,
        CAFE_DESKTOP_ITEM_CHANGED = 1,
        CAFE_DESKTOP_ITEM_DISAPPEARED = 2
} CafeDesktopItemStatus;

#define CAFE_TYPE_DESKTOP_ITEM         (cafe_desktop_item_get_type ())
GType cafe_desktop_item_get_type       (void);

typedef struct _CafeDesktopItem CafeDesktopItem;

/* standard */
#define CAFE_DESKTOP_ITEM_ENCODING	"Encoding" /* string */
#define CAFE_DESKTOP_ITEM_VERSION	"Version"  /* numeric */
#define CAFE_DESKTOP_ITEM_NAME		"Name" /* localestring */
#define CAFE_DESKTOP_ITEM_GENERIC_NAME	"GenericName" /* localestring */
#define CAFE_DESKTOP_ITEM_TYPE		"Type" /* string */
#define CAFE_DESKTOP_ITEM_FILE_PATTERN "FilePattern" /* regexp(s) */
#define CAFE_DESKTOP_ITEM_TRY_EXEC	"TryExec" /* string */
#define CAFE_DESKTOP_ITEM_NO_DISPLAY	"NoDisplay" /* boolean */
#define CAFE_DESKTOP_ITEM_COMMENT	"Comment" /* localestring */
#define CAFE_DESKTOP_ITEM_EXEC		"Exec" /* string */
#define CAFE_DESKTOP_ITEM_ACTIONS	"Actions" /* strings */
#define CAFE_DESKTOP_ITEM_ICON		"Icon" /* string */
#define CAFE_DESKTOP_ITEM_MINI_ICON	"MiniIcon" /* string */
#define CAFE_DESKTOP_ITEM_HIDDEN	"Hidden" /* boolean */
#define CAFE_DESKTOP_ITEM_PATH		"Path" /* string */
#define CAFE_DESKTOP_ITEM_TERMINAL	"Terminal" /* boolean */
#define CAFE_DESKTOP_ITEM_TERMINAL_OPTIONS "TerminalOptions" /* string */
#define CAFE_DESKTOP_ITEM_SWALLOW_TITLE "SwallowTitle" /* string */
#define CAFE_DESKTOP_ITEM_SWALLOW_EXEC	"SwallowExec" /* string */
#define CAFE_DESKTOP_ITEM_MIME_TYPE	"MimeType" /* regexp(s) */
#define CAFE_DESKTOP_ITEM_PATTERNS	"Patterns" /* regexp(s) */
#define CAFE_DESKTOP_ITEM_DEFAULT_APP	"DefaultApp" /* string */
#define CAFE_DESKTOP_ITEM_DEV		"Dev" /* string */
#define CAFE_DESKTOP_ITEM_FS_TYPE	"FSType" /* string */
#define CAFE_DESKTOP_ITEM_MOUNT_POINT	"MountPoint" /* string */
#define CAFE_DESKTOP_ITEM_READ_ONLY	"ReadOnly" /* boolean */
#define CAFE_DESKTOP_ITEM_UNMOUNT_ICON "UnmountIcon" /* string */
#define CAFE_DESKTOP_ITEM_SORT_ORDER	"SortOrder" /* strings */
#define CAFE_DESKTOP_ITEM_URL		"URL" /* string */
#define CAFE_DESKTOP_ITEM_DOC_PATH	"X-CAFE-DocPath" /* string */

/* The vfolder proposal */
#define CAFE_DESKTOP_ITEM_CATEGORIES	"Categories" /* string */
#define CAFE_DESKTOP_ITEM_ONLY_SHOW_IN	"OnlyShowIn" /* string */

typedef enum {
	/* Use the TryExec field to determine if this should be loaded */
        CAFE_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS = 1<<0,
        CAFE_DESKTOP_ITEM_LOAD_NO_TRANSLATIONS = 1<<1
} CafeDesktopItemLoadFlags;

typedef enum {
	/* Never launch more instances even if the app can only
	 * handle one file and we have passed many */
        CAFE_DESKTOP_ITEM_LAUNCH_ONLY_ONE = 1<<0,
	/* Use current directory instead of home directory */
        CAFE_DESKTOP_ITEM_LAUNCH_USE_CURRENT_DIR = 1<<1,
	/* Append the list of URIs to the command if no Exec
	 * parameter is specified, instead of launching the
	 * app without parameters. */
	CAFE_DESKTOP_ITEM_LAUNCH_APPEND_URIS = 1<<2,
	/* Same as above but instead append local paths */
	CAFE_DESKTOP_ITEM_LAUNCH_APPEND_PATHS = 1<<3,
	/* Don't automatically reap child process.  */
	CAFE_DESKTOP_ITEM_LAUNCH_DO_NOT_REAP_CHILD = 1<<4
} CafeDesktopItemLaunchFlags;

typedef enum {
	/* Don't check the kde directories */
        CAFE_DESKTOP_ITEM_ICON_NO_KDE = 1<<0
} CafeDesktopItemIconFlags;

typedef enum {
	CAFE_DESKTOP_ITEM_ERROR_NO_FILENAME /* No filename set or given on save */,
	CAFE_DESKTOP_ITEM_ERROR_UNKNOWN_ENCODING /* Unknown encoding of the file */,
	CAFE_DESKTOP_ITEM_ERROR_CANNOT_OPEN /* Cannot open file */,
	CAFE_DESKTOP_ITEM_ERROR_NO_EXEC_STRING /* Cannot launch due to no execute string */,
	CAFE_DESKTOP_ITEM_ERROR_BAD_EXEC_STRING /* Cannot launch due to bad execute string */,
	CAFE_DESKTOP_ITEM_ERROR_NO_URL /* No URL on a url entry*/,
	CAFE_DESKTOP_ITEM_ERROR_NOT_LAUNCHABLE /* Not a launchable type of item */,
	CAFE_DESKTOP_ITEM_ERROR_INVALID_TYPE /* Not of type application/x-cafe-app-info */
} CafeDesktopItemError;

/* Note that functions can also return the G_FILE_ERROR_* errors */

#define CAFE_DESKTOP_ITEM_ERROR cafe_desktop_item_error_quark ()
GQuark cafe_desktop_item_error_quark (void);

/* Returned item from new*() and copy() methods have a refcount of 1 */
CafeDesktopItem *      cafe_desktop_item_new               (void);
CafeDesktopItem *      cafe_desktop_item_new_from_file     (const char                 *file,
							      CafeDesktopItemLoadFlags   flags,
							      GError                    **error);
CafeDesktopItem *      cafe_desktop_item_new_from_uri      (const char                 *uri,
							      CafeDesktopItemLoadFlags   flags,
							      GError                    **error);
CafeDesktopItem *      cafe_desktop_item_new_from_string   (const char                 *uri,
							      const char                 *string,
							      gssize                      length,
							      CafeDesktopItemLoadFlags   flags,
							      GError                    **error);
CafeDesktopItem *      cafe_desktop_item_new_from_basename (const char                 *basename,
							      CafeDesktopItemLoadFlags   flags,
							      GError                    **error);
CafeDesktopItem *      cafe_desktop_item_copy              (const CafeDesktopItem     *item);

/* if under is NULL save in original location */
gboolean                cafe_desktop_item_save              (CafeDesktopItem           *item,
							      const char                 *under,
							      gboolean			  force,
							      GError                    **error);
CafeDesktopItem *      cafe_desktop_item_ref               (CafeDesktopItem           *item);
void                    cafe_desktop_item_unref             (CafeDesktopItem           *item);
int			cafe_desktop_item_launch	     (const CafeDesktopItem     *item,
							      GList                      *file_list,
							      CafeDesktopItemLaunchFlags flags,
							      GError                    **error);
int			cafe_desktop_item_launch_with_env   (const CafeDesktopItem     *item,
							      GList                      *file_list,
							      CafeDesktopItemLaunchFlags flags,
							      char                      **envp,
							      GError                    **error);

int                     cafe_desktop_item_launch_on_screen  (const CafeDesktopItem       *item,
							      GList                        *file_list,
							      CafeDesktopItemLaunchFlags   flags,
							      GdkScreen                    *screen,
							      int                           workspace,
							      GError                      **error);

/* A list of files or urls dropped onto an icon */
int                     cafe_desktop_item_drop_uri_list     (const CafeDesktopItem     *item,
							      const char                 *uri_list,
							      CafeDesktopItemLaunchFlags flags,
							      GError                    **error);

int                     cafe_desktop_item_drop_uri_list_with_env    (const CafeDesktopItem     *item,
								      const char                 *uri_list,
								      CafeDesktopItemLaunchFlags flags,
								      char                      **envp,
								      GError                    **error);

gboolean                cafe_desktop_item_exists            (const CafeDesktopItem     *item);

CafeDesktopItemType	cafe_desktop_item_get_entry_type    (const CafeDesktopItem	 *item);
/* You could also just use the set_string on the TYPE argument */
void			cafe_desktop_item_set_entry_type    (CafeDesktopItem		 *item,
							      CafeDesktopItemType	  type);

/* Get current location on disk */
const char *            cafe_desktop_item_get_location      (const CafeDesktopItem     *item);
void                    cafe_desktop_item_set_location      (CafeDesktopItem           *item,
							      const char                 *location);
void                    cafe_desktop_item_set_location_file (CafeDesktopItem           *item,
							      const char                 *file);
CafeDesktopItemStatus  cafe_desktop_item_get_file_status   (const CafeDesktopItem     *item);

/*
 * Get the icon, this is not as simple as getting the Icon attr as it actually tries to find
 * it and returns %NULL if it can't
 */
char *                  cafe_desktop_item_get_icon          (const CafeDesktopItem     *item,
							      GtkIconTheme               *icon_theme);

char *                  cafe_desktop_item_find_icon         (GtkIconTheme               *icon_theme,
							      const char                 *icon,
							      /* size is only a suggestion */
							      int                         desired_size,
							      int                         flags);


/*
 * Reading/Writing different sections, NULL is the standard section
 */
gboolean                cafe_desktop_item_attr_exists       (const CafeDesktopItem     *item,
							      const char                 *attr);

/*
 * String type
 */
const char *            cafe_desktop_item_get_string        (const CafeDesktopItem     *item,
							      const char		 *attr);

void                    cafe_desktop_item_set_string        (CafeDesktopItem           *item,
							      const char		 *attr,
							      const char                 *value);

const char *            cafe_desktop_item_get_attr_locale   (const CafeDesktopItem     *item,
							      const char		 *attr);

/*
 * LocaleString type
 */
const char *            cafe_desktop_item_get_localestring  (const CafeDesktopItem     *item,
							      const char		 *attr);
const char *            cafe_desktop_item_get_localestring_lang (const CafeDesktopItem *item,
								  const char		 *attr,
								  const char             *language);
/* use g_list_free only */
GList *                 cafe_desktop_item_get_languages     (const CafeDesktopItem     *item,
							      const char		 *attr);

void                    cafe_desktop_item_set_localestring  (CafeDesktopItem           *item,
							      const char		 *attr,
							      const char                 *value);
void                    cafe_desktop_item_set_localestring_lang  (CafeDesktopItem      *item,
								   const char		 *attr,
								   const char		 *language,
								   const char            *value);
void                    cafe_desktop_item_clear_localestring(CafeDesktopItem           *item,
							      const char		 *attr);

/*
 * Strings, Regexps types
 */

/* use cafe_desktop_item_free_string_list */
char **                 cafe_desktop_item_get_strings       (const CafeDesktopItem     *item,
							      const char		 *attr);

void			cafe_desktop_item_set_strings       (CafeDesktopItem           *item,
							      const char                 *attr,
							      char                      **strings);

/*
 * Boolean type
 */
gboolean                cafe_desktop_item_get_boolean       (const CafeDesktopItem     *item,
							      const char		 *attr);

void                    cafe_desktop_item_set_boolean       (CafeDesktopItem           *item,
							      const char		 *attr,
							      gboolean                    value);

/*
 * Xserver time of user action that caused the application launch to start.
 */
void                    cafe_desktop_item_set_launch_time   (CafeDesktopItem           *item,
							      guint32                     timestamp);

/*
 * Clearing attributes
 */
#define                 cafe_desktop_item_clear_attr(item,attr) \
				cafe_desktop_item_set_string(item,attr,NULL)
void			cafe_desktop_item_clear_section     (CafeDesktopItem           *item,
							      const char                 *section);

G_END_DECLS

#endif /* CAFE_DITEM_H */
