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

#ifndef _GTK_WINDOW_DECORATOR_H
#define _GTK_WINDOW_DECORATOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "decoration.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xregion.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <pango/pango-context.h>
#include <pango/pangocairo.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <libintl.h>
#define _(x)  gettext (x)
#define N_(x) x

#include "gwd-theme.h"

extern const unsigned short ICON_SPACE;

#define META_MAXIMIZED (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY | \
WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY)

#define MWM_HINTS_DECORATIONS (1L << 1)

#define MWM_DECOR_ALL      (1L << 0)
#define MWM_DECOR_BORDER   (1L << 1)
#define MWM_DECOR_HANDLE   (1L << 2)
#define MWM_DECOR_TITLE    (1L << 3)
#define MWM_DECOR_MENU     (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define PROP_MOTIF_WM_HINT_ELEMENTS 3

typedef struct {
unsigned long flags;
unsigned long functions;
unsigned long decorations;
} MwmHints;

extern gboolean minimal;

extern GWDTheme *gwd_theme;

extern gdouble decoration_alpha;

extern Atom frame_input_window_atom;
extern Atom win_decor_atom;
extern Atom win_blur_decor_atom;
extern Atom wm_move_resize_atom;
extern Atom restack_window_atom;
extern Atom select_window_atom;
extern Atom mwm_hints_atom;
extern Atom switcher_fg_atom;
extern Atom compiz_shadow_info_atom;
extern Atom compiz_shadow_color_atom;
extern Atom toolkit_action_atom;
extern Atom toolkit_action_window_menu_atom;
extern Atom toolkit_action_force_quit_dialog_atom;
extern Atom net_wm_state_atom;
extern Atom net_wm_state_modal_atom;
extern Atom decor_request_atom;
extern Atom decor_pending_atom;
extern Atom decor_delete_pixmap_atom;
extern Atom utf8_string_atom;
extern Atom gtk_theme_variant_atom;

extern Time dm_sn_timestamp;

#define C(name) { 0, XC_ ## name }

struct _cursor {
    Cursor	 cursor;
    unsigned int shape;
};

extern struct _cursor cursor[3][3];

#define BUTTON_CLOSE   0
#define BUTTON_MAX     1
#define BUTTON_MIN     2
#define BUTTON_MENU    3
#define BUTTON_SHADE   4
#define BUTTON_ABOVE   5
#define BUTTON_STICK   6
#define BUTTON_UNSHADE 7
#define BUTTON_UNABOVE 8
#define BUTTON_UNSTICK 9
#define BUTTON_NUM     10

struct _pos {
    int x, y, w, h;
    int xw, yh, ww, hh, yth, hth;
};

extern struct _pos pos[3][3], bpos[];

typedef struct _decor_color {
    double r;
    double g;
    double b;
} decor_color_t;

#define IN_EVENT_WINDOW      (1 << 0)
#define PRESSED_EVENT_WINDOW (1 << 1)

typedef struct _decor_event {
    guint time;
    guint window;
    guint x;
    guint y;
    guint x_root;
    guint y_root;
    guint button;
} decor_event;

typedef enum _decor_event_type {
    GButtonPress = 1,
    GButtonRelease,
    GEnterNotify,
    GLeaveNotify,
    GMotionNotify
} decor_event_type;

typedef void (*event_callback) (WnckWindow       *win,
				decor_event      *gtkwd_event,
				decor_event_type gtkwd_type);

typedef struct {
    Window         window;
    Box            pos;
    event_callback callback;
} event_window;

typedef struct _decor_frame decor_frame_t;
typedef struct _decor_shadow_info decor_shadow_info_t;

struct _decor_shadow_info
{
    decor_frame_t *frame;
    unsigned int  state;
    gboolean      active;
};

typedef void (*frame_update_shadow_proc) (Display		 *display,
					  Screen		 *screen,
					  decor_frame_t		 *frame,
					  decor_shadow_t	  **shadow_normal,
					  decor_context_t	  *context_normal,
					  decor_shadow_t	  **shadow_max,
					  decor_context_t	  *context_max,
					  decor_shadow_info_t    *info,
					  decor_shadow_options_t *opt_shadow,
					  decor_shadow_options_t *opt_no_shadow);

typedef decor_frame_t * (*create_frame_proc) (const gchar *);
typedef void (*destroy_frame_proc) (decor_frame_t *);

struct _decor_frame {
    decor_extents_t win_extents;
    decor_extents_t max_win_extents;
    decor_shadow_t *border_shadow_active;
    decor_shadow_t *border_shadow_inactive;
    decor_shadow_t *max_border_shadow_active;
    decor_shadow_t *max_border_shadow_inactive;
    decor_context_t window_context_active;
    decor_context_t window_context_inactive;
    decor_context_t max_window_context_active;
    decor_context_t max_window_context_inactive;
    PangoContext	 *pango_context;
    gint		 text_height;
    gchar		 *type;

    gboolean has_shadow_extents;
    decor_extents_t shadow_extents;
    decor_extents_t max_shadow_extents;

