/*
 * Copyright (C) 2007 Andrew Riedi <andrewriedi@gmail.com>
 *
 * Sticky window handling and OpenGL fixes:
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
 * 
 * Ported to Compiz 0.9:
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * This plug-in for Metacity-like workarounds.
 */

#include "workarounds.h"

bool haveOpenGL;

COMPIZ_PLUGIN_20090315 (workarounds, WorkaroundsPluginVTable);

void
WorkaroundsScreen::checkFunctions (bool checkWindow, bool checkScreen)
{
    if (haveOpenGL && optionGetForceGlxSync () && checkScreen)
    {
	gScreen->glPaintOutputSetEnabled (this, true);
    }
    else if (haveOpenGL && checkScreen)
    {
	gScreen->glPaintOutputSetEnabled (this, false);
    }

    if (haveOpenGL && optionGetForceSwapBuffers () && checkScreen)
    {
	cScreen->preparePaintSetEnabled (this, true);
    }
    else if (haveOpenGL && checkScreen)
    {
	cScreen->preparePaintSetEnabled (this, false);
    }

    if ((optionGetLegacyFullscreen () ||
	optionGetFirefoxMenuFix ()   ||
	optionGetOooMenuFix ()	     ||
	optionGetNotificationDaemonFix () ||
	optionGetJavaFix ()	     ||
	optionGetQtFix ()	     ||
	optionGetConvertUrgency ()   ) && checkScreen)
    {
	screen->handleEventSetEnabled (this, true);
    }
    else if (checkScreen)
    {
	screen->handleEventSetEnabled (this, false);
    }

    if (optionGetLegacyFullscreen () && checkWindow)
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    WORKAROUNDS_WINDOW (w);

	    ww->window->getAllowedActionsSetEnabled (ww, true);
	    ww->window->resizeNotifySetEnabled (ww, true);
	}
    }
    else if (checkWindow)
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    WORKAROUNDS_WINDOW (w);

	    ww->window->getAllowedActionsSetEnabled (ww, false);
	    ww->window->resizeNotifySetEnabled (ww, false);
	}
    }
}
	    

void
WorkaroundsScreen::addToFullscreenList (CompWindow *w)
{
    mfwList.push_back (w->id ());
}

void
WorkaroundsScreen::removeFromFullscreenList (CompWindow *w)
{
    mfwList.remove (w->id ());
}

static void
workaroundsProgramEnvParameter4f (GLenum  target,
				  GLuint  index,
				  GLfloat x,
				  GLfloat y,
				  GLfloat z,
				  GLfloat w)
{
    WorkaroundsScreen *ws;
    GLdouble data[4];

    ws = WorkaroundsScreen::get (screen);

    data[0] = x;
    data[1] = y;
    data[2] = z;
    data[3] = w;

    (*ws->programEnvParameter4dv) (target, index, data);
}

void
WorkaroundsScreen::updateParameterFix ()
{
    if (!GL::programEnvParameter4f || !programEnvParameter4dv)
	return;
    if (optionGetAiglxFragmentFix ())
	GL::programEnvParameter4f = workaroundsProgramEnvParameter4f;
    else
	GL::programEnvParameter4f = origProgramEnvParameter4f;
}

void
WorkaroundsScreen::updateVideoSyncFix ()
{
    if ((!GL::getVideoSync || origGetVideoSync) ||
	(!GL::waitVideoSync || origWaitVideoSync))
	return;
    if (optionGetNoWaitForVideoSync ())
    {
	GL::getVideoSync = NULL;
	GL::waitVideoSync = NULL;
    }
    else
    {
	GL::getVideoSync = origGetVideoSync;
	GL::waitVideoSync = origWaitVideoSync;
    }
}

void
WorkaroundsScreen::preparePaint (int ms)
{
    if (optionGetForceSwapBuffers ())
	cScreen->damageScreen (); // Massive CPU usage here

    cScreen->preparePaint (ms);
}

