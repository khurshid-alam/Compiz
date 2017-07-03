/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>

#include <core/atoms.h>
#include "move.h"

COMPIZ_PLUGIN_20090315 (move, MovePluginVTable)

static const int defaultBorderWidth = 2;
static const int biggerBorderWidthMultiplier = 2;

static bool
moveInitiate (CompAction         *action,
	      CompAction::State  state,
	      CompOption::Vector &options)
{
    CompWindow *w;

    MOVE_SCREEN (screen);

    Window xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findWindow (xid);

    if (w && (w->actions () & CompWindowActionMoveMask))
    {
	CompScreen *s = screen;

	if (s->otherGrabExist ("move", NULL) ||
	    ms->w			     ||
	    w->overrideRedirect ()	     ||
	    w->type () & (CompWindowTypeDesktopMask |
			  CompWindowTypeDockMask    |
			  CompWindowTypeFullscreenMask))
	    return false;

	unsigned int mods = CompOption::getIntOptionNamed (options, "modifiers", 0);

	int x = CompOption::getIntOptionNamed (options, "x", w->geometry ().x () +
					       (w->size ().width () / 2));
	int y = CompOption::getIntOptionNamed (options, "y", w->geometry ().y () +
					       (w->size ().height () / 2));

	int button = CompOption::getIntOptionNamed (options, "button", -1);

	if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);

	if (ms->region)
	{
	    XDestroyRegion (ms->region);
	    ms->region = NULL;
	}

	ms->status = RectangleOut;

	ms->savedX = w->serverGeometry ().x ();
	ms->savedY = w->serverGeometry ().y ();

	ms->x = 0;
	ms->y = 0;

	lastPointerX = x;
	lastPointerY = y;

	bool sourceExternalApp =
	    CompOption::getBoolOptionNamed (options, "external", false);

	ms->yConstrained = sourceExternalApp && ms->optionGetConstrainY ();

	ms->origState = w->state ();

	CompRect workArea (s->getWorkareaForOutput (w->outputDevice ()));

	ms->snapBackX = w->serverGeometry ().x () - workArea.x ();
	ms->snapOffX  = x - workArea.x ();

	ms->snapBackY = w->serverGeometry ().y () - workArea.y ();
	ms->snapOffY  = y - workArea.y ();

	if (!ms->grab)
	{
	    Cursor moveCursor = screen->cursorCache (XC_fleur);

	    if (state & CompAction::StateInitButton)
		ms->grab = s->pushPointerGrab (moveCursor, "move");
	    else
		ms->grab = s->pushGrab (moveCursor, "move");
	}

	if (ms->grab)
	{
	    unsigned int grabMask = CompWindowGrabMoveMask |
				    CompWindowGrabButtonMask;

	    if (sourceExternalApp)
		grabMask |= CompWindowGrabExternalAppMask;

	    ms->w = w;

	    ms->releaseButton = button;

	    w->grabNotify (x, y, mods, grabMask);

	    /* Click raise happens implicitly on buttons 1, 2 and 3 so don't
	     * restack this window again if the action buttonbinding was from
	     * one of those buttons */
	    if (screen->getOption ("raise_on_click")->value ().b () &&
		button != Button1 && button != Button2 && button != Button3)
		w->updateAttributes (CompStackingUpdateModeAboveFullscreen);

	    if (state & CompAction::StateInitKey)
	    {
		int xRoot = w->geometry ().x () + (w->size ().width () / 2);
		int yRoot = w->geometry ().y () + (w->size ().height () / 2);

		s->warpPointer (xRoot - pointerX, yRoot - pointerY);
	    }

	    if (ms->optionGetMode () != MoveOptions::ModeNormal)
	    {
		Box box;

		ms->gScreen->glPaintOutputSetEnabled (ms, true);
		ms->paintRect = true;
		ms->rectX = 0;
		ms->rectY = 0;

		if (ms->getMovingRectangle (&box))
		    ms->damageMovingRectangle (&box);
	    }

	    if (ms->moveOpacity != OPAQUE)
	    {
		MOVE_WINDOW (w);

		if (mw->cWindow)
		    mw->cWindow->addDamage ();

		if (mw->gWindow)
		    mw->gWindow->glPaintSetEnabled (mw, true);
	    }

	    if (ms->optionGetLazyPositioning ())
	    {
		MOVE_WINDOW (w);

		if (mw->gWindow)
		    mw->releasable = w->obtainLockOnConfigureRequests ();
	    }
	}
    }

    return false;
}

