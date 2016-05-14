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

#ifndef GWD_SETTINGS_NOTIFIED_H
#define GWD_SETTINGS_NOTIFIED_H

#include <glib-object.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <gwd-fwd.h>

G_BEGIN_DECLS

#define GWD_TYPE_SETTINGS_NOTIFIED gwd_settings_notified_get_type ()
G_DECLARE_FINAL_TYPE (GWDSettingsNotified, gwd_settings_notified,
                      GWD, SETTINGS_NOTIFIED, GObject)

GWDSettingsNotified *
gwd_settings_notified_new                    (WnckScreen          *screen);

gboolean
gwd_settings_notified_update_decorations     (GWDSettingsNotified *notified);

gboolean
gwd_settings_notified_update_frames          (GWDSettingsNotified *notified);

gboolean
gwd_settings_notified_update_metacity_theme  (GWDSettingsNotified *notified);

gboolean
gwd_settings_notified_metacity_button_layout (GWDSettingsNotified *notified);

G_END_DECLS

#endif
