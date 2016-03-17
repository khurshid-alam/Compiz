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
#include <string.h>

#include "gtk-window-decorator.h"
#include "gwd-settings-writable-interface.h"
#include "gwd-settings-xproperty-storage.h"

struct _GWDSettingsXPropertyStorage
{
    GObject              parent;

    GWDSettingsWritable *writable;

    Display             *xdpy;
    Window               root;
};

enum
{
    PROP_0,

    PROP_WRITABLE_SETTINGS,

    LAST_PROP
};

static GParamSpec *storage_properties[LAST_PROP] = { NULL };

G_DEFINE_TYPE (GWDSettingsXPropertyStorage, gwd_settings_xproperty_storage,
               G_TYPE_OBJECT)

static void
gwd_settings_xproperty_storage_dispose (GObject *object)
{
    GWDSettingsXPropertyStorage *storage;

    storage = GWD_SETTINGS_XPROPERTY_STORAGE (object);

    g_clear_object (&storage->writable);

    G_OBJECT_CLASS (gwd_settings_xproperty_storage_parent_class)->dispose (object);
}

static void
gwd_settings_xproperty_storage_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
    GWDSettingsXPropertyStorage *storage;

    storage = GWD_SETTINGS_XPROPERTY_STORAGE (object);

    switch (property_id) {
        case PROP_WRITABLE_SETTINGS:
            g_return_if_fail (!storage->writable);
            storage->writable = g_value_dup_object (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_settings_xproperty_storage_class_init (GWDSettingsXPropertyStorageClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = gwd_settings_xproperty_storage_dispose;
    object_class->set_property = gwd_settings_xproperty_storage_set_property;

    storage_properties[PROP_WRITABLE_SETTINGS] =
        g_param_spec_object ("writable-settings", "GWDSettingsWritable",
                             "An object that implements GWDSettingsWritable",
                             GWD_TYPE_WRITABLE_SETTINGS_INTERFACE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
                             G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, LAST_PROP,
                                       storage_properties);
}

static void
gwd_settings_xproperty_storage_init (GWDSettingsXPropertyStorage *storage)
{
    GdkDisplay *display;

    display = gdk_display_get_default ();

    storage->xdpy = gdk_x11_display_get_xdisplay (display);
    storage->root = gdk_x11_get_default_root_xwindow ();
}

GWDSettingsXPropertyStorage *
gwd_settings_xproperty_storage_new (GWDSettingsWritable *writable)
{
    return g_object_new (GWD_TYPE_SETTINGS_XPROPERTY_STORAGE,
                         "writable-settings", writable,
                         NULL);
}

gboolean
gwd_settings_xproperty_storage_update_all (GWDSettingsXPropertyStorage *storage)
{
    Atom          actual;
    int           result, format;
    unsigned long n, left;
    unsigned char *prop_data;
    XTextProperty shadow_color_xtp;

    gdouble aradius;
    gdouble aopacity;
    gint    ax_off;
    gint    ay_off;
    char    *active_shadow_color = NULL;

    gdouble iradius;
    gdouble iopacity;
    gint    ix_off;
    gint    iy_off;
    char    *inactive_shadow_color = NULL;

    result = XGetWindowProperty (storage->xdpy, storage->root,
                                 compiz_shadow_info_atom, 0, 32768, 0,
                                 XA_INTEGER, &actual, &format,
                                 &n, &left, &prop_data);

    if (result != Success)
        return FALSE;

    if (n == 8) {
        long *data = (long *) prop_data;
        aradius = data[0];
        aopacity = data[1];
        ax_off = data[2];
        ay_off = data[3];

        iradius = data[4];
        iopacity = data[5];
        ix_off = data[6];
        iy_off = data[7];

        /* Radius and Opacity are multiplied by 1000 to keep precision,
         * divide by that much to get our real radius and opacity
         */
        aradius /= 1000;
        aopacity /= 1000;
        iradius /= 1000;
        iopacity /= 1000;

        XFree (prop_data);
    } else {
        XFree (prop_data);
        return FALSE;
    }

    result = XGetTextProperty (storage->xdpy, storage->root,
                               &shadow_color_xtp, compiz_shadow_color_atom);

    if (shadow_color_xtp.value) {
        int ret_count = 0;
        char **t_data = NULL;

        XTextPropertyToStringList (&shadow_color_xtp, &t_data, &ret_count);

        if (ret_count == 2) {
            active_shadow_color = strdup (t_data[0]);
            inactive_shadow_color = strdup (t_data[1]);

            if (t_data)
                XFreeStringList (t_data);

            XFree (shadow_color_xtp.value);
        } else {
            XFree (shadow_color_xtp.value);
            return FALSE;
        }
    }

    return gwd_settings_writable_shadow_property_changed (storage->writable,
                                                          (gdouble) MAX (0.0, MIN (aradius, 48.0)),
                                                          (gdouble) MAX (0.0, MIN (aopacity, 6.0)),
                                                          (gdouble) MAX (-16, MIN (ax_off, 16)),
                                                          (gdouble) MAX (-16, MIN (ay_off, 16)),
                                                          active_shadow_color,
                                                          (gdouble) MAX (0.0, MIN (iradius, 48.0)),
                                                          (gdouble) MAX (0.0, MIN (iopacity, 6.0)),
                                                          (gdouble) MAX (-16, MIN (ix_off, 16)),
                                                          (gdouble) MAX (-16, MIN (iy_off, 16)),
                                                          inactive_shadow_color);
}