static bool
moveTerminate (CompAction         *action,
	       CompAction::State  state,
	       CompOption::Vector &options)
{
    MOVE_SCREEN (screen);

    if (ms->w)
    {
	MOVE_WINDOW (ms->w);

	if (ms->paintRect)
	{
	    ms->paintRect = false;
	    ms->gScreen->glPaintOutputSetEnabled (ms, false);
	    mw->window->move (ms->rectX, ms->rectY, true);
	}

	if (state & CompAction::StateCancel)
	{
	    ms->w->move (ms->savedX - ms->w->geometry ().x (),
			 ms->savedY - ms->w->geometry ().y (), false);
	}

	/* update window attributes as window constraints may have
	   changed - needed e.g. if a maximized window was moved
	   to another output device */
	ms->w->updateAttributes (CompStackingUpdateModeNone);

	ms->w->ungrabNotify ();

	if (ms->grab)
	{
	    screen->removeGrab (ms->grab, NULL);
	    ms->grab = NULL;
	}

	if (ms->moveOpacity != OPAQUE)
	{
	    if (mw->cWindow)
		mw->cWindow->addDamage ();

	    if (mw->gWindow)
		mw->gWindow->glPaintSetEnabled (mw, false);
	}

	mw->releasable.reset ();

	ms->w             = 0;
	ms->releaseButton = 0;
    }

    action->setState (action->state () & ~(CompAction::StateTermKey |
					   CompAction::StateTermButton));

    return false;
}

/* creates a region containing top and bottom struts. only struts that are
   outside the screen workarea are considered. */
static Region
moveGetYConstrainRegion (CompScreen *s)
{
    CompWindow   *w;
    REGION       r;
    CompRect     workArea;
    BoxRec       extents;

    Region region = XCreateRegion ();

    if (!region)
	return NULL;

    r.rects    = &r.extents;
    r.numRects = r.size = 1;

    r.extents.x1 = MINSHORT;
    r.extents.y1 = 0;
    r.extents.x2 = 0;
    r.extents.y2 = s->height ();

    XUnionRegion (&r, region, region);

    r.extents.x1 = s->width ();
    r.extents.x2 = MAXSHORT;

    XUnionRegion (&r, region, region);

    for (unsigned int i = 0; i < s->outputDevs ().size (); i++)
    {
	XUnionRegion (s->outputDevs ()[i].region (), region, region);

	workArea = s->getWorkareaForOutput (i);
	extents = s->outputDevs ()[i].region ()->extents;

	foreach (w, s->windows ())
	{
	    if (!w->mapNum ())
		continue;

	    if (w->struts ())
	    {
		r.extents.x1 = w->struts ()->top.x;
		r.extents.y1 = w->struts ()->top.y;
		r.extents.x2 = r.extents.x1 + w->struts ()->top.width;
		r.extents.y2 = r.extents.y1 + w->struts ()->top.height;

		if (r.extents.x1 < extents.x1)
		    r.extents.x1 = extents.x1;
		if (r.extents.x2 > extents.x2)
		    r.extents.x2 = extents.x2;
		if (r.extents.y1 < extents.y1)
		    r.extents.y1 = extents.y1;
		if (r.extents.y2 > extents.y2)
		    r.extents.y2 = extents.y2;

		if (r.extents.x1 < r.extents.x2 &&
		    r.extents.y1 < r.extents.y2 &&
		    r.extents.y2 <= workArea.y ())
			XSubtractRegion (region, &r, region);

		r.extents.x1 = w->struts ()->bottom.x;
		r.extents.y1 = w->struts ()->bottom.y;
		r.extents.x2 = r.extents.x1 + w->struts ()->bottom.width;
		r.extents.y2 = r.extents.y1 + w->struts ()->bottom.height;

		if (r.extents.x1 < extents.x1)
		    r.extents.x1 = extents.x1;
		if (r.extents.x2 > extents.x2)
		    r.extents.x2 = extents.x2;
		if (r.extents.y1 < extents.y1)
		    r.extents.y1 = extents.y1;
		if (r.extents.y2 > extents.y2)
		    r.extents.y2 = extents.y2;

		if (r.extents.x1 < r.extents.x2 && r.extents.y1 < r.extents.y2 &&
		    r.extents.y1 >= workArea.bottom ())
			XSubtractRegion (region, &r, region);
	    }
	}
    }

    return region;
}

