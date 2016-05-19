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
#include "gwd-theme-metacity.h"

#ifdef USE_METACITY

static void
decor_update_meta_window_property (decor_t        *d,
                                   MetaTheme      *theme,
                                   MetaFrameFlags  flags,
                                   MetaFrameType   type,
                                   Region          top,
                                   Region          bottom,
                                   Region          left,
                                   Region          right)
{
    long *data;
    GdkDisplay *display;
    Display *xdisplay;
    gint nQuad;
    decor_extents_t win_extents;
    decor_extents_t frame_win_extents;
    decor_extents_t max_win_extents;
    decor_extents_t frame_max_win_extents;
    decor_quad_t quads[N_QUADS_MAX];
    unsigned int nOffset;
    unsigned int frame_type;
    unsigned int frame_state;
    unsigned int frame_actions;
    gint w;
    gint lh;
    gint rh;
    gint top_stretch_offset;
    gint bottom_stretch_offset;
    gint left_stretch_offset;
    gint right_stretch_offset;

    display = gdk_display_get_default ();
    xdisplay = gdk_x11_display_get_xdisplay (display);

    nOffset = 1;

    frame_type = populate_frame_type (d);
    frame_state = populate_frame_state (d);
    frame_actions = populate_frame_actions (d);

    win_extents = frame_win_extents = d->frame->win_extents;
    max_win_extents = frame_max_win_extents = d->frame->max_win_extents;

    /* Add the invisible grab area padding, but only for
     * pixmap type decorations */
    if (!d->frame_window)
    {
        GdkScreen *screen;
        MetaStyleInfo *style_info;
        MetaFrameBorders borders;

        screen = gtk_widget_get_screen (d->frame->style_window_rgba);
        style_info = meta_theme_create_style_info (screen, d->gtk_theme_variant);

        meta_theme_get_frame_borders (theme, style_info, type,
                                      d->frame->text_height,
                                      flags, &borders);

        if (flags & META_FRAME_ALLOWS_HORIZONTAL_RESIZE)
        {
            frame_win_extents.left += borders.invisible.left;
            frame_win_extents.right += borders.invisible.right;
            frame_max_win_extents.left += borders.invisible.left;
            frame_max_win_extents.right += borders.invisible.right;
        }

        if (flags & META_FRAME_ALLOWS_VERTICAL_RESIZE)
        {
            frame_win_extents.bottom += borders.invisible.bottom;
            frame_win_extents.top += borders.invisible.top;
            frame_max_win_extents.bottom += borders.invisible.bottom;
            frame_max_win_extents.top += borders.invisible.top;
        }

        meta_style_info_unref (style_info);
    }

    w = d->border_layout.top.x2 - d->border_layout.top.x1 -
        d->context->left_space - d->context->right_space;

    if (d->border_layout.rotation)
        lh = d->border_layout.left.x2 - d->border_layout.left.x1;
    else
        lh = d->border_layout.left.y2 - d->border_layout.left.y1;

    if (d->border_layout.rotation)
        rh = d->border_layout.right.x2 - d->border_layout.right.x1;
    else
        rh = d->border_layout.right.y2 - d->border_layout.right.y1;

    left_stretch_offset = lh / 2;
    right_stretch_offset = rh / 2;
    top_stretch_offset = w - d->button_width - 1;
    bottom_stretch_offset = (d->border_layout.bottom.x2 - d->border_layout.bottom.x1 -
                             d->context->left_space - d->context->right_space) / 2;

    nQuad = decor_set_lXrXtXbX_window_quads (quads, d->context, &d->border_layout,
                                             left_stretch_offset, right_stretch_offset,
                                             top_stretch_offset, bottom_stretch_offset);

    win_extents.top += d->frame->titlebar_height;
    frame_win_extents.top += d->frame->titlebar_height;
    max_win_extents.top += d->frame->max_titlebar_height;
    frame_max_win_extents.top += d->frame->max_titlebar_height;

    if (d->frame_window)
    {
        data = decor_alloc_property (nOffset, WINDOW_DECORATION_TYPE_WINDOW);
        decor_gen_window_property (data, nOffset - 1, &win_extents, &max_win_extents,
                                   20, 20, frame_type, frame_state, frame_actions);
    }
    else
    {
        data = decor_alloc_property (nOffset, WINDOW_DECORATION_TYPE_PIXMAP);
        decor_quads_to_property (data, nOffset - 1, cairo_xlib_surface_get_drawable (d->surface),
                                 &frame_win_extents, &win_extents,
                                 &frame_max_win_extents, &max_win_extents,
                                 ICON_SPACE + d->button_width,
                                 0, quads, nQuad, frame_type, frame_state, frame_actions);
    }

    gdk_error_trap_push ();

    XChangeProperty (xdisplay, d->prop_xid, win_decor_atom, XA_INTEGER,
                     32, PropModeReplace, (guchar *) data,
                     PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);
    gdk_display_sync (display);

    gdk_error_trap_pop_ignored ();

    free (data);

    decor_update_blur_property (d, w, lh,
                                top, top_stretch_offset,
                                bottom, bottom_stretch_offset,
                                left, left_stretch_offset,
                                right, right_stretch_offset);
}

