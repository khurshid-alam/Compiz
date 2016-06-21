/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

/*
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2010 Sam Spilsbury
 * Copyright (C) 2011 Canonical Ltd.
 * Copyright (C) 2016 Alberts Muktupāvels
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Alberts Muktupāvels <alberts.muktupavels@gmail.com>
 *     David Reveman <davidr@novell.com>
 *     Sam Spilsbury <smspillaz@gmail.com>
 */

#include "config.h"
#include "gtk-window-decorator.h"
#include "gwd-theme-cairo.h"

#define STROKE_ALPHA 0.6f

struct _GWDThemeCairo
{
    GObject       parent;

    decor_color_t title_color[2];
};

G_DEFINE_TYPE (GWDThemeCairo, gwd_theme_cairo, GWD_TYPE_THEME)

static gint
get_titlebar_height (decor_frame_t *frame)
{
    return frame->text_height < 17 ? 17 : frame->text_height;
}

static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b)
{
    gdouble red = *r;
    gdouble green = *g;
    gdouble blue = *b;
    gdouble min;
    gdouble max;
    gdouble h, l, s;
    gdouble delta;

    if (red > green) {
        if (red > blue)
            max = red;
        else
            max = blue;

        if (green < blue)
            min = green;
        else
            min = blue;
    } else {
        if (green > blue)
            max = green;
        else
            max = blue;

        if (red < blue)
            min = red;
        else
            min = blue;
    }

    l = (max + min) / 2;
    s = 0;
    h = 0;

    if (max != min) {
        if (l <= 0.5)
            s = (max - min) / (max + min);
        else
            s = (max - min) / (2 - max - min);

        delta = max -min;
        if (red == max)
            h = (green - blue) / delta;
        else if (green == max)
            h = 2 + (blue - red) / delta;
        else if (blue == max)
            h = 4 + (red - green) / delta;

        h *= 60;
        if (h < 0.0)
            h += 360;
    }

    *r = h;
    *g = l;
    *b = s;
}

static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s)
{
    gdouble lightness = *l;
    gdouble saturation = *s;
    gdouble hue;
    gdouble m1, m2;
    gdouble r, g, b;

    if (lightness <= 0.5)
        m2 = lightness * (1 + saturation);
    else
        m2 = lightness + saturation - lightness * saturation;

    m1 = 2 * lightness - m2;

    if (saturation == 0) {
        *h = lightness;
        *l = lightness;
        *s = lightness;
    } else {
        hue = *h + 120;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            r = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            r = m2;
        else if (hue < 240)
            r = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            r = m1;

        hue = *h;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            g = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            g = m2;
        else if (hue < 240)
            g = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            g = m1;

        hue = *h - 120;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            b = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            b = m2;
        else if (hue < 240)
            b = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            b = m1;

        *h = r;
        *l = g;
        *s = b;
    }
}

static void
shade (const decor_color_t *a,
       decor_color_t       *b,
       gfloat               k)
{
    gdouble red = a->r;
    gdouble green = a->g;
    gdouble blue = a->b;

    rgb_to_hls (&red, &green, &blue);

    green *= k;
    if (green > 1.0)
        green = 1.0;
    else if (green < 0.0)
        green = 0.0;

    blue *= k;
    if (blue > 1.0)
        blue = 1.0;
    else if (blue < 0.0)
        blue = 0.0;

    hls_to_rgb (&red, &green, &blue);

    b->r = red;
    b->g = green;
    b->b = blue;
}

static void
update_title_colors (GWDThemeCairo *cairo)
{
    GWDTheme *theme = GWD_THEME (cairo);
    GtkWidget *style_window = gwd_theme_get_style_window (theme);
    GtkStyleContext *context = gtk_widget_get_style_context (style_window);
    GdkRGBA bg;
    decor_color_t spot_color;

    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_SELECTED);
    gtk_style_context_get_background_color (context, GTK_STATE_FLAG_SELECTED, &bg);
    gtk_style_context_restore (context);

    spot_color.r = bg.red;
    spot_color.g = bg.green;
    spot_color.b = bg.blue;

    shade (&spot_color, &cairo->title_color[0], 1.05);
    shade (&cairo->title_color[0], &cairo->title_color[1], 0.85);
}