static void
moveHandleMotionEvent (CompScreen *s,
		       int	  xRoot,
		       int	  yRoot)
{
    MOVE_SCREEN (s);

    if (ms->grab)
    {
	int	     dx, dy;
	CompWindow   *w;

	w = ms->w;

	int wX      = w->geometry ().x ();
	int wY      = w->geometry ().y ();

	int wWidth  = w->geometry ().widthIncBorders ();
	int wHeight = w->geometry ().heightIncBorders ();

	ms->x += xRoot - lastPointerX;
	ms->y += yRoot - lastPointerY;

	if (w->type () & CompWindowTypeFullscreenMask)
	    dx = dy = 0;
	else
	{
	    int	     min, max;

	    dx = ms->x;
	    dy = ms->y;

	    CompRect workArea = s->getWorkareaForOutput (w->outputDevice ());

	    if (ms->yConstrained)
	    {
		if (!ms->region)
		    ms->region = moveGetYConstrainRegion (s);

		/* make sure that the top border extents or the top row of
		   pixels are within what is currently our valid screen
		   region */
		if (ms->region)
		{
		    int x	   = wX + dx - w->border ().left;
		    int y	   = wY + dy - w->border ().top;
		    int width	   = wWidth + w->border ().left + w->border ().right;
		    int height	   = w->border ().top ? w->border ().top : 1;

		    int status = XRectInRegion (ms->region, x, y,
						(unsigned int) width,
						(unsigned int) height);

		    /* only constrain movement if previous position was valid */
		    if (ms->status == RectangleIn)
		    {
			int xStatus = status;

			while (dx && xStatus != RectangleIn)
			{
			    xStatus = XRectInRegion (ms->region,
						     x, y - dy,
						     (unsigned int) width,
						     (unsigned int) height);

			    if (xStatus != RectangleIn)
				dx += (dx < 0) ? 1 : -1;

			    x = wX + dx - w->border ().left;
			}

			while (dy && status != RectangleIn)
			{
			    status = XRectInRegion (ms->region,
						    x, y,
						    (unsigned int) width,
						    (unsigned int) height);

			    if (status != RectangleIn)
				dy += (dy < 0) ? 1 : -1;

			    y = wY + dy - w->border ().top;
			}
		    }
		    else
			ms->status = status;
		}
	    }
	    if (ms->optionGetSnapoffSemimaximized ())
	    {
		int snapoffDistance  = ms->optionGetSnapoffDistance ();
		int snapbackDistance = ms->optionGetSnapbackDistance ();

		if (w->state () & CompWindowStateMaximizedVertMask)
		{
		    if (abs (yRoot - workArea.y () - ms->snapOffY) >= snapoffDistance)
		    {
			if (!s->otherGrabExist ("move", NULL))
			{
			    int width = w->serverGeometry ().width ();

			    if (w->saveMask () & CWWidth)
				width = w->saveWc ().width;

			    ms->x = ms->y = 0;

			    /* Get a lock on configure requests so that we can make
			     * any movement here atomic */
			    compiz::window::configure_buffers::ReleasablePtr lock (w->obtainLockOnConfigureRequests ());

			    w->maximize (0);

			    XWindowChanges xwc;
			    xwc.x = xRoot - (width / 2);
			    xwc.y = yRoot + w->border ().top / 2;

			    w->configureXWindow (CWX | CWY, &xwc);

			    ms->snapOffY = ms->snapBackY;

			    return;
			}
		    }
		}
		else if (ms->origState & CompWindowStateMaximizedVertMask &&
			 ms->optionGetSnapbackSemimaximized ())
		{
		    if (abs (yRoot - workArea.y () - ms->snapBackY) < snapbackDistance)
		    {
			if (!s->otherGrabExist ("move", NULL))
			{
			    w->maximize (ms->origState);
			    s->warpPointer (0, -snapbackDistance);

			    return;
			}
		    }
		}
		else if (w->state () & CompWindowStateMaximizedHorzMask)
		{
		    if (abs (xRoot - workArea.x () - ms->snapOffX) >= snapoffDistance)
		    {
			if (!s->otherGrabExist ("move", NULL))
			{
			    int width = w->serverGeometry ().width ();

			    w->saveMask () |= CWX | CWY;

			    if (w->saveMask () & CWWidth)
				width = w->saveWc ().width;

			    /* Get a lock on configure requests so that we can make
			     * any movement here atomic */
			    compiz::window::configure_buffers::ReleasablePtr lock (w->obtainLockOnConfigureRequests ());

			    w->maximize (0);

			    XWindowChanges xwc;
			    xwc.x = xRoot - (width / 2);
			    xwc.y = yRoot + w->border ().top / 2;

			    w->configureXWindow (CWX | CWY, &xwc);

			    ms->snapOffX = ms->snapBackX;

			    return;
			}
		    }
		}
		else if (ms->origState & CompWindowStateMaximizedHorzMask &&
			 ms->optionGetSnapbackSemimaximized ())
		{
		    /* TODO: Snapping back horizontally just works for the left side
		     * of the screen for now
		     */
		    if (abs (xRoot - workArea.x () - ms->snapBackX) < snapbackDistance)
		    {
			if (!s->otherGrabExist ("move", NULL))
			{
			    w->maximize (ms->origState);
			/* TODO: Here we should warp the pointer back, but this somehow interrupts
			 * the horizontal maximizing, we should fix it and reenable this warp:
			 * s->warpPointer (workArea.width () / 2 - snapbackDistance, 0);
			 */

			    return;
			}
		    }
		}
	    }

	    if (w->state () & CompWindowStateMaximizedVertMask)
	    {
		min = workArea.y () + w->border ().top;
		max = workArea.bottom () - w->border ().bottom - wHeight;

		if (wY + dy < min)
		    dy = min - wY;
		else if (wY + dy > max)
		    dy = max - wY;
	    }

	    if (w->state () & CompWindowStateMaximizedHorzMask)
	    {
		if (wX > (int) s->width () ||
		    wX + w->size ().width () < 0 ||
		    wX + wWidth < 0)
		    return;

		min = workArea.x () + w->border ().left;
		max = workArea.right () - w->border ().right - wWidth;

		if (wX + dx < min)
		    dx = min - wX;
		else if (wX + dx > max)
		    dx = max - wX;
	    }
	}

	if (dx || dy)
	{
	    if (ms->optionGetMode () == MoveOptions::ModeNormal)
	    {
		w->move (wX + dx - w->geometry ().x (), wY + dy - w->geometry ().y (), false);
	    }
	    else
	    {
		ms->rectX += wX + dx - w->geometry ().x ();
		ms->rectY += wY + dy - w->geometry ().y ();
	    }

	    ms->x -= dx;
	    ms->y -= dy;
	}
    }
}

