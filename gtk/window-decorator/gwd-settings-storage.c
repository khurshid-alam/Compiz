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

#include "gwd-settings-writable-interface.h"
#include "gwd-settings-storage.h"

const gchar * ORG_COMPIZ_GWD = "org.compiz.gwd";
const gchar * ORG_GNOME_METACITY = "org.gnome.metacity";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES = "org.gnome.desktop.wm.preferences";
const gchar * ORG_MATE_MARCO_GENERAL = "org.mate.Marco.general";

const gchar * ORG_COMPIZ_GWD_KEY_USE_TOOLTIPS = "use-tooltips";
const gchar * ORG_COMPIZ_GWD_KEY_BLUR_TYPE = "blur-type";
const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY = "metacity-theme-active-opacity";
const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY = "metacity-theme-inactive-opacity";
const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY = "metacity-theme-active-shade-opacity";
const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY = "metacity-theme-inactive-shade-opacity";
const gchar * ORG_COMPIZ_GWD_KEY_USE_METACITY_THEME = "use-metacity-theme";
const gchar * ORG_COMPIZ_GWD_KEY_MOUSE_WHEEL_ACTION = "mouse-wheel-action";
const gchar * ORG_GNOME_METACITY_THEME = "theme";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_DOUBLE_CLICK_TITLEBAR = "action-double-click-titlebar";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_MIDDLE_CLICK_TITLEBAR = "action-middle-click-titlebar";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_RIGHT_CLICK_TITLEBAR = "action-right-click-titlebar";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_THEME = "theme";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_USES_SYSTEM_FONT = "titlebar-uses-system-font";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_FONT = "titlebar-font";
const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_BUTTON_LAYOUT = "button-layout";
const gchar * ORG_MATE_MARCO_GENERAL_ACTION_DOUBLE_CLICK_TITLEBAR = "action-double-click-titlebar";
const gchar * ORG_MATE_MARCO_GENERAL_ACTION_MIDDLE_CLICK_TITLEBAR = "action-middle-click-titlebar";
const gchar * ORG_MATE_MARCO_GENERAL_ACTION_RIGHT_CLICK_TITLEBAR = "action-right-click-titlebar";
const gchar * ORG_MATE_MARCO_GENERAL_THEME = "theme";
const gchar * ORG_MATE_MARCO_GENERAL_TITLEBAR_USES_SYSTEM_FONT = "titlebar-uses-system-font";
const gchar * ORG_MATE_MARCO_GENERAL_TITLEBAR_FONT = "titlebar-font";
const gchar * ORG_MATE_MARCO_GENERAL_BUTTON_LAYOUT = "button-layout";

struct _GWDSettingsStorage
{
    GObject              parent;

    GWDSettingsWritable *writable;
    gboolean             connect;

    GSettings           *gwd;
    GSettings           *desktop;
    GSettings           *metacity;
    GSettings           *marco;

    gboolean             is_mate_desktop;
};

enum
{
    PROP_0,

    PROP_WRITABLE,
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

static inline gchar *
translate_dashes_to_underscores (const gchar *original)
{
    gint i = 0;
    gchar *copy = g_strdup (original);

    if (!copy)
	return NULL;

    for (; i < strlen (copy); ++i)
    {
	if (copy[i] == '-')
	    copy[i] = '_';
    }

    return copy;
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
    else if (strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY)	     == 0 ||
	     strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY)  == 0 ||
	     strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY)          == 0 ||
	     strcmp (key, ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY)    == 0)
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

    g_clear_object (&storage->writable);

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
        case PROP_WRITABLE:
            storage->writable = g_value_dup_object (value);
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

    storage_properties[PROP_WRITABLE] =
        g_param_spec_object ("writable", "GWDWritableSettings",
                             "A GWDWritableSettings object",
                             GWD_TYPE_WRITABLE_SETTINGS_INTERFACE,
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
    storage->gwd = get_settings_no_abort (ORG_COMPIZ_GWD);
    storage->desktop = get_settings_no_abort (ORG_GNOME_DESKTOP_WM_PREFERENCES);
    storage->metacity = get_settings_no_abort (ORG_GNOME_METACITY);
    storage->marco = get_settings_no_abort (ORG_MATE_MARCO_GENERAL);

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
gwd_settings_storage_new (GWDSettingsWritable *writable,
                          gboolean             connect)
{
    return g_object_new (GWD_TYPE_SETTINGS_STORAGE,
                         "writable", writable,
                         "connect", connect,
                         NULL);
}

gboolean
gwd_settings_storage_update_use_tooltips (GWDSettingsStorage *storage)
{
    if (!storage->gwd)
	return FALSE;

    return gwd_settings_writable_use_tooltips_changed (storage->writable,
						       g_settings_get_boolean (storage->gwd,
									       ORG_COMPIZ_GWD_KEY_USE_TOOLTIPS));
}

gboolean
gwd_settings_storage_update_blur (GWDSettingsStorage *storage)
{
    if (!storage->gwd)
	return FALSE;

    return gwd_settings_writable_blur_changed (storage->writable,
					       g_settings_get_string (storage->gwd,
								      ORG_COMPIZ_GWD_KEY_BLUR_TYPE));
}

gboolean
gwd_settings_storage_update_metacity_theme (GWDSettingsStorage *storage)
{
    gboolean use_metacity_theme;
    gchar *theme;

    if (!storage->gwd)
        return FALSE;

    use_metacity_theme = g_settings_get_boolean (storage->gwd, ORG_COMPIZ_GWD_KEY_USE_METACITY_THEME);

    if (storage->is_mate_desktop)
        theme = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_THEME);
    else if (storage->metacity)
        theme = g_settings_get_string (storage->metacity, ORG_GNOME_METACITY_THEME);
    else
	return FALSE;

    return gwd_settings_writable_metacity_theme_changed (storage->writable,
                                                         use_metacity_theme,
                                                         theme);
}

