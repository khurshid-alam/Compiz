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
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xatom.h>

#include <compiz-core.h>
#include <decoration.h>
#include "staticswitcher_options.h"

static int SwitchDisplayPrivateIndex;

typedef struct _SwitchDisplay {
    int		    screenPrivateIndex;
    HandleEventProc handleEvent;

    Atom selectWinAtom;
    Atom selectFgColorAtom;
} SwitchDisplay;

typedef enum {
    CurrentViewport = 0,
    AllViewports,
    Panels
} SwitchWindowSelection;

typedef struct _SwitchScreen {
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc	   paintOutput;
    PaintWindowProc        paintWindow;
    DamageWindowRectProc   damageWindowRect;

    Window            popupWindow;
    CompTimeoutHandle popupDelayHandle;

    Window selectedWindow;

    unsigned int previewWidth;
    unsigned int previewHeight;
    unsigned int previewBorder;
    unsigned int xCount;

    int  grabIndex;
    Bool switching;

    int     moreAdjust;
    GLfloat mVelocity;

    CompWindow **windows;
    int        windowsSize;
    int        nWindows;

    float pos;
    float move;

    SwitchWindowSelection selection;

    unsigned int fgColor[4];
} SwitchScreen;

#define ICON_SIZE 64

#define PREVIEWSIZE 150
#define BORDER 10

#define SWITCH_DISPLAY(d) PLUGIN_DISPLAY(d, Switch, s)
#define SWITCH_SCREEN(s) PLUGIN_SCREEN(s, Switch, s)

static void
setSelectedWindowHint (CompScreen *s)
{
    SWITCH_DISPLAY (s->display);
    SWITCH_SCREEN (s);

    XChangeProperty (s->display->display, ss->popupWindow, sd->selectWinAtom,
		     XA_WINDOW, 32, PropModeReplace,
		     (unsigned char *) &ss->selectedWindow, 1);
}