void
MoveScreen::handleEvent (XEvent *event)
{
    switch (event->type)
    {
	case ButtonPress:
	case ButtonRelease:
	    if (event->xbutton.root == screen->root () &&
		grab &&
		(releaseButton == -1 ||
		 releaseButton == (int) event->xbutton.button))
		moveTerminate (&optionGetInitiateButton (),
			       CompAction::StateTermButton,
			       noOptions ());
	    break;

	case KeyPress:
	    if (event->xkey.root == screen->root () &&
		grab)
		for (unsigned int i = 0; i < NUM_KEYS; i++)
		    if (event->xkey.keycode == key[i])
		    {
			int moveIncrement = optionGetKeyMoveInc ();

			XWarpPointer (screen->dpy (), None, None,
				      0, 0, 0, 0,
				      mKeys[i].dx * moveIncrement,
				      mKeys[i].dy * moveIncrement);
			break;
		    }
	    break;

	case MotionNotify:
	    if (event->xmotion.root == screen->root ())
		moveHandleMotionEvent (screen, pointerX, pointerY);

	    break;

	case EnterNotify:
	case LeaveNotify:
	    if (event->xcrossing.root == screen->root ())
		moveHandleMotionEvent (screen, pointerX, pointerY);

	    break;

	case ClientMessage:
	    if (event->xclient.message_type == Atoms::wmMoveResize)
	    {
		MOVE_SCREEN (screen);

		unsigned long type = event->xclient.data.l[2];

		if (type == WmMoveResizeMove ||
		    type == WmMoveResizeMoveKeyboard)
		{
		    CompWindow *w;
		    w = screen->findWindow (event->xclient.window);
		    if (w)
		    {
			CompOption::Vector o;

			o.push_back (CompOption ("window", CompOption::TypeInt));
			o[0].value ().set ((int) event->xclient.window);

			o.push_back (CompOption ("external",
				     CompOption::TypeBool));
			o[1].value ().set (true);

			if (event->xclient.data.l[2] == WmMoveResizeMoveKeyboard)
			    moveInitiate (&optionGetInitiateKey (),
					  CompAction::StateInitKey, o);

			/* TODO: not only button 1 */
			else if (pointerMods & Button1Mask)
			{
			    o.push_back (CompOption ("modifiers", CompOption::TypeInt));
			    o[2].value ().set ((int) pointerMods);

			    o.push_back (CompOption ("x", CompOption::TypeInt));
			    o[3].value ().set ((int) event->xclient.data.l[0]);

			    o.push_back (CompOption ("y", CompOption::TypeInt));
			    o[4].value ().set ((int) event->xclient.data.l[1]);

			    o.push_back (CompOption ("button", CompOption::TypeInt));
			    o[5].value ().set ((int) (event->xclient.data.l[3] ?
						      event->xclient.data.l[3] : -1));

			    moveInitiate (&optionGetInitiateButton (),
					  CompAction::StateInitButton, o);

			    moveHandleMotionEvent (screen, pointerX, pointerY);
			}
		    }
		}
		else if (ms->w && type == WmMoveResizeCancel &&
			 ms->w->id () == event->xclient.window)
		    {
			moveTerminate (&optionGetInitiateButton (),
				       CompAction::StateCancel, noOptions ());
			moveTerminate (&optionGetInitiateKey (),
				       CompAction::StateCancel, noOptions ());
		    }
	    }
	    break;

	case DestroyNotify:
	    if (w && w->id () == event->xdestroywindow.window)
	    {
		moveTerminate (&optionGetInitiateButton (), 0, noOptions ());
		moveTerminate (&optionGetInitiateKey (), 0, noOptions ());
	    }
	    break;

	case UnmapNotify:
	    if (w && w->id () == event->xunmap.window)
	    {
		moveTerminate (&optionGetInitiateButton (), 0, noOptions ());
		moveTerminate (&optionGetInitiateKey (), 0, noOptions ());
	    }
	    break;

	default:
	    break;
    }

    screen->handleEvent (event);
}

