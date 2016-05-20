/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

/*
 * Copyright (C) 2016 Alberts Muktupāvels
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <metacity-private/theme.h>

#include "gtk-window-decorator.h"
#include "gwd-theme-metacity.h"

struct _GWDThemeMetacity
{
    GObject    parent;

    MetaTheme *theme;
};

G_DEFINE_TYPE (GWDThemeMetacity, gwd_theme_metacity, GWD_TYPE_THEME)

static GObject *
gwd_theme_metacity_constructor (GType                  type,
                                guint                  n_properties,
                                GObjectConstructParam *properties)
{
    GObject *object;
    GWDThemeMetacity *metacity;

    object = G_OBJECT_CLASS (gwd_theme_metacity_parent_class)->constructor (type, n_properties, properties);
    metacity = GWD_THEME_METACITY (object);

    /* Always valid and current MetaTheme! On theme change new GWDThemeMetacity
     * object will be created and old one destroyed. If Metacity theme is not
     * valid (gwd_metacity_window_decoration_update_meta_theme returns FALSE)
     * then GWDThemeCairo will be created.
     */
    metacity->theme = meta_theme_get_current ();

    return object;
}

static void
gwd_theme_metacity_update_border_extents (GWDTheme      *theme,
                                          decor_frame_t *frame)
{
    GWDThemeMetacity *metacity = GWD_THEME_METACITY (theme);
    GdkScreen *screen = gtk_widget_get_screen (frame->style_window_rgba);
    MetaStyleInfo *style_info = meta_theme_create_style_info (screen, NULL);
    MetaFrameType frame_type;
    MetaFrameBorders borders;

    gwd_decor_frame_ref (frame);

    frame_type = meta_frame_type_from_string (frame->type);
    if (!(frame_type < META_FRAME_TYPE_LAST))
        frame_type = META_FRAME_TYPE_NORMAL;

    meta_theme_get_frame_borders (metacity->theme, style_info, frame_type,
                                  frame->text_height, 0, &borders);

    frame->win_extents.top = frame->win_extents.top;
    frame->win_extents.bottom = borders.visible.bottom;
    frame->win_extents.left = borders.visible.left;
    frame->win_extents.right = borders.visible.right;

    frame->titlebar_height = borders.visible.top - frame->win_extents.top;

    meta_theme_get_frame_borders (metacity->theme, style_info, frame_type,
                                  frame->text_height, META_FRAME_MAXIMIZED,
                                  &borders);

    frame->max_win_extents.top = frame->win_extents.top;
    frame->max_win_extents.bottom = borders.visible.bottom;
    frame->max_win_extents.left = borders.visible.left;
    frame->max_win_extents.right = borders.visible.right;

    frame->max_titlebar_height = borders.visible.top - frame->max_win_extents.top;

    meta_style_info_unref (style_info);

    gwd_decor_frame_unref (frame);
}

static void
gwd_theme_metacity_class_init (GWDThemeMetacityClass *metacity_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (metacity_class);
    GWDThemeClass *theme_class = GWD_THEME_CLASS (metacity_class);

    object_class->constructor = gwd_theme_metacity_constructor;

    theme_class->update_border_extents = gwd_theme_metacity_update_border_extents;
}

static void
gwd_theme_metacity_init (GWDThemeMetacity *metacity)
{
}