static Bool
isSwitchWin (CompWindow *w)
{
    CompScreen *s = w->screen;

    SWITCH_SCREEN (s);

    if (!w->mapNum || w->attrib.map_state != IsViewable)
    {
	if (staticswitcherGetMinimized (s))
	{
	    if (!w->minimized && !w->inShowDesktopMode && !w->shaded)
		return FALSE;
	}
	else
	{
	    return FALSE;
	}
    }

    if (!(w->inputHint || (w->protocols & CompWindowProtocolTakeFocusMask)))
	return FALSE;

    if (w->attrib.override_redirect)
	return FALSE;

    if (ss->selection == Panels)
    {
	if (!(w->type & (CompWindowTypeDockMask | CompWindowTypeDesktopMask)))
	    return FALSE;
    }
    else
    {
	if (w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	    return FALSE;

	if (w->state & CompWindowStateSkipTaskbarMask)
	    return FALSE;
    }

    if (ss->selection == CurrentViewport)
    {
	if (!w->mapNum || w->attrib.map_state != IsViewable)
	{
	    if (w->serverX + w->width  <= 0    ||
		w->serverY + w->height <= 0    ||
		w->serverX >= w->screen->width ||
		w->serverY >= w->screen->height)
		return FALSE;
	}
	else
	{
	    if (!(*w->screen->focusWindow) (w))
		return FALSE;
	}
    }

    if (!matchEval (staticswitcherGetWindowMatch (s), w))
	return FALSE;

    return TRUE;
}

static void
switchActivateEvent (CompScreen *s,
		     Bool	activating)
{
    CompOption o[2];

    o[0].type = CompOptionTypeInt;
    o[0].name = "root";
    o[0].value.i = s->root;

    o[1].type = CompOptionTypeBool;
    o[1].name = "active";
    o[1].value.b = activating;

    (*s->display->handleCompizEvent) (s->display, "switcher", "activate", o, 2);
}

static int
compareWindows (const void *elem1,
		const void *elem2)
{
    CompWindow *w1 = *((CompWindow **) elem1);
    CompWindow *w2 = *((CompWindow **) elem2);

    if (w1->mapNum && !w2->mapNum)
	return -1;

    if (w2->mapNum && !w1->mapNum)
	return 1;

    return w2->activeNum - w1->activeNum;
}

static void
switchAddWindowToList (CompScreen *s,
		       CompWindow *w)
{
    SWITCH_SCREEN (s);

    if (ss->windowsSize <= ss->nWindows)
    {
	ss->windows = realloc (ss->windows,
			       sizeof (CompWindow *) * (ss->nWindows + 32));
	if (!ss->windows)
	    return;

	ss->windowsSize = ss->nWindows + 32;
    }

    ss->windows[ss->nWindows++] = w;
}

static void
switchUpdatePopupWindow (CompScreen *s,
			 int        count)
{
    unsigned int winWidth, winHeight;
    unsigned int xCount, yCount;
    unsigned int w = PREVIEWSIZE, h = PREVIEWSIZE, b = BORDER;
    XSizeHints xsh;
    int x, y;

    SWITCH_SCREEN (s);

    /* maximum window size is 2/3 of the current output */
    winWidth  = s->outputDev[s->currentOutputDev].width * 2 / 3;
    winHeight = s->outputDev[s->currentOutputDev].height * 2 / 3;

    xCount = winWidth / (w + b);
    yCount = winHeight / (h + b);

    while (xCount * yCount < count)
    {
	/* shrink by 10% until all windows fit */
	w = w * 9 / 10;
	h = h * 9 / 10;
	b = b * 9 / 10;

	xCount = winWidth / (w + b);
	yCount = winHeight / (h + b);
    }

    winWidth = MIN (count, xCount);
    winHeight = (count + xCount - 1) / xCount;

    winWidth = winWidth * w + (winWidth + 1) * b;
    winHeight = winHeight * h + (winHeight + 1) * b;
    ss->xCount = MIN (xCount, count);

    ss->previewWidth = w;
    ss->previewHeight = h;
    ss->previewBorder = b;

    xsh.flags       = PSize | PPosition | PWinGravity;
    xsh.width       = winWidth;
    xsh.height      = winHeight;
    xsh.win_gravity = StaticGravity;

    XSetWMNormalHints (s->display->display, ss->popupWindow, &xsh);

    x = s->outputDev[s->currentOutputDev].region.extents.x1 +
	s->outputDev[s->currentOutputDev].width / 2;
    y = s->outputDev[s->currentOutputDev].region.extents.y1 +
	s->outputDev[s->currentOutputDev].height / 2;

    XMoveResizeWindow (s->display->display, ss->popupWindow,
		       x - winWidth / 2, y - winHeight / 2,
		       winWidth, winHeight);
}

static void
switchUpdateWindowList (CompScreen *s,
			int	   count)
{
    SWITCH_SCREEN (s);

    ss->pos  = 0.0;
    ss->move = 0.0;

    ss->selectedWindow = ss->windows[0]->id;

    if (ss->popupWindow)
	switchUpdatePopupWindow (s, count);
}

static void
switchCreateWindowList (CompScreen *s,
			int	   count)
{
    CompWindow *w;

    SWITCH_SCREEN (s);

    ss->nWindows = 0;

    for (w = s->windows; w; w = w->next)
    {
	if (isSwitchWin (w))
	    switchAddWindowToList (s, w);
    }

    qsort (ss->windows, ss->nWindows, sizeof (CompWindow *), compareWindows);

    switchUpdateWindowList (s, count);
}

static void
switchToWindow (CompScreen *s,
		Bool	   toNext)
{
    CompWindow *w;
    int	       cur, nextIdx;

    SWITCH_SCREEN (s);

    if (!ss->grabIndex)
	return;

    for (cur = 0; cur < ss->nWindows; cur++)
    {
	if (ss->windows[cur]->id == ss->selectedWindow)
	    break;
    }

    if (cur == ss->nWindows)
	return;

    if (toNext)
	nextIdx = (cur + 1) % ss->nWindows;
    else
	nextIdx = (cur + ss->nWindows - 1) % ss->nWindows;

    w = ss->windows[nextIdx];

    if (w)
    {
	Window old = ss->selectedWindow;

	if (ss->selection == AllViewports && staticswitcherGetAutoChangeVp (s))
	{
	    XEvent xev;
	    int	   x, y;

	    defaultViewportForWindow (w, &x, &y);

	    xev.xclient.type = ClientMessage;
	    xev.xclient.display = s->display->display;
	    xev.xclient.format = 32;

	    xev.xclient.message_type = s->display->desktopViewportAtom;
	    xev.xclient.window = s->root;

	    xev.xclient.data.l[0] = x * s->width;
	    xev.xclient.data.l[1] = y * s->height;
	    xev.xclient.data.l[2] = 0;
	    xev.xclient.data.l[3] = 0;
	    xev.xclient.data.l[4] = 0;

	    XSendEvent (s->display->display, s->root, FALSE,
			SubstructureRedirectMask | SubstructureNotifyMask,
			&xev);
	}

	ss->selectedWindow = w->id;

	if (old != w->id)
	{
	    ss->move = nextIdx;

	    ss->moreAdjust = 1;
	}

	if (ss->popupWindow)
	{
	    CompWindow *popup;

	    popup = findWindowAtScreen (s, ss->popupWindow);
	    if (popup)
		addWindowDamage (popup);

	    setSelectedWindowHint (s);
	}

	addWindowDamage (w);

	if (old)
	{
	    w = findWindowAtScreen (s, old);
	    if (w)
		addWindowDamage (w);
	}
    }
}

static int
switchCountWindows (CompScreen *s)
{
    CompWindow *w;
    int	       count = 0;

    for (w = s->windows; w; w = w->next)
	if (isSwitchWin (w))
	    count++;

    return count;
}

static Visual *
findArgbVisual (Display *dpy, int scr)
{
    XVisualInfo		*xvi;
    XVisualInfo		template;
    int			nvi;
    int			i;
    XRenderPictFormat	*format;
    Visual		*visual;

    template.screen = scr;
    template.depth  = 32;
    template.class  = TrueColor;

    xvi = XGetVisualInfo (dpy,
			  VisualScreenMask |
			  VisualDepthMask  |
			  VisualClassMask,
			  &template,
			  &nvi);
    if (!xvi)
	return 0;

    visual = 0;
    for (i = 0; i < nvi; i++)
    {
	format = XRenderFindVisualFormat (dpy, xvi[i].visual);
	if (format->type == PictTypeDirect && format->direct.alphaMask)
	{
	    visual = xvi[i].visual;
	    break;
	}
    }

    XFree (xvi);

    return visual;
}

static Bool
switchShowPopup (void *closure)
{
    CompScreen *s = (CompScreen *) closure;
    CompWindow *w;

    SWITCH_SCREEN (s);

    w = findWindowAtScreen (s, ss->popupWindow);
    if (w && (w->state & CompWindowStateHiddenMask))
    {
	w->hidden = FALSE;
	showWindow (w);
    }
    else
    {
	XMapWindow (s->display->display, ss->popupWindow);
    }

    damageScreen (s);

    ss->popupDelayHandle = 0;

    return FALSE;
}

static void
switchInitiate (CompScreen            *s,
		SwitchWindowSelection selection,
		Bool	              showPopup)
{
    int count;

    SWITCH_SCREEN (s);

    if (otherScreenGrabExist (s, "switcher", "scale", "cube", 0))
	return;

    ss->selection      = selection;
    ss->selectedWindow = None;

    count = switchCountWindows (s);
    if (count < 1)
	return;

    if (!ss->popupWindow && showPopup)
    {
	Display		     *dpy = s->display->display;
	XWMHints	     xwmh;
	Atom		     state[4];
	int		     nState = 0;
	XSetWindowAttributes attr;
	Visual		     *visual;

	visual = findArgbVisual (dpy, s->screenNum);
	if (!visual)
	    return;

	xwmh.flags = InputHint;
	xwmh.input = 0;

	attr.background_pixel = 0;
	attr.border_pixel     = 0;
	attr.colormap	      = XCreateColormap (dpy, s->root, visual,
						 AllocNone);

	ss->popupWindow =
	    XCreateWindow (dpy, s->root, -1, -1, 1, 1, 0,
			   32, InputOutput, visual,
			   CWBackPixel | CWBorderPixel | CWColormap, &attr);

	XSetWMProperties (dpy, ss->popupWindow, NULL, NULL,
			  programArgv, programArgc,
			  NULL, &xwmh, NULL);

	state[nState++] = s->display->winStateAboveAtom;
	state[nState++] = s->display->winStateStickyAtom;
	state[nState++] = s->display->winStateSkipTaskbarAtom;
	state[nState++] = s->display->winStateSkipPagerAtom;

	XChangeProperty (dpy, ss->popupWindow,
			 s->display->winStateAtom,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) state, nState);

	XChangeProperty (dpy, ss->popupWindow,
			 s->display->winTypeAtom,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) &s->display->winTypeUtilAtom, 1);

	setWindowProp (s->display, ss->popupWindow,
		       s->display->winDesktopAtom,
		       0xffffffff);

	setSelectedWindowHint (s);
    }

    if (!ss->grabIndex)
	ss->grabIndex = pushScreenGrab (s, s->invisibleCursor, "switcher");

    if (ss->grabIndex)
    {
	if (!ss->switching)
	{
	    switchCreateWindowList (s, count);

	    if (ss->popupWindow && showPopup)
	    {
		unsigned int delay;

		delay = staticswitcherGetPopupDelay (s) * 1000;
		if (delay)
		{
		    if (ss->popupDelayHandle)
			compRemoveTimeout (ss->popupDelayHandle);

		    ss->popupDelayHandle = compAddTimeout (delay,
							   switchShowPopup, s);
		}
		else
		{
		    switchShowPopup (s);
		}
	    }

	    switchActivateEvent (s, TRUE);
	}

	damageScreen (s);

	ss->switching  = TRUE;
	ss->moreAdjust = 1;
    }
}