bool
MoveWindow::glPaint (const GLWindowPaintAttrib &attrib,
		     const GLMatrix            &transform,
		     const CompRegion          &region,
		     unsigned int              mask)
{
    GLWindowPaintAttrib sAttrib = attrib;

    MOVE_SCREEN (screen);

    if (ms->grab &&
	ms->w == window && ms->moveOpacity != OPAQUE)
	    /* modify opacity of windows that are not active */
	    sAttrib.opacity = (sAttrib.opacity * ms->moveOpacity) >> 16;

    bool status = gWindow->glPaint (sAttrib, transform, region, mask);

    return status;
}

void
MoveScreen::updateOpacity ()
{
    moveOpacity = (optionGetOpacity () * OPAQUE) / 100;
}

bool
MoveScreen::registerPaintHandler(compiz::composite::PaintHandler *pHnd)
{
    hasCompositing = true;
    cScreen->registerPaintHandler (pHnd);
    return true;
}

void
MoveScreen::unregisterPaintHandler()
{
    hasCompositing = false;
    cScreen->unregisterPaintHandler ();
}

MoveScreen::MoveScreen (CompScreen *screen) :
    PluginClassHandler<MoveScreen,CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    w (0),
    region (NULL),
    status (RectangleOut),
    releaseButton (0),
    grab (NULL),
    paintRect(false),
    rectX(0),
    rectY(0),
    hasCompositing (false),
    yConstrained (false)
{
    updateOpacity ();

    for (unsigned int i = 0; i < NUM_KEYS; i++)
	key[i] = XKeysymToKeycode (screen->dpy (),
				   XStringToKeysym (mKeys[i].name));

    if (cScreen)
    {
	CompositeScreenInterface::setHandler (cScreen);
	hasCompositing =
	    cScreen->compositingActive ();
    }

    optionSetOpacityNotify (boost::bind (&MoveScreen::updateOpacity, this));

    optionSetInitiateButtonInitiate (moveInitiate);
    optionSetInitiateButtonTerminate (moveTerminate);

    optionSetInitiateKeyInitiate (moveInitiate);
    optionSetInitiateKeyTerminate (moveTerminate);

    ScreenInterface::setHandler (screen);
    GLScreenInterface::setHandler (gScreen);

    gScreen->glPaintOutputSetEnabled (this, false);
}

