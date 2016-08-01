#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (C) 2016 Canonical
#
# Authors:
#  Marco Trevisan <marco.trevisan@canonical.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 3.
#
# This program is distributed in the hope that it will be useful, but WITHOUTa
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

from gi.repository import Gio
import os,sys

COMPIZ_SCHEMA = "org.compiz"
COMPIZ_CORE_PATH = "/org/compiz/profiles/{}/plugins/core/"
UNITY_PROFILES = ["unity", "unity-lowgfx"]
ACTIVE_PLUGINS_KEY = "active-plugins"
OBSOLETE_PLUGINS = ["decor", "gnomecompat", "scalefilter"]

if COMPIZ_SCHEMA not in Gio.Settings.list_schemas():
    print("No compiz schemas found, no migration needed")
    sys.exit(0)

for profile in UNITY_PROFILES:
    core_profile_path = COMPIZ_CORE_PATH.format(profile)
    core_settings = Gio.Settings(schema=COMPIZ_SCHEMA+".core", path=core_profile_path)
    active_plugins = core_settings.get_strv(ACTIVE_PLUGINS_KEY)

    for plugin in OBSOLETE_PLUGINS:
        if not plugin in active_plugins:
            print("No '{}' plugin active in '{}' profile, no migration needed".format(plugin, profile))
            continue

        try:
            active_plugins.remove(plugin)
        except ValueError:
            pass

        core_settings.set_strv(ACTIVE_PLUGINS_KEY, active_plugins)
        Gio.Settings.sync()

        # Sometimes settings don't get written correctly, so in case we fallback to dconf
        if core_settings.get_strv(ACTIVE_PLUGINS_KEY) != active_plugins:
            try:
                from subprocess import Popen, PIPE, STDOUT
                p = Popen(("dconf load "+core_profile_path).split(), stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                p.communicate(input=bytes("[/]\nactive-plugins={}".format(active_plugins), 'utf-8'))
            except:
                pass
