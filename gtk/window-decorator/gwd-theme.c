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
#include "gwd-settings.h"
#include "gwd-theme.h"
#include "gwd-theme-cairo.h"

#ifdef USE_METACITY
#include "gwd-theme-metacity.h"
#endif

typedef struct
{
    GWDSettings *settings;
} GWDThemePrivate;

enum
{
    PROP_0,

    PROP_SETTINGS,

    LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL };

G_DEFINE_TYPE_WITH_PRIVATE (GWDTheme, gwd_theme, G_TYPE_OBJECT)

static void
gwd_theme_dispose (GObject *object)
{
    GWDTheme *theme;
    GWDThemePrivate *priv;

    theme = GWD_THEME (object);
    priv = gwd_theme_get_instance_private (theme);

    g_clear_object (&priv->settings);

    G_OBJECT_CLASS (gwd_theme_parent_class)->dispose (object);
}

static void
gwd_theme_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    GWDTheme *theme;
    GWDThemePrivate *priv;

    theme = GWD_THEME (object);
    priv = gwd_theme_get_instance_private (theme);

    switch (property_id) {
        case PROP_SETTINGS:
            g_value_set_object (value, priv->settings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_theme_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    GWDTheme *theme;
    GWDThemePrivate *priv;

    theme = GWD_THEME (object);
    priv = gwd_theme_get_instance_private (theme);

    switch (property_id) {
        case PROP_SETTINGS:
            priv->settings = g_value_dup_object (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_theme_real_get_shadow (GWDTheme               *theme,
                           decor_frame_t          *frame,
                           decor_shadow_options_t *options,
                           gboolean                active)
{
}

static void
gwd_theme_real_draw_window_decoration (GWDTheme *theme,
                                       decor_t  *decor)
{
}

static gboolean
gwd_theme_real_calc_decoration_size (GWDTheme *theme,
                                     decor_t  *decor,
                                     gint      w,
                                     gint      h,
                                     gint      name_width,
                                     gint     *width,
                                     gint     *height)
{
    return FALSE;
}

static void
gwd_theme_real_update_border_extents (GWDTheme      *theme,
                                      decor_frame_t *frame)
{
}

static void
gwd_theme_real_get_event_window_position (GWDTheme *theme,
                                          decor_t  *decor,
                                          gint      i,
                                          gint      j,
                                          gint      width,
                                          gint      height,
                                          gint     *x,
                                          gint     *y,
                                          gint     *w,
                                          gint     *h)
{
}

static gboolean
gwd_theme_real_get_button_position (GWDTheme *theme,
                                    decor_t  *decor,
                                    gint      i,
                                    gint      width,
                                    gint      height,
                                    gint     *x,
                                    gint     *y,
                                    gint     *w,
                                    gint     *h)
{
    return FALSE;
}

static gfloat
gwd_theme_real_get_title_scale (GWDTheme      *theme,
                                decor_frame_t *frame)
{
    return 1.0;
}

static void
gwd_theme_class_init (GWDThemeClass *theme_class)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (theme_class);

    object_class->dispose = gwd_theme_dispose;
    object_class->get_property = gwd_theme_get_property;
    object_class->set_property = gwd_theme_set_property;

    theme_class->get_shadow = gwd_theme_real_get_shadow;
    theme_class->draw_window_decoration = gwd_theme_real_draw_window_decoration;
    theme_class->calc_decoration_size = gwd_theme_real_calc_decoration_size;
    theme_class->update_border_extents = gwd_theme_real_update_border_extents;
    theme_class->get_event_window_position = gwd_theme_real_get_event_window_position;
    theme_class->get_button_position = gwd_theme_real_get_button_position;
    theme_class->get_title_scale = gwd_theme_real_get_title_scale;

    properties[PROP_SETTINGS] =
        g_param_spec_object ("settings", "GWDSettings", "GWDSettings",
                             GWD_TYPE_SETTINGS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gwd_theme_init (GWDTheme *theme)
{
}

GWDTheme *
gwd_theme_new (GWDThemeType  type,
               GWDSettings  *settings)
{
    GType object_type;

    object_type = GWD_TYPE_THEME_CAIRO;

#ifdef USE_METACITY
    if (type == GWD_THEME_TYPE_METACITY)
        object_type = GWD_TYPE_THEME_METACITY;
#endif

    return g_object_new (object_type,
                         "settings", settings,
                         NULL);
}

void
gwd_theme_get_shadow (GWDTheme               *theme,
                      decor_frame_t          *frame,
                      decor_shadow_options_t *options,
                      gboolean                active)
{
    GWD_THEME_GET_CLASS (theme)->get_shadow (theme, frame, options, active);
}

void
gwd_theme_draw_window_decoration (GWDTheme *theme,
                                  decor_t  *decor)
{
    GWD_THEME_GET_CLASS (theme)->draw_window_decoration (theme, decor);
}

gboolean
gwd_theme_calc_decoration_size (GWDTheme *theme,
                                decor_t  *decor,
                                gint      w,
                                gint      h,
                                gint      name_width,
                                gint     *width,
                                gint     *height)
{
    return GWD_THEME_GET_CLASS (theme)->calc_decoration_size (theme, decor,
                                                              w, h, name_width,
                                                              width, height);
}

void
gwd_theme_update_border_extents (GWDTheme      *theme,
                                 decor_frame_t *frame)
{
    GWD_THEME_GET_CLASS (theme)->update_border_extents (theme, frame);
}

void
gwd_theme_get_event_window_position (GWDTheme *theme,
                                     decor_t  *decor,
                                     gint      i,
                                     gint      j,
                                     gint      width,
                                     gint      height,
                                     gint     *x,
                                     gint     *y,
                                     gint     *w,
                                     gint     *h)
{
    GWD_THEME_GET_CLASS (theme)->get_event_window_position (theme, decor, i, j,
                                                            width, height,
                                                            x, y, w, h);
}

gboolean
gwd_theme_get_button_position (GWDTheme *theme,
                               decor_t  *decor,
                               gint      i,
                               gint      width,
                               gint      height,
                               gint     *x,
                               gint     *y,
                               gint     *w,
                               gint     *h)
{
    return GWD_THEME_GET_CLASS (theme)->get_button_position (theme, decor, i,
                                                             width, height,
                                                             x, y, w, h);
}

gfloat
gwd_theme_get_title_scale (GWDTheme      *theme,
                           decor_frame_t *frame)
{
    return GWD_THEME_GET_CLASS (theme)->get_title_scale (theme, frame);
}