static Bool
switchTerminate (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int	         nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    for (s = d->screens; s; s = s->next)
    {
	SWITCH_SCREEN (s);

	if (xid && s->root != xid)
	    continue;

	if (ss->grabIndex)
	{
	    CompWindow *w;

	    if (ss->popupDelayHandle)
	    {
		compRemoveTimeout (ss->popupDelayHandle);
		ss->popupDelayHandle = 0;
	    }

	    if (ss->popupWindow)
	    {
		w = findWindowAtScreen (s, ss->popupWindow);
		if (w && w->managed && w->mapNum)
		{
		    w->hidden = TRUE;
		    hideWindow (w);
		}
		else
		{
		    XUnmapWindow (s->display->display, ss->popupWindow);
		}
	    }

	    ss->switching = FALSE;

	    if (state & CompActionStateCancel)
		ss->selectedWindow = None;

	    if (state && ss->selectedWindow)
	    {
		w = findWindowAtScreen (s, ss->selectedWindow);
		if (w)
		    sendWindowActivationRequest (w->screen, w->id);
	    }

	    removeScreenGrab (s, ss->grabIndex, 0);
	    ss->grabIndex = 0;

	    ss->selectedWindow = None;

	    switchActivateEvent (s, FALSE);
	    setSelectedWindowHint (s);

	    damageScreen (s);
	}
    }

    if (action)
	action->state &= ~(CompActionStateTermKey | CompActionStateTermButton);

    return FALSE;
}

static Bool
switchInitiateCommon (CompDisplay           *d,
		      CompAction            *action,
		      CompActionState       state,
		      CompOption            *option,
		      int                   nOption,
		      SwitchWindowSelection selection,
		      Bool                  showPopup,
		      Bool                  nextWindow)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	SWITCH_SCREEN (s);

	if (!ss->switching)
	{
	    switchInitiate (s, selection, showPopup);

	    if (state & CompActionStateInitKey)
		action->state |= CompActionStateTermKey;

	    if (state & CompActionStateInitButton)
		action->state |= CompActionStateTermButton;

	    if (state & CompActionStateInitEdge)
		action->state |= CompActionStateTermEdge;

	}

	switchToWindow (s, nextWindow);
    }

    return FALSE;
}

static Bool
switchNext (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int	            nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 CurrentViewport, TRUE, TRUE);
}

static Bool
switchPrev (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
	    CompOption      *option,
	    int	            nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 CurrentViewport, TRUE, FALSE);
}

static Bool
switchNextAll (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int	       nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 AllViewports, TRUE, TRUE);
}

static Bool
switchPrevAll (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int	       nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 AllViewports, TRUE, FALSE);
}

static Bool
switchNextNoPopup (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int	           nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 CurrentViewport, FALSE, TRUE);
}

static Bool
switchPrevNoPopup (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int	           nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 CurrentViewport, FALSE, FALSE);
}