bool
WorkaroundsScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				  const GLMatrix	    &transform,
				  const CompRegion	    &region,
				  CompOutput		    *output,
				  unsigned int		    mask)
{
    if (optionGetForceGlxSync ())
	glXWaitX ();

    return gScreen->glPaintOutput (attrib, transform, region, output, mask);
}

CompString
WorkaroundsWindow::getRoleAtom ()
{
    Atom type;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char *str = NULL;
    int format, result;
    CompString retval;

    WORKAROUNDS_SCREEN (screen);

    result = XGetWindowProperty (screen->dpy (), window->id (), ws->roleAtom,
                                 0, LONG_MAX, FALSE, XA_STRING,
                                 &type, &format, &nItems, &bytesAfter,
                                 (unsigned char **) &str);

    if (result != Success)
        return NULL;

    if (type != XA_STRING)
    {
        XFree (str);
        return NULL;
    }

    retval = CompString ((const char *) str);

    return retval;
}

void
WorkaroundsWindow::removeSticky ()
{
    if (window->state () & CompWindowStateStickyMask && madeSticky)
	window->changeState (window->state () & ~CompWindowStateStickyMask);
    madeSticky = FALSE;
}

void
WorkaroundsWindow::updateSticky ()
{
    Bool makeSticky = FALSE;

    WORKAROUNDS_SCREEN (screen);

    if (ws->optionGetStickyAlldesktops () && window->desktop () == 0xffffffff &&
	ws->optionGetAlldesktopStickyMatch ().evaluate (window))
	makeSticky = TRUE;

    if (makeSticky)
    {
	if (!(window->state () & CompWindowStateStickyMask))
	{
	    madeSticky = TRUE;
	    window->changeState (
				window->state () | CompWindowStateStickyMask);
	}
    }
    else
	removeSticky ();
}

void
WorkaroundsWindow::updateUrgencyState ()
{
    Bool     urgent;
    XWMHints *xwmh;

    xwmh = XGetWMHints (screen->dpy (), window->id ());

    if (!xwmh)
    {
	XFree (xwmh);
	return;
    }

    urgent = (xwmh->flags & XUrgencyHint);

    XFree (xwmh);

    if (urgent)
    {
	madeDemandAttention = TRUE;
	window->changeState (window->state () |
			     CompWindowStateDemandsAttentionMask);
    }
    else if (madeDemandAttention)
    {
	madeDemandAttention = FALSE;
	window->changeState (window->state () &
			     ~CompWindowStateDemandsAttentionMask);
    }
}

void
WorkaroundsWindow::getAllowedActions (unsigned int &setActions,
				      unsigned int &clearActions)
{

    window->getAllowedActions (setActions, clearActions);

    if (isFullscreen)
	setActions |= CompWindowActionFullscreenMask;
}

