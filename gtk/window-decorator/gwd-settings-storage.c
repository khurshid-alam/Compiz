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
#include <glib.h>

#include <gio/gio.h>

#include <string.h>

#include "gwd-settings.h"
#include "gwd-settings-storage.h"

#define ORG_COMPIZ_GWD_KEY_USE_TOOLTIPS "use-tooltips"
#define ORG_COMPIZ_GWD_KEY_BLUR_TYPE "blur-type"
#define ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY "metacity-theme-active-opacity"
#define ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY "metacity-theme-inactive-opacity"
#define ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY "metacity-theme-active-shade-opacity"
#define ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY "metacity-theme-inactive-shade-opacity"
#define ORG_COMPIZ_GWD_KEY_USE_METACITY_THEME "use-metacity-theme"
#define ORG_COMPIZ_GWD_KEY_MOUSE_WHEEL_ACTION "mouse-wheel-action"

#define ORG_GNOME_METACITY_THEME "theme"

#define ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_DOUBLE_CLICK_TITLEBAR "action-double-click-titlebar"
#define ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_MIDDLE_CLICK_TITLEBAR "action-middle-click-titlebar"
#define ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_RIGHT_CLICK_TITLEBAR "action-right-click-titlebar"
#define ORG_GNOME_DESKTOP_WM_PREFERENCES_THEME "theme"
#define ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_USES_SYSTEM_FONT "titlebar-uses-system-font"
#define ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_FONT "titlebar-font"
#define ORG_GNOME_DESKTOP_WM_PREFERENCES_BUTTON_LAYOUT "button-layout"

#define ORG_MATE_MARCO_GENERAL_ACTION_DOUBLE_CLICK_TITLEBAR "action-double-click-titlebar"
#define ORG_MATE_MARCO_GENERAL_ACTION_MIDDLE_CLICK_TITLEBAR "action-middle-click-titlebar"
#define ORG_MATE_MARCO_GENERAL_ACTION_RIGHT_CLICK_TITLEBAR "action-right-click-titlebar"
#define ORG_MATE_MARCO_GENERAL_THEME "theme"
#define ORG_MATE_MARCO_GENERAL_TITLEBAR_USES_SYSTEM_FONT "titlebar-uses-system-font"
#define ORG_MATE_MARCO_GENERAL_TITLEBAR_FONT "titlebar-font"
#define ORG_MATE_MARCO_GENERAL_BUTTON_LAYOUT "button-layout"

struct _GWDSettingsStorage
{
    GObject      parent;

    GWDSettings *settings;
    gboolean     connect;

    GSettings   *gwd;
    GSettings   *desktop;
    GSettings   *metacity;
    GSettings   *marco;

    gboolean     is_mate_desktop;
};

enum
{
    PROP_0,

    PROP_SETTINGS,
    PROP_CONNECT,

    LAST_PROP
};

static GParamSpec *storage_properties[LAST_PROP] = { NULL };

G_DEFINE_TYPE (GWDSettingsStorage, gwd_settings_storage, G_TYPE_OBJECT)

static inline GSettings *
get_settings_no_abort (const gchar *schema)
{
    GSettingsSchemaSource *source;
    GSettings *settings;

    source = g_settings_schema_source_get_default ();
    settings = NULL;

    if (g_settings_schema_source_lookup (source, schema, TRUE))
        settings = g_settings_new (schema);

    return settings;
}

static void
translate_dashes_to_underscores (gchar *original)
{
    gint i = 0;

    for (i = 0; i < strlen (original); ++i) {
        if (original[i] == '-')
            original[i] = '_';
    }
}