static void
meta_get_corner_radius (const MetaFrameGeometry *fgeom,
                        int                     *top_left_radius,
                        int                     *top_right_radius,
                        int                     *bottom_left_radius,
                        int                     *bottom_right_radius)
{
    *top_left_radius     = fgeom->top_left_corner_rounded_radius;
    *top_right_radius    = fgeom->top_right_corner_rounded_radius;
    *bottom_left_radius  = fgeom->bottom_left_corner_rounded_radius;
    *bottom_right_radius = fgeom->bottom_right_corner_rounded_radius;
}

static int
radius_to_width (int radius,
                 int i)
{
    float r1 = sqrt (radius) + radius;
    float r2 = r1 * r1 - (r1 - (i + 0.5)) * (r1 - (i + 0.5));

    return floor (0.5f + r1 - sqrt (r2));
}

static Region
meta_get_top_border_region (const MetaFrameGeometry *fgeom,
                            int                      width)
{
    Region corners_xregion;
    Region border_xregion;
    XRectangle xrect;
    int top_left_radius;
    int top_right_radius;
    int bottom_left_radius;
    int bottom_right_radius;
    int w;
    int i;
    int height;

    corners_xregion = XCreateRegion ();

    meta_get_corner_radius (fgeom, &top_left_radius, &top_right_radius,
                            &bottom_left_radius, &bottom_right_radius);

    width = width - fgeom->borders.invisible.left - fgeom->borders.invisible.right;
    height = fgeom->borders.visible.top;

    if (top_left_radius)
    {
        for (i = 0; i < top_left_radius; ++i)
        {
            w = radius_to_width (top_left_radius, i);

            xrect.x = 0;
            xrect.y = i;
            xrect.width = w;
            xrect.height = 1;

            XUnionRectWithRegion (&xrect, corners_xregion, corners_xregion);
        }
    }

    if (top_right_radius)
    {
        for (i = 0; i < top_right_radius; ++i)
        {
            w = radius_to_width (top_right_radius, i);

            xrect.x = width - w;
            xrect.y = i;
            xrect.width = w;
            xrect.height = 1;

            XUnionRectWithRegion (&xrect, corners_xregion, corners_xregion);
        }
    }

    border_xregion = XCreateRegion ();

    xrect.x = 0;
    xrect.y = 0;
    xrect.width = width;
    xrect.height = height;

    XUnionRectWithRegion (&xrect, border_xregion, border_xregion);

    XSubtractRegion (border_xregion, corners_xregion, border_xregion);

    XDestroyRegion (corners_xregion);

    return border_xregion;
}

static Region
meta_get_bottom_border_region (const MetaFrameGeometry *fgeom,
                               int                      width)
{
    Region corners_xregion;
    Region border_xregion;
    XRectangle xrect;
    int top_left_radius;
    int top_right_radius;
    int bottom_left_radius;
    int bottom_right_radius;
    int w;
    int i;
    int height;

    corners_xregion = XCreateRegion ();

    meta_get_corner_radius (fgeom, &top_left_radius, &top_right_radius,
                            &bottom_left_radius, &bottom_right_radius);

    width = width - fgeom->borders.invisible.left - fgeom->borders.invisible.right;
    height = fgeom->borders.visible.bottom;

    if (bottom_left_radius)
    {
        for (i = 0; i < bottom_left_radius; ++i)
        {
            w = radius_to_width (bottom_left_radius, i);

            xrect.x = 0;
            xrect.y = height - i - 1;
            xrect.width = w;
            xrect.height = 1;

            XUnionRectWithRegion (&xrect, corners_xregion, corners_xregion);
        }
    }

    if (bottom_right_radius)
    {
        for (i = 0; i < bottom_right_radius; ++i)
        {
            w = radius_to_width (bottom_right_radius, i);

            xrect.x = width - w;
            xrect.y = height - i - 1;
            xrect.width = w;
            xrect.height = 1;

            XUnionRectWithRegion (&xrect, corners_xregion, corners_xregion);
        }
    }

    border_xregion = XCreateRegion ();

    xrect.x = 0;
    xrect.y = 0;
    xrect.width = width;
    xrect.height = height;

    XUnionRectWithRegion (&xrect, border_xregion, border_xregion);

    XSubtractRegion (border_xregion, corners_xregion, border_xregion);

    XDestroyRegion (corners_xregion);

    return border_xregion;
}

