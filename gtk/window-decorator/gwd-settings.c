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
#include "gwd-settings-writable-interface.h"
#include "decoration.h"

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

    decor_shadow_options_t  active_shadow;
    decor_shadow_options_t  inactive_shadow;
    gboolean                use_tooltips;
    gint                    blur_type;
    gchar                  *metacity_theme;
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

    guint                   cmdline_opts;

    guint                   freeze_count;
    GList                  *notify_funcs;
};

enum
{
    PROP_0,

    PROP_ACTIVE_SHADOW,
    PROP_INACTIVE_SHADOW,
    PROP_USE_TOOLTIPS,
    PROP_BLUR,
    PROP_METACITY_THEME,
    PROP_ACTIVE_OPACITY,
    PROP_INACTIVE_OPACITY,
    PROP_ACTIVE_SHADE_OPACITY,
    PROP_INACTIVE_SHADE_OPACITY,
    PROP_BUTTON_LAYOUT,
    PROP_TITLEBAR_ACTION_DOUBLE_CLICK,
    PROP_TITLEBAR_ACTION_MIDDLE_CLICK,
    PROP_TITLEBAR_ACTION_RIGHT_CLICK,
    PROP_MOUSE_WHEEL_ACTION,
    PROP_TITLEBAR_FONT,
    PROP_CMDLINE_OPTIONS,

    LAST_PROP
};

static GParamSpec *settings_properties[LAST_PROP] = { NULL };

enum
{
  UPDATE_DECORATIONS,
  UPDATE_FRAMES,
  UPDATE_METACITY_THEME,
  UPDATE_METACITY_BUTTON_LAYOUT,

  LAST_SIGNAL
};

static guint settings_signals[LAST_SIGNAL] = { 0 };

static void gwd_settings_writable_interface_init (GWDSettingsWritableInterface *interface);

G_DEFINE_TYPE_WITH_CODE (GWDSettings, gwd_settings, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GWD_TYPE_WRITABLE_SETTINGS_INTERFACE,
                                                gwd_settings_writable_interface_init))

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
    g_signal_emit (settings, settings_signals[UPDATE_METACITY_THEME], 0);
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

gboolean
gwd_settings_shadow_property_changed (GWDSettingsWritable *writable,
                                      gdouble              active_shadow_radius,
                                      gdouble              active_shadow_opacity,
                                      gdouble              active_shadow_offset_x,
                                      gdouble              active_shadow_offset_y,
                                      const gchar         *active_shadow_color,
                                      gdouble              inactive_shadow_radius,
                                      gdouble              inactive_shadow_opacity,
                                      gdouble              inactive_shadow_offset_x,
                                      gdouble              inactive_shadow_offset_y,
                                      const gchar         *inactive_shadow_color)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

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

static gboolean
gwd_settings_use_tooltips_changed (GWDSettingsWritable *writable,
                                   gboolean             use_tooltips)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

    if (settings->use_tooltips == use_tooltips)
        return FALSE;

    settings->use_tooltips = use_tooltips;

    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

static gboolean
gwd_settings_blur_changed (GWDSettingsWritable *writable,
                           const gchar         *type)

{
    GWDSettings *settings = GWD_SETTINGS (writable);
    gint new_type = -1;

    if (settings->cmdline_opts & CMDLINE_BLUR)
        return FALSE;

    if (strcmp (type, "titlebar") == 0)
        new_type = BLUR_TYPE_TITLEBAR;
    else if (strcmp (type, "all") == 0)
        new_type = BLUR_TYPE_ALL;
    else if (strcmp (type, "none") == 0)
        new_type = BLUR_TYPE_NONE;

    if (new_type == -1)
        return FALSE;

    if (settings->blur_type == new_type)
        return FALSE;

    settings->blur_type = new_type;

    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

static gboolean
gwd_settings_metacity_theme_changed (GWDSettingsWritable *writable,
                                     gboolean             use_metacity_theme,
                                     const gchar         *metacity_theme)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

    if (settings->cmdline_opts & CMDLINE_THEME)
        return FALSE;

    if (!metacity_theme)
        return FALSE;

    if (use_metacity_theme) {
        if (g_strcmp0 (metacity_theme, settings->metacity_theme) == 0)
            return FALSE;

        g_free (settings->metacity_theme);
        settings->metacity_theme = g_strdup (metacity_theme);
    } else {
        g_free (settings->metacity_theme);
        settings->metacity_theme = g_strdup ("");
    }

    append_to_notify_funcs (settings, update_metacity_theme);
    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

static gboolean
gwd_settings_opacity_changed (GWDSettingsWritable *writable,
                              gdouble              active_opacity,
                              gdouble              inactive_opacity,
                              gboolean             active_shade_opacity,
                              gboolean             inactive_shade_opacity)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

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

static gboolean
gwd_settings_button_layout_changed (GWDSettingsWritable *writable,
                                    const gchar         *button_layout)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

    if (!button_layout)
        return FALSE;

    if (g_strcmp0 (settings->metacity_button_layout, button_layout) == 0)
        return FALSE;

    g_free (settings->metacity_button_layout);
    settings->metacity_button_layout = g_strdup (button_layout);

    append_to_notify_funcs (settings, update_metacity_button_layout);
    append_to_notify_funcs (settings, update_decorations);
    release_notify_funcs (settings);

    return TRUE;
}