MoveScreen::~MoveScreen ()
{
    if (region)
	XDestroyRegion (region);
}

bool
MoveScreen::getMovingRectangle (BoxPtr pBox)
{
    MOVE_SCREEN (screen);

    CompWindow *w = ms->w;
    if (!w)
	return false;

    int wX      = w->geometry ().x () - w->border ().left;
    int wY      = w->geometry ().y () - w->border ().top;
    int wWidth  = w->geometry ().widthIncBorders () + w->border ().left + w->border ().right;
    int wHeight = w->geometry ().heightIncBorders () + w->border ().top + w->border ().bottom;

    pBox->x1 = wX + ms->rectX;
    pBox->y1 = wY + ms->rectY;

    pBox->x2 = pBox->x1 + wWidth;
    pBox->y2 = pBox->y1 + wHeight;

    return true;
}

bool
MoveScreen::damageMovingRectangle (BoxPtr pBox)
{
    CompRegion damageRegion;
    int borderWidth;

    if (!cScreen || !pBox)
	return false;

    borderWidth = defaultBorderWidth;

    if (optionGetIncreaseBorderContrast ())
	borderWidth *= biggerBorderWidthMultiplier;

    if (optionGetMode () == MoveOptions::ModeRectangle)
    {
	    CompRect damage (pBox->x1 - borderWidth,
			     pBox->y1 - borderWidth,
			     pBox->x2 - pBox->x1 + borderWidth * 2,
			     pBox->y2 - pBox->y1 + borderWidth * 2);
	    damageRegion += damage;
    }
    else if (optionGetMode () == MoveOptions::ModeOutline)
    {
	    // Top
	    damageRegion += CompRect (pBox->x1 - borderWidth,
				      pBox->y1 - borderWidth,
				      pBox->x2 - pBox->x1 + borderWidth * 2,
				      borderWidth * 2);
	    // Right
	    damageRegion += CompRect (pBox->x2 - borderWidth,
				      pBox->y1 - borderWidth,
				      borderWidth + borderWidth / 2,
				      pBox->y2 - pBox->y1 + borderWidth * 2);
	    // Bottom
	    damageRegion += CompRect (pBox->x1 - borderWidth,
				      pBox->y2 - borderWidth,
				      pBox->x2 - pBox->x1 + borderWidth * 2,
				      borderWidth * 2);
	    // Left
	    damageRegion += CompRect (pBox->x1 - borderWidth,
				      pBox->y1 - borderWidth,
				      borderWidth + borderWidth / 2,
				      pBox->y2 - pBox->y1 + borderWidth * 2);
    }

    if (!damageRegion.isEmpty ())
    {
	cScreen->damageRegion (damageRegion);
	return true;
    }

    return false;
}

