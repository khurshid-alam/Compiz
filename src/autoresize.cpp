/*
 * Compiz autoresize plugin
 * Author : Sam "SmSpillaz" Spilsbury
 * Email  : smspillaz@gmail.com
 *
 * Copyright (C) 2009 Sam Spilsbury
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "autoresize.h"
#include <X11/extensions/Xrandr.h>


COMPIZ_PLUGIN_20090315 (autoresize, AutoresizePluginVTable);

// helper functions

void
AutoresizeScreen::resizeAllWindows ()
{
    foreach (CompWindow *window, screen->windows ())
    {
	if (!window->isMapped ())
	    continue;
	
	if (window->state () & CompWindowStateSkipTaskbarMask)
	    continue;

	if (window->state () & CompWindowStateSkipPagerMask)
	    continue;

	if (!window->managed ())
	    continue;

	if (window->x () < 0)
	    window->move (-1 * window->x (), 0, true);
	if (window->y () < 0)
	    window->move (0, -1 * window->y (), true);
	if (window->x () + window->width () > mWidth)
	    window->resize (window->x (), window->y (), 
				window->width () - (mWidth -
				 (window->x () + window->width ())),
				  window->height ());
	if (window->y () + window->height () > mHeight)
	    window->resize (window->x (), window->y (),
				window->width (), window->height () - (mHeight -
				 (window->y () + window->height ())));
    }
}


// Check if the screen was modified in any way
void
AutoresizeScreen::handleEvent (XEvent *event)
{
    if (event->type == screen->randrEvent ())
    {
	XRRScreenChangeNotifyEvent *rEvent = (XRRScreenChangeNotifyEvent *) event;

	if (mWidth != rEvent->width || mHeight != rEvent->height)
	{
	    mWidth = rEvent->width;
	    mHeight = rEvent->height;
	    resizeAllWindows ();
	}
    }

    screen->handleEvent (event);
}

AutoresizeScreen::AutoresizeScreen (CompScreen *screen) :
    PluginClassHandler <AutoresizeScreen, CompScreen> (screen),
    mWidth (screen->width ()),
    mHeight (screen->height ())
{
    ScreenInterface::setHandler (screen);
}

bool
AutoresizePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}

