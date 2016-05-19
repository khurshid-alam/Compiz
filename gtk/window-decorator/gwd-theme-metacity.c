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
#include "gwd-theme-metacity.h"

struct _GWDThemeMetacity
{
    GObject    parent;

    MetaTheme *theme;
};

G_DEFINE_TYPE (GWDThemeMetacity, gwd_theme_metacity, GWD_TYPE_THEME)

static gboolean
button_present (MetaButtonLayout   *button_layout,
                MetaButtonFunction  function)
{
    int i;

    for (i = 0; i < MAX_BUTTONS_PER_CORNER; ++i)
        if (button_layout->left_buttons[i] == function)
            return TRUE;

    for (i = 0; i < MAX_BUTTONS_PER_CORNER; ++i)
        if (button_layout->right_buttons[i] == function)
            return TRUE;

    return FALSE;
}

static MetaButtonFunction
button_to_meta_button_function (gint i)
{
    switch (i) {
        case BUTTON_MENU:
            return META_BUTTON_FUNCTION_MENU;
        case BUTTON_MIN:
            return META_BUTTON_FUNCTION_MINIMIZE;
        case BUTTON_MAX:
            return META_BUTTON_FUNCTION_MAXIMIZE;
        case BUTTON_CLOSE:
            return META_BUTTON_FUNCTION_CLOSE;
        case BUTTON_SHADE:
            return META_BUTTON_FUNCTION_SHADE;
        case BUTTON_ABOVE:
            return META_BUTTON_FUNCTION_ABOVE;
        case BUTTON_STICK:
            return META_BUTTON_FUNCTION_STICK;
        case BUTTON_UNSHADE:
            return META_BUTTON_FUNCTION_UNSHADE;
        case BUTTON_UNABOVE:
            return META_BUTTON_FUNCTION_UNABOVE;
        case BUTTON_UNSTICK:
            return META_BUTTON_FUNCTION_UNSTICK;
        default:
            break;
    }

    return META_BUTTON_FUNCTION_LAST;
}

