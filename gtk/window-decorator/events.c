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
 * 2D Mode: Copyright © 2010 Sam Spilsbury <smspillaz@gmail.com>
 * Frames Management: Copright © 2011 Canonical Ltd.
 *        Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "gtk-window-decorator.h"
#include "gwd-settings.h"

#define DOUBLE_CLICK_DISTANCE 8.0f

#define WM_MOVERESIZE_SIZE_TOPLEFT      0
#define WM_MOVERESIZE_SIZE_TOP          1
#define WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define WM_MOVERESIZE_SIZE_RIGHT        3
#define WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define WM_MOVERESIZE_SIZE_BOTTOM       5
#define WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define WM_MOVERESIZE_SIZE_LEFT         7
#define WM_MOVERESIZE_MOVE              8
#define WM_MOVERESIZE_SIZE_KEYBOARD     9
#define WM_MOVERESIZE_MOVE_KEYBOARD    10

static double
square (double x)
{
    return x * x;
}

static double
dist (double x1, double y1,
      double x2, double y2)
{
    return sqrt (square (x1 - x2) + square (y1 - y2));
}

static void
move_resize_window (WnckWindow *win,
		    int	       direction,
		    decor_event *gtkwd_event)
{
    Display    *xdisplay;
    GdkDisplay *gdkdisplay;
    GdkScreen  *screen;
    Window     xroot;
    XEvent     ev;

    gdkdisplay = gdk_display_get_default ();
    xdisplay   = GDK_DISPLAY_XDISPLAY (gdkdisplay);
    screen     = gdk_display_get_default_screen (gdkdisplay);
    xroot      = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

    if (action_menu_mapped)
    {
	gtk_widget_destroy (action_menu);
	return;
    }

    ev.xclient.type    = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial	  = 0;
    ev.xclient.send_event = TRUE;

    ev.xclient.window	    = wnck_window_get_xid (win);
    ev.xclient.message_type = wm_move_resize_atom;
    ev.xclient.format	    = 32;

    ev.xclient.data.l[0] = gtkwd_event->x_root;
    ev.xclient.data.l[1] = gtkwd_event->y_root;
    ev.xclient.data.l[2] = direction;
    ev.xclient.data.l[3] = gtkwd_event->button;
    ev.xclient.data.l[4] = 1;

    XUngrabPointer (xdisplay, gtkwd_event->time);
    XUngrabKeyboard (xdisplay, gtkwd_event->time);

    XSendEvent (xdisplay, xroot, FALSE,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&ev);

    XSync (xdisplay, FALSE);
}

static void
common_button_event (WnckWindow *win,
		     decor_event *gtkwd_event,
		     decor_event_type gtkwd_type,
		     int	button,
		     int	max,
		     char	*tooltip)
{
    GWDSettings *settings = gwd_theme_get_settings (gwd_theme);
    gboolean use_tooltips = gwd_settings_get_use_tooltips (settings);
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[button];

    if (use_tooltips)
	handle_tooltip_event (win, gtkwd_event, gtkwd_type, state, tooltip);

    switch (gtkwd_type) {
    case GButtonPress:
	if (gtkwd_event->button <= max)
	    d->button_states[button] |= PRESSED_EVENT_WINDOW;
	break;
    case GButtonRelease:
	if (gtkwd_event->button <= max)
	    d->button_states[button] &= ~PRESSED_EVENT_WINDOW;
	break;
    case GEnterNotify:
	d->button_states[button] |= IN_EVENT_WINDOW;
	break;
    case GLeaveNotify:
	d->button_states[button] &= ~IN_EVENT_WINDOW;
	break;
    default:
	break;
    }

    if (state != d->button_states[button])
	queue_decor_draw (d);
}

#define BUTTON_EVENT_ACTION_STATE (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW)

void
close_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_CLOSE];

    if (action_menu_mapped && gtkwd_type == GButtonPress)
    {
	gtk_widget_destroy (action_menu);
    }

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_CLOSE, 1, _("Close Window"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_close (win, gtkwd_event->time);
	break;
    default:
	break;
    }
}