static Region
meta_get_left_border_region (const MetaFrameGeometry *fgeom,
                             int                      height)
{
    Region border_xregion;
    XRectangle xrect;

    border_xregion = XCreateRegion ();

    xrect.x = 0;
    xrect.y = 0;
    xrect.width = fgeom->borders.visible.left;
    xrect.height = height - fgeom->borders.total.top - fgeom->borders.total.bottom;

    XUnionRectWithRegion (&xrect, border_xregion, border_xregion);

    return border_xregion;
}

static Region
meta_get_right_border_region (const MetaFrameGeometry *fgeom,
                              int                      height)
{
    Region border_xregion;
    XRectangle xrect;

    border_xregion = XCreateRegion ();

    xrect.x = 0;
    xrect.y = 0;
    xrect.width = fgeom->borders.visible.right;
    xrect.height = height - fgeom->borders.total.top - fgeom->borders.total.bottom;

    XUnionRectWithRegion (&xrect, border_xregion, border_xregion);

    return border_xregion;
}

static MetaButtonState
meta_button_state (int state)
{
    if (state & IN_EVENT_WINDOW)
    {
        if (state & PRESSED_EVENT_WINDOW)
            return META_BUTTON_STATE_PRESSED;

        return META_BUTTON_STATE_PRELIGHT;
    }

    return META_BUTTON_STATE_NORMAL;
}

static MetaButtonType
meta_function_to_type (MetaButtonFunction function)
{
    switch (function)
    {
    case META_BUTTON_FUNCTION_MENU:
        return META_BUTTON_TYPE_MENU;
    case META_BUTTON_FUNCTION_MINIMIZE:
        return META_BUTTON_TYPE_MINIMIZE;
    case META_BUTTON_FUNCTION_MAXIMIZE:
        return META_BUTTON_TYPE_MAXIMIZE;
    case META_BUTTON_FUNCTION_CLOSE:
        return META_BUTTON_TYPE_CLOSE;
    case META_BUTTON_FUNCTION_SHADE:
        return META_BUTTON_TYPE_SHADE;
    case META_BUTTON_FUNCTION_ABOVE:
        return META_BUTTON_TYPE_ABOVE;
    case META_BUTTON_FUNCTION_STICK:
        return META_BUTTON_TYPE_STICK;
    case META_BUTTON_FUNCTION_UNSHADE:
        return META_BUTTON_TYPE_UNSHADE;
    case META_BUTTON_FUNCTION_UNABOVE:
        return META_BUTTON_TYPE_UNABOVE;
    case META_BUTTON_FUNCTION_UNSTICK:
        return META_BUTTON_TYPE_UNSTICK;
    default:
        break;
    }

    return META_BUTTON_TYPE_LAST;
}

