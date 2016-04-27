/*
 * Copyright © 2012 Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#include <string.h>
#include <glib.h>
#include "gwd-metacity-window-decoration-util.h"

#ifdef USE_METACITY
MetaTheme *
gwd_metacity_window_decoration_update_meta_theme (MetaThemeType  theme_type,
                                                  const gchar   *theme_name)
{
    MetaTheme *theme;
    GError *error;

    theme = meta_theme_new (theme_type);

    error = NULL;
    if (!meta_theme_load (theme, theme_name, &error)) {
        g_warning ("%s", error->message);
        g_error_free (error);

        g_clear_object (&theme);
    }

    return theme;
}
#endif