static void
org_compiz_gwd_settings_changed (GSettings          *settings,
                                 const gchar        *key,
                                 GWDSettingsStorage *storage)
{
    if (strcmp (key, ORG_COMPIZ_GWD_KEY_MOUSE_WHEEL_ACTION) == 0)
        gwd_settings_storage_update_titlebar_actions (storage);
    else if (strcmp (key, ORG_COMPIZ_GWD_KEY_BLUR_TYPE) == 0)
        gwd_settings_storage_update_blur (storage);
    else if (strcmp (key, ORG_COMPIZ_GWD_KEY_USE_METACITY_THEME) == 0)
        gwd_settings_storage_update_metacity_theme (storage);
    else if (strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY) == 0 ||
             strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY) == 0 ||
             strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY) == 0 ||
             strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY) == 0)
        gwd_settings_storage_update_opacity (storage);
    else if (strcmp (key, ORG_COMPIZ_GWD_KEY_USE_TOOLTIPS) == 0)
        gwd_settings_storage_update_use_tooltips (storage);
}

static void
org_gnome_desktop_wm_preferences_settings_changed (GSettings          *settings,
                                                   const gchar        *key,
                                                   GWDSettingsStorage *storage)
{
    if (strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_USES_SYSTEM_FONT) == 0 ||
        strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_FONT) == 0)
        gwd_settings_storage_update_font (storage);
    else if (strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_FONT) == 0)
        gwd_settings_storage_update_font (storage);
    else if (strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_DOUBLE_CLICK_TITLEBAR) == 0 ||
             strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_MIDDLE_CLICK_TITLEBAR) == 0 ||
             strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_RIGHT_CLICK_TITLEBAR) == 0)
        gwd_settings_storage_update_titlebar_actions (storage);
    else if (strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_THEME) == 0)
        gwd_settings_storage_update_metacity_theme (storage);
    else if (strcmp (key, ORG_GNOME_DESKTOP_WM_PREFERENCES_BUTTON_LAYOUT) == 0)
        gwd_settings_storage_update_button_layout (storage);
}

static void
org_gnome_metacity_settings_changed (GSettings          *settings,
                                     const gchar        *key,
                                     GWDSettingsStorage *storage)
{
    if (strcmp (key, ORG_GNOME_METACITY_THEME) == 0)
        gwd_settings_storage_update_metacity_theme (storage);
}

static void
org_mate_marco_general_settings_changed (GSettings          *settings,
                                         const gchar        *key,
                                         GWDSettingsStorage *storage)
{
    if (strcmp (key, ORG_MATE_MARCO_GENERAL_TITLEBAR_USES_SYSTEM_FONT) == 0 ||
        strcmp (key, ORG_MATE_MARCO_GENERAL_TITLEBAR_FONT) == 0)
        gwd_settings_storage_update_font (storage);
    else if (strcmp (key, ORG_MATE_MARCO_GENERAL_TITLEBAR_FONT) == 0)
        gwd_settings_storage_update_font (storage);
    else if (strcmp (key, ORG_MATE_MARCO_GENERAL_ACTION_DOUBLE_CLICK_TITLEBAR) == 0 ||
             strcmp (key, ORG_MATE_MARCO_GENERAL_ACTION_MIDDLE_CLICK_TITLEBAR) == 0 ||
             strcmp (key, ORG_MATE_MARCO_GENERAL_ACTION_RIGHT_CLICK_TITLEBAR) == 0)
        gwd_settings_storage_update_titlebar_actions (storage);
    else if (strcmp (key, ORG_MATE_MARCO_GENERAL_THEME) == 0)
        gwd_settings_storage_update_metacity_theme (storage);
    else if (strcmp (key, ORG_MATE_MARCO_GENERAL_BUTTON_LAYOUT) == 0)
        gwd_settings_storage_update_button_layout (storage);
}

static void
gwd_settings_storage_constructed (GObject *object)
{
    GWDSettingsStorage *storage;

    storage = GWD_SETTINGS_STORAGE (object);

    G_OBJECT_CLASS (gwd_settings_storage_parent_class)->constructed (object);

    if (storage->gwd && storage->connect) {
        g_signal_connect (storage->gwd, "changed",
                          G_CALLBACK (org_compiz_gwd_settings_changed),
                          storage);
    }

    if (storage->desktop && storage->connect) {
        g_signal_connect (storage->desktop, "changed",
                          G_CALLBACK (org_gnome_desktop_wm_preferences_settings_changed),
                          storage);
    }

    if (storage->metacity && storage->connect) {
        g_signal_connect (storage->metacity, "changed",
                          G_CALLBACK (org_gnome_metacity_settings_changed),
                          storage);
    }

    if (storage->marco && storage->connect) {
        g_signal_connect (storage->marco, "changed",
                          G_CALLBACK (org_mate_marco_general_settings_changed),
                          storage);
    }
}

