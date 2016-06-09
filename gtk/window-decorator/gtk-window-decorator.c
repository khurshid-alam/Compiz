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
 *
 * 2D Mode: Copyright © 2009 Sam Spilsbury <smspillaz@gmail.com>
 * Frames Management: Copright © 2010 Canonical Ltd.
 *        Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "gtk-window-decorator.h"
#include "gwd-settings.h"

GWDTheme *gwd_theme;

gdouble decoration_alpha = 0.5;

gboolean minimal = FALSE;

const unsigned short ICON_SPACE = 20;

Atom frame_input_window_atom;
Atom win_decor_atom;
Atom win_blur_decor_atom;
Atom wm_move_resize_atom;
Atom restack_window_atom;
Atom select_window_atom;
Atom mwm_hints_atom;
Atom switcher_fg_atom;
Atom compiz_shadow_info_atom;
Atom compiz_shadow_color_atom;
Atom toolkit_action_atom;
Atom toolkit_action_window_menu_atom;
Atom toolkit_action_force_quit_dialog_atom;
Atom net_wm_state_atom;
Atom net_wm_state_modal_atom;
Atom decor_request_atom;
Atom decor_pending_atom;
Atom decor_delete_pixmap_atom;
Atom utf8_string_atom;
Atom gtk_theme_variant_atom;

Time dm_sn_timestamp;

struct _cursor cursor[3][3] = {
    { C (top_left_corner),    C (top_side),    C (top_right_corner)    },
    { C (left_side),	      C (left_ptr),    C (right_side)	       },
    { C (bottom_left_corner), C (bottom_side), C (bottom_right_corner) }
};

struct _pos pos[3][3] = {
    {
	{  0,  0, 10, 21,   0, 0, 0, 0, 0, 1 },
	{ 10,  0, -8,  6,   0, 0, 1, 0, 0, 1 },
	{  2,  0, 10, 21,   1, 0, 0, 0, 0, 1 }
    }, {
	{  0, 10,  6, 11,   0, 0, 0, 1, 1, 0 },
	{  6,  6,  0, 15,   0, 0, 1, 0, 0, 1 },
	{  6, 10,  6, 11,   1, 0, 0, 1, 1, 0 }
    }, {
	{  0, 17, 10, 10,   0, 1, 0, 0, 1, 0 },
	{ 10, 21, -8,  6,   0, 1, 1, 0, 1, 0 },
	{  2, 17, 10, 10,   1, 1, 0, 0, 1, 0 }
    }
}, bpos[] = {
    { 0, 6, 16, 16,   1, 0, 0, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0, 0, 0 },
    { 6, 2, 16, 16,   0, 0, 0, 0, 0, 0 }
};

#define WINDOW_TYPE_FRAMES_NUM 5
default_frame_references_t default_frames[WINDOW_TYPE_FRAMES_NUM  * 2] = {
    /* active */
    {"normal", NULL },
    {"dialog", NULL },
    {"modal_dialog", NULL },
    {"menu", NULL },
    {"utility", NULL},
    /* inactive */
    {"normal", NULL },
    {"dialog", NULL },
    {"modal_dialog", NULL },
    {"menu", NULL },
    {"utility", NULL}
};

char *program_name;

GtkWidget     *switcher_label;

GHashTable    *frame_table;
GtkWidget     *action_menu = NULL;
gboolean      action_menu_mapped = FALSE;
gint	     double_click_timeout = 250;

GtkWidget     *tip_window;
GtkWidget     *tip_label;
GTimeVal	     tooltip_last_popdown = { 0, 0 };
gint	     tooltip_timer_tag = 0;

GSList *draw_list = NULL;
guint  draw_idle_id = 0;

Window    switcher_selected_window = None;
decor_t   *switcher_window = NULL;

XRenderPictFormat *xformat_rgba;

const gchar * window_type_frames[WINDOW_TYPE_FRAMES_NUM] = {
    "normal", "modal_dialog", "dialog", "menu", "utility"
};

static void
update_decorations_cb (GWDSettings *settings,
                       gpointer     user_data)
{
    decorations_changed (wnck_screen_get_default ());
}

static void
update_titlebar_font_cb (GWDSettings *settings,
                         gpointer     user_data)
{
    gwd_theme_update_titlebar_font (gwd_theme);
}

static void
update_metacity_theme_cb (GWDSettings *settings,
                          gint         metacity_theme_type,
                          const gchar *metacity_theme_name,
                          gpointer     user_data)
{
    GWDThemeType type = GWD_THEME_TYPE_CAIRO;

    if (metacity_theme_name != NULL)
        type = GWD_THEME_TYPE_METACITY;

    g_set_object (&gwd_theme, gwd_theme_new (type, settings));
}

