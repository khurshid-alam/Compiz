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

#ifndef GWD_SETTINGS_H
#define GWD_SETTINGS_H

#include <glib-object.h>

G_BEGIN_DECLS

enum
{
    BLUR_TYPE_UNSET = -1,
    BLUR_TYPE_NONE = 0,
    BLUR_TYPE_TITLEBAR = 1,
    BLUR_TYPE_ALL = 2
};

enum
{
    CLICK_ACTION_NONE,
    CLICK_ACTION_SHADE,
    CLICK_ACTION_MAXIMIZE,
    CLICK_ACTION_MINIMIZE,
    CLICK_ACTION_RAISE,
    CLICK_ACTION_LOWER,
    CLICK_ACTION_MENU
};

enum
{
    WHEEL_ACTION_NONE,
    WHEEL_ACTION_SHADE
};

extern const gboolean  USE_TOOLTIPS_DEFAULT;

extern const gdouble   ACTIVE_SHADOW_RADIUS_DEFAULT;
extern const gdouble   ACTIVE_SHADOW_OPACITY_DEFAULT;
extern const gint      ACTIVE_SHADOW_OFFSET_X_DEFAULT;
extern const gint      ACTIVE_SHADOW_OFFSET_Y_DEFAULT;
extern const gchar    *ACTIVE_SHADOW_COLOR_DEFAULT;

extern const gdouble   INACTIVE_SHADOW_RADIUS_DEFAULT;
extern const gdouble   INACTIVE_SHADOW_OPACITY_DEFAULT;
extern const gint      INACTIVE_SHADOW_OFFSET_X_DEFAULT;
extern const gint      INACTIVE_SHADOW_OFFSET_Y_DEFAULT;
extern const gchar    *INACTIVE_SHADOW_COLOR_DEFAULT;

extern const gint      BLUR_TYPE_DEFAULT;

extern const gchar    *METACITY_THEME_DEFAULT;
extern const gdouble   METACITY_ACTIVE_OPACITY_DEFAULT;
extern const gdouble   METACITY_INACTIVE_OPACITY_DEFAULT;
extern const gboolean  METACITY_ACTIVE_SHADE_OPACITY_DEFAULT;
extern const gboolean  METACITY_INACTIVE_SHADE_OPACITY_DEFAULT;

extern const gchar    *METACITY_BUTTON_LAYOUT_DEFAULT;

extern const guint     DOUBLE_CLICK_ACTION_DEFAULT;
extern const guint     MIDDLE_CLICK_ACTION_DEFAULT;
extern const guint     RIGHT_CLICK_ACTION_DEFAULT;
extern const guint     WHEEL_ACTION_DEFAULT;

extern const gchar    *TITLEBAR_FONT_DEFAULT;

#define GWD_TYPE_SETTINGS gwd_settings_get_type ()
G_DECLARE_FINAL_TYPE (GWDSettings, gwd_settings, GWD, SETTINGS, GObject)

GWDSettings *
gwd_settings_new                        (gint                 blur,
                                         const gchar         *metacity_theme);

const gchar *
gwd_settings_get_metacity_button_layout (GWDSettings         *settings);

const gchar *
gwd_settings_get_metacity_theme         (GWDSettings         *settings);

const gchar *
gwd_settings_get_titlebar_font          (GWDSettings         *settings);

void
gwd_settings_freeze_updates             (GWDSettings         *settings);

void
gwd_settings_thaw_updates               (GWDSettings         *settings);

gboolean
gwd_settings_shadow_property_changed    (GWDSettings         *settings,
                                         gdouble              active_shadow_radius,
                                         gdouble              active_shadow_opacity,
                                         gdouble              active_shadow_offset_x,
                                         gdouble              active_shadow_offset_y,
                                         const gchar         *active_shadow_color,
                                         gdouble              inactive_shadow_radius,
                                         gdouble              inactive_shadow_opacity,
                                         gdouble              inactive_shadow_offset_x,
                                         gdouble              inactive_shadow_offset_y,
                                         const gchar         *inactive_shadow_color);

gboolean
gwd_settings_use_tooltips_changed       (GWDSettings         *settings,
                                         gboolean             use_tooltips);

gboolean
gwd_settings_blur_changed               (GWDSettings         *settings,
                                         const gchar         *blur_type);

gboolean
gwd_settings_metacity_theme_changed     (GWDSettings         *settings,
                                         gboolean             use_metacity_theme,
                                         const gchar         *metacity_theme);

gboolean
gwd_settings_opacity_changed            (GWDSettings         *settings,
                                         gdouble              active_opacity,
                                         gdouble              inactive_opacity,
                                         gboolean             active_shade_opacity,
                                         gboolean             inactive_shade_opacity);

gboolean
gwd_settings_button_layout_changed      (GWDSettings         *settings,
                                         const gchar         *button_layout);

gboolean
gwd_settings_font_changed               (GWDSettings         *settings,
                                         gboolean             titlebar_uses_system_font,
                                         const gchar         *titlebar_font);

gboolean
gwd_settings_titlebar_actions_changed   (GWDSettings         *settings,
                                         const gchar         *action_double_click_titlebar,
                                         const gchar         *action_middle_click_titlebar,
                                         const gchar         *action_right_click_titlebar,
                                         const gchar         *mouse_wheel_action);

G_END_DECLS

#endif
