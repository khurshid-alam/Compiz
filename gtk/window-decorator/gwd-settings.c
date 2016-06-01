/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

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

#include <stdlib.h>
#include <stdio.h>

#include "gwd-settings.h"

const gboolean  USE_TOOLTIPS_DEFAULT = FALSE;

const gdouble   ACTIVE_SHADOW_RADIUS_DEFAULT = 8.0;
const gdouble   ACTIVE_SHADOW_OPACITY_DEFAULT = 0.5;
const gint      ACTIVE_SHADOW_OFFSET_X_DEFAULT = 1;
const gint      ACTIVE_SHADOW_OFFSET_Y_DEFAULT = 1;
const gchar    *ACTIVE_SHADOW_COLOR_DEFAULT = "#00000000";

const gdouble   INACTIVE_SHADOW_RADIUS_DEFAULT = 8.0;
const gdouble   INACTIVE_SHADOW_OPACITY_DEFAULT = 0.5;
const gint      INACTIVE_SHADOW_OFFSET_X_DEFAULT = 1;
const gint      INACTIVE_SHADOW_OFFSET_Y_DEFAULT = 1;
const gchar    *INACTIVE_SHADOW_COLOR_DEFAULT = "#00000000";

const gint      BLUR_TYPE_DEFAULT = BLUR_TYPE_NONE;

const gchar    *METACITY_THEME_DEFAULT = "Adwaita";
const gdouble   METACITY_ACTIVE_OPACITY_DEFAULT = 1.0;
const gdouble   METACITY_INACTIVE_OPACITY_DEFAULT = 0.75;
const gboolean  METACITY_ACTIVE_SHADE_OPACITY_DEFAULT = TRUE;
const gboolean  METACITY_INACTIVE_SHADE_OPACITY_DEFAULT = TRUE;

const gchar    *METACITY_BUTTON_LAYOUT_DEFAULT = ":minimize,maximize,close";

const guint     DOUBLE_CLICK_ACTION_DEFAULT = CLICK_ACTION_MAXIMIZE;
const guint     MIDDLE_CLICK_ACTION_DEFAULT = CLICK_ACTION_LOWER;
const guint     RIGHT_CLICK_ACTION_DEFAULT = CLICK_ACTION_MENU;
const guint     WHEEL_ACTION_DEFAULT = WHEEL_ACTION_SHADE;

const gchar    *TITLEBAR_FONT_DEFAULT = "Sans 12";

enum
{
    CMDLINE_BLUR = (1 << 0),
    CMDLINE_THEME = (1 << 1)
};

typedef void (*NotifyFunc) (GWDSettings *settings);

struct _GWDSettings
{
    GObject                 parent;

    gint                    blur_type;
    gchar                  *metacity_theme_name;
    gint                    metacity_theme_type;
    guint                   cmdline_opts;

    decor_shadow_options_t  active_shadow;
    decor_shadow_options_t  inactive_shadow;
    gboolean                use_tooltips;
    gdouble                 metacity_active_opacity;
    gdouble                 metacity_inactive_opacity;
    gboolean                metacity_active_shade_opacity;
    gboolean                metacity_inactive_shade_opacity;
    gchar                  *metacity_button_layout;
    gint                    titlebar_double_click_action;
    gint                    titlebar_middle_click_action;
    gint                    titlebar_right_click_action;
    gint                    mouse_wheel_action;
    gchar                  *titlebar_font;

    guint                   freeze_count;
    GList                  *notify_funcs;
};

enum
{
    PROP_0,

    PROP_BLUR_TYPE,
    PROP_METACITY_THEME_NAME,
    PROP_CMDLINE_OPTIONS,

    LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL };

enum
{
  UPDATE_DECORATIONS,
  UPDATE_FRAMES,
  UPDATE_METACITY_THEME,
  UPDATE_METACITY_BUTTON_LAYOUT,

  LAST_SIGNAL
};

static guint settings_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GWDSettings, gwd_settings, G_TYPE_OBJECT)

