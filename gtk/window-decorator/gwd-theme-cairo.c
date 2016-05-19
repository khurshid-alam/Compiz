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

    if (!decor->frame_window) {
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
    } else {
        calc_button_size (decor);

        /* _default_win_extents + top height */

        top_width = name_width + decor->button_width + ICON_SPACE;
        if (w < top_width)
            top_width = MAX (ICON_SPACE + decor->button_width, w);

        decor_get_default_layout (&decor->frame->window_context_no_shadow,
                                  decor->client_width, decor->client_height,
                                  &layout);

        *width = layout.width;
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
    gint height;

    frame = gwd_decor_frame_ref (frame);

    gwd_cairo_window_decoration_get_extents (&frame->win_extents,
                                             &frame->max_win_extents);

    height = (frame->text_height < 17) ? 17 : frame->text_height;
    frame->titlebar_height = frame->max_titlebar_height = height;

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
    if (decor->frame_window) {
        *x = pos[i][j].x + pos[i][j].xw * width + decor->frame->win_extents.left;
        *y = pos[i][j].y + decor->frame->win_extents.top +
             pos[i][j].yh * height + pos[i][j].yth * (decor->frame->titlebar_height - 17);

        if (i == 0 && (j == 0 || j == 2))
            *y -= decor->frame->titlebar_height;
    } else {
        *x = pos[i][j].x + pos[i][j].xw * width;
        *y = pos[i][j].y +
             pos[i][j].yh * height + pos[i][j].yth * (decor->frame->titlebar_height - 17);
    }

    if ((decor->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY) && (j == 0 || j == 2)) {
        *w = 0;
    } else {
        *w = pos[i][j].w + pos[i][j].ww * width;
    }

    if ((decor->state & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY) && (i == 0 || i == 2)) {
        *h = 0;
    } else {
        *h = pos[i][j].h +
             pos[i][j].hh * height + pos[i][j].hth * (decor->frame->titlebar_height - 17);
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
    GWDThemeClass *theme_class;

    theme_class = GWD_THEME_CLASS (cairo_class);

    theme_class->calc_decoration_size = gwd_theme_cairo_calc_decoration_size;
    theme_class->update_border_extents = gwd_theme_cairo_update_border_extents;
    theme_class->get_event_window_position = gwd_theme_cairo_get_event_window_position;
    theme_class->get_button_position = gwd_theme_cairo_get_button_position;
}

static void
gwd_theme_cairo_init (GWDThemeCairo *cairo)
{
}
