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

void
decor_update_switcher_property (decor_t *d)
{
    long	 *data;
    Display	 *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    gint	 nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    unsigned int    nOffset = 1;
    unsigned int   frame_type = populate_frame_type (d);
    unsigned int   frame_state = populate_frame_state (d);
    unsigned int   frame_actions = populate_frame_actions (d);
    GtkStyleContext *context;
    GdkRGBA fg;
    long         fgColor[4];
    
    nQuad = decor_set_lSrStSbX_window_quads (quads, &d->frame->window_context_active,
					     &d->border_layout,
					     d->border_layout.top.x2 -
					     d->border_layout.top.x1 -
					     d->frame->window_context_active.extents.left -
						 d->frame->window_context_active.extents.right -
						     32);
    
    data = decor_alloc_property (nOffset, WINDOW_DECORATION_TYPE_PIXMAP);
    decor_quads_to_property (data, nOffset - 1, cairo_xlib_surface_get_drawable (d->surface),
			     &d->frame->win_extents, &d->frame->win_extents,
			     &d->frame->win_extents, &d->frame->win_extents,
			     0, 0, quads, nQuad, frame_type, frame_state, frame_actions);

    context = gtk_widget_get_style_context (d->frame->style_window_rgba);

    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_restore (context);

    fgColor[0] = fg.red;
    fgColor[1] = fg.green;
    fgColor[2] = fg.blue;
    fgColor[3] = SWITCHER_ALPHA;
    
    gdk_error_trap_push ();
    XChangeProperty (xdisplay, d->prop_xid,
		     win_decor_atom,
		     XA_INTEGER,
		     32, PropModeReplace, (guchar *) data,
		     PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);
    XChangeProperty (xdisplay, d->prop_xid, switcher_fg_atom,
		     XA_INTEGER, 32, PropModeReplace, (guchar *) fgColor, 4);
    gdk_display_sync (gdk_display_get_default ());
    gdk_error_trap_pop_ignored ();

    free (data);
}