static void
decor_update_window_property (decor_t *d)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display *xdisplay = gdk_x11_display_get_xdisplay (display);
    decor_extents_t extents = d->frame->win_extents;
    unsigned int nOffset = 1;
    unsigned int frame_type = populate_frame_type (d);
    unsigned int frame_state = populate_frame_state (d);
    unsigned int frame_actions = populate_frame_actions (d);
    long *data;
    gint nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    int w, h;
    gint stretch_offset;
    REGION top, bottom, left, right;

    w = d->border_layout.top.x2 - d->border_layout.top.x1 -
	d->context->left_space - d->context->right_space;

    if (d->border_layout.rotation)
        h = d->border_layout.left.x2 - d->border_layout.left.x1;
    else
        h = d->border_layout.left.y2 - d->border_layout.left.y1;

    stretch_offset = w - d->button_width - 1;

    nQuad = decor_set_lSrStXbS_window_quads (quads, d->context,
                                             &d->border_layout,
                                             stretch_offset);

    data = decor_alloc_property (nOffset, WINDOW_DECORATION_TYPE_PIXMAP);
    decor_quads_to_property (data, nOffset - 1, cairo_xlib_surface_get_drawable (d->surface),
                             &extents, &extents,
                             &extents, &extents,
                             ICON_SPACE + d->button_width,
                             0,
                             quads, nQuad, frame_type, frame_state, frame_actions);

    gdk_error_trap_push ();
    XChangeProperty (xdisplay, d->prop_xid, win_decor_atom, XA_INTEGER,
                     32, PropModeReplace, (guchar *) data,
                     PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);
    gdk_display_sync (display);
    gdk_error_trap_pop_ignored ();

    top.rects = &top.extents;
    top.numRects = top.size = 1;

    top.extents.x1 = -extents.left;
    top.extents.y1 = -extents.top;
    top.extents.x2 = w + extents.right;
    top.extents.y2 = 0;

    bottom.rects = &bottom.extents;
    bottom.numRects = bottom.size = 1;

    bottom.extents.x1 = -extents.left;
    bottom.extents.y1 = 0;
    bottom.extents.x2 = w + extents.right;
    bottom.extents.y2 = extents.bottom;

    left.rects = &left.extents;
    left.numRects = left.size = 1;

    left.extents.x1 = -extents.left;
    left.extents.y1 = 0;
    left.extents.x2 = 0;
    left.extents.y2 = h;

    right.rects = &right.extents;
    right.numRects = right.size = 1;

    right.extents.x1 = 0;
    right.extents.y1 = 0;
    right.extents.x2 = extents.right;
    right.extents.y2 = h;

    decor_update_blur_property (d, w, h, &top, stretch_offset,
                                &bottom, w / 2, &left, h / 2, &right, h / 2);

    free (data);
}

static void
button_state_offsets (gdouble  x,
                      gdouble  y,
                      guint    state,
                      gdouble *return_x,
                      gdouble *return_y)
{
    static gdouble off[] = { 0.0, 0.0, 0.0, 0.5 };

    *return_x  = x + off[state];
    *return_y  = y + off[state];
}

static void
button_state_paint (cairo_t         *cr,
                    GtkStyleContext *context,
                    decor_color_t   *color,
                    guint            state)
{
    GdkRGBA fg;

#define IN_STATE (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW)

    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_restore (context);

    fg.alpha = STROKE_ALPHA;

    if ((state & IN_STATE) == IN_STATE) {
        if (state & IN_EVENT_WINDOW)
            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        else
            cairo_set_source_rgba (cr, color->r, color->g, color->b, 0.95);

        cairo_fill_preserve (cr);

        gdk_cairo_set_source_rgba (cr, &fg);

        cairo_set_line_width (cr, 1.0);
        cairo_stroke (cr);
        cairo_set_line_width (cr, 2.0);
    } else {
        gdk_cairo_set_source_rgba (cr, &fg);
        cairo_stroke_preserve (cr);

        if (state & IN_EVENT_WINDOW)
            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        else
            cairo_set_source_rgba (cr, color->r, color->g, color->b, 0.95);

        cairo_fill (cr);
    }
}