static void
gwd_settings_storage_dispose (GObject *object)
{
    GWDSettingsStorage *storage;

    storage = GWD_SETTINGS_STORAGE (object);

    g_clear_object (&storage->settings);

    g_clear_object (&storage->gwd);
    g_clear_object (&storage->desktop);
    g_clear_object (&storage->metacity);
    g_clear_object (&storage->marco);

    G_OBJECT_CLASS (gwd_settings_storage_parent_class)->dispose (object);
}

static void
gwd_settings_storage_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
    GWDSettingsStorage *storage;

    storage = GWD_SETTINGS_STORAGE (object);

    switch (property_id) {
        case PROP_SETTINGS:
            storage->settings = g_value_dup_object (value);
            break;

        case PROP_CONNECT:
            storage->connect = g_value_get_boolean (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_settings_storage_class_init (GWDSettingsStorageClass *storage_class)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (storage_class);

    object_class->constructed = gwd_settings_storage_constructed;
    object_class->dispose = gwd_settings_storage_dispose;
    object_class->set_property = gwd_settings_storage_set_property;

    storage_properties[PROP_SETTINGS] =
        g_param_spec_object ("settings", "GWDSettings", "GWDSettings",
                             GWD_TYPE_SETTINGS,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
                             G_PARAM_STATIC_STRINGS);

    storage_properties[PROP_CONNECT] =
        g_param_spec_boolean ("connect", "Connect", "Connect",
                              TRUE,
                              G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
                              G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, LAST_PROP,
                                       storage_properties);
}

void
gwd_settings_storage_init (GWDSettingsStorage *storage)
{
    storage->gwd = get_settings_no_abort ("org.compiz.gwd");
    storage->desktop = get_settings_no_abort ("org.gnome.desktop.wm.preferences");
    storage->metacity = get_settings_no_abort ("org.gnome.metacity");
    storage->marco = get_settings_no_abort ("org.mate.Marco.general");

    if (storage->marco) {
        const gchar *xdg_current_desktop;

        xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");

        if (xdg_current_desktop) {
            gchar **desktops;
            gint i;

            desktops = g_strsplit (xdg_current_desktop, ":", -1);

            for (i = 0; desktops[i] != NULL; i++) {
                if (g_strcmp0 (desktops[i], "MATE") == 0) {
                    storage->is_mate_desktop = TRUE;
                    break;
                }
            }

            g_strfreev (desktops);
        }
    }
}

GWDSettingsStorage *
gwd_settings_storage_new (GWDSettings *settings,
                          gboolean     connect)
{
    return g_object_new (GWD_TYPE_SETTINGS_STORAGE,
                         "settings", settings,
                         "connect", connect,
                         NULL);
}

void
gwd_settings_storage_update_use_tooltips (GWDSettingsStorage *storage)
{
    gboolean use_tooltips;

    if (!storage->gwd)
        return;

    use_tooltips = g_settings_get_boolean (storage->gwd, ORG_COMPIZ_GWD_KEY_USE_TOOLTIPS);

    gwd_settings_use_tooltips_changed (storage->settings, use_tooltips);
}

void
gwd_settings_storage_update_blur (GWDSettingsStorage *storage)
{
    gchar *blur_type;

    if (!storage->gwd)
        return;

    blur_type = g_settings_get_string (storage->gwd, ORG_COMPIZ_GWD_KEY_BLUR_TYPE);
    gwd_settings_blur_changed (storage->settings, blur_type);
    g_free (blur_type);
}

void
gwd_settings_storage_update_metacity_theme (GWDSettingsStorage *storage)
{
    gboolean use_metacity_theme;
    gchar *theme;

    if (!storage->gwd)
        return;

    use_metacity_theme = g_settings_get_boolean (storage->gwd, ORG_COMPIZ_GWD_KEY_USE_METACITY_THEME);

    if (storage->is_mate_desktop)
        theme = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_THEME);
    else if (storage->metacity)
        theme = g_settings_get_string (storage->metacity, ORG_GNOME_METACITY_THEME);
    else
        return;

    gwd_settings_metacity_theme_changed (storage->settings, use_metacity_theme, theme);
    g_free (theme);
}