bool MoveScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				const GLMatrix &transform,
				const CompRegion &region,
				CompOutput *output,
				unsigned int mask)
{
    bool status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (status && paintRect)
    {
	unsigned short *borderColor = optionGetBorderColor ();
	unsigned short *fillColor = NULL;

	if (optionGetMode() == MoveOptions::ModeRectangle)
	    fillColor = optionGetFillColor ();

	return glPaintMovingRectangle (transform, output, borderColor, fillColor);
    }

    return status;
}

bool
MoveScreen::glPaintMovingRectangle (const GLMatrix &transform,
				    CompOutput *output,
				    unsigned short *borderColor,
				    unsigned short *fillColor)
{
    BoxRec box;
    if (!getMovingRectangle(&box))
	return false;

    const unsigned short MaxUShort = std::numeric_limits <unsigned short>::max ();
    const float MaxUShortFloat = MaxUShort;
    GLVertexBuffer *streamingBuffer = GLVertexBuffer::streamingBuffer ();
    GLMatrix sTransform (transform);
    bool usingAverageColors = false;

    GLfloat vertexData[12];
    GLfloat vertexData2[24];
    GLushort fc[4], bc[4], averageFillColor[4];
    GLint origSrc, origDst;
#ifdef USE_GLES
    GLint origSrcAlpha, origDstAlpha;
#endif

    if (optionGetUseDesktopAverageColor ())
    {
	const unsigned short *averageColor = screen->averageColor ();

	if (averageColor)
	{
	    usingAverageColors = true;
	    borderColor = const_cast<unsigned short *>(averageColor);
	    memcpy (averageFillColor, averageColor, 4 * sizeof (unsigned short));
	    averageFillColor[3] = MaxUShort * 0.6;

	    if (fillColor)
		fillColor = averageFillColor;
	}
    }

    bool blend = optionGetBlend ();

    if (blend && borderColor[3] == MaxUShort)
    {
	if (optionGetMode () == MoveOptions::ModeOutline || fillColor[3] == MaxUShort)
	    blend = false;
    }

    if (blend)
    {
#ifdef USE_GLES
	glGetIntegerv (GL_BLEND_SRC_RGB, &origSrc);
	glGetIntegerv (GL_BLEND_DST_RGB, &origDst);
	glGetIntegerv (GL_BLEND_SRC_ALPHA, &origSrcAlpha);
	glGetIntegerv (GL_BLEND_DST_ALPHA, &origDstAlpha);
#else
	glGetIntegerv (GL_BLEND_SRC, &origSrc);
	glGetIntegerv (GL_BLEND_DST, &origDst);
#endif
    }

    vertexData[0] = box.x1;
    vertexData[1] = box.y1;
    vertexData[2] = 0.0f;
    vertexData[3] = box.x1;
    vertexData[4] = box.y2;
    vertexData[5] = 0.0f;
    vertexData[6] = box.x2;
    vertexData[7] = box.y1;
    vertexData[8] = 0.0f;
    vertexData[9] = box.x2;
    vertexData[10] = box.y2;
    vertexData[11] = 0.0f;

    vertexData2[0] = box.x1;
    vertexData2[1] = box.y1;
    vertexData2[2] = 0.0f;
    vertexData2[3] = box.x1;
    vertexData2[4] = box.y2;
    vertexData2[5] = 0.0f;
    vertexData2[6] = box.x1;
    vertexData2[7] = box.y2;
    vertexData2[8] = 0.0f;
    vertexData2[9] = box.x2;
    vertexData2[10] = box.y2;
    vertexData2[11] = 0.0f;
    vertexData2[12] = box.x2;
    vertexData2[13] = box.y2;
    vertexData2[14] = 0.0f;
    vertexData2[15] = box.x2;
    vertexData2[16] = box.y1;
    vertexData2[17] = 0.0f;
    vertexData2[18] = box.x2;
    vertexData2[19] = box.y1;
    vertexData2[20] = 0.0f;
    vertexData2[21] = box.x1;
    vertexData2[22] = box.y1;
    vertexData2[23] = 0.0f;

    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    if (blend)
    {
	glEnable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    /* fill rectangle */
    if (fillColor)
    {
	fc[3] = blend ? fillColor[3] : 0.85f * MaxUShortFloat;
	fc[0] = fillColor[0] * (unsigned long) fc[3] / MaxUShortFloat;
	fc[1] = fillColor[1] * (unsigned long) fc[3] / MaxUShortFloat;
	fc[2] = fillColor[2] * (unsigned long) fc[3] / MaxUShortFloat;

	streamingBuffer->begin (GL_TRIANGLE_STRIP);
	streamingBuffer->addColors (1, fc);
	streamingBuffer->addVertices (4, &vertexData[0]);
	streamingBuffer->end ();
	streamingBuffer->render (sTransform);
    }

    /* draw outline */
    int borderWidth = defaultBorderWidth;

    if (optionGetIncreaseBorderContrast() || usingAverageColors)
    {
	// Generate a lighter color based on border to create more contrast
	unsigned int averageColorLevel = (borderColor[0] + borderColor[1] + borderColor[2]) / 3;

	float colorMultiplier;
	if (averageColorLevel > MaxUShort * 0.3)
	    colorMultiplier = 0.7; // make it darker
	else
	    colorMultiplier = 2.0; // make it lighter

	bc[3] = borderColor[3];
	bc[0] = MIN(MaxUShortFloat, ((float) borderColor[0]) * colorMultiplier) * bc[3] / MaxUShortFloat;
	bc[1] = MIN(MaxUShortFloat, ((float) borderColor[1]) * colorMultiplier) * bc[3] / MaxUShortFloat;
	bc[2] = MIN(MaxUShortFloat, ((float) borderColor[2]) * colorMultiplier) * bc[3] / MaxUShortFloat;

	if (optionGetIncreaseBorderContrast ())
	{
	    borderWidth *= biggerBorderWidthMultiplier;

	    glLineWidth (borderWidth);
	    streamingBuffer->begin (GL_LINES);
	    streamingBuffer->addVertices (8, &vertexData2[0]);
	    streamingBuffer->addColors (1, bc);
	    streamingBuffer->end ();
	    streamingBuffer->render (sTransform);
	} else if (usingAverageColors) {
	    borderColor = bc;
	}
    }

    bc[3] = blend ? borderColor[3] : MaxUShortFloat;
    bc[0] = borderColor[0] * bc[3] / MaxUShortFloat;
    bc[1] = borderColor[1] * bc[3] / MaxUShortFloat;
    bc[2] = borderColor[2] * bc[3] / MaxUShortFloat;

    glLineWidth (defaultBorderWidth);
    streamingBuffer->begin (GL_LINES);
    streamingBuffer->addColors (1, bc);
    streamingBuffer->addVertices (8, &vertexData2[0]);
    streamingBuffer->end ();
    streamingBuffer->render (sTransform);

    if (blend)
    {
	glDisable (GL_BLEND);
#ifdef USE_GLES
	glBlendFuncSeparate (origSrc, origDst,
			     origSrcAlpha, origDstAlpha);
#else
	glBlendFunc (origSrc, origDst);
#endif
    }

    damageMovingRectangle (&box);

    return true;
}

bool
MovePluginVTable::init ()
{
    if (CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return true;

    return false;
}