    frame_update_shadow_proc update_shadow;
    gint		refcount;
};

typedef struct _decor {
    WnckWindow	      *win;
    decor_frame_t     *frame;
    event_window      event_windows[3][3];
    event_window      button_windows[BUTTON_NUM];
    guint	      button_states[BUTTON_NUM];
    Pixmap            x11Pixmap;
    cairo_surface_t   *surface;
    cairo_surface_t   *buffer_surface;
    cairo_t	      *cr;
    decor_layout_t    border_layout;
    decor_context_t   *context;
    decor_shadow_t    *shadow;
    Picture	      picture;
    gint	      button_width;
    gint	      width;
    gint	      height;
    gint	      client_width;
    gint	      client_height;
    gboolean	      decorated;
    gboolean	      active;
    PangoLayout	      *layout;
    gchar	      *name;
    gchar	      *gtk_theme_variant;
    cairo_pattern_t   *icon;
    cairo_surface_t   *icon_surface;
    GdkPixbuf	      *icon_pixbuf;
    WnckWindowState   state;
    WnckWindowActions actions;
    XID		      prop_xid;
    GtkWidget	      *force_quit_dialog;
    Bool	      created;
    void	      (*draw) (struct _decor *d);
} decor_t;

#define WINDOW_TYPE_FRAMES_NUM 5
typedef struct _default_frame_references
{
    char     *name;
    decor_t  *d;
} default_frame_references_t;

extern default_frame_references_t default_frames[WINDOW_TYPE_FRAMES_NUM * 2];
const gchar * window_type_frames[WINDOW_TYPE_FRAMES_NUM];

extern char *program_name;

/* list of all decorations */
extern GHashTable    *frame_table;

/* action menu */
extern GtkWidget     *action_menu;
extern gboolean      action_menu_mapped;
extern gint	     double_click_timeout;


/* tooltip */
extern GtkWidget     *tip_window;
extern GtkWidget     *tip_label;
extern GTimeVal	     tooltip_last_popdown;
extern gint	     tooltip_timer_tag;

extern GSList *draw_list;
extern guint  draw_idle_id;

/* switcher */
extern Window     switcher_selected_window;
extern GtkWidget  *switcher_label;
extern decor_t    *switcher_window;

extern XRenderPictFormat *xformat_rgba;

/* frames.c */

void
initialize_decorations ();

decor_frame_t *
gwd_get_decor_frame (const gchar *);

decor_frame_t *
gwd_decor_frame_ref (decor_frame_t *);

decor_frame_t *
gwd_decor_frame_unref (decor_frame_t *);

void
gwd_frames_foreach (GHFunc   foreach_func,
		    gpointer user_data);

void
gwd_process_frames (GHFunc	foreach_func,
		    const gchar	*frame_keys[],
		    gint	frame_keys_num,
		    gpointer	user_data);

decor_frame_t *
decor_frame_new (const gchar *type);

void
decor_frame_destroy (decor_frame_t *);

/* decorator.c */

void
frame_update_shadow (decor_frame_t	    *frame,
		     decor_shadow_info_t    *info,
		     decor_shadow_options_t *opt_shadow,
		     decor_shadow_options_t *opt_no_shadow);

void
frame_update_titlebar_font (decor_frame_t *frame);

decor_frame_t *
create_normal_frame (const gchar *type);

void
destroy_normal_frame ();

decor_frame_t *
create_bare_frame (const gchar *type);

void
destroy_bare_frame ();

/* Don't use directly */
gboolean
update_window_decoration_size (WnckWindow *win);

gboolean
request_update_window_decoration_size (WnckWindow *win);

unsigned int
populate_frame_type (decor_t *d);

unsigned int
populate_frame_state (decor_t *d);

unsigned int
populate_frame_actions (decor_t *d);

void
update_default_decorations (GdkScreen *screen);

void
update_window_decoration_state (WnckWindow *win);

void
update_window_decoration_actions (WnckWindow *win);

void
update_window_decoration_icon (WnckWindow *win);

void
update_event_windows (WnckWindow *win);

int
update_shadow (void);

void
update_window_decoration (WnckWindow *win);

void
queue_decor_draw (decor_t *d);

void
copy_to_front_buffer (decor_t *d);

/* wnck.c*/

const gchar *
get_frame_type (WnckWindow *win);

void
decorations_changed (WnckScreen *screen);

void
connect_screen (WnckScreen *screen);

void
window_closed (WnckScreen *screen,
	       WnckWindow *window);

void
add_frame_window (WnckWindow *win,
                  Window     frame);

void
remove_frame_window (WnckWindow *win);

void
restack_window (WnckWindow *win,
		int	   stack_mode);

/* blur.c */

void
decor_update_blur_property (decor_t *d,
			    int     width,
			    int     height,
			    Region  top_region,
			    int     top_offset,
			    Region  bottom_region,
			    int     bottom_offset,
			    Region  left_region,
			    int     left_offset,
			    Region  right_region,
			    int     right_offset);

/* cairo.c */