static Bool
switchNextPanel (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int	         nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 Panels, FALSE, TRUE);
}

static Bool
switchPrevPanel (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int	         nOption)
{
    return switchInitiateCommon (d, action, state, option, nOption,
				 Panels, FALSE, FALSE);
}

static void
switchWindowRemove (CompDisplay *d,
		    Window	id)
{
    CompWindow *w;

    w = findWindowAtDisplay (d, id);
    if (w)
    {
	Bool   inList = FALSE;
	int    count, j, i = 0;
	Window selected, old;
	CompScreen *s = w->screen;

	SWITCH_SCREEN (s);

	if (isSwitchWin (w))
	    return;

	old = selected = ss->selectedWindow;

	while (i < ss->nWindows)
	{
	    if (ss->windows[i] == w)
	    {
		inList = TRUE;

		if (w->id == selected)
		{
		    if (i < ss->nWindows)
			selected = ss->windows[i + 1]->id;
		    else
			selected = ss->windows[0]->id;
		}

		ss->nWindows--;
		for (j = i; j < ss->nWindows; j++)
		    ss->windows[j] = ss->windows[j + 1];
	    }
	    else
	    {
		i++;
	    }
	}

	if (!inList)
	    return;

	count = ss->nWindows;

	if (ss->nWindows == 0)
	{
	    CompOption o;

	    o.type    = CompOptionTypeInt;
	    o.name    = "root";
	    o.value.i = w->screen->root;

	    switchTerminate (d, NULL, 0, &o, 1);
	    return;
	}

	if (!ss->grabIndex)
	    return;

	switchUpdateWindowList (w->screen, count);

	for (i = 0; i < ss->nWindows; i++)
	{
	    ss->selectedWindow = ss->windows[i]->id;
	    ss->move = ss->pos = i;

	    if (ss->selectedWindow == selected)
		break;
	}

	if (ss->popupWindow)
	{
	    CompWindow *popup;

	    popup = findWindowAtScreen (w->screen, ss->popupWindow);
	    if (popup)
		addWindowDamage (popup);

	    setSelectedWindowHint (w->screen);
	}

	if (old != ss->selectedWindow)
	{
	    addWindowDamage (w);

	    w = findWindowAtScreen (w->screen, old);
	    if (w)
		addWindowDamage (w);

	    ss->moreAdjust = 1;
	}
    }
}

static void
updateForegroundColor (CompScreen *s)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *propData;

    SWITCH_SCREEN (s);
    SWITCH_DISPLAY (s->display);

    if (!ss->popupWindow)
	return;


    result = XGetWindowProperty (s->display->display, ss->popupWindow,
				 sd->selectFgColorAtom, 0L, 4L, FALSE,
				 XA_INTEGER, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	if (n == 3 || n == 4)
	{
	    long *data = (long *) propData;

	    ss->fgColor[0] = MIN (0xffff, data[0]);
	    ss->fgColor[1] = MIN (0xffff, data[1]);
	    ss->fgColor[2] = MIN (0xffff, data[2]);

	    if (n == 4)
		ss->fgColor[3] = MIN (0xffff, data[3]);
	}

	XFree (propData);
    }
    else
    {
	ss->fgColor[0] = 0;
	ss->fgColor[1] = 0;
	ss->fgColor[2] = 0;
	ss->fgColor[3] = 0xffff;
    }
}