void
WorkaroundsWindow::fixupFullscreen ()
{
    Bool   isFullSize;
    int    output;
    BoxPtr box;

    WORKAROUNDS_SCREEN (screen);

    if (!ws->optionGetLegacyFullscreen ())
	return;

    if (window->wmType () & CompWindowTypeDesktopMask)
    {
	/* desktop windows are implicitly fullscreen */
	isFullSize = FALSE;
    }
    else
    {
    	/* get output region for window */
	output = screen->outputDeviceForGeometry (window->geometry ());
	box = &screen->outputDevs ().at (output).region ()->extents;

	/* does the size match the output rectangle? */
	isFullSize = (window->serverX () == box->x1) &&
		     (window->serverY () == box->y1) &&
	             (window->serverWidth () == (box->x2 - box->x1)) &&
		     (window->serverHeight () == (box->y2 - box->y1));

	/* if not, check if it matches the whole screen */
	if (!isFullSize)
	{
	    if ((window->serverX () == 0) && (window->serverY () == 0) &&
		(window->serverWidth () == screen->width ()) &&
		(window->serverHeight () == screen->height ()))
	    {
		isFullSize = TRUE;
	    }
	}
    }

    isFullscreen = isFullSize;
    if (isFullSize && !(window->state () & CompWindowStateFullscreenMask))
    {
	unsigned int state = window->state () &
				~CompWindowStateFullscreenMask;

	if (isFullSize)
	    state |= CompWindowStateFullscreenMask;
	madeFullscreen = isFullSize;

	if (state != window->state ())
	{
	    window->changeState (state);
	    window->updateAttributes (CompStackingUpdateModeNormal);

	    /* keep track of windows that we interact with */
	    ws->addToFullscreenList (window);
	}
    }
    else if (!isFullSize && !ws->mfwList.empty () &&
	     (window->state () & CompWindowStateFullscreenMask))
    {
	/* did we set the flag? */

	foreach (Window mfw, ws->mfwList) 
	{
	    if (mfw == window->id ()) 
	    {
		unsigned int state = window->state () &
					~CompWindowStateFullscreenMask;

		if (isFullSize)
		    state |= CompWindowStateFullscreenMask;

		madeFullscreen = isFullSize;

		if (state != window->state ())
		{
		    window->changeState (state);
		    window->updateAttributes (CompStackingUpdateModeNormal);
		}

		ws->removeFromFullscreenList (window);
		break;
	    }
    	}
   }
}

void
WorkaroundsWindow::updateFixedWindow (unsigned int newWmType)
{
    if (newWmType != window->wmType ())
    {
	adjustedWinType = TRUE;

	window->recalcType ();
	window->recalcActions ();

	screen->matchPropertyChanged (window);
    }
}

unsigned int
WorkaroundsWindow::getFixedWindowType ()
{
    unsigned int newWmType;
    XClassHint classHint;
    CompString resName;

    WORKAROUNDS_SCREEN (screen);

    newWmType = window->type (); // ???

    if (!XGetClassHint (screen->dpy (), window->id (), &classHint) != Success)
	return newWmType;

    if (classHint.res_name)
    {
	resName = CompString (classHint.res_name);
	XFree (classHint.res_name);
    }

    /* FIXME: Is this the best way to detect a notification type window? */
    if (ws->optionGetNotificationDaemonFix ())
    {
        if (newWmType == CompWindowTypeNormalMask &&
            window->overrideRedirect () && !resName.empty () &&
            resName.compare("notification-daemon") == 0)
        {
            newWmType = CompWindowTypeNotificationMask;
	    updateFixedWindow (newWmType);
	    return newWmType;
        }
    }

    if (ws->optionGetFirefoxMenuFix ())
    {
        if (newWmType == CompWindowTypeNormalMask &&
            window->overrideRedirect () && !resName.empty ())
	{
	    if ((resName.compare ( "gecko") == 0) ||
		(resName.compare ( "popup") == 0))
	    {
		newWmType = CompWindowTypeDropdownMenuMask;
	        updateFixedWindow (newWmType);
	        return newWmType;
	    }
	}
    }

    if (ws->optionGetOooMenuFix ())
    {
        if (newWmType == CompWindowTypeNormalMask &&
            window->overrideRedirect () && !resName.empty ())
	{
	    if (resName.compare ( "VCLSalFrame") == 0)
	    {
		newWmType = CompWindowTypeDropdownMenuMask;
	        updateFixedWindow (newWmType);
	        return newWmType;
	    }
	}
    }
    /* FIXME: Basic hack to get Java windows working correctly. */
    if (ws->optionGetJavaFix () && !resName.empty ())
    {
        if ((resName.compare ( "sun-awt-X11-XMenuWindow") == 0) ||
            (resName.compare ( "sun-awt-X11-XWindowPeer") == 0))
        {
            newWmType = CompWindowTypeDropdownMenuMask;
	    updateFixedWindow (newWmType);
	    return newWmType;
        }
        else if (resName.compare ( "sun-awt-X11-XDialogPeer") == 0)
        {
            newWmType = CompWindowTypeDialogMask;
	    updateFixedWindow (newWmType);
	    return newWmType;
        }
        else if (resName.compare ( "sun-awt-X11-XFramePeer") == 0)
        {
            newWmType = CompWindowTypeNormalMask;
	    updateFixedWindow (newWmType);
	    return newWmType;
        }
    }

    if (ws->optionGetQtFix ())
    {
        CompString windowRole;

        /* fix tooltips */
        windowRole = getRoleAtom ();
        if (!windowRole.empty ())
        {
            if ((windowRole.compare ("toolTipTip") == 0) ||
                (windowRole.compare ("qtooltip_label") == 0))
            {
                newWmType = CompWindowTypeTooltipMask;
		updateFixedWindow (newWmType);
	        return newWmType;
            }
        }

        /* fix Qt transients - FIXME: is there a better way to detect them? */
	if (resName.empty () && window->overrideRedirect () &&
	    (window->windowClass () == InputOutput) &&
	    (newWmType == CompWindowTypeUnknownMask))
	{
	    newWmType = CompWindowTypeDropdownMenuMask;
	    updateFixedWindow (newWmType);
	    return newWmType;
	}
    }

    return newWmType;
}

