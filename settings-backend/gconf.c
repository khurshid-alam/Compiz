/**
 *
 * GConf libccs backend
 *
 * gconf.c
 *
 * Copyright (c) 2007 Danny Baumann <maniac@opencompositing.org>
 *
 * Parts of this code are taken from libberylsettings 
 * gconf backend, written by:
 *
 * Copyright (c) 2006 Robert Carr <racarr@opencompositing.org>
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h>

#include <ccs.h>
#include <ccs-backend.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

#define GNOME        "/desktop/gnome"
#define METACITY     "/apps/metacity"
#define COMPIZ       "/apps/compiz"
#define COMPIZCONFIG "/apps/compizconfig"
#define PROFILEPATH  COMPIZCONFIG "/profiles"
#define DEFAULTPROF "Default"
#define CORE_NAME   "core"

#define BUFSIZE 512

#define KEYNAME     char keyName[BUFSIZE]; \
                    if (setting->isScreen) \
                        snprintf (keyName, BUFSIZE, "screen%d", \
				  setting->screenNum); \
                    else \
                        snprintf (keyName, BUFSIZE, "allscreens");

#define PATHNAME    char pathName[BUFSIZE]; \
                    if (!setting->parent->name || \
			strcmp (setting->parent->name, "core") == 0) \
                        snprintf (pathName, BUFSIZE, \
				 "%s/general/%s/options/%s", COMPIZ, \
				 keyName, setting->name); \
                    else \
			snprintf(pathName, BUFSIZE, \
				 "%s/plugins/%s/%s/options/%s", COMPIZ, \
				 setting->parent->name, keyName, setting->name);

static GConfClient *client = NULL;
static GConfEngine *conf = NULL;
static guint backendNotifyId = 0;
static guint metacityNotifyId = 0;
static guint gnomeNotifyId = 0;
static char *currentProfile = NULL;

/* some forward declarations */
static Bool readInit (CCSContext * context);
static void readSetting (CCSContext * context, CCSSetting * setting);
static Bool readOption (CCSSetting * setting);
static Bool writeInit (CCSContext * context);
static void writeIntegratedOption (CCSContext *context, CCSSetting *setting,
				   int        index);

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
    OptionString,
    OptionSpecial,
} SpecialOptionType;

typedef struct _SpecialOption {
    const char*       settingName;
    const char*       pluginName;
    Bool	      screen;
    const char*       gnomeName;
    SpecialOptionType type;
} SpecialOption;