static void
switchHandleEvent (CompDisplay *d,
		   XEvent      *event)
{
    CompWindow *w;
    SWITCH_DISPLAY (d);

    UNWRAP (sd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (sd, d, handleEvent, switchHandleEvent);

    switch (event->type) {
    case UnmapNotify:
	switchWindowRemove (d, event->xunmap.window);
	break;
    case DestroyNotify:
	switchWindowRemove (d, event->xdestroywindow.window);
	break;
    case PropertyNotify:
	if (event->xproperty.atom == sd->selectFgColorAtom)
        {
            w = findWindowAtDisplay (d, event->xproperty.window);
            if (w)
            {
		CompScreen *s = w->screen;

		SWITCH_SCREEN (s);

		if (event->xproperty.window == ss->popupWindow)
		    updateForegroundColor (s);
            }
        }

    default:
	break;
    }
}

static int
adjustSwitchVelocity (CompScreen *s)
{
    float dx, adjust, amount;

    SWITCH_SCREEN (s);

    dx = ss->move - ss->pos;
    if (abs (dx) > abs (dx + ss->nWindows))
	dx += ss->nWindows;
    if (abs (dx) > abs (dx - ss->nWindows))
	dx -= ss->nWindows;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    ss->mVelocity = (amount * ss->mVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.001f && fabs (ss->mVelocity) < 0.001f)
    {
	ss->mVelocity = 0.0f;
	return 0;
    }

    return 1;
}

static void
switchPreparePaintScreen (CompScreen *s,
			  int	     msSinceLastPaint)
{
    SWITCH_SCREEN (s);

    if (ss->moreAdjust)
    {
	int   steps;
	float amount, chunk;

	amount = msSinceLastPaint * 0.05f * staticswitcherGetSpeed (s);
	steps  = amount / (0.5f * staticswitcherGetTimestep (s));
	if (!steps) steps = 1;
	chunk  = amount / (float) steps;

	while (steps--)
	{
	    ss->moreAdjust = adjustSwitchVelocity (s);
	    if (!ss->moreAdjust)
	    {
		ss->pos = ss->move;
		break;
	    }

	    ss->pos += ss->mVelocity * chunk;
	    ss->pos = fmod (ss->pos, ss->nWindows);
	    if (ss->pos < 0.0)
		ss->pos += ss->nWindows;
	}
    }

    UNWRAP (ss, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ss, s, preparePaintScreen, switchPreparePaintScreen);
}

static Bool
switchPaintOutput (CompScreen		   *s,
		   const ScreenPaintAttrib *sAttrib,
		   const CompTransform	   *transform,
		   Region		   region,
		   CompOutput		   *output,
		   unsigned int		   mask)
{
    Bool status;

    SWITCH_SCREEN (s);

    if (ss->grabIndex)
    {
	CompWindow    *zoomed;
	CompWindow    *switcher;
	Window	      zoomedAbove = None;
	Bool	      saveDestroyed = FALSE;

	switcher = findWindowAtScreen (s, ss->popupWindow);
	if (switcher)
	{
	    saveDestroyed = switcher->destroyed;
	    switcher->destroyed = TRUE;
	}

	if (staticswitcherGetBringToFront (s))
	{
	    zoomed = findWindowAtScreen (s, ss->selectedWindow);
	    if (zoomed)
	    {
		CompWindow *w;

		for (w = zoomed->prev; w && w->id <= 1; w = w->prev);
		zoomedAbove = (w) ? w->id : None;

		unhookWindowFromScreen (s, zoomed);
		insertWindowIntoScreen (s, zoomed, s->reverseWindows->id);
	    }
	}
	else
	{
	    zoomed = NULL;
	}

	UNWRAP (ss, s, paintOutput);
	status = (*s->paintOutput) (s, sAttrib, transform,
				    region, output, mask);
	WRAP (ss, s, paintOutput, switchPaintOutput);

	if (zoomed)
	{
	    unhookWindowFromScreen (s, zoomed);
	    insertWindowIntoScreen (s, zoomed, zoomedAbove);
	}

	if (switcher)
	{
	    CompTransform sTransform = *transform;

	    switcher->destroyed = saveDestroyed;

	    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	    glPushMatrix ();
	    glLoadMatrixf (sTransform.m);

	    if (!switcher->destroyed			 &&
		switcher->attrib.map_state == IsViewable &&
		switcher->damaged)
	    {
		(*s->paintWindow) (switcher, &switcher->paint, &sTransform,
				   &infiniteRegion, 0);
	    }

	    glPopMatrix ();
	}
    }
    else
    {
	UNWRAP (ss, s, paintOutput);
	status = (*s->paintOutput) (s, sAttrib, transform, region, output,
				    mask);
	WRAP (ss, s, paintOutput, switchPaintOutput);
    }

    return status;
}

static void
switchDonePaintScreen (CompScreen *s)
{
    SWITCH_SCREEN (s);

    if (ss->grabIndex && ss->moreAdjust)
    {
	CompWindow *w;

	w = findWindowAtScreen (s, ss->popupWindow);
	if (w)
	    addWindowDamage (w);
    }

    UNWRAP (ss, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ss, s, donePaintScreen, switchDonePaintScreen);
}

static void
switchPaintThumb (CompWindow		  *w,
		  const WindowPaintAttrib *attrib,
		  const CompTransform	  *transform,
		  unsigned int		  mask,
		  int			  x,
		  int			  y)
{
    CompScreen *s = w->screen;
    WindowPaintAttrib sAttrib = *attrib;
    int		      wx, wy;
    float	      width, height;
    CompIcon	      *icon = NULL;

    SWITCH_SCREEN (s);

    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

    if (w->mapNum)
    {
	if (!w->texture->pixmap && !w->bindFailed)
	    bindWindow (w);
    }

    if (w->texture->pixmap)
    {
	AddWindowGeometryProc oldAddWindowGeometry;
	FragmentAttrib	      fragment;
	CompTransform	      wTransform = *transform;
	int		      ww, wh;

	width  = ss->previewWidth;
	height = ss->previewHeight;

	ww = w->width  + w->input.left + w->input.right;
	wh = w->height + w->input.top  + w->input.bottom;

	if (ww > width)
	    sAttrib.xScale = width / ww;
	else
	    sAttrib.xScale = 1.0f;

	if (wh > height)
	    sAttrib.yScale = height / wh;
	else
	    sAttrib.yScale = 1.0f;

	if (sAttrib.xScale < sAttrib.yScale)
	    sAttrib.yScale = sAttrib.xScale;
	else
	    sAttrib.xScale = sAttrib.yScale;

	width  = ww * sAttrib.xScale;
	height = wh * sAttrib.yScale;

	wx = x + (ss->previewWidth / 2) - (width / 2);
	wy = y + (ss->previewHeight / 2) - (height / 2);

#if 0
	if (w->id != ss->selectedWindow)
	{
	    sAttrib.brightness /= 2;
	    sAttrib.saturation /= 2;
	}
#endif

	sAttrib.xTranslate = wx - w->attrib.x + w->input.left * sAttrib.xScale;
	sAttrib.yTranslate = wy - w->attrib.y + w->input.top  * sAttrib.yScale;

	initFragmentAttrib (&fragment, &sAttrib);

	if (w->alpha || fragment.opacity != OPAQUE)
	    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	matrixTranslate (&wTransform, w->attrib.x, w->attrib.y, 0.0f);
	matrixScale (&wTransform, sAttrib.xScale, sAttrib.yScale, 1.0f);
	matrixTranslate (&wTransform,
			 sAttrib.xTranslate / sAttrib.xScale - w->attrib.x,
			 sAttrib.yTranslate / sAttrib.yScale - w->attrib.y,
			 0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.m);

	/* XXX: replacing the addWindowGeometry function like this is
	   very ugly but necessary until the vertex stage has been made
	   fully pluggable. */
	oldAddWindowGeometry = w->screen->addWindowGeometry;
	w->screen->addWindowGeometry = addWindowGeometry;
	(w->screen->drawWindow) (w, &wTransform, &fragment, &infiniteRegion,
				 mask);
	w->screen->addWindowGeometry = oldAddWindowGeometry;

	glPopMatrix ();

	if (staticswitcherGetIcon (s))
	{
	    icon = getWindowIcon (w, ICON_SIZE, ICON_SIZE);
	    if (icon)
	    {
		sAttrib.xScale = (float) ss->previewWidth / PREVIEWSIZE;
		sAttrib.yScale = sAttrib.xScale;

		wx = x + ss->previewWidth - (sAttrib.xScale * icon->width);
		wy = y + ss->previewHeight - (sAttrib.yScale * icon->height);
	    }
	}
    }
    else
    {
	width  = ss->previewWidth * 3 / 4;
	height = ss->previewHeight * 3 / 4;

	icon = getWindowIcon (w, width, height);
	if (!icon)
	    icon = w->screen->defaultIcon;

	if (icon)
	{
	    int iw, ih;

	    iw = width;
	    ih = height;

	    if (icon->width < (iw >> 1))
		sAttrib.xScale = (iw / icon->width);
	    else
		sAttrib.xScale = 1.0f;

	    if (icon->height < (ih >> 1))
		sAttrib.yScale = (ih / icon->height);
	    else
		sAttrib.yScale = 1.0f;

	    if (sAttrib.xScale < sAttrib.yScale)
		sAttrib.yScale = sAttrib.xScale;
	    else
		sAttrib.xScale = sAttrib.yScale;

	    width  = icon->width  * sAttrib.xScale;
	    height = icon->height * sAttrib.yScale;

	    wx = x + (ss->previewWidth / 2) - (width / 2);
	    wy = y + (ss->previewHeight / 2) - (height / 2);
	}
    }

    if (icon && (icon->texture.name || iconToTexture (w->screen, icon)))
    {
	REGION     iconReg;
	CompMatrix matrix;

	mask |= PAINT_WINDOW_BLEND_MASK;

	iconReg.rects    = &iconReg.extents;
	iconReg.numRects = 1;

	iconReg.extents.x1 = w->attrib.x;
	iconReg.extents.y1 = w->attrib.y;
	iconReg.extents.x2 = w->attrib.x + icon->width;
	iconReg.extents.y2 = w->attrib.y + icon->height;

	matrix = icon->texture.matrix;
	matrix.x0 -= (w->attrib.x * icon->texture.matrix.xx);
	matrix.y0 -= (w->attrib.y * icon->texture.matrix.yy);

	sAttrib.xTranslate = wx - w->attrib.x;
	sAttrib.yTranslate = wy - w->attrib.y;

	w->vCount = w->indexCount = 0;
	addWindowGeometry (w, &matrix, 1, &iconReg, &infiniteRegion);
	if (w->vCount)
	{
	    FragmentAttrib fragment;
	    CompTransform  wTransform = *transform;

	    initFragmentAttrib (&fragment, &sAttrib);

	    matrixTranslate (&wTransform, w->attrib.x, w->attrib.y, 0.0f);
	    matrixScale (&wTransform, sAttrib.xScale, sAttrib.yScale, 1.0f);
	    matrixTranslate (&wTransform,
			     sAttrib.xTranslate / sAttrib.xScale - w->attrib.x,
			     sAttrib.yTranslate / sAttrib.yScale - w->attrib.y,
			     0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.m);

	    (*w->screen->drawWindowTexture) (w,
					     &icon->texture, &fragment,
					     mask);

	    glPopMatrix ();
	}
    }
}

static void
switchPaintSelectionRect (SwitchScreen *ss,
			  int          x,
			  int          y,
			  float        dx,
			  float        dy,
			  unsigned int opacity)
{
    int            i;
    float          color[4], op;
    unsigned int   w, h;

    w = ss->previewWidth + ss->previewBorder;
    h = ss->previewHeight + ss->previewBorder;

    glEnable (GL_BLEND);

    if (dx > ss->xCount - 1)
	op = 1.0 - MIN (1.0, dx - (ss->xCount - 1));
    else if (dx + (dy * ss->xCount) > ss->nWindows - 1)
	op = 1.0 - MIN (1.0, dx - (ss->nWindows - 1 - (dy * ss->xCount)));
    else if (dx < 0.0)
	op = 1.0 + MAX (-1.0, dx);
    else
	op = 1.0;

    for (i = 0; i < 4; i++)
	color[i] = (float)ss->fgColor[i] * opacity * op / 0xffffffff;

    glColor4fv (color);
    glPushMatrix ();
    glTranslatef (x + ss->previewBorder / 2 + (dx * w),
		  y + ss->previewBorder / 2 + (dy * h), 0.0f);

    glBegin (GL_QUADS);
    glVertex2i (-1, -1);
    glVertex2i (-1, 1);
    glVertex2i (w + 2, 1);
    glVertex2i (w + 2, -1);
    glVertex2i (-1, h);
    glVertex2i (-1, h + 2);
    glVertex2i (w + 1, h + 2);
    glVertex2i (w + 1, h);
    glVertex2i (-1, 1);
    glVertex2i (-1, h);
    glVertex2i (1, h);
    glVertex2i (1, 1);
    glVertex2i (w, 1);
    glVertex2i (w, h);
    glVertex2i (w + 2, h);
    glVertex2i (w + 2, 1);
    glEnd ();

    glPopMatrix ();
    glColor4usv (defaultColor);
    glDisable (GL_BLEND);
}

static Bool
switchPaintWindow (CompWindow		   *w,
		   const WindowPaintAttrib *attrib,
		   const CompTransform	   *transform,
		   Region		   region,
		   unsigned int		   mask)
{
    CompScreen *s = w->screen;
    Bool       status;

    SWITCH_SCREEN (s);

    if (w->id == ss->popupWindow)
    {
	GLenum         filter;
	int            x, y, offX, i;
	float          px, py, pos;

	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
	    return FALSE;

	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ss, s, paintWindow, switchPaintWindow);

	if (!(mask & PAINT_WINDOW_TRANSFORMED_MASK) && region->numRects == 0)
	    return TRUE;

	filter = s->display->textureFilter;

	if (staticswitcherGetMipmap (s))
	    s->display->textureFilter = GL_LINEAR_MIPMAP_LINEAR;

	glPushAttrib (GL_SCISSOR_BIT);

	glEnable (GL_SCISSOR_TEST);
	glScissor (w->attrib.x, 0, w->width, w->screen->height);

	for (i = 0; i < ss->nWindows; i++)
	{
	    x = i % ss->xCount;
	    x = x * ss->previewWidth + (x + 1) * ss->previewBorder;
	    y = (i / ss->xCount);
	    if (ss->nWindows - (y * ss->xCount) < ss->xCount)
		offX = (ss->xCount - ss->nWindows + (y * ss->xCount)) *
		       (ss->previewWidth + ss->previewBorder) / 2;
	    else
		offX = 0;
	    y = y * ss->previewHeight + (y + 1) * ss->previewBorder;

	    switchPaintThumb (ss->windows[i], &w->lastPaint, transform,
			      mask, offX + x + w->attrib.x, y + w->attrib.y);
	}

	s->display->textureFilter = filter;

	pos = fmod (ss->pos, ss->nWindows);
	px  = fmod (pos, ss->xCount);
	py  = floor (pos / ss->xCount);

	y = py;
	if (ss->nWindows - (y * ss->xCount) < ss->xCount)
	    offX = (ss->xCount - ss->nWindows + (y * ss->xCount)) *
		   (ss->previewWidth + ss->previewBorder) / 2;
	else
	    offX = 0;

	if (pos > ss->nWindows - 1)
	{
	    px = fmod (pos - ss->nWindows, ss->xCount);
	    switchPaintSelectionRect (ss, w->attrib.x, w->attrib.y, px, 0.0,
				      w->lastPaint.opacity);

	    px = fmod (pos, ss->xCount);

	    switchPaintSelectionRect (ss, w->attrib.x + offX, w->attrib.y,
				      px, py, w->lastPaint.opacity);
	}
	if (px > ss->xCount - 1)
	{
	    switchPaintSelectionRect (ss, w->attrib.x, w->attrib.y, px, py,
				      w->lastPaint.opacity);
	    py = fmod (py + 1, ceil ((double) ss->nWindows / ss->xCount));
	    y = py;
	    if (ss->nWindows - (y * ss->xCount) < ss->xCount)
		offX = (ss->xCount - ss->nWindows + (y * ss->xCount)) *
		       (ss->previewWidth + ss->previewBorder) / 2;
	    else
		offX = 0;
	    switchPaintSelectionRect (ss, w->attrib.x + offX, w->attrib.y,
				      px - ss->xCount, py,
				      w->lastPaint.opacity);
	}
	else
	{
	    switchPaintSelectionRect (ss, w->attrib.x + offX, w->attrib.y,
				      px, py, w->lastPaint.opacity);
	}
	glDisable (GL_SCISSOR_TEST);
	glPopAttrib ();
    }
    else if (ss->switching && !ss->popupDelayHandle &&
	     (w->id != ss->selectedWindow))
    {
	WindowPaintAttrib sAttrib = *attrib;
	GLuint            value;

	value = staticswitcherGetSaturation (s);
	if (value != 100)
	    sAttrib.saturation = sAttrib.saturation * value / 100;

	value = staticswitcherGetBrightness (s);
	if (value != 100)
	    sAttrib.brightness = sAttrib.brightness * value / 100;

	if (w->wmType & ~(CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	{
	    value = staticswitcherGetOpacity (s);
	    if (value != 100)
		sAttrib.opacity = sAttrib.opacity * value / 100;
	}

	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, &sAttrib, transform, region, mask);
	WRAP (ss, s, paintWindow, switchPaintWindow);
    }
    else
    {
	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ss, s, paintWindow, switchPaintWindow);
    }

    return status;
}

