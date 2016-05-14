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

#include <glib-object.h>

#include "gwd-settings-notified.h"
#include "gwd-metacity-window-decoration-util.h"
#include "gtk-window-decorator.h"

struct _GWDSettingsNotified
{
    GObject     parent;

    WnckScreen *screen;
};

enum
{
    PROP_0,

    PROP_WNCK_SCREEN,

    LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL };

G_DEFINE_TYPE (GWDSettingsNotified, gwd_settings_notified, G_TYPE_OBJECT)

void
set_frame_scale (decor_frame_t *frame,
		 const gchar   *font_str)
{
    gfloat	  scale = 1.0f;

    gwd_decor_frame_ref (frame);

    if (frame->titlebar_font)
    {
        pango_font_description_free (frame->titlebar_font);
        frame->titlebar_font = NULL;
    }

    if (font_str)
    {
        gint size;

        frame->titlebar_font = pango_font_description_from_string (font_str);

        scale = (*theme_get_title_scale) (frame);
        size = MAX (pango_font_description_get_size (frame->titlebar_font) * scale, 1);

        pango_font_description_set_size (frame->titlebar_font, size);
    }

    gwd_decor_frame_unref (frame);
}

void
set_frames_scales (gpointer key,
		   gpointer value,
		   gpointer user_data)
{
    decor_frame_t *frame = (decor_frame_t *) value;
    gchar	  *font_str = (gchar *) user_data;

    gwd_decor_frame_ref (frame);

    set_frame_scale (frame, font_str);

    gwd_decor_frame_unref (frame);
}

static void
gwd_settings_notified_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    GWDSettingsNotified *notified;

    notified = GWD_SETTINGS_NOTIFIED (object);

    switch (property_id) {
        case PROP_WNCK_SCREEN:
            notified->screen = g_value_get_object (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_settings_notified_class_init (GWDSettingsNotifiedClass *notified_class)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (notified_class);

    object_class->set_property = gwd_settings_notified_set_property;

    properties[PROP_WNCK_SCREEN] =
        g_param_spec_object ("wnck-screen", "WnckScreen", "A WnckScreen",
                             WNCK_TYPE_SCREEN,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
                             G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gwd_settings_notified_init (GWDSettingsNotified *notified)
{
}

GWDSettingsNotified *
gwd_settings_notified_new (WnckScreen *screen)
{
    return g_object_new (GWD_TYPE_SETTINGS_NOTIFIED,
                         "wnck-screen", screen,
                         NULL);
}

gboolean
gwd_settings_notified_update_decorations (GWDSettingsNotified *notified)
{
    decorations_changed (notified->screen);

    return TRUE;
}

gboolean
gwd_settings_notified_update_frames (GWDSettingsNotified *notified)
{
    const gchar *titlebar_font;

    g_object_get (settings, "titlebar-font", &titlebar_font, NULL);

    gwd_frames_foreach (set_frames_scales, (gpointer) titlebar_font);

    return TRUE;
}

gboolean
gwd_settings_notified_update_metacity_theme (GWDSettingsNotified *notified)
{
#ifdef USE_METACITY
    const gchar *meta_theme;

    g_object_get (settings, "metacity-theme", &meta_theme, NULL);

    if (gwd_metacity_window_decoration_update_meta_theme (meta_theme,
                                                          meta_theme_get_current,
                                                          meta_theme_set_current)) {
        theme_draw_window_decoration = meta_draw_window_decoration;
        theme_calc_decoration_size = meta_calc_decoration_size;
        theme_update_border_extents = meta_update_border_extents;
        theme_get_event_window_position = meta_get_event_window_position;
        theme_get_button_position = meta_get_button_position;
        theme_get_title_scale = meta_get_title_scale;
        theme_get_shadow = meta_get_shadow;
    } else {
        g_log ("gtk-window-decorator", G_LOG_LEVEL_INFO, "using cairo decoration");

        theme_draw_window_decoration = draw_window_decoration;
        theme_calc_decoration_size = calc_decoration_size;
        theme_update_border_extents = update_border_extents;
        theme_get_event_window_position = get_event_window_position;
        theme_get_button_position = get_button_position;
        theme_get_title_scale = get_title_scale;
        theme_get_shadow = cairo_get_shadow;
    }

    return TRUE;
#else
    theme_draw_window_decoration = draw_window_decoration;
    theme_calc_decoration_size = calc_decoration_size;
    theme_update_border_extents = update_border_extents;
    theme_get_event_window_position = get_event_window_position;
    theme_get_button_position = get_button_position;
    theme_get_title_scale = get_title_scale;
    theme_get_shadow = cairo_get_shadow;

    return FALSE;
#endif
}

gboolean
gwd_settings_notified_metacity_button_layout (GWDSettingsNotified *notified)
{
#ifdef USE_METACITY
    const gchar *button_layout;

    g_object_get (settings, "metacity-button-layout", &button_layout, NULL);

    if (button_layout) {
        meta_update_button_layout (button_layout);

        meta_button_layout_set = TRUE;
        return TRUE;
    }

    if (meta_button_layout_set) {
        meta_button_layout_set = FALSE;
        return TRUE;
    }

#endif
    return FALSE;
}