static void
draw_close_button (cairo_t *cr,
                   gdouble  s)
{
    cairo_rel_move_to (cr, 0.0, s);

    cairo_rel_line_to (cr, s, -s);
    cairo_rel_line_to (cr, s, s);
    cairo_rel_line_to (cr, s, -s);
    cairo_rel_line_to (cr, s, s);

    cairo_rel_line_to (cr, -s, s);
    cairo_rel_line_to (cr, s, s);
    cairo_rel_line_to (cr, -s, s);
    cairo_rel_line_to (cr, -s, -s);

    cairo_rel_line_to (cr, -s, s);
    cairo_rel_line_to (cr, -s, -s);
    cairo_rel_line_to (cr, s, -s);

    cairo_close_path (cr);
}

static void
draw_max_button (cairo_t *cr,
                 gdouble  s)
{
    cairo_rel_line_to (cr, 12.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 12.0);
    cairo_rel_line_to (cr, -12.0, 0.0);

    cairo_close_path (cr);

    cairo_rel_move_to (cr, 2.0, s);

    cairo_rel_line_to (cr, 12.0 - 4.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 12.0 - s - 2.0);
    cairo_rel_line_to (cr, -(12.0 - 4.0), 0.0);

    cairo_close_path (cr);
}

static void
draw_unmax_button (cairo_t *cr,
                   gdouble  s)
{
    cairo_rel_move_to (cr, 1.0, 1.0);

    cairo_rel_line_to (cr, 10.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 10.0);
    cairo_rel_line_to (cr, -10.0, 0.0);

    cairo_close_path (cr);

    cairo_rel_move_to (cr, 2.0, s);

    cairo_rel_line_to (cr, 10.0 - 4.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 10.0 - s - 2.0);
    cairo_rel_line_to (cr, -(10.0 - 4.0), 0.0);

    cairo_close_path (cr);
}

static void
draw_min_button (cairo_t *cr,
                 gdouble  s)
{
    cairo_rel_move_to (cr, 0.0, 8.0);

    cairo_rel_line_to (cr, 12.0, 0.0);
    cairo_rel_line_to (cr, 0.0, s);
    cairo_rel_line_to (cr, -12.0, 0.0);

    cairo_close_path (cr);
}

static void
calc_button_size (decor_t *decor)
{
    gint button_width = 0;

    if (decor->actions & WNCK_WINDOW_ACTION_CLOSE)
        button_width += 17;

    if (decor->actions & (WNCK_WINDOW_ACTION_MAXIMIZE_HORIZONTALLY |
                          WNCK_WINDOW_ACTION_MAXIMIZE_VERTICALLY |
                          WNCK_WINDOW_ACTION_UNMAXIMIZE_HORIZONTALLY |
                          WNCK_WINDOW_ACTION_UNMAXIMIZE_VERTICALLY))
        button_width += 17;

    if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE)
        button_width += 17;

    if (button_width)
        ++button_width;

    decor->button_width = button_width;
}

static gboolean
button_present (decor_t *decor,
                gint     i)
{
    switch (i) {
        case BUTTON_MIN:
            if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE)
                return TRUE;
            break;

        case BUTTON_MAX:
            if (decor->actions & WNCK_WINDOW_ACTION_MAXIMIZE)
                return TRUE;
            break;

        case BUTTON_CLOSE:
            if (decor->actions & WNCK_WINDOW_ACTION_CLOSE)
                return TRUE;
            break;

        case BUTTON_MENU:
        case BUTTON_SHADE:
        case BUTTON_ABOVE:
        case BUTTON_STICK:
        case BUTTON_UNSHADE:
        case BUTTON_UNABOVE:
        case BUTTON_UNSTICK:
            break;

        default:
            break;
    }

    return FALSE;
}