void
gwd_settings_storage_update_opacity (GWDSettingsStorage *storage)
{
    gdouble active;
    gdouble inactive;
    gboolean active_shade;
    gboolean inactive_shade;

    if (!storage->gwd)
        return;

    active = g_settings_get_double (storage->gwd, ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY);
    inactive = g_settings_get_double (storage->gwd, ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY);
    active_shade = g_settings_get_boolean (storage->gwd, ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY);
    inactive_shade = g_settings_get_boolean (storage->gwd, ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY);

    gwd_settings_opacity_changed (storage->settings, active, inactive,
                                  active_shade, inactive_shade);
}

void
gwd_settings_storage_update_button_layout (GWDSettingsStorage *storage)
{
    gchar *button_layout;

    if (storage->is_mate_desktop)
        button_layout = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_BUTTON_LAYOUT);
    else if (storage->desktop)
        button_layout = g_settings_get_string (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_BUTTON_LAYOUT);
    else
        return;

    gwd_settings_button_layout_changed (storage->settings, button_layout);
    g_free (button_layout);
}

void
gwd_settings_storage_update_font (GWDSettingsStorage *storage)
{
    gchar *titlebar_font;
    gboolean titlebar_system_font;

    if (storage->is_mate_desktop) {
        titlebar_font = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_TITLEBAR_FONT);
        titlebar_system_font = g_settings_get_boolean (storage->marco, ORG_MATE_MARCO_GENERAL_TITLEBAR_USES_SYSTEM_FONT);
    } else if (storage->desktop) {
        titlebar_font = g_settings_get_string (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_FONT);
        titlebar_system_font = g_settings_get_boolean (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_USES_SYSTEM_FONT);
    } else
        return;

    gwd_settings_font_changed (storage->settings, titlebar_system_font, titlebar_font);
    g_free (titlebar_font);
}

void
gwd_settings_storage_update_titlebar_actions (GWDSettingsStorage *storage)
{
    gchar *double_click_action;
    gchar *middle_click_action;
    gchar *right_click_action;
    gchar *mouse_wheel_action;

    if (!storage->gwd)
        return;

    if (storage->is_mate_desktop) {
        double_click_action = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_ACTION_DOUBLE_CLICK_TITLEBAR);
        middle_click_action = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_ACTION_MIDDLE_CLICK_TITLEBAR);
        right_click_action = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_ACTION_RIGHT_CLICK_TITLEBAR);
    } else if (storage->desktop) {
        double_click_action = g_settings_get_string (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_DOUBLE_CLICK_TITLEBAR);
        middle_click_action = g_settings_get_string (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_MIDDLE_CLICK_TITLEBAR);
        right_click_action = g_settings_get_string (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_RIGHT_CLICK_TITLEBAR);
    } else
        return;

    translate_dashes_to_underscores (double_click_action);
    translate_dashes_to_underscores (middle_click_action);
    translate_dashes_to_underscores (right_click_action);

    mouse_wheel_action = g_settings_get_string (storage->gwd, ORG_COMPIZ_GWD_KEY_MOUSE_WHEEL_ACTION);

    gwd_settings_titlebar_actions_changed (storage->settings, double_click_action,
                                           middle_click_action, right_click_action,
                                           mouse_wheel_action);

    g_free (double_click_action);
    g_free (middle_click_action);
    g_free (right_click_action);
    g_free (mouse_wheel_action);
}