static MetaButtonState
meta_button_state_for_button_type (decor_t        *d,
                                   MetaButtonType  type)
{
    switch (type)
    {
    case META_BUTTON_TYPE_LEFT_LEFT_BACKGROUND:
        type = meta_function_to_type (meta_button_layout.left_buttons[0]);
        break;
    case META_BUTTON_TYPE_LEFT_MIDDLE_BACKGROUND:
        type = meta_function_to_type (meta_button_layout.left_buttons[1]);
        break;
    case META_BUTTON_TYPE_LEFT_RIGHT_BACKGROUND:
        type = meta_function_to_type (meta_button_layout.left_buttons[2]);
        break;
    case META_BUTTON_TYPE_RIGHT_LEFT_BACKGROUND:
        type = meta_function_to_type (meta_button_layout.right_buttons[0]);
        break;
    case META_BUTTON_TYPE_RIGHT_MIDDLE_BACKGROUND:
        type = meta_function_to_type (meta_button_layout.right_buttons[1]);
        break;
    case META_BUTTON_TYPE_RIGHT_RIGHT_BACKGROUND:
        type = meta_function_to_type (meta_button_layout.right_buttons[2]);
    default:
        break;
    }

    switch (type)
    {
    case META_BUTTON_TYPE_CLOSE:
        return meta_button_state (d->button_states[BUTTON_CLOSE]);
    case META_BUTTON_TYPE_MAXIMIZE:
        return meta_button_state (d->button_states[BUTTON_MAX]);
    case META_BUTTON_TYPE_MINIMIZE:
        return meta_button_state (d->button_states[BUTTON_MIN]);
    case META_BUTTON_TYPE_MENU:
        return meta_button_state (d->button_states[BUTTON_MENU]);
    case META_BUTTON_TYPE_SHADE:
        return meta_button_state (d->button_states[BUTTON_SHADE]);
    case META_BUTTON_TYPE_ABOVE:
        return meta_button_state (d->button_states[BUTTON_ABOVE]);
    case META_BUTTON_TYPE_STICK:
        return meta_button_state (d->button_states[BUTTON_STICK]);
    case META_BUTTON_TYPE_UNSHADE:
        return meta_button_state (d->button_states[BUTTON_UNSHADE]);
    case META_BUTTON_TYPE_UNABOVE:
        return meta_button_state (d->button_states[BUTTON_UNABOVE]);
    case META_BUTTON_TYPE_UNSTICK:
        return meta_button_state (d->button_states[BUTTON_UNSTICK]);
    default:
        break;
    }

    return META_BUTTON_STATE_NORMAL;
}