static void
gwd_theme_cairo_constructed (GObject *object)
{
    GWDThemeCairo *cairo = GWD_THEME_CAIRO (object);

    G_OBJECT_CLASS (gwd_theme_cairo_parent_class)->constructed (object);

    update_title_colors (cairo);
}

static void
gwd_theme_cairo_style_updated (GWDTheme *theme)
{
    GWDThemeCairo *cairo = GWD_THEME_CAIRO (theme);

    update_title_colors (cairo);
}

static void
gwd_theme_cairo_draw_window_decoration (GWDTheme *theme,
                                        decor_t  *decor)
{
    GWDThemeCairo *cairo = GWD_THEME_CAIRO (theme);
    GtkWidget *style_window = gwd_theme_get_style_window (theme);
    GtkStyleContext *context = gtk_widget_get_style_context (style_window);
    cairo_t *cr;
    GdkRGBA bg, fg;
    cairo_surface_t *surface;
    decor_color_t color;
    gdouble alpha;
    gdouble x1, y1, x2, y2, x, y, h;
    gint corners = SHADE_LEFT | SHADE_RIGHT | SHADE_TOP | SHADE_BOTTOM;
    gint top;
    gint button_x;
    gint titlebar_height;

    if (!decor->surface)
        return;

    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_get_background_color (context, GTK_STATE_FLAG_NORMAL, &bg);
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_restore (context);

    if (decor->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                        WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
        corners = 0;

    color.r = bg.red;
    color.g = bg.green;
    color.b = bg.blue;

    if (decor->buffer_surface)
        surface = decor->buffer_surface;
    else
        surface = decor->surface;

    cr = cairo_create (surface);

    if (!cr)
        return;

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    top = decor->frame->win_extents.top;

    x1 = decor->context->left_space - decor->frame->win_extents.left;
    y1 = decor->context->top_space - decor->frame->win_extents.top;
    x2 = decor->width - decor->context->right_space + decor->frame->win_extents.right;
    y2 = decor->height - decor->context->bottom_space + decor->frame->win_extents.bottom;

    h = decor->height - decor->context->top_space - decor->context->bottom_space;

    cairo_set_line_width (cr, 1.0);

    draw_shadow_background (decor, cr, decor->shadow, decor->context);

    if (decor->active) {
        decor_color_t *title_color = cairo->title_color;

        alpha = decoration_alpha + 0.3;

        fill_rounded_rectangle (cr,
                                x1 + 0.5,
                                y1 + 0.5,
                                decor->frame->win_extents.left - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPLEFT & corners,
                                &title_color[0], 1.0, &title_color[1], alpha,
                                SHADE_TOP | SHADE_LEFT);

        fill_rounded_rectangle (cr,
                                x1 + decor->frame->win_extents.left,
                                y1 + 0.5,
                                x2 - x1 - decor->frame->win_extents.left -
                                decor->frame->win_extents.right,
                                top - 0.5,
                                5.0, 0,
                                &title_color[0], 1.0, &title_color[1], alpha,
                                SHADE_TOP);

        fill_rounded_rectangle (cr,
                                x2 - decor->frame->win_extents.right,
                                y1 + 0.5,
                                decor->frame->win_extents.right - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPRIGHT & corners,
                                &title_color[0], 1.0, &title_color[1], alpha,
                                SHADE_TOP | SHADE_RIGHT);
    } else {
        alpha = decoration_alpha;

        fill_rounded_rectangle (cr,
                                x1 + 0.5,
                                y1 + 0.5,
                                decor->frame->win_extents.left - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPLEFT & corners,
                                &color, 1.0, &color, alpha,
                                SHADE_TOP | SHADE_LEFT);

        fill_rounded_rectangle (cr,
                                x1 + decor->frame->win_extents.left,
                                y1 + 0.5,
                                x2 - x1 - decor->frame->win_extents.left -
                                decor->frame->win_extents.right,
                                top - 0.5,
                                5.0, 0,
                                &color, 1.0, &color, alpha,
                                SHADE_TOP);

        fill_rounded_rectangle (cr,
                                x2 - decor->frame->win_extents.right,
                                y1 + 0.5,
                                decor->frame->win_extents.right - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPRIGHT & corners,
                                &color, 1.0, &color, alpha,
                                SHADE_TOP | SHADE_RIGHT);
    }

    fill_rounded_rectangle (cr,
                            x1 + 0.5,
                            y1 + top,
                            decor->frame->win_extents.left - 0.5,
                            h,
                            5.0, 0,
                            &color, 1.0, &color, alpha,
                            SHADE_LEFT);

    fill_rounded_rectangle (cr,
                            x2 - decor->frame->win_extents.right,
                            y1 + top,
                            decor->frame->win_extents.right - 0.5,
                            h,
                            5.0, 0,
                            &color, 1.0, &color, alpha,
                            SHADE_RIGHT);


    fill_rounded_rectangle (cr,
                            x1 + 0.5,
                            y2 - decor->frame->win_extents.bottom,
                            decor->frame->win_extents.left - 0.5,
                            decor->frame->win_extents.bottom - 0.5,
                            5.0, CORNER_BOTTOMLEFT & corners,
                            &color, 1.0, &color, alpha,
                            SHADE_BOTTOM | SHADE_LEFT);

    fill_rounded_rectangle (cr,
                            x1 + decor->frame->win_extents.left,
                            y2 - decor->frame->win_extents.bottom,
                            x2 - x1 - decor->frame->win_extents.left -
                            decor->frame->win_extents.right,
                            decor->frame->win_extents.bottom - 0.5,
                            5.0, 0,
                            &color, 1.0, &color, alpha,
                            SHADE_BOTTOM);

    fill_rounded_rectangle (cr,
                            x2 - decor->frame->win_extents.right,
                            y2 - decor->frame->win_extents.bottom,
                            decor->frame->win_extents.right - 0.5,
                            decor->frame->win_extents.bottom - 0.5,
                            5.0, CORNER_BOTTOMRIGHT & corners,
                            &color, 1.0, &color, alpha,
                            SHADE_BOTTOM | SHADE_RIGHT);

    cairo_rectangle (cr,
                     decor->context->left_space,
                     decor->context->top_space,
                     decor->width - decor->context->left_space -
                     decor->context->right_space,
                     h);
    gdk_cairo_set_source_rgba (cr, &bg);
    cairo_fill (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    if (decor->active) {
        fg.alpha = 0.7;
        gdk_cairo_set_source_rgba (cr, &fg);

        cairo_move_to (cr, x1 + 0.5, y1 + top - 0.5);
        cairo_rel_line_to (cr, x2 - x1 - 1.0, 0.0);

        cairo_stroke (cr);
    }

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    cairo_clip (cr);

    cairo_translate (cr, 1.0, 1.0);

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.4);

    cairo_stroke (cr);

    cairo_translate (cr, -2.0, -2.0);

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.1);

    cairo_stroke (cr);

    cairo_translate (cr, 1.0, 1.0);

    cairo_reset_clip (cr);

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    fg.alpha = alpha;
    gdk_cairo_set_source_rgba (cr, &fg);

    cairo_stroke (cr);

    cairo_set_line_width (cr, 2.0);

    button_x = decor->width - decor->context->right_space - 13;
    titlebar_height = get_titlebar_height (decor->frame);

    if (decor->actions & WNCK_WINDOW_ACTION_CLOSE) {
        button_state_offsets (button_x,
                              y1 - 3.0 + titlebar_height / 2,
                              decor->button_states[BUTTON_CLOSE], &x, &y);

        button_x -= 17;

        if (decor->active) {
            cairo_move_to (cr, x, y);
            draw_close_button (cr, 3.0);
            button_state_paint (cr, context, &color,
                                decor->button_states[BUTTON_CLOSE]);
        } else {
            fg.alpha = alpha * 0.75;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);
            draw_close_button (cr, 3.0);
            cairo_fill (cr);
        }
    }

    if (decor->actions & WNCK_WINDOW_ACTION_MAXIMIZE) {
        button_state_offsets (button_x,
                              y1 - 3.0 + titlebar_height / 2,
                              decor->button_states[BUTTON_MAX], &x, &y);

        button_x -= 17;

        cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

        if (decor->active) {
            fg.alpha = STROKE_ALPHA;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);

            if (decor->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                                WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
                draw_unmax_button (cr, 4.0);
            else
                draw_max_button (cr, 4.0);

            button_state_paint (cr, context, &color,
                                decor->button_states[BUTTON_MAX]);
        } else {
            fg.alpha = alpha * 0.75;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);

            if (decor->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                                WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
                draw_unmax_button (cr, 4.0);
            else
                draw_max_button (cr, 4.0);

            cairo_fill (cr);
        }
    }

    if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE) {
        button_state_offsets (button_x,
                              y1 - 3.0 + titlebar_height / 2,
                              decor->button_states[BUTTON_MIN], &x, &y);

        button_x -= 17;

        if (decor->active) {
            fg.alpha = STROKE_ALPHA;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);
            draw_min_button (cr, 4.0);
            button_state_paint (cr, context, &color,
                                decor->button_states[BUTTON_MIN]);
        } else {
            fg.alpha = alpha * 0.75;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);
            draw_min_button (cr, 4.0);
            cairo_fill (cr);
        }
    }

    if (decor->layout) {
        if (decor->active) {
            cairo_move_to (cr,
                           decor->context->left_space + 21.0,
                           y1 + 2.0 + (titlebar_height - decor->frame->text_height) / 2.0);

            fg.alpha = STROKE_ALPHA;
            gdk_cairo_set_source_rgba (cr, &fg);

            pango_cairo_layout_path (cr, decor->layout);
            cairo_stroke (cr);

            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        } else {
            fg.alpha = alpha;
            gdk_cairo_set_source_rgba (cr, &fg);
        }

        cairo_move_to (cr,
                       decor->context->left_space + 21.0,
                        y1 + 2.0 + (titlebar_height - decor->frame->text_height) / 2.0);

        pango_cairo_show_layout (cr, decor->layout);
    }

    if (decor->icon) {
        cairo_translate (cr,
                         decor->context->left_space + 1,
                         y1 - 5.0 + titlebar_height / 2);
        cairo_set_source (cr, decor->icon);
        cairo_rectangle (cr, 0.0, 0.0, 16.0, 16.0);
        cairo_clip (cr);

        if (decor->active)
            cairo_paint (cr);
        else
            cairo_paint_with_alpha (cr, alpha);
    }

    cairo_destroy (cr);

    copy_to_front_buffer (decor);

    if (decor->prop_xid) {
        decor_update_window_property (decor);
        decor->prop_xid = 0;
    }
}

