/*
 * Copyright © 2006 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "gtk-window-decorator.h"
#include "gwd-settings-storage.h"
#include "gwd-settings-xproperty-storage.h"

GWDSettingsStorage *storage = NULL;
GWDSettingsXPropertyStorage *xprop_storage = NULL;

void
init_settings (GWDSettings *settings)
{
    storage = gwd_settings_storage_new (settings, TRUE);
    xprop_storage = gwd_settings_xproperty_storage_new (settings);

    gwd_process_decor_shadow_property_update ();
}

void
fini_settings ()
{
    g_clear_object (&storage);
    g_clear_object (&xprop_storage);
}

gboolean
gwd_process_decor_shadow_property_update ()
{
    return gwd_settings_xproperty_storage_update_all (xprop_storage);
}