void
WorkaroundsScreen::optionChanged (CompOption		      *opt,
				  WorkaroundsOptions::Options num)
{
    checkFunctions (true, true);

    foreach (CompWindow *w, screen->windows ())
	WorkaroundsWindow::get (w)->updateSticky ();

    if (haveOpenGL)
    {
	updateParameterFix ();
	updateVideoSyncFix ();

	if (optionGetFglrxXglFix ())
	    GL::copySubBuffer = NULL;
	else
	    GL::copySubBuffer = origCopySubBuffer;
    }
}

void
WorkaroundsScreen::handleEvent (XEvent *event)
{
    CompWindow *w;

    switch (event->type) {
    case ConfigureRequest:
	w = screen->findWindow (event->xconfigurerequest.window);
	if (w)
	{
	    WORKAROUNDS_WINDOW (w);

	    if (ww->madeFullscreen)
		w->changeState (w->state () &= ~CompWindowStateFullscreenMask);
	}
	break;
    case MapRequest:
	w = screen->findWindow (event->xmaprequest.window);
	if (w)
	{
	    WORKAROUNDS_WINDOW (w);
	    ww->updateSticky ();
	    w->wmType () = ww->getFixedWindowType ();
	    ww->fixupFullscreen ();
	}
	break;
    case MapNotify:
	w = screen->findWindow (event->xmap.window);
	if (w && w->overrideRedirect ())
	{
	    WORKAROUNDS_WINDOW (w);
	    w->wmType () = ww->getFixedWindowType ();
	}
	break;
    case DestroyNotify:
	w = screen->findWindow (event->xdestroywindow.window);
	if (w)
	    removeFromFullscreenList (w);
	break;
    }
 
    screen->handleEvent (event);

    switch (event->type) {
    case ConfigureRequest:
	w = screen->findWindow (event->xconfigurerequest.window);
	if (w)
	{
	    WORKAROUNDS_WINDOW (w);

	    if (ww->madeFullscreen)
		w->state () |= CompWindowStateFullscreenMask;
	}
	break;
    case ClientMessage:
	if (event->xclient.message_type == Atoms::winDesktop)
        {
            w = screen->findWindow (event->xclient.window);
	    if (w)
	    {
		WORKAROUNDS_WINDOW (w);
	        ww->updateSticky ();
	    }
        }
	break;
    case PropertyNotify:
	if ((event->xproperty.atom == XA_WM_CLASS) ||
	    (event->xproperty.atom == Atoms::winType))
	{
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
		WORKAROUNDS_WINDOW (w);
		w->wmType () = ww->getFixedWindowType ();
	    }
	}
	else if (event->xproperty.atom == XA_WM_HINTS)
	{
	    if (optionGetConvertUrgency ())
	    {
		w = screen->findWindow (event->xproperty.window);
		if (w)
		{
		    WORKAROUNDS_WINDOW (w);
		    ww->updateUrgencyState ();
		}
	    }
	}
	break;
    default:
	break;
    }
}