#define CORNER_TOPLEFT     (1 << 0)
#define CORNER_TOPRIGHT    (1 << 1)
#define CORNER_BOTTOMRIGHT (1 << 2)
#define CORNER_BOTTOMLEFT  (1 << 3)

#define SHADE_LEFT   (1 << 0)
#define SHADE_RIGHT  (1 << 1)
#define SHADE_TOP    (1 << 2)
#define SHADE_BOTTOM (1 << 3)

void
draw_shadow_background (decor_t		*d,
			cairo_t		*cr,
			decor_shadow_t  *s,
			decor_context_t *c);

void
fill_rounded_rectangle (cairo_t       *cr,
			double        x,
			double        y,
			double        w,
			double        h,
			double	      radius,
			int	      corner,
			decor_color_t *c0,
			double        alpha0,
			decor_color_t *c1,
			double	      alpha1,
			int	      gravity);

void
rounded_rectangle (cairo_t *cr,
		   double  x,
		   double  y,
		   double  w,
		   double  h,
		   double  radius,
		   int	   corner);

/* gdk.c */

cairo_surface_t *
create_surface (int	 w,
	       int	 h,
	       GtkWidget *parent_style_window);

cairo_surface_t *
create_native_surface_and_wrap (int	  w,
			       int	  h,
			       GtkWidget *parent_style_window);

/* switcher.c */

#define SWITCHER_ALPHA 0xa0a0

void
switcher_frame_update_shadow (Display		  *xdisplay,
			      Screen		  *screen,
			      decor_frame_t	  *frame,
			      decor_shadow_t	  **shadow_normal,
			      decor_context_t	  *context_normal,
			      decor_shadow_t	  **shadow_max,
			      decor_context_t	  *context_max,
			      decor_shadow_info_t    *info,
			      decor_shadow_options_t *opt_shadow,
			      decor_shadow_options_t *opt_no_shadow);

decor_frame_t *
create_switcher_frame (const gchar *);

void
destroy_switcher_frame ();

gboolean
update_switcher_window (Window     popup,
			Window     selected);

/* events.c */

void
close_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type);

void
max_button_event (WnckWindow *win,
		  decor_event *gtkwd_event,
		  decor_event_type gtkwd_type);

void
min_button_event (WnckWindow *win,
		  decor_event *gtkwd_event,
		  decor_event_type gtkwd_type);

void
menu_button_event (WnckWindow *win,
		   decor_event *gtkwd_event,
		   decor_event_type gtkwd_type);

void
shade_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type);

void
above_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type);

void
stick_button_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type);
void
unshade_button_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type gtkwd_type);

void
unabove_button_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type gtkwd_type);

void
unstick_button_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type gtkwd_type);

void
title_event (WnckWindow       *win,
	     decor_event      *gtkwd_event,
	     decor_event_type gtkwd_type);

void
top_left_event (WnckWindow       *win,
		decor_event      *gtkwd_event,
		decor_event_type gtkwd_type);

void
top_event (WnckWindow       *win,
	   decor_event      *gtkwd_event,
	   decor_event_type gtkwd_type);

void
top_right_event (WnckWindow       *win,
		 decor_event      *gtkwd_event,
		 decor_event_type gtkwd_type);

void
left_event (WnckWindow       *win,
	    decor_event      *gtkwd_event,
	    decor_event_type gtkwd_type);
void
right_event (WnckWindow       *win,
	     decor_event      *gtkwd_event,
	     decor_event_type gtkwd_type);

void
bottom_left_event (WnckWindow *win,
		   decor_event *gtkwd_event,
		   decor_event_type gtkwd_type);

void
bottom_event (WnckWindow *win,
	      decor_event *gtkwd_event,
	      decor_event_type gtkwd_type);
void
bottom_right_event (WnckWindow *win,
		    decor_event *gtkwd_event,
		    decor_event_type gtkwd_type);

GdkFilterReturn
selection_event_filter_func (GdkXEvent *gdkxevent,
			     GdkEvent  *event,
			     gpointer  data);

GdkFilterReturn
event_filter_func (GdkXEvent *gdkxevent,
		   GdkEvent  *event,
		   gpointer  data);

/* tooltip.c */

gboolean
create_tooltip_window (void);

void
handle_tooltip_event (WnckWindow *win,
		      decor_event *gtkwd_event,
		      decor_event_type   gtkwd_type,
		      guint	 state,
		      const char *tip);

/* forcequit.c */

void
show_force_quit_dialog (WnckWindow *win,
			Time        timestamp);

void
hide_force_quit_dialog (WnckWindow *win);

/* actionmenu.c */

void
action_menu_map (WnckWindow *win,
		 long	     button,
		 Time	     time);

/* util.c */

gboolean
get_window_prop (Window xwindow,
		 Atom   atom,
		 Window *val);

unsigned int
get_mwm_prop (Window xwindow);

gchar *
get_gtk_theme_variant (Window xwindow);

/* settings.c */

void
init_settings (GWDSettings *settings);

void
fini_settings ();

gboolean
gwd_process_decor_shadow_property_update ();

#endif