void
meta_draw_window_decoration (decor_t *d)
{
    GdkDisplay *display;
    GdkScreen *screen;
    Display *xdisplay;
    cairo_surface_t *surface;
    Picture src;
    MetaButtonState button_states [META_BUTTON_TYPE_LAST];
    MetaButtonLayout button_layout;
    MetaFrameGeometry fgeom;
    MetaFrameFlags flags;
    MetaFrameType frame_type;
    MetaTheme *theme;
    MetaStyleInfo *style_info;
    GtkStyleContext *context;
    cairo_t *cr;
    gint i;
    Region top_region;
    Region bottom_region;
    Region left_region;
    Region right_region;
    gdouble meta_active_opacity;
    gdouble meta_inactive_opacity;
    gboolean meta_active_shade_opacity;
    gboolean meta_inactive_shade_opacity;
    double alpha;
    gboolean shade_alpha;
    MetaFrameStyle *frame_style;
    GtkWidget *style_window;
    GdkRGBA bg_rgba;

    if (!d->surface || !d->picture)
        return;

    display = gdk_display_get_default ();
    xdisplay = gdk_x11_display_get_xdisplay (display);

    top_region = NULL;
    bottom_region = NULL;
    left_region = NULL;
    right_region = NULL;

    g_object_get (settings, "metacity-active-opacity", &meta_active_opacity, NULL);
    g_object_get (settings, "metacity-inactive-opacity", &meta_inactive_opacity, NULL);
    g_object_get (settings, "metacity-active-shade-opacity", &meta_active_shade_opacity, NULL);
    g_object_get (settings, "metacity-inactive-shade-opacity", &meta_inactive_shade_opacity, NULL);

    alpha = (d->active) ? meta_active_opacity : meta_inactive_opacity;
    shade_alpha = (d->active) ? meta_active_shade_opacity : meta_inactive_shade_opacity;

    if (decoration_alpha == 1.0)
        alpha = 1.0;

    if (cairo_xlib_surface_get_depth (d->surface) == 32)
        style_window = d->frame->style_window_rgba;
    else
        style_window = d->frame->style_window_rgb;

    context = gtk_widget_get_style_context (style_window);

    cr = cairo_create (d->buffer_surface ? d->buffer_surface : d->surface);

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    theme = meta_theme_get_current ();

    frame_type = meta_frame_type_from_string (d->frame->type);

    if (frame_type == META_FRAME_TYPE_LAST)
      frame_type = META_FRAME_TYPE_NORMAL;

    gwd_theme_metacity_get_decoration_geometry (GWD_THEME_METACITY (gwd_theme),
                                                d, &flags, &fgeom,
                                                &button_layout, frame_type);

    if ((d->prop_xid || !d->buffer_surface) && !d->frame_window)
        draw_shadow_background (d, cr, d->shadow, d->context);

    for (i = 0; i < META_BUTTON_TYPE_LAST; ++i)
        button_states[i] = meta_button_state_for_button_type (d, i);

    frame_style = meta_theme_get_frame_style (theme, frame_type, flags);

    gtk_style_context_get_background_color (context, GTK_STATE_FLAG_NORMAL, &bg_rgba);
    bg_rgba.alpha = 1.0;

    if (frame_style->window_background_color)
    {
        meta_color_spec_render (frame_style->window_background_color,
                                context, &bg_rgba);

        bg_rgba.alpha = frame_style->window_background_alpha / 255.0;
    }

    /* Draw something that will be almost invisible to user. This is hacky way
     * to fix invisible decorations. */
    cairo_set_source_rgba (cr, 0, 0, 0, 0.01);
    cairo_rectangle (cr, 0, 0, 1, 1);
    cairo_fill (cr);
    /* ------------ */

    cairo_destroy (cr);

    if (d->frame_window)
        surface = create_surface (fgeom.width, fgeom.height, d->frame->style_window_rgb);
    else
        surface = create_surface (fgeom.width, fgeom.height, d->frame->style_window_rgba);

    cr = cairo_create (surface);
    gdk_cairo_set_source_rgba (cr, &bg_rgba);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    src = XRenderCreatePicture (xdisplay, cairo_xlib_surface_get_drawable (surface),
                                get_format_for_surface (d, surface), 0, NULL);

    screen = gtk_widget_get_screen (d->frame->style_window_rgba);
    style_info = meta_theme_create_style_info (screen, d->gtk_theme_variant);

    cairo_paint (cr);
    meta_theme_draw_frame (theme, style_info, cr, frame_type, flags,
                           fgeom.width - fgeom.borders.total.left - fgeom.borders.total.right,
                           fgeom.height - fgeom.borders.total.top - fgeom.borders.total.bottom,
                           d->layout, d->frame->text_height, &button_layout,
                           button_states, d->icon_pixbuf, NULL);

    meta_style_info_unref (style_info);

    if (fgeom.borders.visible.top)
    {
        top_region = meta_get_top_border_region (&fgeom, fgeom.width);

        decor_blend_border_picture (xdisplay, d->context, src,
                                    fgeom.borders.invisible.left,
                                    fgeom.borders.invisible.top,
                                    d->picture, &d->border_layout,
                                    BORDER_TOP, top_region,
                                    alpha * 0xffff, shade_alpha, 0);
    }

    if (fgeom.borders.visible.bottom)
    {
        bottom_region = meta_get_bottom_border_region (&fgeom, fgeom.width);

        decor_blend_border_picture (xdisplay, d->context, src,
                                    fgeom.borders.invisible.left,
                                    fgeom.height - fgeom.borders.total.bottom,
                                    d->picture, &d->border_layout,
                                    BORDER_BOTTOM, bottom_region,
                                    alpha * 0xffff, shade_alpha, 0);
    }

    if (fgeom.borders.visible.left)
    {
        left_region = meta_get_left_border_region (&fgeom, fgeom.height);

        decor_blend_border_picture (xdisplay, d->context, src,
                                    fgeom.borders.invisible.left,
                                    fgeom.borders.total.top,
                                    d->picture, &d->border_layout,
                                    BORDER_LEFT, left_region,
                                    alpha * 0xffff, shade_alpha, 0);
    }

    if (fgeom.borders.visible.right)
    {
        right_region = meta_get_right_border_region (&fgeom, fgeom.height);

        decor_blend_border_picture (xdisplay, d->context, src,
                                    fgeom.width - fgeom.borders.total.right,
                                    fgeom.borders.total.top,
                                    d->picture, &d->border_layout,
                                    BORDER_RIGHT, right_region,
                                    alpha * 0xffff, shade_alpha, 0);
    }

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    XRenderFreePicture (xdisplay, src);

    copy_to_front_buffer (d);

    if (d->frame_window)
    {
        GdkWindow *gdk_frame_window;
        GdkPixbuf *pixbuf;

        gdk_frame_window = gtk_widget_get_window (d->decor_window);

        pixbuf = gdk_pixbuf_get_from_surface (d->surface, 0, 0, d->width, d->height);
        gtk_image_set_from_pixbuf (GTK_IMAGE (d->decor_image), pixbuf);
        g_object_unref (pixbuf);

        gdk_window_move_resize (gdk_frame_window,
                                d->context->left_corner_space - 1,
                                d->context->top_corner_space - 1,
                                d->width,
                                d->height);

        gdk_window_lower (gdk_frame_window);
    }

    if (d->prop_xid)
    {
        /* translate from frame to client window space */
        if (top_region)
            XOffsetRegion (top_region, -fgeom.borders.total.left, -fgeom.borders.total.top);
        if (bottom_region)
            XOffsetRegion (bottom_region, -fgeom.borders.total.left, 0);
        if (left_region)
            XOffsetRegion (left_region, -fgeom.borders.total.left, 0);

        decor_update_meta_window_property (d, theme, flags, frame_type,
                                           top_region, bottom_region,
                                           left_region, right_region);

        d->prop_xid = 0;
    }

    if (top_region)
        XDestroyRegion (top_region);
    if (bottom_region)
        XDestroyRegion (bottom_region);
    if (left_region)
        XDestroyRegion (left_region);
    if (right_region)
        XDestroyRegion (right_region);
}