static gboolean
gwd_theme_cairo_calc_decoration_size (GWDTheme *theme,
                                      decor_t  *decor,
                                      gint      w,
                                      gint      h,
                                      gint      name_width,
                                      gint     *width,
                                      gint     *height)
{
    decor_layout_t layout;
    gint top_width;

    if (!decor->decorated)
        return FALSE;

    /* To avoid wasting texture memory, we only calculate the minimal
     * required decoration size then clip and stretch the texture where
     * appropriate
     */

    calc_button_size (decor);

    if (w < ICON_SPACE + decor->button_width)
        return FALSE;

    top_width = name_width + decor->button_width + ICON_SPACE;
    if (w < top_width)
        top_width = MAX (ICON_SPACE + decor->button_width, w);

    if (decor->active)
        decor_get_default_layout (&decor->frame->window_context_active, top_width, 1, &layout);
    else
        decor_get_default_layout (&decor->frame->window_context_inactive, top_width, 1, &layout);

    if (!decor->context || memcmp (&layout, &decor->border_layout, sizeof (layout))) {
        *width  = layout.width;
        *height = layout.height;

        decor->border_layout = layout;
        if (decor->active) {
            decor->context = &decor->frame->window_context_active;
            decor->shadow = decor->frame->border_shadow_active;
        } else {
            decor->context = &decor->frame->window_context_inactive;
            decor->shadow = decor->frame->border_shadow_inactive;
        }

        return TRUE;
    }

    return FALSE;
}