void
max_button_event (WnckWindow *win,
		  decor_event *gtkwd_event,
		  decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_MAX];

    if (action_menu_mapped && gtkwd_type == GButtonPress)
    {
	gtk_widget_destroy (action_menu);
    }

    if (wnck_window_is_maximized (win))
	common_button_event (win, gtkwd_event, gtkwd_type, BUTTON_MAX,
			     3, _("Unmaximize Window"));
    else
	common_button_event (win, gtkwd_event, gtkwd_type, BUTTON_MAX,
			     3, _("Maximize Window"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button <= 3)
	{
	    if (state == BUTTON_EVENT_ACTION_STATE)
	    {
		if (gtkwd_event->button == 1)
		{
		    if (wnck_window_is_maximized (win))
			wnck_window_unmaximize (win);
		    else if (wnck_window_is_maximized_vertically (win))
			wnck_window_unmaximize_vertically (win);
		    else if (wnck_window_is_maximized_horizontally (win))
			wnck_window_unmaximize_horizontally (win);
		    else
			wnck_window_maximize (win);
		}
		else if (gtkwd_event->button == 2)
		{
		    if (wnck_window_is_maximized_vertically (win))
			wnck_window_unmaximize_vertically (win);
		    else
			wnck_window_maximize_vertically (win);
		}
		else if (gtkwd_event->button == 3)
		{
		    if (wnck_window_is_maximized_horizontally (win))
			wnck_window_unmaximize_horizontally (win);
		    else
			wnck_window_maximize_horizontally (win);
		}
	    }
	}
	break;
    default:
	break;
    }
}

void
min_button_event (WnckWindow *win,
		  decor_event *gtkwd_event,
		  decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_MIN];

    if (action_menu_mapped && gtkwd_type == GButtonPress)
    {
	gtk_widget_destroy (action_menu);
    }

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_MIN, 1, _("Minimize Window"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_minimize (win);
	break;
    default:
	break;
    }
}

void
menu_button_event (WnckWindow *win,
		   decor_event *gtkwd_event,
		   decor_event_type gtkwd_type)
{

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_MENU, 1, _("Window Menu"));

    switch (gtkwd_type) {
    case GButtonPress:
	if (gtkwd_event->button == 1)
	    action_menu_map (win,
			     gtkwd_event->button,
			     gtkwd_event->time);
	break;
    default:
	break;
    }
}

void
shade_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_SHADE];

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_SHADE, 1, _("Shade"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	{
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_shade (win);
	}
	break;
    default:
	break;
    }
}

void
above_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_ABOVE];

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_ABOVE, 1, _("Make Above"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_make_above (win);
	break;
    default:
	break;
    }
}

void
stick_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_STICK];

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_STICK, 1, _("Stick"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_stick (win);
	break;
    default:
	break;
    }
}

void
unshade_button_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_UNSHADE];

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_UNSHADE, 1, _("Unshade"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_unshade (win);
	break;
    default:
	break;
    }
}

void
unabove_button_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_UNABOVE];

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_UNABOVE, 1, _("Unmake Above"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_unmake_above (win);
	break;
    default:
	break;
    }
}

void
unstick_button_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type gtkwd_type)
{
    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");
    guint   state = d->button_states[BUTTON_UNSTICK];

    common_button_event (win, gtkwd_event, gtkwd_type,
			 BUTTON_UNSTICK, 1, _("Unstick"));

    switch (gtkwd_type) {
    case GButtonRelease:
	if (gtkwd_event->button == 1)
	    if (state == BUTTON_EVENT_ACTION_STATE)
		wnck_window_unstick (win);
	break;
    default:
	break;
    }
}

static void
handle_title_button_event (WnckWindow   *win,
			   int          action,
			   decor_event *gtkwd_event)
{
    switch (action) {
    case CLICK_ACTION_SHADE:
	if (wnck_window_is_shaded (win))
	    wnck_window_unshade (win);
	else
	    wnck_window_shade (win);
	break;
    case CLICK_ACTION_MAXIMIZE:
	if (wnck_window_is_maximized (win))
	    wnck_window_unmaximize (win);
	else
	    wnck_window_maximize (win);
	break;
    case CLICK_ACTION_MINIMIZE:
	if (!wnck_window_is_minimized (win))
	    wnck_window_minimize (win);
	break;
    case CLICK_ACTION_RAISE:
	restack_window (win, Above);
	break;
    case CLICK_ACTION_LOWER:
	restack_window (win, Below);
	break;
    case CLICK_ACTION_MENU:
	action_menu_map (win, gtkwd_event->button, gtkwd_event->time);
	break;
    }
}

static void
handle_mouse_wheel_title_event (WnckWindow   *win,
				unsigned int button)
{
    GWDSettings *settings = gwd_theme_get_settings (gwd_theme);
    gint wheel_action = gwd_settings_get_mouse_wheel_action (settings);

    switch (wheel_action) {
    case WHEEL_ACTION_SHADE:
	if (button == 4)
	{
	    if (!wnck_window_is_shaded (win))
		wnck_window_shade (win);
	}
	else if (button == 5)
	{
	    if (wnck_window_is_shaded (win))
		wnck_window_unshade (win);
	}
	break;
    default:
	break;
    }
}