static MetaButtonFunction
meta_button_function_from_string (const char *str)
{
    if (strcmp (str, "menu") == 0)
        return META_BUTTON_FUNCTION_MENU;
    else if (strcmp (str, "appmenu") == 0)
        return META_BUTTON_FUNCTION_APPMENU;
    else if (strcmp (str, "minimize") == 0)
        return META_BUTTON_FUNCTION_MINIMIZE;
    else if (strcmp (str, "maximize") == 0)
        return META_BUTTON_FUNCTION_MAXIMIZE;
    else if (strcmp (str, "close") == 0)
        return META_BUTTON_FUNCTION_CLOSE;
    else if (strcmp (str, "shade") == 0)
        return META_BUTTON_FUNCTION_SHADE;
    else if (strcmp (str, "above") == 0)
        return META_BUTTON_FUNCTION_ABOVE;
    else if (strcmp (str, "stick") == 0)
        return META_BUTTON_FUNCTION_STICK;
    else if (strcmp (str, "unshade") == 0)
        return META_BUTTON_FUNCTION_UNSHADE;
    else if (strcmp (str, "unabove") == 0)
        return META_BUTTON_FUNCTION_UNABOVE;
    else if (strcmp (str, "unstick") == 0)
        return META_BUTTON_FUNCTION_UNSTICK;
    else
        return META_BUTTON_FUNCTION_LAST;
}

static MetaButtonFunction
meta_button_opposite_function (MetaButtonFunction ofwhat)
{
    switch (ofwhat)
    {
    case META_BUTTON_FUNCTION_SHADE:
        return META_BUTTON_FUNCTION_UNSHADE;
    case META_BUTTON_FUNCTION_UNSHADE:
        return META_BUTTON_FUNCTION_SHADE;

    case META_BUTTON_FUNCTION_ABOVE:
        return META_BUTTON_FUNCTION_UNABOVE;
    case META_BUTTON_FUNCTION_UNABOVE:
        return META_BUTTON_FUNCTION_ABOVE;

    case META_BUTTON_FUNCTION_STICK:
        return META_BUTTON_FUNCTION_UNSTICK;
    case META_BUTTON_FUNCTION_UNSTICK:
        return META_BUTTON_FUNCTION_STICK;

    default:
        return META_BUTTON_FUNCTION_LAST;
    }
}

static void
meta_initialize_button_layout (MetaButtonLayout *layout)
{
    int	i;

    for (i = 0; i < MAX_BUTTONS_PER_CORNER; ++i)
    {
        layout->left_buttons[i] = META_BUTTON_FUNCTION_LAST;
        layout->right_buttons[i] = META_BUTTON_FUNCTION_LAST;
        layout->left_buttons_has_spacer[i] = FALSE;
        layout->right_buttons_has_spacer[i] = FALSE;
    }
}