static gboolean
gwd_settings_font_changed (GWDSettingsWritable *writable,
                           gboolean             titlebar_uses_system_font,
                           const gchar         *titlebar_font)
{
    GWDSettings *settings = GWD_SETTINGS (writable);
    const gchar *no_font = NULL;
    const gchar *use_font = NULL;

    if (!titlebar_font)
        return FALSE;

    if (titlebar_uses_system_font)
        use_font = no_font;
    else
        use_font = titlebar_font;

    if (g_strcmp0 (settings->titlebar_font, use_font) == 0)
        return FALSE;

    g_free (settings->titlebar_font);
    settings->titlebar_font = use_font ? g_strdup (use_font) : NULL;

    append_to_notify_funcs (settings, update_decorations);
    append_to_notify_funcs (settings, update_frames);
    release_notify_funcs (settings);

    return TRUE;
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

static gboolean
gwd_settings_actions_changed (GWDSettingsWritable *writable,
                              const gchar         *action_double_click_titlebar,
                              const gchar         *action_middle_click_titlebar,
                              const gchar         *action_right_click_titlebar,
                              const gchar         *mouse_wheel_action)
{
    GWDSettings *settings = GWD_SETTINGS (writable);
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

static void
gwd_settings_freeze_updates (GWDSettingsWritable *writable)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

    ++settings->freeze_count;
}

static void
gwd_settings_thaw_updates (GWDSettingsWritable *writable)
{
    GWDSettings *settings = GWD_SETTINGS (writable);

    if (settings->freeze_count)
        --settings->freeze_count;

    release_notify_funcs (settings);
}

static void
gwd_settings_writable_interface_init (GWDSettingsWritableInterface *interface)
{
    interface->shadow_property_changed = gwd_settings_shadow_property_changed;
    interface->use_tooltips_changed = gwd_settings_use_tooltips_changed;
    interface->blur_changed = gwd_settings_blur_changed;
    interface->metacity_theme_changed = gwd_settings_metacity_theme_changed;
    interface->opacity_changed = gwd_settings_opacity_changed;
    interface->button_layout_changed = gwd_settings_button_layout_changed;
    interface->font_changed = gwd_settings_font_changed;
    interface->titlebar_actions_changed = gwd_settings_actions_changed;
    interface->freeze_updates = gwd_settings_freeze_updates;
    interface->thaw_updates = gwd_settings_thaw_updates;
}

static void
gwd_settings_finalize (GObject *object)
{
    GWDSettings *settings;

    settings = GWD_SETTINGS (object);

    g_clear_pointer (&settings->metacity_theme, g_free);
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

        case PROP_BLUR:
            settings->blur_type = g_value_get_int (value);
            break;

        case PROP_METACITY_THEME:
            g_free (settings->metacity_theme);
            settings->metacity_theme = g_value_dup_string (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_settings_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    GWDSettings *settings;

    settings = GWD_SETTINGS (object);

    switch (property_id) {
        case PROP_ACTIVE_SHADOW:
            g_value_set_pointer (value, &settings->active_shadow);
            break;

        case PROP_INACTIVE_SHADOW:
            g_value_set_pointer (value, &settings->inactive_shadow);
            break;

        case PROP_USE_TOOLTIPS:
            g_value_set_boolean (value, settings->use_tooltips);
            break;

        case PROP_BLUR:
            g_value_set_int (value, settings->blur_type);
            break;

        case PROP_METACITY_THEME:
            g_value_set_string (value, settings->metacity_theme);
            break;

        case PROP_ACTIVE_OPACITY:
            g_value_set_double (value, settings->metacity_active_opacity);
            break;

        case PROP_INACTIVE_OPACITY:
            g_value_set_double (value, settings->metacity_inactive_opacity);
            break;

        case PROP_ACTIVE_SHADE_OPACITY:
            g_value_set_boolean (value, settings->metacity_active_shade_opacity);
            break;

        case PROP_INACTIVE_SHADE_OPACITY:
            g_value_set_boolean (value, settings->metacity_inactive_shade_opacity);
            break;

        case PROP_BUTTON_LAYOUT:
            g_value_set_string (value, settings->metacity_button_layout);
            break;

        case PROP_TITLEBAR_ACTION_DOUBLE_CLICK:
            g_value_set_int (value, settings->titlebar_double_click_action);
            break;

        case PROP_TITLEBAR_ACTION_MIDDLE_CLICK:
            g_value_set_int (value, settings->titlebar_middle_click_action);
            break;

        case PROP_TITLEBAR_ACTION_RIGHT_CLICK:
            g_value_set_int (value, settings->titlebar_right_click_action);
            break;

        case PROP_MOUSE_WHEEL_ACTION:
            g_value_set_int (value, settings->mouse_wheel_action);
            break;

        case PROP_TITLEBAR_FONT:
            g_value_set_string (value, settings->titlebar_font);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_settings_class_init (GWDSettingsClass *settings_class)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (settings_class);

    object_class->finalize = gwd_settings_finalize;
    object_class->get_property = gwd_settings_get_property;
    object_class->set_property = gwd_settings_set_property;

    settings_properties[PROP_ACTIVE_SHADOW] =
        g_param_spec_pointer ("active-shadow",
                              "Active Shadow",
                              "Active Shadow Settings",
                              G_PARAM_READABLE);

    settings_properties[PROP_INACTIVE_SHADOW] =
        g_param_spec_pointer ("inactive-shadow",
                              "Inactive Shadow",
                              "Inactive Shadow",
                              G_PARAM_READABLE);

    settings_properties[PROP_USE_TOOLTIPS] =
        g_param_spec_boolean ("use-tooltips",
                              "Use Tooltips",
                              "Use Tooltips Setting",
                              USE_TOOLTIPS_DEFAULT,
                              G_PARAM_READABLE);

    settings_properties[PROP_BLUR] =
        g_param_spec_int ("blur",
                          "Blur Type",
                          "Blur type property",
                          BLUR_TYPE_NONE,
                          BLUR_TYPE_ALL,
                          BLUR_TYPE_NONE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    settings_properties[PROP_METACITY_THEME] =
        g_param_spec_string ("metacity-theme",
                             "Metacity Theme",
                             "Metacity Theme Setting",
                             METACITY_THEME_DEFAULT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    settings_properties[PROP_ACTIVE_OPACITY] =
        g_param_spec_double ("metacity-active-opacity",
                             "Metacity Active Opacity",
                             "Metacity Active Opacity",
                             0.0,
                             1.0,
                             METACITY_ACTIVE_OPACITY_DEFAULT,
                             G_PARAM_READABLE);

    settings_properties[PROP_INACTIVE_OPACITY] =
        g_param_spec_double ("metacity-inactive-opacity",
                             "Metacity Inactive Opacity",
                             "Metacity Inactive Opacity",
                             0.0,
                             1.0,
                             METACITY_INACTIVE_OPACITY_DEFAULT,
                             G_PARAM_READABLE);

    settings_properties[PROP_ACTIVE_SHADE_OPACITY] =
        g_param_spec_boolean ("metacity-active-shade-opacity",
                              "Metacity Active Shade Opacity",
                              "Metacity Active Shade Opacity",
                              METACITY_ACTIVE_SHADE_OPACITY_DEFAULT,
                              G_PARAM_READABLE);

    settings_properties[PROP_INACTIVE_SHADE_OPACITY] =
        g_param_spec_boolean ("metacity-inactive-shade-opacity",
                              "Metacity Inactive Shade Opacity",
                              "Metacity Inactive Shade Opacity",
                              METACITY_INACTIVE_SHADE_OPACITY_DEFAULT,
                              G_PARAM_READABLE);

    settings_properties[PROP_BUTTON_LAYOUT] =
        g_param_spec_string ("metacity-button-layout",
                             "Metacity Button Layout",
                             "Metacity Button Layout",
                             METACITY_BUTTON_LAYOUT_DEFAULT,
                             G_PARAM_READABLE);

    settings_properties[PROP_TITLEBAR_ACTION_DOUBLE_CLICK] =
        g_param_spec_int ("titlebar-double-click-action",
                          "Titlebar Action Double Click",
                          "Titlebar Action Double Click",
                          CLICK_ACTION_NONE,
                          CLICK_ACTION_MENU,
                          DOUBLE_CLICK_ACTION_DEFAULT,
                          G_PARAM_READABLE);

    settings_properties[PROP_TITLEBAR_ACTION_MIDDLE_CLICK] =
        g_param_spec_int ("titlebar-middle-click-action",
                          "Titlebar Action Middle Click",
                          "Titlebar Action Middle Click",
                          CLICK_ACTION_NONE,
                          CLICK_ACTION_MENU,
                          MIDDLE_CLICK_ACTION_DEFAULT,
                          G_PARAM_READABLE);

    settings_properties[PROP_TITLEBAR_ACTION_RIGHT_CLICK] =
        g_param_spec_int ("titlebar-right-click-action",
                          "Titlebar Action Right Click",
                          "Titlebar Action Right Click",
                          CLICK_ACTION_NONE,
                          CLICK_ACTION_MENU,
                          RIGHT_CLICK_ACTION_DEFAULT,
                          G_PARAM_READABLE);

    settings_properties[PROP_MOUSE_WHEEL_ACTION] =
        g_param_spec_int ("mouse-wheel-action",
                          "Mouse Wheel Action",
                          "Mouse Wheel Action",
                          WHEEL_ACTION_NONE,
                          WHEEL_ACTION_SHADE,
                          WHEEL_ACTION_DEFAULT,
                          G_PARAM_READABLE);

    settings_properties[PROP_TITLEBAR_FONT] =
        g_param_spec_string ("titlebar-font",
                             "Titlebar Font",
                             "Titlebar Font",
                             TITLEBAR_FONT_DEFAULT,
                             G_PARAM_READABLE);

    settings_properties[PROP_CMDLINE_OPTIONS] =
        g_param_spec_int ("cmdline-options",
                          "Command line options",
                          "Which options were specified on the command line",
                          0, G_MAXINT32, 0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties (object_class, LAST_PROP,
                                       settings_properties);

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
                      0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    settings_signals[UPDATE_METACITY_BUTTON_LAYOUT] =
        g_signal_new ("update-metacity-button-layout",
                      GWD_TYPE_SETTINGS, G_SIGNAL_RUN_LAST,
                      0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
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
    settings->metacity_theme = g_strdup (METACITY_THEME_DEFAULT);
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

static gboolean
set_blur_construction_value (gint       *blur,
                             GParameter *params,
                             GValue     *blur_value)
{
    if (blur) {
        g_value_set_int (blur_value, *blur);

        params->name = "blur";
        params->value = *blur_value;

        return TRUE;
    }

    return FALSE;
}

static gboolean
set_metacity_theme_construction_value (const gchar **metacity_theme,
                                       GParameter   *params,
                                       GValue       *metacity_theme_value)
{
    if (metacity_theme) {
        g_value_set_string (metacity_theme_value, *metacity_theme);

        params->name = "metacity-theme";
        params->value = *metacity_theme_value;

        return TRUE;
    }

    return FALSE;
}

static guint
set_flag_and_increment (guint  n_param,
                        guint *flags,
                        guint  flag)
{
    if (!flags)
        return n_param;

    *flags |= flag;
    return n_param + 1;
}

GWDSettings *
gwd_settings_new (gint         *blur,
                  const gchar **metacity_theme)
{
    /* Always N command line parameters + 2 for command line options enum */
    const guint     gwd_settings_impl_n_construction_params = 4;
    GParameter      param[gwd_settings_impl_n_construction_params];
    GWDSettings     *settings = NULL;

    int      n_param = 0;
    guint    cmdline_opts = 0;

    GValue blur_value = G_VALUE_INIT;
    GValue metacity_theme_value = G_VALUE_INIT;
    GValue cmdline_opts_value = G_VALUE_INIT;

    g_value_init (&blur_value, G_TYPE_INT);
    g_value_init (&metacity_theme_value, G_TYPE_STRING);
    g_value_init (&cmdline_opts_value, G_TYPE_INT);

    if (set_blur_construction_value (blur, &param[n_param], &blur_value))
        n_param = set_flag_and_increment (n_param, &cmdline_opts, CMDLINE_BLUR);

    if (set_metacity_theme_construction_value (metacity_theme, &param[n_param], &metacity_theme_value))
        n_param = set_flag_and_increment (n_param, &cmdline_opts, CMDLINE_THEME);

    g_value_set_int (&cmdline_opts_value, cmdline_opts);

    param[n_param].name = "cmdline-options";
    param[n_param].value = cmdline_opts_value;

    ++n_param;

    settings = g_object_newv (GWD_TYPE_SETTINGS, n_param, param);

    g_value_unset (&blur_value);
    g_value_unset (&metacity_theme_value);
    g_value_unset (&cmdline_opts_value);

    return settings;
}

const gchar *
gwd_settings_get_metacity_theme (GWDSettings *settings)
{
    return settings->metacity_theme;
}

const gchar *
gwd_settings_get_titlebar_font (GWDSettings *settings)
{
    return settings->titlebar_font;
}
