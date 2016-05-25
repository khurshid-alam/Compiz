/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

/*
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
 */

#include "config.h"
#include "gtk-window-decorator.h"
#include "gwd-cairo-window-decoration-util.h"
#include "gwd-theme-cairo.h"

struct _GWDThemeCairo
{
    GObject parent;
};

G_DEFINE_TYPE (GWDThemeCairo, gwd_theme_cairo, GWD_TYPE_THEME)

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
gwd_theme_cairo_update_border_extents (GWDTheme      *theme,
                                       decor_frame_t *frame)
{
    frame = gwd_decor_frame_ref (frame);

    gwd_cairo_window_decoration_get_extents (&frame->win_extents,
                                             &frame->max_win_extents);

    frame->titlebar_height = frame->max_titlebar_height =
        (frame->text_height < 17) ? 17 : frame->text_height;

    gwd_decor_frame_unref (frame);
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
    if (i > BUTTON_MENU)
        return FALSE;

    if (decor->frame_window) {
        *x = bpos[i].x + bpos[i].xw * width + decor->frame->win_extents.left + 4;
        *y = bpos[i].y + bpos[i].yh * height + bpos[i].yth *
             (decor->frame->titlebar_height - 17) + decor->frame->win_extents.top + 2;
    } else {
        *x = bpos[i].x + bpos[i].xw * width;
        *y = bpos[i].y + bpos[i].yh * height + bpos[i].yth *
             (decor->frame->titlebar_height - 17);
    }

    *w = bpos[i].w + bpos[i].ww * width;
    *h = bpos[i].h + bpos[i].hh * height + bpos[i].hth +
         (decor->frame->titlebar_height - 17);

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
    GWDThemeClass *theme_class = GWD_THEME_CLASS (cairo_class);

    theme_class->update_border_extents = gwd_theme_cairo_update_border_extents;
    theme_class->get_button_position = gwd_theme_cairo_get_button_position;
}

static void
gwd_theme_cairo_init (GWDThemeCairo *cairo)
{
}