void
meta_update_button_layout (const char *value)
{
    MetaButtonLayout new_layout;
    MetaButtonFunction f;
    char **sides;
    int i;

    meta_initialize_button_layout (&new_layout);

    sides = g_strsplit (value, ":", 2);

    if (sides[0] != NULL)
    {
        char **buttons;
        int b;
        gboolean used[META_BUTTON_FUNCTION_LAST];

        for (i = 0; i < META_BUTTON_FUNCTION_LAST; ++i)
            used[i] = FALSE;

        buttons = g_strsplit (sides[0], ",", -1);

        i = b = 0;
        while (buttons[b] != NULL)
        {
            f = meta_button_function_from_string (buttons[b]);
            if (i > 0 && strcmp ("spacer", buttons[b]) == 0)
            {
                new_layout.left_buttons_has_spacer[i - 1] = TRUE;
                f = meta_button_opposite_function (f);

                if (f != META_BUTTON_FUNCTION_LAST)
                    new_layout.left_buttons_has_spacer[i - 2] = TRUE;
            }
            else
            {
                if (f != META_BUTTON_FUNCTION_LAST && !used[f])
                {
                    used[f] = TRUE;
                    new_layout.left_buttons[i++] = f;

                    f = meta_button_opposite_function (f);

                    if (f != META_BUTTON_FUNCTION_LAST)
                        new_layout.left_buttons[i++] = f;

                }
                else
                {
                    fprintf (stderr, "%s: Ignoring unknown or already-used "
                             "button name \"%s\"\n", program_name, buttons[b]);
                }
            }
            ++b;
        }

        new_layout.left_buttons[i] = META_BUTTON_FUNCTION_LAST;

        g_strfreev (buttons);

        if (sides[1] != NULL)
        {
            for (i = 0; i < META_BUTTON_FUNCTION_LAST; ++i)
                used[i] = FALSE;

            buttons = g_strsplit (sides[1], ",", -1);

            i = b = 0;
            while (buttons[b] != NULL)
            {
                f = meta_button_function_from_string (buttons[b]);
                if (i > 0 && strcmp ("spacer", buttons[b]) == 0)
                {
                    new_layout.right_buttons_has_spacer[i - 1] = TRUE;
                    f = meta_button_opposite_function (f);
                    if (f != META_BUTTON_FUNCTION_LAST)
                        new_layout.right_buttons_has_spacer[i - 2] = TRUE;
                }
                else
                {
                    if (f != META_BUTTON_FUNCTION_LAST && !used[f])
                    {
                        used[f] = TRUE;
                        new_layout.right_buttons[i++] = f;

                        f = meta_button_opposite_function (f);

                        if (f != META_BUTTON_FUNCTION_LAST)
                            new_layout.right_buttons[i++] = f;
                    }
                    else
                    {
                        fprintf (stderr, "%s: Ignoring unknown or "
                                 "already-used button name \"%s\"\n",
                                 program_name, buttons[b]);
                    }
                }
                ++b;
            }

            new_layout.right_buttons[i] = META_BUTTON_FUNCTION_LAST;

            g_strfreev (buttons);
        }
    }

    g_strfreev (sides);

    /* Invert the button layout for RTL languages */
    if (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)
    {
        MetaButtonLayout rtl_layout;
        int j;

        meta_initialize_button_layout (&rtl_layout);

        i = 0;
        while (new_layout.left_buttons[i] != META_BUTTON_FUNCTION_LAST)
            ++i;

        for (j = 0; j < i; ++j)
        {
            rtl_layout.right_buttons[j] = new_layout.left_buttons[i - j - 1];
            if (j == 0)
                rtl_layout.right_buttons_has_spacer[i - 1] = new_layout.left_buttons_has_spacer[i - j - 1];
            else
                rtl_layout.right_buttons_has_spacer[j - 1] = new_layout.left_buttons_has_spacer[i - j - 1];
        }

        i = 0;
        while (new_layout.right_buttons[i] != META_BUTTON_FUNCTION_LAST)
            ++i;

        for (j = 0; j < i; ++j)
        {
            rtl_layout.left_buttons[j] = new_layout.right_buttons[i - j - 1];
            if (j == 0)
                rtl_layout.left_buttons_has_spacer[i - 1] = new_layout.right_buttons_has_spacer[i - j - 1];
            else
                rtl_layout.left_buttons_has_spacer[j - 1] = new_layout.right_buttons_has_spacer[i - j - 1];
        }

        new_layout = rtl_layout;
    }

    meta_button_layout = new_layout;
}

#endif