void
WorkaroundsWindow::resizeNotify (int dx, int dy,
                                 int dwidth, int dheight)
{
    if (window->isViewable ())
	fixupFullscreen ();

    window->resizeNotify (dx, dy, dwidth, dheight);
}

WorkaroundsScreen::WorkaroundsScreen (CompScreen *screen) :
    PluginClassHandler <WorkaroundsScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    roleAtom (XInternAtom (screen->dpy (), "WM_WINDOW_ROLE", 0))
{
    ScreenInterface::setHandler (screen, false);
    if (haveOpenGL)
    {
	CompositeScreenInterface::setHandler (cScreen, false);
	GLScreenInterface::setHandler (gScreen, false);
    }

    optionSetStickyAlldesktopsNotify (boost::bind (
					&WorkaroundsScreen::optionChanged, this,
					_1, _2));
    optionSetAlldesktopStickyMatchNotify (boost::bind (
					&WorkaroundsScreen::optionChanged, this,
					_1, _2));

    optionSetAiglxFragmentFixNotify (boost::bind (
					&WorkaroundsScreen::optionChanged, this,
					_1, _2));

    optionSetFglrxXglFixNotify (boost::bind (
					&WorkaroundsScreen::optionChanged, this,
					_1, _2));
    optionSetForceSwapBuffersNotify (boost::bind (
					&WorkaroundsScreen::optionChanged, this,
					_1, _2));
    optionSetNoWaitForVideoSyncNotify (boost::bind (
					&WorkaroundsScreen::optionChanged, this,
					_1, _2));

    if (haveOpenGL)
    {
	origProgramEnvParameter4f = GL::programEnvParameter4f;
	programEnvParameter4dv  = (GLProgramParameter4dvProc)
    		       gScreen->getProcAddress ("glProgramEnvParameter4dvARB");
	origCopySubBuffer = GL::copySubBuffer;

	origGetVideoSync = GL::getVideoSync;
	origWaitVideoSync = GL::waitVideoSync;

	updateParameterFix ();
	updateVideoSyncFix ();
    }

    if (optionGetFglrxXglFix () && haveOpenGL)
	GL::copySubBuffer = NULL;

    checkFunctions (false, true);
}

WorkaroundsScreen::~WorkaroundsScreen ()
{
    if (haveOpenGL)
    {
	GL::copySubBuffer = origCopySubBuffer;
	GL::getVideoSync = origGetVideoSync;
	GL::waitVideoSync = origWaitVideoSync;
    }
}

WorkaroundsWindow::WorkaroundsWindow (CompWindow *window) :
    PluginClassHandler <WorkaroundsWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    adjustedWinType (false),
    madeSticky (false),
    madeFullscreen (false),
    isFullscreen (false),
    madeDemandAttention (false)
{
    WindowInterface::setHandler (window, false);

    WORKAROUNDS_SCREEN (screen);

    if (ws->optionGetLegacyFullscreen ())
    {
	window->getAllowedActionsSetEnabled (this, false);
	window->resizeNotifySetEnabled (this, false);
    }
}


WorkaroundsWindow::~WorkaroundsWindow ()
{
    if (!window->destroyed ())
    {
	if (adjustedWinType)
	{
	    window->wmType () = window->type ();
	    window->recalcType ();
	    window->recalcActions ();
	}

	if (window->state () & CompWindowStateStickyMask && madeSticky)
	{
	    window->state () &= ~CompWindowStateStickyMask;
	}
    }
}

bool
WorkaroundsPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    if ((CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI)) &&
        (CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI)))
	haveOpenGL = true;
    else
	haveOpenGL = false;

    return true;
}