void
title_event (WnckWindow       *win,
	     decor_event      *gtkwd_event,
	     decor_event_type gtkwd_type)
{
    GWDSettings *settings = gwd_theme_get_settings (gwd_theme);
    static Window last_button_xwindow = None;
    static Time	  last_button_time = 0;
    gint	  titlebar_action = 0;

    if (gtkwd_type != GButtonPress)
	return;

    if (gtkwd_event->button == 1)
    {
	static int last_button_num = 0;
	static int last_button_x = 0;
	static int last_button_y = 0;
	if (gtkwd_event->button == last_button_num		        &&
	    gtkwd_event->window == last_button_xwindow		        &&
	    gtkwd_event->time < last_button_time + double_click_timeout &&
	    dist (gtkwd_event->x, gtkwd_event->y,
		  last_button_x, last_button_y) < DOUBLE_CLICK_DISTANCE)
	{
	    titlebar_action = gwd_settings_get_titlebar_double_click_action (settings);
	    handle_title_button_event (win, titlebar_action,
				       gtkwd_event);

	    last_button_num	= 0;
	    last_button_xwindow = None;
	    last_button_time	= 0;
	    last_button_x	= 0;
	    last_button_y	= 0;
	}
	else
	{
	    last_button_num	= gtkwd_event->button;
	    last_button_xwindow = gtkwd_event->window;
	    last_button_time	= gtkwd_event->time;
	    last_button_x	= gtkwd_event->x;
	    last_button_y	= gtkwd_event->y;

	    restack_window (win, Above);

	    move_resize_window (win, WM_MOVERESIZE_MOVE, gtkwd_event);
	}
    }
    else if (gtkwd_event->button == 2)
    {
	titlebar_action = gwd_settings_get_titlebar_middle_click_action (settings);
	handle_title_button_event (win, titlebar_action,
				   gtkwd_event);
    }
    else if (gtkwd_event->button == 3)
    {
	titlebar_action = gwd_settings_get_titlebar_right_click_action (settings);
	handle_title_button_event (win, titlebar_action,
				   gtkwd_event);
    }
    else if (gtkwd_event->button == 4 ||
	     gtkwd_event->button == 5)
    {
	handle_mouse_wheel_title_event (win, gtkwd_event->button);
    }
}

static void
frame_common_event (WnckWindow       *win,
		    int              direction,
		    decor_event      *gtkwd_event,
		    decor_event_type gtkwd_type)
{
    GWDSettings *settings = gwd_theme_get_settings (gwd_theme);
    gint    titlebar_action = 0;

    if (gtkwd_type != GButtonPress)
	return;

    switch (gtkwd_event->button) {
    case 1:
	move_resize_window (win, direction, gtkwd_event);
	restack_window (win, Above);
	break;
    case 2:
	titlebar_action = gwd_settings_get_titlebar_middle_click_action (settings);
	handle_title_button_event (win, titlebar_action,
				   gtkwd_event);
	break;
    case 3:
	titlebar_action = gwd_settings_get_titlebar_right_click_action (settings);
	handle_title_button_event (win, titlebar_action,
				   gtkwd_event);
	break;
    }
}

void
top_left_event (WnckWindow       *win,
		decor_event      *gtkwd_event,
		decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_TOPLEFT,
		        gtkwd_event, gtkwd_type);
}

void
top_event (WnckWindow       *win,
	   decor_event      *gtkwd_event,
	   decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_TOP,
			gtkwd_event, gtkwd_type);
}

void
top_right_event (WnckWindow       *win,
		 decor_event      *gtkwd_event,
		 decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_TOPRIGHT,
			gtkwd_event, gtkwd_type);
}

void
left_event (WnckWindow       *win,
	    decor_event      *gtkwd_event,
	    decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_LEFT,
			gtkwd_event, gtkwd_type);
}

void
right_event (WnckWindow       *win,
	     decor_event      *gtkwd_event,
	     decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_RIGHT,
			gtkwd_event, gtkwd_type);
}

void
bottom_left_event (WnckWindow *win,
		   decor_event *gtkwd_event,
		   decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_BOTTOMLEFT,
			gtkwd_event, gtkwd_type);
}

void
bottom_event (WnckWindow *win,
	      decor_event *gtkwd_event,
	      decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_BOTTOM,
			gtkwd_event, gtkwd_type);
}

void
bottom_right_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type)
{
    frame_common_event (win, WM_MOVERESIZE_SIZE_BOTTOMRIGHT,
			gtkwd_event, gtkwd_type);
}