static void
gwd_theme_cairo_update_border_extents (GWDTheme      *theme,
                                       decor_frame_t *frame)
{
    decor_extents_t win_extents = { 6, 6, 10, 6 };
    decor_extents_t max_win_extents = { 6, 6, 4, 6 };
    gint titlebar_height = get_titlebar_height (frame);

    frame = gwd_decor_frame_ref (frame);

    win_extents.top += titlebar_height;
    max_win_extents.top += titlebar_height;

    frame->win_extents = win_extents;
    frame->max_win_extents = max_win_extents;

    gwd_decor_frame_unref (frame);
}

static void
gwd_theme_cairo_get_event_window_position (GWDTheme *theme,
                                           decor_t  *decor,
                                           gint      i,
                                           gint      j,
                                           gint      width,
                                           gint      height,
                                           gint     *x,
                                           gint     *y,
                                           gint     *w,
                                           gint     *h)
{
    gint titlebar_height = get_titlebar_height (decor->frame);

    *x = pos[i][j].x + pos[i][j].xw * width;
    *y = pos[i][j].y +
         pos[i][j].yh * height + pos[i][j].yth * (titlebar_height - 17);

    if ((decor->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY) && (j == 0 || j == 2)) {
        *w = 0;
    } else {
        *w = pos[i][j].w + pos[i][j].ww * width;
    }

    if ((decor->state & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY) && (i == 0 || i == 2)) {
        *h = 0;
    } else {
        *h = pos[i][j].h +
             pos[i][j].hh * height + pos[i][j].hth * (titlebar_height - 17);
    }
}