gboolean
gwd_settings_storage_update_opacity (GWDSettingsStorage *storage)
{
    if (!storage->gwd)
	return FALSE;

    return gwd_settings_writable_opacity_changed (storage->writable,
						  g_settings_get_double (storage->gwd,
									 ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY),
						  g_settings_get_double (storage->gwd,
									 ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY),
						  g_settings_get_boolean (storage->gwd,
									  ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY),
						  g_settings_get_boolean (storage->gwd,
									  ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY));
}

gboolean
gwd_settings_storage_update_button_layout (GWDSettingsStorage *storage)
{
    gchar *button_layout;

    if (storage->is_mate_desktop)
	button_layout = g_settings_get_string (storage->marco, ORG_MATE_MARCO_GENERAL_BUTTON_LAYOUT);
    else if (storage->desktop)
	button_layout = g_settings_get_string (storage->desktop, ORG_GNOME_DESKTOP_WM_PREFERENCES_BUTTON_LAYOUT);
    else
	return FALSE;

    return gwd_settings_writable_button_layout_changed (storage->writable,
							button_layout);
}

gboolean
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
	return FALSE;

    return gwd_settings_writable_font_changed (storage->writable,
					       titlebar_system_font,
					       titlebar_font);
}

gboolean
gwd_settings_storage_update_titlebar_actions (GWDSettingsStorage *storage)
{
    gchar *double_click_action, *middle_click_action, *right_click_action;

    if (!storage->gwd)
	return FALSE;

    if (storage->is_mate_desktop) {
	double_click_action = translate_dashes_to_underscores (g_settings_get_string (storage->marco,
										      ORG_MATE_MARCO_GENERAL_ACTION_DOUBLE_CLICK_TITLEBAR));
	middle_click_action = translate_dashes_to_underscores (g_settings_get_string (storage->marco,
										      ORG_MATE_MARCO_GENERAL_ACTION_MIDDLE_CLICK_TITLEBAR));
	right_click_action = translate_dashes_to_underscores (g_settings_get_string (storage->marco,
										      ORG_MATE_MARCO_GENERAL_ACTION_RIGHT_CLICK_TITLEBAR));
    } else if (storage->desktop) {
	double_click_action = translate_dashes_to_underscores (g_settings_get_string (storage->desktop,
											 ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_DOUBLE_CLICK_TITLEBAR));
	middle_click_action = translate_dashes_to_underscores (g_settings_get_string (storage->desktop,
											 ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_MIDDLE_CLICK_TITLEBAR));
	right_click_action = translate_dashes_to_underscores (g_settings_get_string (storage->desktop,
											 ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_RIGHT_CLICK_TITLEBAR));
    } else
	return FALSE;

    return gwd_settings_writable_titlebar_actions_changed (storage->writable,
							   double_click_action,
							   middle_click_action,
							   right_click_action,
							   g_settings_get_string (storage->gwd,
										  ORG_COMPIZ_GWD_KEY_MOUSE_WHEEL_ACTION));

    if (double_click_action)
	g_free (double_click_action);

    if (middle_click_action)
	g_free (middle_click_action);

    if (right_click_action)
	g_free (right_click_action);
}

GSettings *
gwd_get_org_compiz_gwd_settings (GWDSettingsStorage *storage)
{
    return storage->gwd;
}

GSettings *
gwd_get_org_gnome_desktop_wm_preferences_settings (GWDSettingsStorage *storage)
{
    return storage->desktop;
}

GSettings *
gwd_get_org_gnome_metacity_settings (GWDSettingsStorage *storage)
{
    return storage->metacity;
}

GSettings *
gwd_get_org_mate_marco_general_settings (GWDSettingsStorage *storage)
{
    return storage->marco;
}