int
main (int argc, char *argv[])
{
    GdkDisplay    *gdkdisplay;
    Display       *xdisplay;
    GdkScreen     *gdkscreen;
    WnckScreen    *screen;
    GWDSettings   *settings;
    gint          i, j, status;
    gboolean      replace = FALSE;
    unsigned int  nchildren;
    Window        root_ret, parent_ret;
    Window        *children = NULL;
    GList	  *windows, *win;
    decor_frame_t *bare_p, *switcher_p;

    const char *option_meta_theme = NULL;
    gint       option_blur_type = BLUR_TYPE_UNSET;

    program_name = argv[0];

    gtk_init (&argc, &argv);

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    for (i = 0; i < argc; ++i)
    {
	if (strcmp (argv[i], "--minimal") == 0)
	{
	    minimal = TRUE;
	}
	else if (strcmp (argv[i], "--replace") == 0)
	{
	    replace = TRUE;
	}
	else if (strcmp (argv[i], "--blur") == 0)
	{
	    if (argc > ++i)
	    {
		if (strcmp (argv[i], "titlebar") == 0)
		    option_blur_type = BLUR_TYPE_TITLEBAR;
		else if (strcmp (argv[i], "all") == 0)
		    option_blur_type = BLUR_TYPE_ALL;
	    }
	}

#ifdef USE_METACITY
	else if (strcmp (argv[i], "--metacity-theme") == 0)
	{
	    if (argc > ++i)
		option_meta_theme = argv[i];
	}
#endif

	else if (strcmp (argv[i], "--help") == 0)
	{
	    fprintf (stderr, "%s "
		     "[--minimal] "
		     "[--replace] "
		     "[--blur none|titlebar|all] "

#ifdef USE_METACITY
		     "[--metacity-theme THEME] "
#endif

		     "[--help]"

		     "\n", program_name);
	    return 0;
	}
    }

    gdkdisplay = gdk_display_get_default ();
    xdisplay   = gdk_x11_display_get_xdisplay (gdkdisplay);
    gdkscreen  = gdk_display_get_default_screen (gdkdisplay);

    frame_input_window_atom  = XInternAtom (xdisplay,
					    DECOR_INPUT_FRAME_ATOM_NAME, FALSE);

    win_decor_atom	= XInternAtom (xdisplay, DECOR_WINDOW_ATOM_NAME, FALSE);
    win_blur_decor_atom	= XInternAtom (xdisplay, DECOR_BLUR_ATOM_NAME, FALSE);
    wm_move_resize_atom = XInternAtom (xdisplay, "_NET_WM_MOVERESIZE", FALSE);
    restack_window_atom = XInternAtom (xdisplay, "_NET_RESTACK_WINDOW", FALSE);
    select_window_atom	= XInternAtom (xdisplay, DECOR_SWITCH_WINDOW_ATOM_NAME,
				       FALSE);
    mwm_hints_atom	= XInternAtom (xdisplay, "_MOTIF_WM_HINTS", FALSE);
    switcher_fg_atom    = XInternAtom (xdisplay,
				       DECOR_SWITCH_FOREGROUND_COLOR_ATOM_NAME,
				       FALSE);
    
    compiz_shadow_info_atom  = XInternAtom (xdisplay, "_COMPIZ_NET_CM_SHADOW_PROPERTIES", FALSE);
    compiz_shadow_color_atom = XInternAtom (xdisplay, "_COMPIZ_NET_CM_SHADOW_COLOR", FALSE);

    toolkit_action_atom			  =
	XInternAtom (xdisplay, "_COMPIZ_TOOLKIT_ACTION", FALSE);
    toolkit_action_window_menu_atom	  =
	XInternAtom (xdisplay, "_COMPIZ_TOOLKIT_ACTION_WINDOW_MENU", FALSE);
    toolkit_action_force_quit_dialog_atom =
	XInternAtom (xdisplay, "_COMPIZ_TOOLKIT_ACTION_FORCE_QUIT_DIALOG",
		     FALSE);

    net_wm_state_atom = XInternAtom (xdisplay,"_NET_WM_STATE", 0);
    net_wm_state_modal_atom = XInternAtom (xdisplay, "_NET_WM_STATE_MODAL", 0);

    decor_request_atom = XInternAtom (xdisplay, DECOR_REQUEST_PIXMAP_ATOM_NAME, 0);
    decor_pending_atom = XInternAtom (xdisplay, DECOR_PIXMAP_PENDING_ATOM_NAME, 0);
    decor_delete_pixmap_atom = XInternAtom (xdisplay, DECOR_DELETE_PIXMAP_ATOM_NAME, 0);

    utf8_string_atom = XInternAtom (xdisplay, "UTF8_STRING", 0); 
    gtk_theme_variant_atom = XInternAtom (xdisplay, "_GTK_THEME_VARIANT", 0);

    status = decor_acquire_dm_session (xdisplay,
				       gdk_screen_get_number (gdkscreen),
				       "gwd", replace, &dm_sn_timestamp);
    if (status != DECOR_ACQUIRE_STATUS_SUCCESS)
    {
	if (status == DECOR_ACQUIRE_STATUS_FAILED)
	{
	    fprintf (stderr,
		     "%s: Could not acquire decoration manager "
		     "selection on screen %d display \"%s\"\n",
		     program_name, gdk_screen_get_number (gdkscreen),
		     DisplayString (xdisplay));
	}
	else if (status == DECOR_ACQUIRE_STATUS_OTHER_DM_RUNNING)
	{
	    fprintf (stderr,
		     "%s: Screen %d on display \"%s\" already "
		     "has a decoration manager; try using the "
		     "--replace option to replace the current "
		     "decoration manager.\n",
		     program_name, gdk_screen_get_number (gdkscreen),
		     DisplayString (xdisplay));
	}

	return 1;
    }

    screen = wnck_screen_get_default ();

    initialize_decorations ();

    settings = gwd_settings_new (option_blur_type, option_meta_theme);

    if (!settings)
        return 1;

    g_signal_connect (settings, "update-decorations",
                      G_CALLBACK (update_decorations_cb), NULL);
    g_signal_connect (settings, "update-titlebar-font",
                      G_CALLBACK (update_titlebar_font_cb), NULL);
    g_signal_connect (settings, "update-metacity-theme",
                      G_CALLBACK (update_metacity_theme_cb), NULL);

    gwd_settings_freeze_updates (settings);

    init_settings (settings);

    for (i = 0; i < 3; ++i)
    {
	for (j = 0; j < 3; j++)
	{
	    if (cursor[i][j].shape != XC_left_ptr)
		cursor[i][j].cursor =
		    XCreateFontCursor (xdisplay, cursor[i][j].shape);
	}
    }

    xformat_rgba = XRenderFindStandardFormat (xdisplay, PictStandardARGB32);

    frame_table = g_hash_table_new (NULL, NULL);

    if (!create_tooltip_window ())
    {
	g_object_unref (settings);

	fprintf (stderr, "%s, Couldn't create tooltip window\n", argv[0]);
	return 1;
    }

    wnck_set_client_type (WNCK_CLIENT_TYPE_PAGER);

    gdk_window_add_filter (NULL,
			   selection_event_filter_func,
			   NULL);

    if (!minimal)
    {
	GdkWindow *root = gdk_x11_window_foreign_new_for_display (gdk_display_get_default (), gdk_x11_get_default_root_xwindow ());

 	gdk_window_add_filter (NULL,
 			       event_filter_func,
 			       NULL);
			       
	XQueryTree (xdisplay, gdk_x11_get_default_root_xwindow (),
		    &root_ret, &parent_ret, &children, &nchildren);

	for (i = 0; i < nchildren; ++i)
	{
	    GdkWindow *toplevel = gdk_x11_window_foreign_new_for_display (gdk_display_get_default (), children[i]);

	    /* Need property notify on all windows */

	    gdk_window_set_events (toplevel,
				   gdk_window_get_events (toplevel) |
				   GDK_PROPERTY_CHANGE_MASK);
	}

	/* Need MapNotify on new windows */
	gdk_window_set_events (root, gdk_window_get_events (root) |
			       GDK_STRUCTURE_MASK |
			       GDK_PROPERTY_CHANGE_MASK |
			       GDK_VISIBILITY_NOTIFY_MASK |
			       GDK_SUBSTRUCTURE_MASK);
 
 	connect_screen (screen);
    }

    decor_set_dm_check_hint (xdisplay, gdk_screen_get_number (gdkscreen),
                             WINDOW_DECORATION_TYPE_PIXMAP);

    /* Update the decorations based on the settings */
    gwd_settings_thaw_updates (settings);

    /* Keep the default, bare and switcher decorations around
     * since otherwise they will be spuriously recreated */

    bare_p = gwd_get_decor_frame ("bare");
    switcher_p = gwd_get_decor_frame ("switcher");

    update_default_decorations (gdkscreen);

    gtk_main ();

    win = windows = wnck_screen_get_windows (screen);

    while (win != NULL)
    {
	WnckWindow *w = (WnckWindow *) win->data;

	window_closed (screen, w);

	win = g_list_next (win);
    }

    g_list_free (windows);

    if (tip_label)
	gtk_widget_destroy (GTK_WIDGET (tip_label));

    if (tip_window)
	gtk_widget_destroy (GTK_WIDGET (tip_window));

    gwd_decor_frame_unref (bare_p);
    gwd_decor_frame_unref (switcher_p);

    g_clear_object (&gwd_theme);
    fini_settings ();
    g_clear_object (&settings);

    return 0;
}