static Bool
switchDamageWindowRect (CompWindow *w,
			Bool	   initial,
			BoxPtr     rect)
{
    CompScreen *s = w->screen;
    Bool status;

    SWITCH_SCREEN (s);

    if (ss->grabIndex)
    {
	CompWindow *popup;
	int	   i;

	for (i = 0; i < ss->nWindows; i++)
	{
	    if (ss->windows[i] == w)
	    {
		popup = findWindowAtScreen (s, ss->popupWindow);
		if (popup)
		    addWindowDamage (popup);

		break;
	    }
	}
    }

    UNWRAP (ss, s, damageWindowRect);
    status = (*s->damageWindowRect) (w, initial, rect);
    WRAP (ss, s, damageWindowRect, switchDamageWindowRect);

    return status;
}

static Bool
switchInitDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    SwitchDisplay *sd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    sd = malloc (sizeof (SwitchDisplay));
    if (!sd)
	return FALSE;

    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (sd->screenPrivateIndex < 0)
    {
	free (sd);
	return FALSE;
    }

    staticswitcherSetNextButtonInitiate (d, switchNext);
    staticswitcherSetNextButtonTerminate (d, switchTerminate);
    staticswitcherSetNextKeyInitiate (d, switchNext);
    staticswitcherSetNextKeyTerminate (d, switchTerminate);
    staticswitcherSetPrevButtonInitiate (d, switchPrev);
    staticswitcherSetPrevButtonTerminate (d, switchTerminate);
    staticswitcherSetPrevKeyInitiate (d, switchPrev);
    staticswitcherSetPrevKeyTerminate (d, switchTerminate);
    staticswitcherSetNextAllButtonInitiate (d, switchNextAll);
    staticswitcherSetNextAllButtonTerminate (d, switchTerminate);
    staticswitcherSetNextAllKeyInitiate (d, switchNextAll);
    staticswitcherSetNextAllKeyTerminate (d, switchTerminate);
    staticswitcherSetPrevAllButtonInitiate (d, switchPrevAll);
    staticswitcherSetPrevAllButtonTerminate (d, switchTerminate);
    staticswitcherSetPrevAllKeyInitiate (d, switchPrevAll);
    staticswitcherSetPrevAllKeyTerminate (d, switchTerminate);
    staticswitcherSetNextNoPopupButtonInitiate (d, switchNextNoPopup);
    staticswitcherSetNextNoPopupButtonTerminate (d, switchTerminate);
    staticswitcherSetNextNoPopupKeyInitiate (d, switchNextNoPopup);
    staticswitcherSetNextNoPopupKeyTerminate (d, switchTerminate);
    staticswitcherSetPrevNoPopupButtonInitiate (d, switchPrevNoPopup);
    staticswitcherSetPrevNoPopupButtonTerminate (d, switchTerminate);
    staticswitcherSetPrevNoPopupKeyInitiate (d, switchPrevNoPopup);
    staticswitcherSetPrevNoPopupKeyTerminate (d, switchTerminate);
    staticswitcherSetNextPanelButtonInitiate (d, switchNextPanel);
    staticswitcherSetNextPanelButtonTerminate (d, switchTerminate);
    staticswitcherSetNextPanelKeyInitiate (d, switchNextPanel);
    staticswitcherSetNextPanelKeyTerminate (d, switchTerminate);
    staticswitcherSetPrevPanelButtonInitiate (d, switchPrevPanel);
    staticswitcherSetPrevPanelButtonTerminate (d, switchTerminate);
    staticswitcherSetPrevPanelKeyInitiate (d, switchPrevPanel);
    staticswitcherSetPrevPanelKeyTerminate (d, switchTerminate);

    sd->selectWinAtom     = XInternAtom (d->display,
					 DECOR_SWITCH_WINDOW_ATOM_NAME, 0);
    sd->selectFgColorAtom =
	XInternAtom (d->display, DECOR_SWITCH_FOREGROUND_COLOR_ATOM_NAME, 0);

    WRAP (sd, d, handleEvent, switchHandleEvent);

    d->base.privates[SwitchDisplayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
switchFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    SWITCH_DISPLAY (d);

    freeScreenPrivateIndex (d, sd->screenPrivateIndex);

    UNWRAP (sd, d, handleEvent);

    free (sd);
}