static void
update_decorations (GWDSettings *settings)
{
    g_signal_emit (settings, settings_signals[UPDATE_DECORATIONS], 0);
}

static void
update_frames (GWDSettings *settings)
{
    g_signal_emit (settings, settings_signals[UPDATE_FRAMES], 0);
}

static void
update_metacity_theme (GWDSettings *settings)
{
    g_signal_emit (settings, settings_signals[UPDATE_METACITY_THEME], 0,
                   settings->metacity_theme_type, settings->metacity_theme_name);
}

static void
update_metacity_button_layout (GWDSettings *settings)
{
    g_signal_emit (settings, settings_signals[UPDATE_METACITY_BUTTON_LAYOUT],
                   0, settings->metacity_button_layout);
}

static void
append_to_notify_funcs (GWDSettings *settings,
                        NotifyFunc   func)
{
    /* Remove if found, the new one will replace the old one */
    GList *link = g_list_find (settings->notify_funcs, func);

    if (link)
        settings->notify_funcs = g_list_remove_link (settings->notify_funcs, link);

    settings->notify_funcs = g_list_append (settings->notify_funcs, (gpointer) func);
}

static void
invoke_notify_func (gpointer data,
                    gpointer user_data)
{
    NotifyFunc func;
    GWDSettings *settings;

    func = (NotifyFunc) data;
    settings = GWD_SETTINGS (user_data);

    (*func) (settings);
}

static void
release_notify_funcs (GWDSettings *settings)
{
    if (settings->freeze_count)
        return;

    g_list_foreach (settings->notify_funcs, invoke_notify_func, settings);
    g_list_free (settings->notify_funcs);
    settings->notify_funcs = NULL;
}

static gboolean
get_click_action_value (const gchar *action,
                        gint        *action_value,
                        gint         default_value)
{
    if (!action_value)
        return FALSE;

    *action_value = -1;

    if (strcmp (action, "toggle_shade") == 0)
        *action_value = CLICK_ACTION_SHADE;
    else if (strcmp (action, "toggle_maximize") == 0)
        *action_value = CLICK_ACTION_MAXIMIZE;
    else if (strcmp (action, "minimize") == 0)
        *action_value = CLICK_ACTION_MINIMIZE;
    else if (strcmp (action, "raise") == 0)
        *action_value = CLICK_ACTION_RAISE;
    else if (strcmp (action, "lower") == 0)
        *action_value = CLICK_ACTION_LOWER;
    else if (strcmp (action, "menu") == 0)
        *action_value = CLICK_ACTION_MENU;
    else if (strcmp (action, "none") == 0)
        *action_value = CLICK_ACTION_NONE;

    if (*action_value == -1) {
        *action_value = default_value;
        return FALSE;
    }

    return TRUE;
}

static gboolean
get_wheel_action_value (const gchar *action,
                        gint        *action_value,
                        gint         default_value)
{
    if (!action_value)
        return FALSE;

    *action_value = -1;

    if (strcmp (action, "shade") == 0)
        *action_value = WHEEL_ACTION_SHADE;
    else if (strcmp (action, "none") == 0)
        *action_value = WHEEL_ACTION_NONE;

    if (*action_value == -1) {
        *action_value = default_value;
        return FALSE;
    }

    return TRUE;
}

static void
gwd_settings_finalize (GObject *object)
{
    GWDSettings *settings;

    settings = GWD_SETTINGS (object);

    g_clear_pointer (&settings->metacity_theme_name, g_free);
    g_clear_pointer (&settings->metacity_button_layout, g_free);
    g_clear_pointer (&settings->titlebar_font, g_free);

    G_OBJECT_CLASS (gwd_settings_parent_class)->finalize (object);
}

