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

#ifndef GWD_SETTINGS_XPROPERTY_STORAGE_H
#define GWD_SETTINGS_XPROPERTY_STORAGE_H

#include "gwd-settings.h"

G_BEGIN_DECLS

#define GWD_TYPE_SETTINGS_XPROPERTY_STORAGE gwd_settings_xproperty_storage_get_type ()
G_DECLARE_FINAL_TYPE (GWDSettingsXPropertyStorage, gwd_settings_xproperty_storage,
                      GWD, SETTINGS_XPROPERTY_STORAGE, GObject)

GWDSettingsXPropertyStorage *
gwd_settings_xproperty_storage_new        (GWDSettings                 *settings);

gboolean
gwd_settings_xproperty_storage_update_all (GWDSettingsXPropertyStorage *storage);

G_END_DECLS

#endif
