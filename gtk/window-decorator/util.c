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
 */

#include "gtk-window-decorator.h"

gboolean
get_window_prop (Window xwindow,
		 Atom   atom,
		 Window *val)
{
    Atom   type;
    int	   format;
    gulong nitems;
    gulong bytes_after;
    Window *w;
    int    err, result;

    *val = 0;

    gdk_error_trap_push ();

    type = None;
    result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
				 xwindow,
				 atom,
				 0, G_MAXLONG,
				 False, XA_WINDOW, &type, &format, &nitems,
				 &bytes_after, (void*) &w);
    err = gdk_error_trap_pop ();
    if (err != Success || result != Success)
	return FALSE;

    if (type != XA_WINDOW)
    {
	XFree (w);
	return FALSE;
    }

    *val = *w;
    XFree (w);

    return TRUE;
}

unsigned int
get_mwm_prop (Window xwindow)
{
    Display	  *xdisplay;
    Atom	  actual;
    int		  err, result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned int  decor = MWM_DECOR_ALL;

    xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

    gdk_error_trap_push ();

    result = XGetWindowProperty (xdisplay, xwindow, mwm_hints_atom,
				 0L, 20L, FALSE, AnyPropertyType,
				 &actual, &format, &n, &left, &data);

    err = gdk_error_trap_pop ();
    if (err != Success || result != Success)
	return decor;

    if (data)
    {
	MwmHints *mwm_hints = (MwmHints *) data;

	if (n >= PROP_MOTIF_WM_HINT_ELEMENTS)
	{
	    if (mwm_hints->flags & MWM_HINTS_DECORATIONS)
		decor = mwm_hints->decorations;
	}

	XFree (data);
    }

    return decor;
}

static gboolean
get_prefer_dark_theme (void)
{
  gboolean prefer_dark_theme;

  g_object_get (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", &prefer_dark_theme,
                NULL);

  return prefer_dark_theme;
}

gchar *
get_gtk_theme_variant (Window xwindow)
{
  GdkDisplay *display;
  Display *xdisplay;
  gchar *gtk_theme_variant;
  gint result;
  Atom actual;
  gint format;
  gulong n;
  gulong left;
  guchar *data;

  display = gdk_display_get_default ();
  xdisplay = gdk_x11_display_get_xdisplay (display);
  gtk_theme_variant = NULL;

  gdk_error_trap_push ();
  result = XGetWindowProperty (xdisplay, xwindow, gtk_theme_variant_atom,
                               0L, 1024L, False, utf8_string_atom,
                               &actual, &format, &n, &left, &data);
  gdk_error_trap_pop_ignored ();

  if (result == Success && data)
    {
      if (n)
        gtk_theme_variant = g_strdup ((gchar *) data);

      XFree (data);
    }

  if (gtk_theme_variant == NULL && get_prefer_dark_theme ())
    gtk_theme_variant = g_strdup ("dark");

  return gtk_theme_variant;
}