static Bool
switchInitScreen (CompPlugin *p,
		  CompScreen *s)
{
    SwitchScreen *ss;

    SWITCH_DISPLAY (s->display);

    ss = malloc (sizeof (SwitchScreen));
    if (!ss)
	return FALSE;

    ss->popupWindow      = None;
    ss->popupDelayHandle = 0;

    ss->selectedWindow = None;

    ss->windows     = 0;
    ss->nWindows    = 0;
    ss->windowsSize = 0;

    ss->pos  = 0;
    ss->move = 0;

    ss->switching = FALSE;
    ss->grabIndex = 0;

    ss->moreAdjust = 0;
    ss->mVelocity  = 0.0f;

    ss->selection = CurrentViewport;

    ss->fgColor[0] = 0;
    ss->fgColor[1] = 0;
    ss->fgColor[2] = 0;
    ss->fgColor[3] = 0xffff;

    WRAP (ss, s, preparePaintScreen, switchPreparePaintScreen);
    WRAP (ss, s, donePaintScreen, switchDonePaintScreen);
    WRAP (ss, s, paintOutput, switchPaintOutput);
    WRAP (ss, s, paintWindow, switchPaintWindow);
    WRAP (ss, s, damageWindowRect, switchDamageWindowRect);

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    return TRUE;
}

static void
switchFiniScreen (CompPlugin *p,
		  CompScreen *s)
{
    SWITCH_SCREEN (s);

    UNWRAP (ss, s, preparePaintScreen);
    UNWRAP (ss, s, donePaintScreen);
    UNWRAP (ss, s, paintOutput);
    UNWRAP (ss, s, paintWindow);
    UNWRAP (ss, s, damageWindowRect);

    if (ss->popupDelayHandle)
	compRemoveTimeout (ss->popupDelayHandle);

    if (ss->popupWindow)
	XDestroyWindow (s->display->display, ss->popupWindow);

    if (ss->windows)
	free (ss->windows);

    free (ss);
}

static CompBool
switchInitObject (CompPlugin *p,
		  CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) switchInitDisplay,
	(InitPluginObjectProc) switchInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
switchFiniObject (CompPlugin *p,
		  CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) switchFiniDisplay,
	(FiniPluginObjectProc) switchFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
switchInit (CompPlugin *p)
{
    SwitchDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (SwitchDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
switchFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (SwitchDisplayPrivateIndex);
}

CompPluginVTable switchVTable = {
    "staticswitcher",
    0,
    switchInit,
    switchFini,
    switchInitObject,
    switchFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &switchVTable;
}
