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
