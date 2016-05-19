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

#ifndef GWD_SETTINGS_STORAGE_H
#define GWD_SETTINGS_STORAGE_H

#include <glib-object.h>
#include <gwd-fwd.h>

G_BEGIN_DECLS

extern const gchar * ORG_COMPIZ_GWD;
extern const gchar * ORG_GNOME_METACITY;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES;
extern const gchar * ORG_MATE_MARCO_GENERAL;

extern const gchar * ORG_COMPIZ_GWD_KEY_USE_TOOLTIPS;
extern const gchar * ORG_COMPIZ_GWD_KEY_BLUR_TYPE;
extern const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_OPACITY;
extern const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_OPACITY;
extern const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_ACTIVE_SHADE_OPACITY;
extern const gchar * ORG_COMPIZ_GWD_KEY_METACITY_THEME_INACTIVE_SHADE_OPACITY;
extern const gchar * ORG_COMPIZ_GWD_KEY_USE_METACITY_THEME;
extern const gchar * ORG_COMPIZ_GWD_KEY_MOUSE_WHEEL_ACTION;
extern const gchar * ORG_GNOME_METACITY_THEME;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_DOUBLE_CLICK_TITLEBAR;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_MIDDLE_CLICK_TITLEBAR;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_ACTION_RIGHT_CLICK_TITLEBAR;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_THEME;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_USES_SYSTEM_FONT;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_TITLEBAR_FONT;
extern const gchar * ORG_GNOME_DESKTOP_WM_PREFERENCES_BUTTON_LAYOUT;
extern const gchar * ORG_MATE_MARCO_GENERAL_ACTION_DOUBLE_CLICK_TITLEBAR;
extern const gchar * ORG_MATE_MARCO_GENERAL_ACTION_MIDDLE_CLICK_TITLEBAR;
extern const gchar * ORG_MATE_MARCO_GENERAL_ACTION_RIGHT_CLICK_TITLEBAR;
extern const gchar * ORG_MATE_MARCO_GENERAL_THEME;
extern const gchar * ORG_MATE_MARCO_GENERAL_TITLEBAR_USES_SYSTEM_FONT;
extern const gchar * ORG_MATE_MARCO_GENERAL_TITLEBAR_FONT;
extern const gchar * ORG_MATE_MARCO_GENERAL_BUTTON_LAYOUT;

#define GWD_TYPE_SETTINGS_STORAGE gwd_settings_storage_get_type ()
G_DECLARE_FINAL_TYPE (GWDSettingsStorage, gwd_settings_storage,
                      GWD, SETTINGS_STORAGE, GObject)

GWDSettingsStorage *
gwd_settings_storage_new                          (GWDSettings        *settings,
                                                   gboolean            connect);

gboolean
gwd_settings_storage_update_use_tooltips          (GWDSettingsStorage *storage);

gboolean
gwd_settings_storage_update_blur                  (GWDSettingsStorage *storage);

gboolean
gwd_settings_storage_update_metacity_theme        (GWDSettingsStorage *storage);

gboolean
gwd_settings_storage_update_opacity               (GWDSettingsStorage *storage);

gboolean
gwd_settings_storage_update_button_layout         (GWDSettingsStorage *storage);

gboolean
gwd_settings_storage_update_font                  (GWDSettingsStorage *storage);

gboolean
gwd_settings_storage_update_titlebar_actions      (GWDSettingsStorage*storage);

GSettings *
gwd_get_org_compiz_gwd_settings                   (GWDSettingsStorage *storage);

GSettings *
gwd_get_org_gnome_desktop_wm_preferences_settings (GWDSettingsStorage *storage);

GSettings *
gwd_get_org_gnome_metacity_settings               (GWDSettingsStorage *storage);

GSettings *
gwd_get_org_mate_marco_general_settings           (GWDSettingsStorage *storage);

G_END_DECLS

#endif