static void
gwd_settings_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    GWDSettings *settings;

    settings = GWD_SETTINGS (object);

    switch (property_id) {
        case PROP_CMDLINE_OPTIONS:
            settings->cmdline_opts = g_value_get_int (value);
            break;

        case PROP_BLUR_TYPE:
            settings->blur_type = g_value_get_int (value);
            break;

        case PROP_METACITY_THEME_NAME:
            g_free (settings->metacity_theme_name);
            settings->metacity_theme_name = g_value_dup_string (value);
            settings->metacity_theme_type = -1;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_settings_class_init (GWDSettingsClass *settings_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (settings_class);

    object_class->finalize = gwd_settings_finalize;
    object_class->set_property = gwd_settings_set_property;

    properties[PROP_BLUR_TYPE] =
        g_param_spec_int ("blur-type",
                          "Blur Type",
                          "Blur type property",
                          BLUR_TYPE_NONE,
                          BLUR_TYPE_ALL,
                          BLUR_TYPE_NONE,
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    properties[PROP_METACITY_THEME_NAME] =
        g_param_spec_string ("metacity-theme-name",
                             "Metacity Theme Name",
                             "Metacity Theme Name",
                             METACITY_THEME_DEFAULT,
                             G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    properties[PROP_CMDLINE_OPTIONS] =
        g_param_spec_int ("cmdline-options",
                          "Command line options",
                          "Which options were specified on the command line",
                          0, G_MAXINT32, 0,
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, LAST_PROP, properties);

    settings_signals[UPDATE_DECORATIONS] =
        g_signal_new ("update-decorations",
                      GWD_TYPE_SETTINGS, G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    settings_signals[UPDATE_FRAMES] =
        g_signal_new ("update-frames",
                      GWD_TYPE_SETTINGS, G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    settings_signals[UPDATE_METACITY_THEME] =
        g_signal_new ("update-metacity-theme",
                      GWD_TYPE_SETTINGS, G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL, G_TYPE_NONE, 2,
                      G_TYPE_INT, G_TYPE_STRING);

    settings_signals[UPDATE_METACITY_BUTTON_LAYOUT] =
        g_signal_new ("update-metacity-button-layout",
                      GWD_TYPE_SETTINGS, G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                      G_TYPE_STRING);
}

static void
gwd_settings_init (GWDSettings *settings)
{
    settings->use_tooltips = USE_TOOLTIPS_DEFAULT;
    settings->active_shadow.shadow_radius = ACTIVE_SHADOW_RADIUS_DEFAULT;
    settings->active_shadow.shadow_opacity = ACTIVE_SHADOW_OPACITY_DEFAULT;
    settings->active_shadow.shadow_offset_x = ACTIVE_SHADOW_OFFSET_X_DEFAULT;
    settings->active_shadow.shadow_offset_y = ACTIVE_SHADOW_OFFSET_Y_DEFAULT;
    settings->active_shadow.shadow_color[0] = 0;
    settings->active_shadow.shadow_color[1] = 0;
    settings->active_shadow.shadow_color[2] = 0;
    settings->inactive_shadow.shadow_radius = INACTIVE_SHADOW_RADIUS_DEFAULT;
    settings->inactive_shadow.shadow_opacity = INACTIVE_SHADOW_OPACITY_DEFAULT;
    settings->inactive_shadow.shadow_offset_x = INACTIVE_SHADOW_OFFSET_X_DEFAULT;
    settings->inactive_shadow.shadow_offset_y = INACTIVE_SHADOW_OFFSET_Y_DEFAULT;
    settings->inactive_shadow.shadow_color[0] = 0;
    settings->inactive_shadow.shadow_color[1] = 0;
    settings->inactive_shadow.shadow_color[2] = 0;
    settings->blur_type = BLUR_TYPE_DEFAULT;
    settings->metacity_theme_name = g_strdup (METACITY_THEME_DEFAULT);
    settings->metacity_theme_type = -1;
    settings->metacity_active_opacity = METACITY_ACTIVE_OPACITY_DEFAULT;
    settings->metacity_inactive_opacity = METACITY_INACTIVE_OPACITY_DEFAULT;
    settings->metacity_active_shade_opacity = METACITY_ACTIVE_SHADE_OPACITY_DEFAULT;
    settings->metacity_inactive_shade_opacity = METACITY_INACTIVE_SHADE_OPACITY_DEFAULT;
    settings->metacity_button_layout = g_strdup (METACITY_BUTTON_LAYOUT_DEFAULT);
    settings->titlebar_double_click_action = DOUBLE_CLICK_ACTION_DEFAULT;
    settings->titlebar_middle_click_action = MIDDLE_CLICK_ACTION_DEFAULT;
    settings->titlebar_right_click_action = RIGHT_CLICK_ACTION_DEFAULT;
    settings->mouse_wheel_action = WHEEL_ACTION_DEFAULT;
    settings->titlebar_font = g_strdup (TITLEBAR_FONT_DEFAULT);
    settings->cmdline_opts = 0;
    settings->freeze_count = 0;

    /* Append all notify funcs so that external state can be updated in case
     * the settings backend can't do it itself */
    append_to_notify_funcs (settings, update_metacity_theme);
    append_to_notify_funcs (settings, update_metacity_button_layout);
    append_to_notify_funcs (settings, update_frames);
    append_to_notify_funcs (settings, update_decorations);
}

GWDSettings *
gwd_settings_new (gint         blur_type,
                  const gchar *metacity_theme_name)
{
    guint cmdline_opts = 0;

    if (blur_type != BLUR_TYPE_UNSET)
        cmdline_opts |= CMDLINE_BLUR;
    else
        blur_type = BLUR_TYPE_DEFAULT;

    if (metacity_theme_name != NULL)
        cmdline_opts |= CMDLINE_THEME;
    else
        metacity_theme_name = METACITY_THEME_DEFAULT;

    return g_object_new (GWD_TYPE_SETTINGS,
                         "blur-type", blur_type,
                         "metacity-theme-name", metacity_theme_name,
                         "cmdline-options", cmdline_opts,
                         NULL);
}

gint
gwd_settings_get_blur_type (GWDSettings *settings)
{
    return settings->blur_type;
}

const gchar *
gwd_settings_get_metacity_button_layout (GWDSettings *settings)
{
    return settings->metacity_button_layout;
}

const gchar *
gwd_settings_get_metacity_theme_name (GWDSettings *settings)
{
    return settings->metacity_theme_name;
}

gint
gwd_settings_get_metacity_theme_type (GWDSettings *settings)
{
    return settings->metacity_theme_type;
}

const gchar *
gwd_settings_get_titlebar_font (GWDSettings *settings)
{
    return settings->titlebar_font;
}

decor_shadow_options_t
gwd_settings_get_active_shadow (GWDSettings *settings)
{
    return settings->active_shadow;
}

decor_shadow_options_t
gwd_settings_get_inactive_shadow (GWDSettings *settings)
{
    return settings->inactive_shadow;
}

gboolean
gwd_settings_get_use_tooltips (GWDSettings *settings)
{
    return settings->use_tooltips;
}

gdouble
gwd_settings_get_metacity_active_opacity (GWDSettings *settings)
{
    return settings->metacity_active_opacity;
}

gdouble
gwd_settings_get_metacity_inactive_opacity (GWDSettings *settings)
{
    return settings->metacity_inactive_opacity;
}

gboolean
gwd_settings_get_metacity_active_shade_opacity (GWDSettings *settings)
{
    return settings->metacity_active_shade_opacity;
}

gboolean
gwd_settings_get_metacity_inactive_shade_opacity (GWDSettings *settings)
{
    return settings->metacity_inactive_shade_opacity;
}

gint
gwd_settings_get_titlebar_double_click_action (GWDSettings *settings)
{
    return settings->titlebar_double_click_action;
}

gint
gwd_settings_get_titlebar_middle_click_action (GWDSettings *settings)
{
    return settings->titlebar_middle_click_action;
}

gint
gwd_settings_get_titlebar_right_click_action (GWDSettings *settings)
{
    return settings->titlebar_right_click_action;
}

gint
gwd_settings_get_mouse_wheel_action (GWDSettings *settings)
{
    return settings->mouse_wheel_action;
}

void
gwd_settings_freeze_updates (GWDSettings *settings)
{
    ++settings->freeze_count;
}

void
gwd_settings_thaw_updates (GWDSettings *settings)
{
    if (settings->freeze_count)
        --settings->freeze_count;

    release_notify_funcs (settings);
}

gboolean
gwd_settings_shadow_property_changed (GWDSettings *settings,
                                      gdouble      active_shadow_radius,
                                      gdouble      active_shadow_opacity,
                                      gdouble      active_shadow_offset_x,
                                      gdouble      active_shadow_offset_y,
                                      const gchar *active_shadow_color,
                                      gdouble      inactive_shadow_radius,
                                      gdouble      inactive_shadow_opacity,
                                      gdouble      inactive_shadow_offset_x,
                                      gdouble      inactive_shadow_offset_y,
                                      const gchar *inactive_shadow_color)
{
    decor_shadow_options_t active_shadow, inactive_shadow;
    unsigned int           c[4];
    gboolean               changed = FALSE;

    active_shadow.shadow_radius = active_shadow_radius;
    active_shadow.shadow_opacity = active_shadow_opacity;
    active_shadow.shadow_offset_x = active_shadow_offset_x;
    active_shadow.shadow_offset_y = active_shadow_offset_y;

    if (sscanf (active_shadow_color, "#%2x%2x%2x%2x",
                &c[0], &c[1], &c[2], &c[3]) == 4) {
        active_shadow.shadow_color[0] = c[0] << 8 | c[0];
        active_shadow.shadow_color[1] = c[1] << 8 | c[1];
        active_shadow.shadow_color[2] = c[2] << 8 | c[2];
    } else {
        return FALSE;
    }

    if (sscanf (inactive_shadow_color, "#%2x%2x%2x%2x",
                &c[0], &c[1], &c[2], &c[3]) == 4) {
        inactive_shadow.shadow_color[0] = c[0] << 8 | c[0];
        inactive_shadow.shadow_color[1] = c[1] << 8 | c[1];
        inactive_shadow.shadow_color[2] = c[2] << 8 | c[2];
    } else {
        return FALSE;
    }

    inactive_shadow.shadow_radius = inactive_shadow_radius;
    inactive_shadow.shadow_opacity = inactive_shadow_opacity;
    inactive_shadow.shadow_offset_x = inactive_shadow_offset_x;
    inactive_shadow.shadow_offset_y = inactive_shadow_offset_y;

    if (decor_shadow_options_cmp (&settings->inactive_shadow, &inactive_shadow)) {
        changed |= TRUE;
        settings->inactive_shadow = inactive_shadow;
    }

    if (decor_shadow_options_cmp (&settings->active_shadow, &active_shadow)) {
        changed |= TRUE;
        settings->active_shadow = active_shadow;
    }

    if (changed) {
        append_to_notify_funcs (settings, update_decorations);
        release_notify_funcs (settings);
    }

    return changed;
}

gboolean
gwd_settings_use_tooltips_changed (GWDSettings *settings,
                                   gboolean     use_tooltips)
{
    if (settings->use_tooltips == use_tooltips)
        return FALSE;

    settings->use_tooltips = use_tooltips;

    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

gboolean
gwd_settings_blur_changed (GWDSettings *settings,
                           const gchar *type)
{
    gint new_type = BLUR_TYPE_UNSET;

    if (settings->cmdline_opts & CMDLINE_BLUR)
        return FALSE;

    if (strcmp (type, "titlebar") == 0)
        new_type = BLUR_TYPE_TITLEBAR;
    else if (strcmp (type, "all") == 0)
        new_type = BLUR_TYPE_ALL;
    else if (strcmp (type, "none") == 0)
        new_type = BLUR_TYPE_NONE;

    if (new_type == BLUR_TYPE_UNSET)
        return FALSE;

    if (settings->blur_type == new_type)
        return FALSE;

    settings->blur_type = new_type;

    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

gboolean
gwd_settings_metacity_theme_changed (GWDSettings *settings,
                                     gboolean     use_metacity_theme,
                                     gint         metacity_theme_type,
                                     const gchar *metacity_theme_name)
{
    if (settings->cmdline_opts & CMDLINE_THEME)
        return FALSE;

    if (use_metacity_theme) {
        if (g_strcmp0 (metacity_theme_name, settings->metacity_theme_name) == 0 &&
            metacity_theme_type == settings->metacity_theme_type)
            return FALSE;

        g_free (settings->metacity_theme_name);
        settings->metacity_theme_name = g_strdup (metacity_theme_name);
    } else {
        g_free (settings->metacity_theme_name);
        settings->metacity_theme_name = NULL;
    }

    settings->metacity_theme_type = metacity_theme_type;

    append_to_notify_funcs (settings, update_metacity_theme);
    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

gboolean
gwd_settings_opacity_changed (GWDSettings *settings,
                              gdouble      active_opacity,
                              gdouble      inactive_opacity,
                              gboolean     active_shade_opacity,
                              gboolean     inactive_shade_opacity)
{
    if (settings->metacity_active_opacity == active_opacity &&
        settings->metacity_inactive_opacity == inactive_opacity &&
        settings->metacity_active_shade_opacity == active_shade_opacity &&
        settings->metacity_inactive_shade_opacity == inactive_shade_opacity)
        return FALSE;

    settings->metacity_active_opacity = active_opacity;
    settings->metacity_inactive_opacity = inactive_opacity;
    settings->metacity_active_shade_opacity = active_shade_opacity;
    settings->metacity_inactive_shade_opacity = inactive_shade_opacity;

    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

gboolean
gwd_settings_button_layout_changed (GWDSettings *settings,
                                    const gchar *button_layout)
{
    if (g_strcmp0 (settings->metacity_button_layout, button_layout) == 0)
        return FALSE;

    g_free (settings->metacity_button_layout);
    settings->metacity_button_layout = g_strdup (button_layout);

    append_to_notify_funcs (settings, update_metacity_button_layout);
    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

gboolean
gwd_settings_font_changed (GWDSettings *settings,
                           gboolean     titlebar_uses_system_font,
                           const gchar *titlebar_font)
{
    const gchar *use_font = titlebar_font;

    if (titlebar_uses_system_font)
        use_font = NULL;

    if (g_strcmp0 (settings->titlebar_font, use_font) == 0)
        return FALSE;

    g_free (settings->titlebar_font);
    settings->titlebar_font = g_strdup (use_font);

    append_to_notify_funcs (settings, update_decorations);
    append_to_notify_funcs (settings, update_frames);
    release_notify_funcs (settings);

    return TRUE;
}

gboolean
gwd_settings_titlebar_actions_changed (GWDSettings *settings,
                                       const gchar *action_double_click_titlebar,
                                       const gchar *action_middle_click_titlebar,
                                       const gchar *action_right_click_titlebar,
                                       const gchar *mouse_wheel_action)
{
    gboolean ret = FALSE;

    ret |= get_click_action_value (action_double_click_titlebar,
                                   &settings->titlebar_double_click_action,
                                   DOUBLE_CLICK_ACTION_DEFAULT);
    ret |= get_click_action_value (action_middle_click_titlebar,
                                   &settings->titlebar_middle_click_action,
                                   MIDDLE_CLICK_ACTION_DEFAULT);
    ret |= get_click_action_value (action_right_click_titlebar,
                                   &settings->titlebar_right_click_action,
                                   RIGHT_CLICK_ACTION_DEFAULT);
    ret |= get_wheel_action_value (mouse_wheel_action,
                                   &settings->mouse_wheel_action,
                                   WHEEL_ACTION_DEFAULT);

    return ret;
}