const SpecialOption specialOptions[] = {
    {"run", "core", FALSE,
     METACITY "/global_keybindings/panel_run_dialog", OptionKey},
    {"main_menu", "core", FALSE,
     METACITY "/global_keybindings/panel_main_menu", OptionKey},
    {"run_command_screenshot", "core", FALSE,
     METACITY "/global_keybindings/run_command_screenshot", OptionKey},
    {"run_command_window_screenshot", "core", FALSE,
     METACITY "/global_keybindings/run_command_window_screenshot", OptionKey},
    {"run_command_terminal", "core", FALSE,
     METACITY "/global_keybindings/run_command_terminal", OptionKey},

    {"toggle_window_maximized", "core", FALSE,
     METACITY "/window_keybindings/toggle_maximized", OptionKey},
    {"minimize_window", "core", FALSE,
     METACITY "/window_keybindings/minimize", OptionKey},
    {"maximize_window", "core", FALSE,
     METACITY "/window_keybindings/maximize", OptionKey},
    {"unmaximize_window", "core", FALSE,
     METACITY "/window_keybindings/unmaximize", OptionKey},
    {"toggle_window_maximized_horizontally", "core", FALSE,
     METACITY "/window_keybindings/maximize_horizontally", OptionKey},
    {"toggle_window_maximized_vertically", "core", FALSE,
     METACITY "/window_keybindings/maximize_vertically", OptionKey},
    {"raise_window", "core", FALSE,
     METACITY "/window_keybindings/raise", OptionKey},
    {"lower_window", "core", FALSE,
     METACITY "/window_keybindings/lower", OptionKey},
    {"close_window", "core", FALSE,
     METACITY "/window_keybindings/close", OptionKey},
    {"toggle_window_shaded", "core", FALSE,
     METACITY "/window_keybindings/toggle_shaded", OptionKey},

    {"show_desktop", "core", FALSE,
     METACITY "/global_keybindings/show_desktop", OptionKey},

    {"initiate", "move", FALSE,
     METACITY "/window_keybindings/begin_move", OptionSpecial},
    {"initiate", "resize", FALSE,
     METACITY "/window_keybindings/begin_resize", OptionSpecial},
    {"window_menu", "core", FALSE,
     METACITY "/window_keybindings/activate_window_menu", OptionSpecial},
    /* this option does not exist in Compiz */
    {"mouse_button_modifier", NULL, FALSE,
     METACITY "/general/mouse_button_modifier", OptionSpecial},

    {"next", "switcher", FALSE,
     METACITY "/global_keybindings/switch_windows", OptionKey},
    {"prev", "switcher", FALSE,
     METACITY "/global_keybindings/switch_windows_backward", OptionKey},

    {"toggle_sticky", "extrawm", FALSE,
     METACITY "/window_keybindings/toggle_on_all_workspaces", OptionKey},
    {"toggle_fullscreen", "extrawm", FALSE,
     METACITY "/window_keybindings/toggle_fullscreen", OptionKey},

    {"command0", "core", FALSE,
     METACITY "/keybinding_commands/command_1", OptionString},
    {"command1", "core", FALSE,
     METACITY "/keybinding_commands/command_2", OptionString},
    {"command2", "core", FALSE,
     METACITY "/keybinding_commands/command_3", OptionString},
    {"command3", "core", FALSE,
     METACITY "/keybinding_commands/command_4", OptionString},
    {"command4", "core", FALSE,
     METACITY "/keybinding_commands/command_5", OptionString},
    {"command5", "core", FALSE,
     METACITY "/keybinding_commands/command_6", OptionString},
    {"command6", "core", FALSE,
     METACITY "/keybinding_commands/command_7", OptionString},
    {"command7", "core", FALSE,
     METACITY "/keybinding_commands/command_8", OptionString},
    {"command8", "core", FALSE,
     METACITY "/keybinding_commands/command_9", OptionString},
    {"command9", "core", FALSE,
     METACITY "/keybinding_commands/command_10", OptionString},
    {"command10", "core", FALSE,
     METACITY "/keybinding_commands/command_11", OptionString},
    {"command11", "core", FALSE,
     METACITY "/keybinding_commands/command_12", OptionString},

    {"run_command0", "core", FALSE,
     METACITY "/global_keybindings/run_command_1", OptionKey},
    {"run_command1", "core", FALSE,
     METACITY "/global_keybindings/run_command_2", OptionKey},
    {"run_command2", "core", FALSE,
     METACITY "/global_keybindings/run_command_3", OptionKey},
    {"run_command3", "core", FALSE,
     METACITY "/global_keybindings/run_command_4", OptionKey},
    {"run_command4", "core", FALSE,
     METACITY "/global_keybindings/run_command_5", OptionKey},
    {"run_command5", "core", FALSE,
     METACITY "/global_keybindings/run_command_6", OptionKey},
    {"run_command6", "core", FALSE,
     METACITY "/global_keybindings/run_command_7", OptionKey},
    {"run_command7", "core", FALSE,
     METACITY "/global_keybindings/run_command_8", OptionKey},
    {"run_command8", "core", FALSE,
     METACITY "/global_keybindings/run_command_9", OptionKey},
    {"run_command9", "core", FALSE,
     METACITY "/global_keybindings/run_command_10", OptionKey},
    {"run_command10", "core", FALSE,
     METACITY "/global_keybindings/run_command_11", OptionKey},
    {"run_command11", "core", FALSE,
     METACITY "/global_keybindings/run_command_12", OptionKey},

    {"rotate_to_1", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
    {"rotate_to_2", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
    {"rotate_to_3", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
    {"rotate_to_4", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
    {"rotate_to_5", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
    {"rotate_to_6", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
    {"rotate_to_7", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
    {"rotate_to_8", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
    {"rotate_to_9", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
    {"rotate_to_10", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
    {"rotate_to_11", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
    {"rotate_to_12", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

    {"rotate_left", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_left", OptionKey},
    {"rotate_right", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_right", OptionKey},

    {"plane_to_1", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
    {"plane_to_2", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
    {"plane_to_3", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
    {"plane_to_4", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
    {"plane_to_5", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
    {"plane_to_6", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
    {"plane_to_7", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
    {"plane_to_8", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
    {"plane_to_9", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
    {"plane_to_10", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
    {"plane_to_11", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
    {"plane_to_12", "plane", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

    {"switch_to_1", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
    {"switch_to_2", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
    {"switch_to_3", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
    {"switch_to_4", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
    {"switch_to_5", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
    {"switch_to_6", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
    {"switch_to_7", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
    {"switch_to_8", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
    {"switch_to_9", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
    {"switch_to_10", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
    {"switch_to_11", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
    {"switch_to_12", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

    {"up", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_up", OptionKey},
    {"down", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_down", OptionKey},
    {"left", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_left", OptionKey},
    {"right", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_right", OptionKey},
    {"left_window", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_left", OptionKey},
    {"right_window", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_right", OptionKey},
    {"up_window", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_up", OptionKey},
    {"down_window", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_down", OptionKey},

    {"put_topleft", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_nw", OptionKey},
    {"put_topright", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_ne", OptionKey},
    {"put_bottomleft", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_sw", OptionKey},
    {"put_bottomright", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_se", OptionKey},
    {"put_left", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_w", OptionKey},
    {"put_right", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_e", OptionKey},
    {"put_top", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_n", OptionKey},
    {"put_bottom", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_s", OptionKey},

    {"rotate_to_1_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_1", OptionKey},
    {"rotate_to_2_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_2", OptionKey},
    {"rotate_to_3_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_3", OptionKey},
    {"rotate_to_4_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_4", OptionKey},
    {"rotate_to_5_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_5", OptionKey},
    {"rotate_to_6_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_6", OptionKey},
    {"rotate_to_7_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_7", OptionKey},
    {"rotate_to_8_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_8", OptionKey},
    {"rotate_to_9_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_9", OptionKey},
    {"rotate_to_10_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_10", OptionKey},
    {"rotate_to_11_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_11", OptionKey},
    {"rotate_to_12_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_12", OptionKey},

    {"rotate_left_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_left", OptionKey},
    {"rotate_right_window", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_right", OptionKey},

    {"command_screenshot", "core", FALSE,
     METACITY "/keybinding_commands/command_screenshot", OptionString},
    {"command_window_screenshot", "core", FALSE,
     METACITY "/keybinding_commands/command_window_screenshot", OptionString},
    {"command_terminal", "core", FALSE,
     GNOME "/applications/terminal/exec", OptionString},

    {"autoraise", "core", FALSE,
     METACITY "/general/auto_raise", OptionBool},
    {"autoraise_delay", "core", FALSE,
     METACITY "/general/auto_raise_delay", OptionInt},
    {"raise_on_click", "core", FALSE,
     METACITY "/general/raise_on_click", OptionBool},
    {"click_to_focus", "core", FALSE,
     METACITY "/general/focus_mode", OptionSpecial},

    {"audible_bell", "core", FALSE,
     METACITY "/general/audible_bell", OptionBool},
    {"hsize", "core", TRUE,
     METACITY "/general/num_workspaces", OptionInt},
};

#define N_SOPTIONS (sizeof (specialOptions) / sizeof (struct _SpecialOption))

static CCSSetting *
findDisplaySettingForPlugin (CCSContext *context,
			     char       *plugin,
			     char       *setting)
{
    CCSPlugin  *p;
    CCSSetting *s;

    p = ccsFindPlugin (context, plugin);
    if (!p)
	return NULL;

    s = ccsFindSetting (p, setting, FALSE, 0);
    if (!s)
	return NULL;

    return s;
}

static Bool
isIntegratedOption (CCSSetting *setting,
		    int        *index)
{
    unsigned int i;

    for (i = 0; i < N_SOPTIONS; i++)
    {
	if ((strcmp (setting->name, specialOptions[i].settingName) == 0) &&
	    ((!setting->parent->name && !specialOptions[i].pluginName) ||
	     (setting->parent->name && specialOptions[i].pluginName &&
	      (strcmp (setting->parent->name,
		       specialOptions[i].pluginName) == 0))) &&
	    ((setting->isScreen && specialOptions[i].screen) ||
	     (!setting->isScreen && !specialOptions[i].screen)))
	{
	    if (index)
		*index = i;
	    return TRUE;
	}
    }
    return FALSE;
}

static void
valueChanged (GConfClient *client,
	      guint       cnxn_id,
	      GConfEntry  *entry,
	      gpointer    user_data)
{
    CCSContext   *context = (CCSContext *)user_data;
    char         *keyName = (char*) gconf_entry_get_key (entry);
    char         *pluginName;
    char         *token;
    int          index;
    Bool         isScreen;
    unsigned int screenNum;
    CCSPlugin    *plugin;
    CCSSetting   *setting;

    keyName += strlen (COMPIZ) + 1;

    token = strsep (&keyName, "/"); /* plugin */
    if (!token)
	return;

    if (strcmp (token, "general") == 0)
    {
	pluginName = "core";
    }
    else
    {
	token = strsep (&keyName, "/");
	if (!token)
	    return;
	pluginName = token;
    }

    plugin = ccsFindPlugin (context, pluginName);
    if (!plugin)
	return;

    token = strsep (&keyName, "/");
    if (!token)
	return;

    if (strcmp (token, "allscreens") == 0)
	isScreen = FALSE;
    else
    {
	isScreen = TRUE;
	sscanf (token, "screen%d", &screenNum);
    }

    token = strsep (&keyName, "/"); /* 'options' */
    if (!token)
	return;

    token = strsep (&keyName, "/");
    if (!token)
	return;

    setting = ccsFindSetting (plugin, token, isScreen, screenNum);
    if (!setting)
    {
	/* maybe it's an action which has a name_button/... naming scheme */
	const char *prefix[] = { "_key", "_button", "_edge", "_edgebutton", "_bell" };
	int        i;
	int        prefixLen, len = strlen (token);

	for (i = 0; i < sizeof (prefix) / sizeof (prefix[0]); i++)
	{
	    prefixLen = strlen (prefix[i]);
	    if (len < prefixLen)
		continue;

	    if (strcmp (token + len - prefixLen, prefix[i]) == 0)
	    {
		char *buffer = strndup (token, len - prefixLen);
		if (buffer)
		{
		    setting = ccsFindSetting (plugin, buffer,
					      isScreen, screenNum);
		    free (buffer);
		}
		break;
	    }
	}
    }
    if (!setting)
	return;

    readInit (context);
    if (!readOption (setting))
	ccsResetToDefault (setting);

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (context);
	writeIntegratedOption (context, setting, index);
    }
}

static void
gnomeValueChanged (GConfClient *client,
		   guint       cnxn_id,
		   GConfEntry  *entry,
		   gpointer    user_data)
{
    CCSContext *context = (CCSContext *)user_data;
    char       *keyName = (char*) gconf_entry_get_key (entry);
    int        i, last = 0, num = 0;
    Bool       needInit = TRUE;

    if (!ccsGetIntegrationEnabled (context))
	return;

    /* we have to loop multiple times here, because one Gnome
       option may be integrated with multiple Compiz options */

    while (1)
    {
	for (i = last, num = -1; i < N_SOPTIONS; i++)
	{
	    if (strcmp (specialOptions[i].gnomeName, keyName) == 0)
	    {
		num = i;
		last = i + 1;
		break;
	    }
	}

	if (num < 0)
	    break;

	if (strcmp (specialOptions[num].settingName,
		    "mouse_button_modifier") == 0)
	{
	    CCSSetting *s;

	    if (needInit)
	    {
		readInit (context);
		needInit = FALSE;
	    }

	    s = findDisplaySettingForPlugin (context, "core", "window_menu");
	    if (s)
		readSetting (context, s);

	    s = findDisplaySettingForPlugin (context, "move", "initiate");
	    if (s)
		readSetting (context, s);

	    s = findDisplaySettingForPlugin (context, "resize", "initiate");
	    if (s)
		readSetting (context, s);
	}
	else
	{
	    CCSPlugin     *plugin = NULL;
	    CCSSetting    *setting;
	    SpecialOption *opt = (SpecialOption *) &specialOptions[num];

	    plugin = ccsFindPlugin (context, (char*) opt->pluginName);
	    if (plugin)
	    {
		for (i = 0; i < context->numScreens; i++)
		{
		    unsigned int screen;

		    if (opt->screen)
			screen = context->screens[i];
		    else
			screen = 0;

		    setting = ccsFindSetting (plugin, (char*) opt->settingName,
					      opt->screen, screen);

		    if (setting)
		    {
			if (needInit)
			{
			    readInit (context);
			    needInit = FALSE;
			}
			readSetting (context, setting);
		    }

		    /* do not read display settings multiple
		       times for multiscreen environments */
		    if (!opt->screen)
			i = context->numScreens;
		}
	    }
	}
    }
}

static void
initClient (CCSContext *context)
{
    client = gconf_client_get_for_engine (conf);

    backendNotifyId = gconf_client_notify_add (client, COMPIZ,
					       valueChanged, context,
					       NULL, NULL);

    metacityNotifyId = gconf_client_notify_add (client, METACITY,
	   					gnomeValueChanged, context,
	   					NULL,NULL);

    gnomeNotifyId = gconf_client_notify_add (client,
					     GNOME "/applications/terminal",
      					     gnomeValueChanged, context,
					     NULL,NULL);

    gconf_client_add_dir (client, COMPIZ, GCONF_CLIENT_PRELOAD_NONE, NULL);
    gconf_client_add_dir (client, METACITY, GCONF_CLIENT_PRELOAD_NONE, NULL);
    gconf_client_add_dir (client, GNOME "/applications/terminal",
			  GCONF_CLIENT_PRELOAD_NONE, NULL);
}

static void
finiClient (void)
{
    if (backendNotifyId)
    {
	gconf_client_notify_remove (client, backendNotifyId);
	backendNotifyId = 0;
    }
    if (metacityNotifyId)
    {
	gconf_client_notify_remove (client, metacityNotifyId);
	metacityNotifyId = 0;
    }
    if (gnomeNotifyId)
    {
	gconf_client_notify_remove (client, gnomeNotifyId);
	gnomeNotifyId = 0;
    }

    gconf_client_remove_dir (client, COMPIZ, NULL);
    gconf_client_remove_dir (client, METACITY, NULL);
    gconf_client_remove_dir (client, GNOME "/applications/terminal", NULL);
    gconf_client_suggest_sync (client, NULL);

    g_object_unref (client);
    client = NULL;
}

static void
copyGconfValues (GConfEngine *conf,
		 const gchar *from,
		 const gchar *to,
		 Bool        associate,
		 const gchar *schemaPath)
{
    GSList *values, *tmp;
    GError *err = NULL;

    values = gconf_engine_all_entries (conf, from, &err);
    tmp = values;

    while (tmp)
    {
	GConfEntry *entry = tmp->data;
	GConfValue *value;
	const char *key = gconf_entry_get_key (entry);
	char       *name, *newKey, *newSchema = NULL;

	name = strrchr (key, '/');
	if (!name)
	    continue;

	if (to)
	{
	    asprintf (&newKey, "%s/%s", to, name + 1);

	    if (associate && schemaPath)
		asprintf (&newSchema, "%s/%s", schemaPath, name + 1);

	    value = gconf_engine_get_without_default (conf, key, NULL);
	    if (value && newKey)
	    {
		if (newSchema)
		    gconf_engine_associate_schema (conf, newKey,
						   newSchema, NULL);
		gconf_engine_set (conf, newKey, value, NULL);

		gconf_value_free (value);
	    }
	    if (newSchema)
		free (newSchema);
	    if (newKey)
		free (newKey);
	}
	else
	{
	    if (associate)
		gconf_engine_associate_schema (conf, key, NULL, NULL);
	    gconf_engine_unset (conf, key, NULL);
	}

	gconf_entry_unref (entry);
	tmp = g_slist_next (tmp);
    }

    if (values)
	g_slist_free (values);
}

static void
copyGconfRecursively (GConfEngine *conf,
		      GSList      *subdirs,
		      const gchar *to,
		      Bool        associate,
		      const gchar *schemaPath)
{
    GSList* tmp;

    tmp = subdirs;

    while (tmp)
    {
 	gchar *path = tmp->data;
	char  *newKey, *newSchema = NULL, *name;

	name = strrchr (path, '/');
	if (name)
	{
  	    if (to)
		asprintf (&newKey, "%s/%s", to, name + 1);
	    else
		newKey = NULL;

	    if (associate && schemaPath)
		asprintf (&newSchema, "%s/%s", schemaPath, name + 1);

	    copyGconfValues (conf, path, newKey, associate, newSchema);
	    copyGconfRecursively (conf,
				  gconf_engine_all_dirs (conf, path, NULL),
				  newKey, associate, newSchema);

	    if (newSchema)
		free (newSchema);

	    if (to)
	    {
		if (newKey)
		    free (newKey);
	    }
	    else
		gconf_engine_remove_dir (conf, path, NULL);
	}

	g_free (path);
	tmp = g_slist_next (tmp);
    }

    if (subdirs)
	g_slist_free (subdirs);
}

static void
copyGconfTree (CCSContext  *context,
	       const gchar *from,
	       const gchar *to,
	       Bool        associate,
	       const gchar *schemaPath)
{
    GSList* subdirs;

    /* we aren't allowed to have an open GConfClient object while
       using GConfEngine, so shut it down and open it again afterwards */
    finiClient ();

    subdirs = gconf_engine_all_dirs (conf, from, NULL);
    gconf_engine_suggest_sync (conf, NULL);

    copyGconfRecursively (conf, subdirs, to, associate, schemaPath);

    gconf_engine_suggest_sync (conf, NULL);

    initClient (context);
}

static Bool
readActionValue (CCSSetting *setting,
		 char       *pathName)
{
    char                  itemPath[BUFSIZE];
    GError                *err = NULL;
    Bool                  ret = FALSE;
    GConfValue            *gconfValue;
    CCSSettingActionValue action;

    memset (&action, 0, sizeof (CCSSettingActionValue));

    snprintf (itemPath, 512, "%s_bell", pathName);
    gconfValue = gconf_client_get (client, itemPath, &err);
    if (!err && gconfValue)
    {
	action.onBell = gconf_value_get_bool (gconfValue);
	ret = TRUE;
    }
    if (err)
    {
	g_error_free (err);
	err = NULL;
    }
    if (gconfValue)
	gconf_value_free (gconfValue);

    snprintf (itemPath, 512, "%s_edge", pathName);
    gconfValue = gconf_client_get (client, itemPath, &err);
    if (!err && gconfValue)
    {
	if ((gconfValue->type == GCONF_VALUE_LIST) &&
	    (gconf_value_get_list_type (gconfValue) == GCONF_VALUE_STRING))
	{
	    GSList        *list;
	    CCSStringList edgeList = NULL;

	    list = gconf_value_get_list (gconfValue);
	    for ( ; list; list = list->next)
	    {
		GConfValue *value = (GConfValue *) list->data;
		const char *edge = gconf_value_get_string (value);

		if (edge)
		    edgeList = ccsStringListAppend (edgeList, (char*) edge);
	    }

	    ccsStringListToEdges (edgeList, &action);

	    if (edgeList)
		ccsStringListFree (edgeList, FALSE);
	}
    }
    if (err)
    {
	g_error_free (err);
	err = NULL;
    }
    if (gconfValue)
	gconf_value_free (gconfValue);

    snprintf (itemPath, 512, "%s_edgebutton", pathName);
    gconfValue = gconf_client_get (client, itemPath, &err);
    if (!err && gconfValue)
    {
	action.edgeButton = gconf_value_get_int (gconfValue);
	ret = TRUE;
    }
    if (err)
    {
	g_error_free (err);
	err = NULL;
    }
    if (gconfValue)
	gconf_value_free (gconfValue);

    snprintf (itemPath, 512, "%s_key", pathName);
    gconfValue = gconf_client_get (client, itemPath, &err);
    if (!err && gconfValue)
    {
	const char* buffer;
	buffer = gconf_value_get_string (gconfValue);
	if (buffer)
	{
	    ccsStringToKeyBinding (buffer, &action);
	    ret = TRUE;
	}
    }
    if (err)
    {
	g_error_free (err);
	err = NULL;
    }
    if (gconfValue)
	gconf_value_free (gconfValue);

    snprintf (itemPath, 512, "%s_button", pathName);
    gconfValue = gconf_client_get (client, itemPath, &err);
    if (!err && gconfValue)
    {
	const char* buffer;
	buffer = gconf_value_get_string (gconfValue);
	if (buffer)
	{
	    ccsStringToButtonBinding (buffer, &action);
	    ret = TRUE;
	}
    }
    if (err)
    {
	g_error_free (err);
	err = NULL;
    }
    if (gconfValue)
	gconf_value_free (gconfValue);

    if (ret)
	ccsSetAction (setting, action);

    return ret;
}

static Bool
readListValue (CCSSetting *setting,
	       GConfValue *gconfValue)
{
    GConfValueType      valueType;
    unsigned int        nItems, i = 0;
    CCSSettingValueList list = NULL;
    GSList              *valueList = NULL;

    switch (setting->info.forList.listType)
    {
    case TypeString:
    case TypeMatch:
    case TypeColor:
	valueType = GCONF_VALUE_STRING;
	break;
    case TypeBool:
	valueType = GCONF_VALUE_BOOL;
	break;
    case TypeInt:
	valueType = GCONF_VALUE_INT;
	break;
    case TypeFloat:
	valueType = GCONF_VALUE_FLOAT;
	break;
    default:
	valueType = GCONF_VALUE_INVALID;
	break;
    }

    if (valueType == GCONF_VALUE_INVALID)
	return FALSE;

    if (valueType != gconf_value_get_list_type (gconfValue))
	return FALSE;

    valueList = gconf_value_get_list (gconfValue);
    if (!valueList)
	return FALSE;

    nItems = g_slist_length (valueList);

    switch (setting->info.forList.listType)
    {
    case TypeBool:
	{
	    Bool *array = malloc (nItems * sizeof (Bool));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] =
		    gconf_value_get_bool (valueList->data) ? TRUE : FALSE;
	    list = ccsGetValueListFromBoolArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeInt:
	{
	    int *array = malloc (nItems * sizeof (int));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] = gconf_value_get_int (valueList->data);
	    list = ccsGetValueListFromIntArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeFloat:
	{
	    float *array = malloc (nItems * sizeof (float));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] = gconf_value_get_float (valueList->data);
	    list = ccsGetValueListFromFloatArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeString:
    case TypeMatch:
	{
	    char **array = malloc (nItems * sizeof (char*));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] = strdup (gconf_value_get_string (valueList->data));
	    list = ccsGetValueListFromStringArray (array, nItems, setting);
	    for (i = 0; i < nItems; i++)
		if (array[i])
		    free (array[i]);
	    free (array);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue *array;
	    array = malloc (nItems * sizeof (CCSSettingColorValue));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
    	    {
		memset (&array[i], 0, sizeof (CCSSettingColorValue));
		ccsStringToColor (gconf_value_get_string (valueList->data),
				  &array[i]);
	    }
	    list = ccsGetValueListFromColorArray (array, nItems, setting);
	    free (array);
	}
	break;
    default:
	break;
    }

    if (list)
    {
	ccsSetList (setting, list);
	ccsSettingValueListFree (list, TRUE);
	return TRUE;
    }

    return FALSE;
}

static unsigned int
getGnomeMouseButtonModifier(void)
{
    unsigned int modMask = 0;
    GError       *err = NULL;
    char         *value;

    value = gconf_client_get_string (client,
				     METACITY "/general/mouse_button_modifier",
				     &err);

    if (err)
    {
	g_error_free (err);
	return 0;
    }

    if (!value)
	return 0;

    modMask = ccsStringToModifiers (value);
    g_free (value);

    return modMask;
}

static Bool
readIntegratedOption (CCSContext *context,
		      CCSSetting *setting,
		      int        index)
{
    GError *err = NULL;
    Bool   ret = FALSE;

    switch (specialOptions[index].type)
    {
    case OptionInt:
	{
	    guint value;
	    value = gconf_client_get_int (client,
					  specialOptions[index].gnomeName,
					  &err);

	    if (!err)
	    {
		ccsSetInt (setting, value);
		ret = TRUE;
	    }
	}
	break;
    case OptionBool:
	{
	    gboolean value;
	    value = gconf_client_get_bool (client,
					   specialOptions[index].gnomeName,
					   &err);

	    if (!err)
	    {
		ccsSetBool (setting, value ? TRUE : FALSE);
		ret = TRUE;
	    }
	}
	break;
    case OptionString:
	{
	    char *value;
	    value = gconf_client_get_string (client,
					     specialOptions[index].gnomeName,
					     &err);

	    if (!err && value)
    	    {
		ccsSetString (setting, value);
		ret = TRUE;
		g_free (value);
	    }
	}
	break;
    case OptionKey:
	{
	    char *value;
	    value = gconf_client_get_string (client,
					     specialOptions[index].gnomeName,
					     &err);

	    if (!err && value)
    	    {
		CCSSettingActionValue action;
		memset (&action, 0, sizeof(CCSSettingActionValue));
		ccsGetAction (setting, &action);
		if (ccsStringToKeyBinding (value, &action))
		{
		    ccsSetAction (setting, action);
		    ret = TRUE;
		}
		g_free (value);
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = specialOptions[index].settingName;
	    const char *pluginName  = specialOptions[index].pluginName;

	    if (strcmp (settingName, "click_to_focus") == 0)
	    {
		char       *focusMode;
		const char *name;

		name = specialOptions[index].gnomeName;
		focusMode = gconf_client_get_string (client, name, &err);

		if (!err && focusMode)
		{
		    Bool clickToFocus = (strcmp (focusMode, "click") == 0);
		    ccsSetBool (setting, clickToFocus);
		    ret = TRUE;
		    g_free (focusMode);
		}
	    }
	    else if (((strcmp (settingName, "initiate") == 0) &&
		      ((strcmp (pluginName, "move") == 0) ||
		       (strcmp (pluginName, "resize") == 0))) ||
		     (strcmp (settingName, "window_menu") == 0))
	    {
		char       *value;
		const char *name;

		name = specialOptions[index].gnomeName;
		value = gconf_client_get_string (client, name, &err);

		if (!err && value)
		{
		    CCSSettingActionValue action;
		    memset (&action, 0, sizeof(CCSSettingActionValue));
    		    ccsGetAction (setting, &action);
	    	    if (ccsStringToKeyBinding (value, &action))
		    {
			action.buttonModMask = getGnomeMouseButtonModifier ();
			if (strcmp (settingName, "window_menu") == 0)
    			    action.button = 3;
			else if (strcmp (pluginName, "resize") == 0)
	    		    action.button = 2;
			else
			    action.button = 1;

			ccsSetAction (setting, action);
			ret = TRUE;
		    }
		    g_free (value);
		}
	    }
	}
     	break;
    default:
	break;
    }

    if (err)
	g_error_free (err);

    return ret;
}

static Bool
readOption (CCSSetting * setting)
{
    GConfValue *gconfValue = NULL;
    GError     *err = NULL;
    Bool       ret = FALSE;

    KEYNAME;
    PATHNAME;

    /* first check if the key is set, but only if the setting
       type is not action - actions are in a subtree and handled
       separately */
    if (setting->type != TypeAction)
    {
	Bool valid = TRUE;
	gconfValue = gconf_client_get_without_default (client, pathName, &err);
	if (err)
	{
	    g_error_free (err);
	    return FALSE;
	}
	if (!gconfValue)
	    /* value is not set */
	    return FALSE;

	/* setting type sanity check */
	switch (setting->type)
	{
	case TypeString:
    	case TypeMatch:
	case TypeColor:
	    valid = (gconfValue->type == GCONF_VALUE_STRING);
	    break;
	case TypeInt:
	    valid = (gconfValue->type == GCONF_VALUE_INT);
	    break;
	case TypeBool:
	    valid = (gconfValue->type == GCONF_VALUE_BOOL);
	    break;
	case TypeFloat:
	    valid = (gconfValue->type == GCONF_VALUE_FLOAT);
	    break;
	case TypeList:
	    valid = (gconfValue->type == GCONF_VALUE_LIST);
	    break;
	default:
	    break;
	}
	if (!valid)
	{
	    printf ("GConf backend: There is an unsupported value at path %s. "
		    "Settings from this path won't be read. Try to remove "
		    "that value so that operation can continue properly.\n",
		    pathName);
	    return FALSE;
	}
    }

    switch (setting->type)
    {
    case TypeString:
	{
	    const char *value;
	    value = gconf_value_get_string (gconfValue);
	    if (value)
	    {
		ccsSetString (setting, value);
		ret = TRUE;
	    }
	}
	break;
    case TypeMatch:
	{
	    const char * value;
	    value = gconf_value_get_string (gconfValue);
	    if (value)
	    {
		ccsSetMatch (setting, value);
		ret = TRUE;
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    value = gconf_value_get_int (gconfValue);

	    ccsSetInt (setting, value);
	    ret = TRUE;
	}
	break;
    case TypeBool:
	{
	    gboolean value;
	    value = gconf_value_get_bool (gconfValue);

	    ccsSetBool (setting, value ? TRUE : FALSE);
	    ret = TRUE;
	}
	break;
    case TypeFloat:
	{
	    double value;
	    value = gconf_value_get_float (gconfValue);

	    ccsSetFloat (setting, (float)value);
    	    ret = TRUE;
	}
	break;
    case TypeColor:
	{
	    const char           *value;
	    CCSSettingColorValue color;
	    value = gconf_value_get_string (gconfValue);

	    if (value && ccsStringToColor (value, &color))
	    {
		ccsSetColor (setting, color);
		ret = TRUE;
	    }
	}
	break;
    case TypeList:
	ret = readListValue (setting, gconfValue);
	break;
    case TypeAction:
	ret = readActionValue (setting, pathName);
	break;
    default:
	printf("GConf backend: attempt to read unsupported setting type %d!\n",
	       setting->type);
	break;
    }

    if (gconfValue)
	gconf_value_free (gconfValue);

    return ret;
}

static void
writeActionValue(CCSSettingActionValue *action,
		 char                  *pathName)
{
    char          *buffer;
    char          itemPath[BUFSIZE];
    CCSStringList edgeList, l;
    GSList        *list = NULL;

    snprintf (itemPath, BUFSIZE, "%s_edge", pathName);
    edgeList = ccsEdgesToStringList (action);
    for (l = edgeList; l; l = l->next)
	list = g_slist_append (list, l->data);

    gconf_client_set_list (client, itemPath, GCONF_VALUE_STRING, list, NULL);
    if (edgeList)
	ccsStringListFree (edgeList, TRUE);
    if (list)
	g_slist_free (list);

    snprintf (itemPath, BUFSIZE, "%s_bell", pathName);
    gconf_client_set_bool (client, itemPath, action->onBell, NULL);

    snprintf (itemPath, BUFSIZE, "%s_edgebutton", pathName);
    gconf_client_set_int (client, itemPath, action->edgeButton, NULL);

    snprintf (itemPath, BUFSIZE, "%s_button", pathName);
    buffer = ccsButtonBindingToString (action);
    if (buffer)
    {
	gconf_client_set_string (client, itemPath, buffer, NULL);
	free (buffer);
    }

    snprintf (itemPath, BUFSIZE, "%s_key", pathName);
    buffer = ccsKeyBindingToString (action);
    if (buffer)
    {
	gconf_client_set_string (client, itemPath, buffer, NULL);
	free (buffer);
    }
}

static void
writeListValue (CCSSetting *setting,
		char       *pathName)
{
    GSList              *valueList = NULL;
    GConfValueType      valueType;
    Bool                freeItems = FALSE;
    CCSSettingValueList list;
    gpointer            data;

    if (!ccsGetList (setting, &list))
	return;

    switch (setting->info.forList.listType)
    {
    case TypeBool:
	{
	    while (list)
	    {
		data = GINT_TO_POINTER (list->data->value.asBool);
		valueList = g_slist_append (valueList, data);
		list = list->next;
	    }
	    valueType = GCONF_VALUE_BOOL;
	}
	break;
    case TypeInt:
	{
	    while (list)
	    {
		data = GINT_TO_POINTER (list->data->value.asInt);
		valueList = g_slist_append(valueList, data);
		list = list->next;
    	    }
	    valueType = GCONF_VALUE_INT;
	}
	break;
    case TypeFloat:
	{
	    float *item;
	    while (list)
	    {
		item = malloc (sizeof (float));
		if (item)
		{
		    *item = list->data->value.asFloat;
		    valueList = g_slist_append (valueList, item);
		}
		list = list->next;
	    }
	    freeItems = TRUE;
	    valueType = GCONF_VALUE_FLOAT;
	}
	break;
    case TypeString:
	{
	    while (list)
	    {
		valueList = g_slist_append(valueList,
		   			   list->data->value.asString);
		list = list->next;
	    }
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    case TypeMatch:
	{
	    while (list)
	    {
		valueList = g_slist_append(valueList,
		   			   list->data->value.asMatch);
		list = list->next;
	    }
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    case TypeColor:
	{
	    char *item;
	    while (list)
	    {
		item = ccsColorToString (&list->data->value.asColor);
		valueList = g_slist_append (valueList, item);
		list = list->next;
	    }
	    freeItems = TRUE;
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    default:
	printf("GConf backend: attempt to write unsupported list type %d!\n",
	       setting->info.forList.listType);
	valueType = GCONF_VALUE_INVALID;
	break;
    }

    if (valueType != GCONF_VALUE_INVALID)
    {
	gconf_client_set_list (client, pathName, valueType, valueList, NULL);

	if (freeItems)
	{
	    GSList *tmpList = valueList;
	    for (; tmpList; tmpList = tmpList->next)
		if (tmpList->data)
		    free (tmpList->data);
	}
    }
    if (valueList)
	g_slist_free (valueList);
}

static void
setGnomeMouseButtonModifier (unsigned int modMask)
{
    char   *modifiers, *currentValue;
    GError *err = NULL;

    modifiers = ccsModifiersToString (modMask);

    if (!modifiers)
	modifiers = strdup ("");
    if (!modifiers)
	return;

    currentValue =
	gconf_client_get_string(client,
				METACITY "/general/mouse_button_modifier",
				&err);
    if (err)
    {
	free (modifiers);
	g_error_free (err);
	return;
    }

    if (!currentValue || (strcmp (currentValue, modifiers) != 0))
	gconf_client_set_string (client,
				 METACITY "/general/mouse_button_modifier",
				 modifiers, NULL);
    if (currentValue)
	g_free (currentValue);

    free (modifiers);
}

static void
setButtonBindingForSetting (CCSContext   *context,
			    const char   *plugin,
			    const char   *setting,
			    unsigned int button,
			    unsigned int buttonModMask)
{
    CCSSetting            *s;
    CCSSettingActionValue action;

    s = findDisplaySettingForPlugin (context, (char*) plugin, (char*) setting);
    if (!s)
	return;

    if (s->type != TypeAction)
	return;

    action = s->value->value.asAction;

    if ((action.button != button) || (action.buttonModMask != buttonModMask))
    {
	action.button = button;
	action.buttonModMask = buttonModMask;

	ccsSetAction (s, action);
    }
}

static void
writeIntegratedOption (CCSContext *context,
		       CCSSetting *setting,
		       int        index)
{
    GError     *err = NULL;
    const char *optionName = specialOptions[index].gnomeName;

    switch (specialOptions[index].type)
    {
    case OptionInt:
	{
	    int newValue, currentValue;
	    if (!ccsGetInt (setting, &newValue))
		break;
	    currentValue = gconf_client_get_int (client, optionName, &err);

	    if (!err && (currentValue != newValue))
		gconf_client_set_int(client, specialOptions[index].gnomeName,
				     newValue, NULL);
	}
	break;
    case OptionBool:
	{
	    Bool     newValue;
	    gboolean currentValue;
	    if (!ccsGetBool (setting, &newValue))
		break;
    	    currentValue = gconf_client_get_bool (client, optionName, &err);

	    if (!err && ((currentValue && !newValue) ||
			 (!currentValue && newValue)))
		gconf_client_set_bool (client, specialOptions[index].gnomeName,
				       newValue, NULL);
	}
	break;
    case OptionString:
	{
	    char  *newValue;
	    gchar *currentValue;
	    if (!ccsGetString (setting, &newValue))
		break;
	    currentValue = gconf_client_get_string (client, optionName, &err);

    	    if (!err && currentValue)
	    {
		if (strcmp (currentValue, newValue) != 0)
		    gconf_client_set_string (client, optionName,
		     			     newValue, NULL);
		g_free (currentValue);
	    }
	}
	break;
    case OptionKey:
	{
	    char  *newValue;
	    gchar *currentValue;

	    newValue = ccsKeyBindingToString (&setting->value->value.asAction);
	    if (newValue)
    	    {
		currentValue = gconf_client_get_string(client,
						       optionName, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (client, optionName,
				 		 newValue, NULL);
    		    g_free (currentValue);
		}
		free (newValue);
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = specialOptions[index].settingName;
	    const char *pluginName  = specialOptions[index].pluginName;

	    if (strcmp (settingName, "click_to_focus") == 0)
	    {
		Bool  clickToFocus;
		gchar *newValue, *currentValue;
		if (!ccsGetBool (setting, &clickToFocus))
		    break;

		newValue = clickToFocus ? "click" : "mouse";
		currentValue = gconf_client_get_string(client,
						       optionName, &err);

		if (!err && currentValue)
		{
		    if (strcmp(currentValue, newValue) != 0)
			gconf_client_set_string (client, optionName,
						 newValue,NULL);
		    g_free (currentValue);
		}
	    }
	    else if (((strcmp (settingName, "initiate") == 0) &&
	    	      ((strcmp (pluginName, "move") == 0) ||
		       (strcmp (pluginName, "resize") == 0))) ||
		     (strcmp (settingName, "window_menu") == 0))
	    {
		char         *newValue;
		gchar        *currentValue;
		unsigned int modMask;

		modMask = setting->value->value.asAction.buttonModMask;
		newValue =
		    ccsKeyBindingToString (&setting->value->value.asAction);
		if (newValue)
		{
		    currentValue = gconf_client_get_string(client,
							   optionName, &err);

		    if (!err && currentValue)
		    {
			if (strcmp (currentValue, newValue) != 0)
    			    gconf_client_set_string(client, optionName,
						    newValue, NULL);
			g_free (currentValue);
    		    }
		    free (newValue);
		}

		setGnomeMouseButtonModifier (modMask);
		setButtonBindingForSetting (context, "move",
					    "initiate", 1, modMask);
		setButtonBindingForSetting (context, "resize",
					    "initiate", 2, modMask);
		setButtonBindingForSetting (context, "core",
					    "window_menu", 3, modMask);
	    }
	}
     	break;
    }

    if (err)
	g_error_free (err);
}

static void
resetOptionToDefault (CCSSetting * setting)
{
    KEYNAME;
    PATHNAME;

    if (setting->type != TypeAction)
	gconf_client_recursive_unset (client, pathName, 0, NULL);
    else
    {
	char itemPath[BUFSIZE];

	snprintf (itemPath, BUFSIZE, "%s_edge", pathName);
	gconf_client_recursive_unset (client, itemPath, 0, NULL);

	snprintf (itemPath, BUFSIZE, "%s_edgebutton", pathName);
	gconf_client_recursive_unset (client, itemPath, 0, NULL);

	snprintf (itemPath, BUFSIZE, "%s_button", pathName);
	gconf_client_recursive_unset (client, itemPath, 0, NULL);

	snprintf (itemPath, BUFSIZE, "%s_key", pathName);
	gconf_client_recursive_unset (client, itemPath, 0, NULL);

	snprintf (itemPath, BUFSIZE, "%s_bell", pathName);
	gconf_client_recursive_unset (client, itemPath, 0, NULL);
    }

    gconf_client_suggest_sync (client, NULL);
}

static void
writeOption (CCSSetting * setting)
{
    KEYNAME;
    PATHNAME;

    switch (setting->type)
    {
    case TypeString:
	{
	    char *value;
	    if (ccsGetString (setting, &value))
		gconf_client_set_string (client, pathName, value, NULL);
	}
	break;
    case TypeMatch:
	{
	    char *value;
	    if (ccsGetMatch (setting, &value))
		gconf_client_set_string (client, pathName, value, NULL);
	}
    case TypeFloat:
	{
	    float value;
	    if (ccsGetFloat (setting, &value))
		gconf_client_set_float (client, pathName, value, NULL);
	}
	break;
    case TypeInt:
	{
	    int value;
	    if (ccsGetInt (setting, &value))
		gconf_client_set_int (client, pathName, value, NULL);
	}
	break;
    case TypeBool:
	{
	    Bool value;
	    if (ccsGetBool (setting, &value))
		gconf_client_set_bool (client, pathName, value, NULL);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue value;
	    char                 *colString;

	    if (!ccsGetColor (setting, &value))
		break;

	    colString = ccsColorToString (&value);
	    if (!colString)
		break;

	    gconf_client_set_string (client, pathName, colString, NULL);
	    free (colString);
	}
	break;
    case TypeAction:
	{
	    CCSSettingActionValue value;
	    if (!ccsGetAction (setting, &value))
		break;

	    writeActionValue (&value, pathName);
	}
	break;
    case TypeList:
	writeListValue (setting, pathName);
	break;
    default:
	printf("GConf backend: attempt to write unsupported setting type %d\n",
	       setting->type);
	break;
    }
}

static void
updateCurrentProfileName (char *profile)
{
    GConfSchema *schema;
    GConfValue  *value;
    
    schema = gconf_schema_new ();
    if (!schema)
	return;

    value = gconf_value_new (GCONF_VALUE_STRING);
    if (!value)
    {
	gconf_schema_free (schema);
	return;
    }

    gconf_schema_set_type (schema, GCONF_VALUE_STRING);
    gconf_schema_set_locale (schema, "C");
    gconf_schema_set_short_desc (schema, "Current profile");
    gconf_schema_set_long_desc (schema, "Current profile of gconf backend");
    gconf_schema_set_owner (schema, "compizconfig");
    gconf_value_set_string (value, profile);
    gconf_schema_set_default_value (schema, value);

    gconf_client_set_schema (client, COMPIZCONFIG "/current_profile",
			     schema, NULL);

    gconf_schema_free (schema);
    gconf_value_free (value);
}

static char*
getCurrentProfileName (void)
{
    GConfSchema *schema = NULL;

    schema = gconf_client_get_schema (client,
    				      COMPIZCONFIG "/current_profile", NULL);

    if (schema)
    {
	GConfValue *value;
	char       *ret = NULL;

	value = gconf_schema_get_default_value (schema);
	if (value)
	    ret = strdup (gconf_value_get_string (value));
	gconf_schema_free (schema);

	return ret;
    }

    return strdup (DEFAULTPROF);
}

static Bool
checkProfile (CCSContext *context)
{
    char *profile, *lastProfile;

    lastProfile = currentProfile;

    profile = ccsGetProfile (context);
    if (!profile || !strlen (profile))
	currentProfile = strdup (DEFAULTPROF);
    else
	currentProfile = strdup (profile);

    if (strcmp (lastProfile, currentProfile) != 0)
    {
	char *pathName;

	/* copy /apps/compiz tree to profile path */
	asprintf (&pathName, "%s/%s", PROFILEPATH, lastProfile);
	if (pathName)
	{
	    copyGconfTree (context, "/apps/compiz", pathName,
	    		   TRUE, "/schemas/apps/compiz");
	    free (pathName);
	}

	/* reset /apps/compiz tree */
	gconf_client_recursive_unset (client, COMPIZ, 0, NULL);

	/* copy new profile tree to /apps/compiz */
	asprintf (&pathName, "%s/%s", PROFILEPATH, currentProfile);
	if (pathName)
	{
    	    copyGconfTree (context, pathName, COMPIZ, FALSE, NULL);

    	    /* delete the new profile tree in /apps/compizconfig
    	       to avoid user modification in the wrong tree */
    	    copyGconfTree (context, pathName, NULL, TRUE, NULL);
    	    free (pathName);
	}

	/* update current profile name */
	updateCurrentProfileName (currentProfile);
    }

    free (lastProfile);

    return TRUE;
}

static void
processEvents (unsigned int flags)
{
    if (!(flags & ProcessEventsNoGlibMainLoopMask))
    {
	while (g_main_context_pending(NULL))
	    g_main_context_iteration(NULL, FALSE);
    }
}

static Bool
initBackend (CCSContext * context)
{
    g_type_init ();

    conf = gconf_engine_get_default ();
    initClient (context);

    currentProfile = getCurrentProfileName ();

    return TRUE;
}

static Bool
finiBackend (CCSContext * context)
{
    processEvents (0);

    gconf_client_clear_cache (client);
    finiClient ();

    if (currentProfile)
    {
	free (currentProfile);
	currentProfile = NULL;
    }

    gconf_engine_unref (conf);
    conf = NULL;

    processEvents (0);
    return TRUE;
}

static Bool
readInit (CCSContext * context)
{
    return checkProfile (context);
}

static void
readSetting (CCSContext *context,
	     CCSSetting *setting)
{
    Bool status;
    int  index;

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	status = readIntegratedOption (context, setting, index);
    }
    else
	status = readOption (setting);

    if (!status)
	ccsResetToDefault (setting);
}

static Bool
writeInit (CCSContext * context)
{
    return checkProfile (context);
}

static void
writeSetting (CCSContext *context,
	      CCSSetting *setting)
{
    int index;

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeIntegratedOption (context, setting, index);
    }
    else if (setting->isDefault &&
	     (strcmp (setting->name, "____plugin_enabled") != 0))
    {
	resetOptionToDefault (setting);
    }
    else
	writeOption (setting);

}

static Bool
getSettingIsIntegrated (CCSSetting * setting)
{
    if (!ccsGetIntegrationEnabled (setting->parent->context))
	return FALSE;

    if (!isIntegratedOption (setting, NULL))
	return FALSE;

    return TRUE;
}

static Bool
getSettingIsReadOnly (CCSSetting * setting)
{
    /* FIXME */
    return FALSE;
}

static CCSStringList
getExistingProfiles (CCSContext *context)
{
    GSList        *data, *tmp;
    CCSStringList ret = NULL;
    char          *name;

    gconf_client_suggest_sync (client, NULL);
    data = gconf_client_all_dirs (client, PROFILEPATH, NULL);

    for (tmp = data; tmp; tmp = g_slist_next (tmp))
    {
	name = strrchr (tmp->data, '/');
	if (name && (strcmp (name + 1, DEFAULTPROF) != 0))
	    ret = ccsStringListAppend (ret, strdup (name + 1));

	g_free (tmp->data);
    }

    g_slist_free (data);

    name = getCurrentProfileName ();
    if (strcmp (name, DEFAULTPROF) != 0)
	ret = ccsStringListAppend (ret, name);
    else
	free (name);

    return ret;
}

static Bool
deleteProfile (CCSContext *context,
	       char       *profile)
{
    char     path[BUFSIZE];
    gboolean status = FALSE;

    checkProfile (context);

    snprintf (path, BUFSIZE, "%s/%s", PROFILEPATH, profile);

    if (gconf_client_dir_exists (client, path, NULL))
    {
	status =
	    gconf_client_recursive_unset (client, path,
	   				  GCONF_UNSET_INCLUDING_SCHEMA_NAMES,
					  NULL);
	gconf_client_suggest_sync (client, NULL);
    }

    return status;
}

static CCSBackendVTable gconfVTable = {
    "gconf",
    "GConf Configuration Backend",
    "GConf Configuration Backend for libccs",
    TRUE,
    TRUE,
    processEvents,
    initBackend,
    finiBackend,
    readInit,
    readSetting,
    0,
    writeInit,
    writeSetting,
    0,
    getSettingIsIntegrated,
    getSettingIsReadOnly,
    getExistingProfiles,
    deleteProfile
};

CCSBackendVTable *
getBackendInfo (void)
{
    return &gconfVTable;
}