void
gwd_theme_metacity_get_decoration_geometry (GWDThemeMetacity  *metacity,
                                            decor_t           *decor,
                                            MetaFrameFlags    *flags,
                                            MetaFrameGeometry *fgeom,
                                            MetaButtonLayout  *button_layout,
                                            MetaFrameType      frame_type)
{
    GdkScreen *screen;
    MetaStyleInfo *style_info;
    gint client_width;
    gint client_height;

    if (!(frame_type < META_FRAME_TYPE_LAST))
        frame_type = META_FRAME_TYPE_NORMAL;

    if (meta_button_layout_set) {
        *button_layout = meta_button_layout;
    } else {
        gint i;

        button_layout->left_buttons[0] = META_BUTTON_FUNCTION_MENU;

        for (i = 1; i < MAX_BUTTONS_PER_CORNER; ++i)
            button_layout->left_buttons[i] = META_BUTTON_FUNCTION_LAST;

        button_layout->right_buttons[0] = META_BUTTON_FUNCTION_MINIMIZE;
        button_layout->right_buttons[1] = META_BUTTON_FUNCTION_MAXIMIZE;
        button_layout->right_buttons[2] = META_BUTTON_FUNCTION_CLOSE;

        for (i = 3; i < MAX_BUTTONS_PER_CORNER; ++i)
            button_layout->right_buttons[i] = META_BUTTON_FUNCTION_LAST;
    }

    *flags = 0;

    if (decor->actions & WNCK_WINDOW_ACTION_CLOSE)
        *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_DELETE;

    if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE)
        *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_MINIMIZE;

    if (decor->actions & WNCK_WINDOW_ACTION_MAXIMIZE)
        *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_MAXIMIZE;

    *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_MENU;

    if (decor->actions & WNCK_WINDOW_ACTION_RESIZE) {
        if (!(decor->state & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
            *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_VERTICAL_RESIZE;

        if (!(decor->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY))
            *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_HORIZONTAL_RESIZE;
    }

    if (decor->actions & WNCK_WINDOW_ACTION_MOVE)
        *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_MOVE;

    if (decor->actions & WNCK_WINDOW_ACTION_MAXIMIZE)
        *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_MAXIMIZE;

    if (decor->actions & WNCK_WINDOW_ACTION_SHADE)
        *flags |= (MetaFrameFlags ) META_FRAME_ALLOWS_SHADE;

    if (decor->active)
        *flags |= (MetaFrameFlags ) META_FRAME_HAS_FOCUS;

    if ((decor->state & META_MAXIMIZED) == META_MAXIMIZED)
        *flags |= (MetaFrameFlags ) META_FRAME_MAXIMIZED;

    if (decor->state & WNCK_WINDOW_STATE_STICKY)
        *flags |= (MetaFrameFlags ) META_FRAME_STUCK;

    if (decor->state & WNCK_WINDOW_STATE_FULLSCREEN)
        *flags |= (MetaFrameFlags ) META_FRAME_FULLSCREEN;

    if (decor->state & WNCK_WINDOW_STATE_SHADED)
        *flags |= (MetaFrameFlags ) META_FRAME_SHADED;

    if (decor->state & WNCK_WINDOW_STATE_ABOVE)
        *flags |= (MetaFrameFlags ) META_FRAME_ABOVE;

    client_width = decor->border_layout.top.x2 - decor->border_layout.top.x1;
    client_width -= decor->context->right_space + decor->context->left_space;

    if (decor->border_layout.rotation)
        client_height = decor->border_layout.left.x2 - decor->border_layout.left.x1;
    else
        client_height = decor->border_layout.left.y2 - decor->border_layout.left.y1;

    screen = gtk_widget_get_screen (decor->frame->style_window_rgba);
    style_info = meta_theme_create_style_info (screen, decor->gtk_theme_variant);

    meta_theme_calc_geometry (metacity->theme, style_info, frame_type,
                              decor->frame->text_height, *flags, client_width,
                              client_height, button_layout, fgeom);

    meta_style_info_unref (style_info);
}

static GObject *
gwd_theme_metacity_constructor (GType                  type,
                                guint                  n_properties,
                                GObjectConstructParam *properties)
{
    GObject *object;
    GWDThemeMetacity *metacity;

    object = G_OBJECT_CLASS (gwd_theme_metacity_parent_class)->constructor (type, n_properties, properties);
    metacity = GWD_THEME_METACITY (object);

    metacity->theme = meta_theme_get_current ();

    return object;
}

static void
gwd_theme_metacity_update_border_extents (GWDTheme      *theme,
                                          decor_frame_t *frame)
{
    GWDThemeMetacity *metacity;
    GdkScreen *screen;
    MetaStyleInfo *style_info;
    MetaFrameBorders borders;
    MetaFrameType frame_type;

    metacity = GWD_THEME_METACITY (theme);

    gwd_decor_frame_ref (frame);

    frame_type = meta_frame_type_from_string (frame->type);
    if (!(frame_type < META_FRAME_TYPE_LAST))
        frame_type = META_FRAME_TYPE_NORMAL;

    screen = gtk_widget_get_screen (frame->style_window_rgba);
    style_info = meta_theme_create_style_info (screen, NULL);

    meta_theme_get_frame_borders (metacity->theme, style_info, frame_type,
                                  frame->text_height, 0, &borders);

    frame->win_extents.top = frame->win_extents.top;
    frame->win_extents.bottom = borders.visible.bottom;
    frame->win_extents.left = borders.visible.left;
    frame->win_extents.right = borders.visible.right;

    frame->titlebar_height = borders.visible.top - frame->win_extents.top;

    meta_theme_get_frame_borders (metacity->theme, style_info, frame_type,
                                  frame->text_height, META_FRAME_MAXIMIZED,
                                  &borders);

    frame->max_win_extents.top = frame->win_extents.top;
    frame->max_win_extents.bottom = borders.visible.bottom;
    frame->max_win_extents.left = borders.visible.left;
    frame->max_win_extents.right = borders.visible.right;

    frame->max_titlebar_height = borders.visible.top - frame->max_win_extents.top;

    meta_style_info_unref (style_info);

    gwd_decor_frame_unref (frame);
}

static gboolean
gwd_theme_metacity_get_button_position (GWDTheme *theme,
                                        decor_t  *decor,
                                        gint      i,
                                        gint      width,
                                        gint      height,
                                        gint     *x,
                                        gint     *y,
                                        gint     *w,
                                        gint     *h)
{
    GWDThemeMetacity *metacity;
    MetaFrameGeometry fgeom;
    MetaFrameType frame_type;
    MetaFrameFlags flags;
    MetaButtonLayout button_layout;
    MetaButtonFunction button_function;
    MetaButtonSpace *space;

    if (!decor->context) {
        /* undecorated windows implicitly have no buttons */
        return FALSE;
    }

    metacity = GWD_THEME_METACITY (theme);

    frame_type = meta_frame_type_from_string (decor->frame->type);
    if (!(frame_type < META_FRAME_TYPE_LAST))
        frame_type = META_FRAME_TYPE_NORMAL;

    gwd_theme_metacity_get_decoration_geometry (metacity, decor, &flags, &fgeom,
                                                &button_layout, frame_type);

    button_function = button_to_meta_button_function (i);
    if (!button_present (&button_layout, button_function))
        return FALSE;

    switch (i) {
        case BUTTON_MENU:
            space = &fgeom.menu_rect;
            break;
        case BUTTON_MIN:
            space = &fgeom.min_rect;
            break;
        case BUTTON_MAX:
            space = &fgeom.max_rect;
            break;
        case BUTTON_CLOSE:
            space = &fgeom.close_rect;
            break;
        case BUTTON_SHADE:
            space = &fgeom.shade_rect;
            break;
        case BUTTON_ABOVE:
            space = &fgeom.above_rect;
            break;
        case BUTTON_STICK:
            space = &fgeom.stick_rect;
            break;
        case BUTTON_UNSHADE:
            space = &fgeom.unshade_rect;
            break;
        case BUTTON_UNABOVE:
            space = &fgeom.unabove_rect;
            break;
        case BUTTON_UNSTICK:
            space = &fgeom.unstick_rect;
            break;
        default:
            return FALSE;
    }

    if (!space->clickable.width && !space->clickable.height)
        return FALSE;

    *x = space->clickable.x;
    *y = space->clickable.y;
    *w = space->clickable.width;
    *h = space->clickable.height;

    if (decor->frame_window) {
        *x += decor->frame->win_extents.left + 4;
        *y += decor->frame->win_extents.top + 2;
    }

    return TRUE;
}

static gfloat
gwd_theme_metacity_get_title_scale (GWDTheme      *theme,
                                    decor_frame_t *frame)
{
    GWDThemeMetacity *metacity;
    MetaFrameType type;
    MetaFrameFlags flags; 

    metacity = GWD_THEME_METACITY (theme);
    type = meta_frame_type_from_string (frame->type);
    flags = 0xc33; /* FIXME */

    if (type == META_FRAME_TYPE_LAST)
        return 1.0f;

    return meta_theme_get_title_scale (metacity->theme, type, flags);
}

static void
gwd_theme_metacity_class_init (GWDThemeMetacityClass *metacity_class)
{
    GObjectClass *object_class;
    GWDThemeClass *theme_class;

    object_class = G_OBJECT_CLASS (metacity_class);
    theme_class = GWD_THEME_CLASS (metacity_class);

    object_class->constructor = gwd_theme_metacity_constructor;

    theme_class->update_border_extents = gwd_theme_metacity_update_border_extents;
    theme_class->get_button_position = gwd_theme_metacity_get_button_position;
    theme_class->get_title_scale = gwd_theme_metacity_get_title_scale;
}

static void
gwd_theme_metacity_init (GWDThemeMetacity *metacity)
{
}