static gboolean
gwd_theme_cairo_get_button_position (GWDTheme *theme,
                                     decor_t  *decor,
                                     gint      i,
                                     gint      width,
                                     gint      height,
                                     gint     *x,
                                     gint     *y,
                                     gint     *w,
                                     gint     *h)
{
    gint titlebar_height = get_titlebar_height (decor->frame);

    if (i > BUTTON_MENU)
        return FALSE;

    *x = bpos[i].x + bpos[i].xw * width;
    *y = bpos[i].y + bpos[i].yh * height + bpos[i].yth * (titlebar_height - 17);

    *w = bpos[i].w + bpos[i].ww * width;
    *h = bpos[i].h + bpos[i].hh * height + bpos[i].hth + (titlebar_height - 17);

    /* hack to position multiple buttons on the right */
    if (i != BUTTON_MENU) {
        gint position = 0;
        gint button = 0;

        while (button != i) {
            if (button_present (decor, button))
                position++;
            button++;
        }

        *x -= 10 + 16 * position;
    }

    return TRUE;
}

static void
gwd_theme_cairo_class_init (GWDThemeCairoClass *cairo_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (cairo_class);
    GWDThemeClass *theme_class = GWD_THEME_CLASS (cairo_class);

    object_class->constructed = gwd_theme_cairo_constructed;

    theme_class->style_updated = gwd_theme_cairo_style_updated;
    theme_class->draw_window_decoration = gwd_theme_cairo_draw_window_decoration;
    theme_class->calc_decoration_size = gwd_theme_cairo_calc_decoration_size;
    theme_class->update_border_extents = gwd_theme_cairo_update_border_extents;
    theme_class->get_event_window_position = gwd_theme_cairo_get_event_window_position;
    theme_class->get_button_position = gwd_theme_cairo_get_button_position;
}

static void
gwd_theme_cairo_init (GWDThemeCairo *cairo)
{
}
