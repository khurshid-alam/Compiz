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

#ifndef GWD_THEME_H
#define GWD_THEME_H

#include <decoration.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _decor decor_t;
typedef struct _decor_frame decor_frame_t;
typedef struct _GWDSettings GWDSettings;

#define GWD_TYPE_THEME gwd_theme_get_type ()
G_DECLARE_DERIVABLE_TYPE (GWDTheme, gwd_theme, GWD, THEME, GObject)

struct _GWDThemeClass
{
    GObjectClass parent_class;

    void     (* scale_changed)             (GWDTheme                   *theme);

    void     (* style_updated)             (GWDTheme                   *theme);

    void     (* get_shadow)                (GWDTheme                   *theme,
                                            decor_frame_t              *frame,
                                            decor_shadow_options_t     *options,
                                            gboolean                    active);

    void     (* draw_window_decoration)    (GWDTheme                   *theme,
                                            decor_t                    *decor);

    gboolean (* calc_decoration_size)      (GWDTheme                   *theme,
                                            decor_t                    *decor,
                                            gint                        w,
                                            gint                        h,
                                            gint                        name_width,
                                            gint                       *width,
                                            gint                       *height);

    void     (* update_border_extents)     (GWDTheme                   *theme,
                                            decor_frame_t              *frame);

    void     (* get_event_window_position) (GWDTheme                   *theme,
                                            decor_t                    *decor,
                                            gint                        i,
                                            gint                        j,
                                            gint                        width,
                                            gint                        height,
                                            gint                       *x,
                                            gint                       *y,
                                            gint                       *w,
                                            gint                       *h);

    gboolean (* get_button_position)       (GWDTheme                   *theme,
                                            decor_t                    *decor,
                                            gint                        i,
                                            gint                        width,
                                            gint                        height,
                                            gint                       *x,
                                            gint                       *y,
                                            gint                       *w,
                                            gint                       *h);

    void     (* update_titlebar_font)      (GWDTheme                   *theme,
                                            const PangoFontDescription *titlebar_font);
};

typedef enum
{
    GWD_THEME_TYPE_CAIRO,
    GWD_THEME_TYPE_METACITY
} GWDThemeType;

GWDTheme *
gwd_theme_new                       (GWDThemeType            type,
                                     GWDSettings            *settings);

GWDSettings *
gwd_theme_get_settings              (GWDTheme               *theme);

gint
gwd_theme_get_scale                 (GWDTheme               *theme);

GtkWidget *
gwd_theme_get_style_window          (GWDTheme               *theme);

void
gwd_theme_get_shadow                (GWDTheme               *theme,
                                     decor_frame_t          *frame,
                                     decor_shadow_options_t *options,
                                     gboolean                active);

void
gwd_theme_draw_window_decoration    (GWDTheme               *theme,
                                     decor_t                *decor);

gboolean
gwd_theme_calc_decoration_size      (GWDTheme               *theme,
                                     decor_t                *decor,
                                     gint                    w,
                                     gint                    h,
                                     gint                    name_width,
                                     gint                   *width,
                                     gint                   *height);

void
gwd_theme_update_border_extents     (GWDTheme               *theme,
                                     decor_frame_t          *frame);

void
gwd_theme_get_event_window_position (GWDTheme               *theme,
                                     decor_t                *decor,
                                     gint                    i,
                                     gint                    j,
                                     gint                    width,
                                     gint                    height,
                                     gint                   *x,
                                     gint                   *y,
                                     gint                   *w,
                                     gint                   *h);

gboolean
gwd_theme_get_button_position       (GWDTheme               *theme,
                                     decor_t                *decor,
                                     gint                    i,
                                     gint                    width,
                                     gint                    height,
                                     gint                   *x,
                                     gint                   *y,
                                     gint                   *w,
                                     gint                   *h);

void
gwd_theme_update_titlebar_font      (GWDTheme               *theme);

PangoFontDescription *
gwd_theme_get_titlebar_font         (GWDTheme               *theme,
                                     decor_frame_t          *frame);

G_END_DECLS

#endif