GdkFilterReturn
event_filter_func (GdkXEvent *gdkxevent,
		   GdkEvent  *event,
		   gpointer  data)
{
    GdkDisplay *gdkdisplay;
    XEvent     *xevent = gdkxevent;
    gulong     xid = 0;
    Window     select = 0;

    gdkdisplay = gdk_display_get_default ();

    switch (xevent->type) {
    case CreateNotify:
	{
	    if (!wnck_window_get (xevent->xcreatewindow.window))
	    {
		gdk_error_trap_push ();
		GdkWindow *toplevel = gdk_x11_window_foreign_new_for_display (gdkdisplay, xevent->xcreatewindow.window);

		if (toplevel)
		{
		    gdk_window_set_events (toplevel,
					   gdk_window_get_events (toplevel) |
					   GDK_PROPERTY_CHANGE_MASK);

		    /* check if the window is a switcher and update accordingly */

		    if (get_window_prop (xevent->xcreatewindow.window, select_window_atom, &select))
			update_switcher_window (xevent->xcreatewindow.window, select);
		}
	    gdk_error_trap_pop_ignored ();
	    }
	}
	break;
    case ButtonPress:
    case ButtonRelease:
	xid = (gulong)
	    g_hash_table_lookup (frame_table,
				 GINT_TO_POINTER (xevent->xbutton.window));
	break;
    case EnterNotify:
    case LeaveNotify:
	xid = (gulong)
	    g_hash_table_lookup (frame_table,
				 GINT_TO_POINTER (xevent->xcrossing.window));
	break;
    case MotionNotify:
	xid = (gulong)
	    g_hash_table_lookup (frame_table,
				 GINT_TO_POINTER (xevent->xmotion.window));
	break;
    case PropertyNotify:
	if (xevent->xproperty.atom == frame_input_window_atom)
	{
	    WnckWindow *win;

	    xid = xevent->xproperty.window;

	    win = wnck_window_get (xid);
	    if (win)
	    {
		Window frame;

		if (!get_window_prop (xid, select_window_atom, &select))
		{
		    if (get_window_prop (xid, frame_input_window_atom, &frame))
			add_frame_window (win, frame);
		    else
			remove_frame_window (win);
		}
	    }
	}
	else if (xevent->xproperty.atom == compiz_shadow_info_atom ||
		 xevent->xproperty.atom == compiz_shadow_color_atom)
	{
	    GdkScreen  *g_screen = gdk_display_get_default_screen (gdkdisplay);
	    Window     root = GDK_WINDOW_XID (gdk_screen_get_root_window (g_screen));
	    WnckScreen *screen;

	    screen = wnck_screen_get_for_root (root);

	    if (screen)
	    {
		if (gwd_process_decor_shadow_property_update ())
		    decorations_changed (screen);
	    }
	}
	else if (xevent->xproperty.atom == mwm_hints_atom)
	{
	    WnckWindow *win;

	    xid = xevent->xproperty.window;

	    win = wnck_window_get (xid);
	    if (win)
	    {
		decor_t  *d = g_object_get_data (G_OBJECT (win), "decor");
		gboolean decorated = FALSE;

		/* Only decorations that are actually bound to windows can be decorated
		 * ignore cases where a broken application which shouldn't be decorated
		 * sets the decoration hint */
		if (get_mwm_prop (xid) & (MWM_DECOR_ALL | MWM_DECOR_TITLE) && d->win)
		    decorated = TRUE;

		if (decorated != d->decorated)
		{
		    d->decorated = decorated;
		    if (decorated)
		    {
			d->context = NULL;
			d->width = d->height = 0;

			d->frame = gwd_get_decor_frame (get_frame_type (win));

			update_window_decoration_state (win);
			update_window_decoration_actions (win);
			update_window_decoration_icon (win);
			request_update_window_decoration_size (win);
			update_event_windows (win);
		    }
		    else
		    {
			remove_frame_window (win);
		    }
		}
	    }
	}
	else if (xevent->xproperty.atom == select_window_atom)
	{
	    Window select;

	    if (get_window_prop (xevent->xproperty.window, select_window_atom, &select))
		update_switcher_window (xevent->xproperty.window, select);
	}
	else if (xevent->xproperty.atom == gtk_theme_variant_atom)
	{
	    WnckWindow *win;

	    xid = xevent->xproperty.window;

	    win = wnck_window_get (xid);
	    if (win)
	    {
		decor_t  *d = g_object_get_data (G_OBJECT (win), "decor");
		gchar *old_variant = d->gtk_theme_variant;

		d->gtk_theme_variant = get_gtk_theme_variant (xid);
		if (g_strcmp0 (d->gtk_theme_variant, old_variant) != 0)
			decorations_changed (wnck_screen_get_default ());

		g_free (old_variant);

	    }
	}
	break;
    case DestroyNotify:
	g_hash_table_remove (frame_table,
			     GINT_TO_POINTER (xevent->xproperty.window));
	break;
    case ClientMessage:
	if (xevent->xclient.message_type == toolkit_action_atom)
	{
	    long action;

	    action = xevent->xclient.data.l[0];
	    if (action == toolkit_action_window_menu_atom)
	    {
		WnckWindow *win;

		win = wnck_window_get (xevent->xclient.window);
		if (win)
		{
		    action_menu_map (win,
				     xevent->xclient.data.l[2],
				     xevent->xclient.data.l[1]);
		}
	    }
	    else if (action == toolkit_action_force_quit_dialog_atom)
	    {
		WnckWindow *win;

		win = wnck_window_get (xevent->xclient.window);
		if (win)
		{
		    if (xevent->xclient.data.l[2])
			show_force_quit_dialog (win,
						xevent->xclient.data.l[1]);
		    else
			hide_force_quit_dialog (win);
		}
	    }
	}
	else if (xevent->xclient.message_type == decor_request_atom)
	{
	    WnckWindow *win = wnck_window_get (xevent->xclient.window);

	    if (win)
		update_window_decoration_size (win);
	}
    default:
	break;
    }

    if (xid)
    {
	WnckWindow *win;

	win = wnck_window_get (xid);
	if (win)
	{
	    decor_t *d = g_object_get_data (G_OBJECT (win), "decor");

	    if (d->decorated)
	    {
		gint             i, j;
		event_callback   cb = NULL;
		Window           w = xevent->xany.window;

		for (i = 0; i < 3; ++i)
		    for (j = 0; j < 3; ++j)
			if (d->event_windows[i][j].window == w)
			    cb = d->event_windows[i][j].callback;

		if (!cb)
		{
		    for (i = 0; i < BUTTON_NUM; ++i)
			if (d->button_windows[i].window == w)
			    cb = d->button_windows[i].callback;
		}

		if (cb)
		{
		    decor_event      gtkwd_event;
		    decor_event_type gtkwd_type;

		    gtkwd_event.window = w;

		    switch (xevent->type)
		    {
			case ButtonPress:
			case ButtonRelease:
			    if (xevent->type == ButtonPress)
				gtkwd_type = GButtonPress;
			    else
				gtkwd_type = GButtonRelease;
			    gtkwd_event.button = xevent->xbutton.button;
			    gtkwd_event.x = xevent->xbutton.x;
			    gtkwd_event.y = xevent->xbutton.y;
			    gtkwd_event.x_root = xevent->xbutton.x_root;
			    gtkwd_event.y_root = xevent->xbutton.y_root;
			    gtkwd_event.time = xevent->xbutton.time;
			    break;
			case EnterNotify:
			case LeaveNotify:
			    if (xevent->type == EnterNotify)
				gtkwd_type = GEnterNotify;
			    else
				gtkwd_type = GLeaveNotify;
			    gtkwd_event.x = xevent->xcrossing.x;
			    gtkwd_event.y = xevent->xcrossing.y;
			    gtkwd_event.x_root = xevent->xcrossing.x_root;
			    gtkwd_event.y_root = xevent->xcrossing.y_root;
			    gtkwd_event.time = xevent->xcrossing.time;
			    break;
			default:
			    cb = NULL;
			    break;
		    }
		    if (cb)
			(*cb) (win, &gtkwd_event, gtkwd_type);
		}
	    }
	}
    }

    return GDK_FILTER_CONTINUE;
}

GdkFilterReturn
selection_event_filter_func (GdkXEvent *gdkxevent,
			     GdkEvent  *event,
			     gpointer  data)
{
    Display    *xdisplay;
    GdkDisplay *gdkdisplay;
    XEvent     *xevent = gdkxevent;
    int	       status;

    gdkdisplay = gdk_display_get_default ();
    xdisplay   = GDK_DISPLAY_XDISPLAY (gdkdisplay);

    switch (xevent->type) {
    case SelectionRequest:
	decor_handle_selection_request (xdisplay, xevent, dm_sn_timestamp);
	break;
    case SelectionClear:
	status = decor_handle_selection_clear (xdisplay, xevent, 0);
	if (status == DECOR_SELECTION_GIVE_UP)
	    gtk_main_quit ();
    default:
	break;
    }

    return GDK_FILTER_CONTINUE;
}
