/*
 *
 * Compiz scale plugin addon plugin
 *
 * scaleaddon.cpp
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Organic scale mode taken from Beryl's scale.c, written by
 * Copyright : (C) 2006 Diogo Ferreira
 * E-mail    : diogo@underdev.org
 *
 * Ported to Compiz 0.9 by:
 * Copyright : (C) 2009 by Sam Spilsbury
 * E-mail    : smspillaz@gmail.com
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
 *
 */

#include "scaleaddon.h"

COMPIZ_PLUGIN_20090315 (scaleaddon, ScaleAddonPluginVTable);

bool textAvailable;

void
ScaleAddonWindow::freeTitle ()
{
    CompText empty;

    text = empty;

}

void
ScaleAddonWindow::renderTitle ()
{
    CompText::Attrib            attrib;
    float                       scale;

    ADDON_SCREEN (screen);

    freeTitle ();

    if (!textAvailable)
	return;

    if (!sWindow->hasSlot ())
	return;

    if (as->optionGetWindowTitle () ==
				        ScaleaddonOptions::WindowTitleNoDisplay)
	return;

    if (as->optionGetWindowTitle () ==
			  ScaleaddonOptions::WindowTitleHighlightedWindowOnly &&
			  as->highlightedWindow != window->id ())
    {
	return;
    }

    scale = sWindow->getSlot ().scale;
    attrib.maxWidth = window->width () * scale;
    attrib.maxHeight = window->height () * scale;

    attrib.family = "Sans";
    attrib.size = as->optionGetTitleSize ();
    attrib.color[0] = as->optionGetFontColorRed ();
    attrib.color[1] = as->optionGetFontColorGreen ();
    attrib.color[2] = as->optionGetFontColorBlue ();
    attrib.color[3] = as->optionGetFontColorAlpha ();

    attrib.flags = CompText::WithBackground | CompText::Ellipsized;
    if (as->optionGetTitleBold ())
	attrib.flags |= CompText::StyleBold;

    attrib.bgHMargin = as->optionGetBorderSize ();
    attrib.bgVMargin = as->optionGetBorderSize ();
    attrib.bgColor[0] = as->optionGetBackColorRed ();
    attrib.bgColor[1] = as->optionGetBackColorGreen ();
    attrib.bgColor[2] = as->optionGetBackColorBlue ();
    attrib.bgColor[3] = as->optionGetBackColorAlpha ();

    text.renderWindowTitle (window->id (), as->sScreen->getType () == ScaleTypeAll, attrib);
}

void
ScaleAddonWindow::drawTitle ()
{
    float      x, y, width, height;
    ScalePosition pos = sWindow->getCurrentPosition ();

    width = text.getWidth ();
    height = text.getHeight ();

    x = pos.x () + window->x () + ((WIN_W (window) * pos.scale) / 2) - (width / 2);
    y = pos.y () + window->y () + ((WIN_H (window) * pos.scale) / 2) - (height / 2);

    text.draw (floor (x), floor (y), 1.0f);
}

void
ScaleAddonWindow::drawHighlight ()
{
    GLboolean  wasBlend;
    GLint      oldBlendSrc, oldBlendDst;
    float      x, y, width, height;
    ScalePosition pos = sWindow->getCurrentPosition ();

    ADDON_SCREEN (screen);

    if (rescaled)
	return;

    x      = pos.x () + window->x () - (window->input ().left * sWindow->getCurrentPosition ().scale);
    y      = pos.y () + window->y () - (window->input ().top * sWindow->getCurrentPosition ().scale);
    width  = WIN_W (window) * sWindow->getCurrentPosition ().scale;
    height = WIN_H (window) * sWindow->getCurrentPosition ().scale;

    /* we use a poor replacement for roundf()
     * (available in C99 only) here */
    x = floor (x + 0.5f);
    y = floor (y + 0.5f);

    wasBlend = glIsEnabled (GL_BLEND);
    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4us (as->optionGetHighlightColorRed (),
		as->optionGetHighlightColorGreen (),
		as->optionGetHighlightColorBlue (),
		as->optionGetHighlightColorAlpha ());

    glRectf (x, y + height, x + width, y);

    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

void
ScaleAddonScreen::checkWindowHighlight ()
{
    if (highlightedWindow != lastHighlightedWindow)
    {
	CompWindow *w;

	w = screen->findWindow (highlightedWindow);
	if (w)
	{
	    ADDON_WINDOW (w);
	    aw->renderTitle ();
	    aw->cWindow->addDamage ();
	}

	w = screen->findWindow (lastHighlightedWindow);
	if (w)
	{
	    ADDON_WINDOW (w);
	    aw->renderTitle ();
	    aw->cWindow->addDamage (w);
	}

	lastHighlightedWindow = highlightedWindow;
    }
}

bool
ScaleAddonScreen::closeWindow (CompAction         *action,
			       CompAction::State  state,
			       CompOption::Vector options)
{
    CompWindow *w;

    if (!sScreen->hasGrab ())
        return false;

    w = screen->findWindow (highlightedWindow);
    if (w)
    {
        w->close (screen->getCurrentTime ());
        return true;
    }

    return true;
}

bool
ScaleAddonScreen::pullWindow (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector options)
{
    CompWindow *w;

    if (!sScreen->hasGrab ())
        return false;

    w = screen->findWindow (highlightedWindow);
    if (w)
    {
        int x, y, vx, vy;
	CompPoint vp;

        vp = w->defaultViewport ();

	vx = vp.x ();
	vy = vp.y ();

        x = w->x () + (screen->vp ().x () - vx) * screen->width ();
        y = w->y () + (screen->vp ().y () - vy) * screen->height ();

        if (optionGetConstrainPullToScreen ())
        {
            CompRect            workArea;
	    CompWindowExtents   extents;

	    workArea = screen->outputDevs ()[w->outputDevice ()].workArea ();

	    extents.left   = x - w->input ().left;
	    extents.right  = x + w->width () + w->input ().right;
	    extents.top    = y - w->input ().top;
	    extents.bottom = y + w->height () + w->input ().bottom;

	    if (extents.left < workArea.x ())
	        x += workArea.x () - extents.left;
	    else if (extents.right > workArea.x () + workArea.width ())
	        x += workArea.x () + workArea.width () - extents.right;

	    if (extents.top < workArea.y ())
	        y += workArea.y () - extents.top;
	    else if (extents.bottom > workArea.y () + workArea.height ())
	        y += workArea.y () + workArea.height () - extents.bottom;
        }

        if (x != w->x () || y != w->y ())
        {
	    ADDON_WINDOW (w);
	    ScalePosition pos;

	    w->moveToViewportPosition (x, y, true);

	    /* Select this window when ending scale */
	    aw->sWindow->scaleSelectWindow ();

	    /* stop scaled window dissapearing */
	    pos.setX (aw->sWindow->getCurrentPosition ().x () - (screen->vp ().x () - vx) * screen->width ());
	    pos.setY (aw->sWindow->getCurrentPosition ().y () -(screen->vp ().y () - vy) * screen->height ());

	    if (optionGetExitAfterPull ())
	    {
	        CompOption         opt, o1 ("root", CompOption::TypeInt);
	        CompAction         action;
	        CompOption::Vector o;
		CompOption::Value  root = CompOption::Value ((int) screen->root ());

	        opt = *(CompOption::findOption (sScreen->getOptions (),
					       "initiate_key", 0));
	        action = opt.value ().action ();

	        o1.set (root);

		o.push_back (o1);

	        if (action.terminate ())
		    action.terminate () (&action, 0, o);
	    }
	    else
	    {
	        /* provide a simple animation */
	        aw->cWindow->addDamage ();

	        pos.setX (aw->sWindow->getCurrentPosition ().x () -  (aw->sWindow->getSlot ().x2 () - aw->sWindow->getSlot ().x1 ()) / 20);
	        pos.setY (aw->sWindow->getCurrentPosition ().y () - (aw->sWindow->getSlot ().y2 () - aw->sWindow->getSlot ().y1 ()) / 20);
	        pos.scale = aw->sWindow->getCurrentPosition ().scale * 1.1f;

		aw->sWindow->setCurrentPosition (pos);

	        aw->cWindow->addDamage ();
	    }
	
	    return true;
        }
    }

    return true;
}

bool
ScaleAddonScreen::zoomWindow (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector options)
{
    CompWindow *w;

    if (!sScreen->hasGrab ())
        return false;

    w = screen->findWindow (highlightedWindow);
    if (w)
    {
        ADDON_WINDOW (w);

        XRectangle outputRect;
        BOX        outputBox;
        int        head;

        if (!aw->sWindow->hasSlot ())
	    return false;

        head      = screen->outputDeviceForPoint ( aw->sWindow->getSlot ().x1 (),
						   aw->sWindow->getSlot ().y1 ());
        outputBox = screen->outputDevs ()[head].region ()->extents;

        outputRect.x      = outputBox.x1;
        outputRect.y      = outputBox.y1;
        outputRect.width  = outputBox.x2 - outputBox.x1;
        outputRect.height = outputBox.y2 - outputBox.y1;

        /* damage old rect */
        aw->cWindow->addDamage ();

        if (!aw->rescaled)
        {
	    ScaleSlot slot;
	    int x1, x2, y1, y2;

	    aw->oldAbove = w->next;
	    w->raise ();

	    /* backup old values */
	    aw->origSlot = aw->sWindow->getSlot ();

	    aw->rescaled = true;

	    x1 = (outputRect.width / 2) - (WIN_W(w) / 2) +
		           w->input ().left + outputRect.x;
	    y1 = (outputRect.height / 2) - (WIN_H(w) / 2) +
		           w->input ().top + outputRect.y;
	    x2 = aw->sWindow->getSlot ().x1 () + WIN_W (w);
	    y2 = aw->sWindow->getSlot ().y1 () + WIN_H (w);
	    slot.scale = 1.0f;
	    slot.setGeometry (x1, y1, x2 - x1, y2 - y1);

	    aw->sWindow->setSlot (slot);

        }
        else
        {
	    if (aw->oldAbove)
	        w->restackBelow (aw->oldAbove);

	    aw->rescaled = false;
	    aw->sWindow->setSlot (aw->origSlot);
        }

        /* slot size may have changed, so
         * update window title */
        aw->renderTitle ();

        aw->cWindow->addDamage ();

        return true;
    }

    return true;
}

void
ScaleAddonScreen::handleEvent (XEvent *event)
{
    screen->handleEvent (event);

    switch (event->type)
    {
    case PropertyNotify:
	{
	    if (event->xproperty.atom == XA_WM_NAME)
	    {
		CompWindow *w;

		w = screen->findWindow (event->xproperty.window);
		if (w)
		{
		    ADDON_WINDOW (w);
		    if (sScreen->hasGrab ())
		    {
			aw->renderTitle ();
			aw->cWindow->addDamage ();
		    }
		}
	    }
	}
	break;
    case MotionNotify:
	{
	    if (sScreen->hasGrab ())
	    {
		highlightedWindow = sScreen->getHoveredWindow ();
		checkWindowHighlight ();
	    }
	}
	break;
    default:
	break;
    }
}

void
ScaleAddonWindow::scalePaintDecoration (const GLWindowPaintAttrib &attrib,
				        const GLMatrix	     	  &transform,
					const CompRegion          &region,
					unsigned int		  mask)
{
    ADDON_SCREEN (screen);

    sWindow->scalePaintDecoration (attrib, transform, region, mask);

    if ((as->sScreen->getState () == ScaleScreen::Wait) || (as->sScreen->getState () == ScaleScreen::Out))
    {
	if (as->optionGetWindowHighlight ())
	{
	    if (window->id () == as->highlightedWindow)
		drawHighlight ();
	}

	if (textAvailable)
	    drawTitle ();
    }
}

void
ScaleAddonWindow::scaleSelectWindow ()
{
    ADDON_SCREEN (screen);

    as->highlightedWindow     = window->id ();

    as->checkWindowHighlight ();

    sWindow->scaleSelectWindow ();
}

void
ScaleAddonScreen::donePaint ()
{
    if (sScreen->getState () != ScaleScreen::Idle &&
	lastState == ScaleScreen::Idle)
    {
	foreach (CompWindow *w, screen->windows ())
	    ScaleAddonWindow::get (w)->renderTitle ();
    }
    else if (sScreen->getState () == ScaleScreen::Idle &&
	     lastState != ScaleScreen::Idle)
    {
	foreach (CompWindow *w, screen->windows ())
	    ScaleAddonWindow::get (w)->freeTitle ();
    }

    if (sScreen->getState () == ScaleScreen::Out &&
	lastState != ScaleScreen::Out)
    {
	lastHighlightedWindow = None;
	checkWindowHighlight ();
    }

    lastState = sScreen->getState ();

    cScreen->donePaint ();
}

void
ScaleAddonScreen::handleCompizEvent (const char  *pluginName,
			     	     const char  *eventName,
			     	     CompOption::Vector  &options)
{
    screen->handleCompizEvent (pluginName, eventName, options);

    if ((strcmp (pluginName, "scale") == 0) &&
	(strcmp (eventName, "activate") == 0))
    {
	bool activated = CompOption::getBoolOptionNamed (options, "active", false);

	if (activated)
	{
	    screen->addAction (&optionGetCloseKey ());
	    screen->addAction (&optionGetZoomKey ());
	    screen->addAction (&optionGetPullKey ());
	    screen->addAction (&optionGetCloseButton ());
	    screen->addAction (&optionGetZoomButton ());
	    screen->addAction (&optionGetPullButton ());

	    /* TODO: or better
	       ad->highlightedWindow     = sd->selectedWindow;
	       here? do we want to show up the highlight without
	       mouse move initially? */

	    highlightedWindow     = None;
	    lastHighlightedWindow = None;
	    checkWindowHighlight ();
	}
	else
	{
	    foreach (CompWindow *w, screen->windows ())
	    {
		ADDON_WINDOW (w);
		aw->rescaled = false;
	    }

	    screen->removeAction (&optionGetCloseKey ());
	    screen->removeAction (&optionGetZoomKey ());
	    screen->removeAction (&optionGetPullKey ());
	    screen->removeAction (&optionGetCloseButton ());
	    screen->removeAction (&optionGetZoomButton ());
	    screen->removeAction (&optionGetPullButton ());
	}
    }
}

/**
 * experimental organic layout method
 * inspired by smallwindows (smallwindows.sf.net) by Jens Egeblad
 * FIXME: broken.
 * */
#define ORGANIC_STEP 0.05
#if 0
static int
organicCompareWindows (const void *elem1,
		       const void *elem2)
{
    CompWindow *w1 = *((CompWindow **) elem1);
    CompWindow *w2 = *((CompWindow **) elem2);

    return (WIN_X (w1) + WIN_Y (w1)) - (WIN_X (w2) + WIN_Y (w2));
}

static double
layoutOrganicCalculateOverlap (CompScreen *s,
			       int        win,
			       int        x,
			       int        y)
{
    int    i;
    int    x1, y1, x2, y2;
    int    overlapX, overlapY;
    int    xMin, xMax, yMin, yMax;
    double result = -0.01;

    SCALE_SCREEN ();
    ADDON_SCREEN ();

    x1 = x;
    y1 = y;
    x2 = x1 + WIN_W (ss->windows[win]) * as->scale;
    y2 = y1 + WIN_H (ss->windows[win]) * as->scale;

    for (i = 0; i < ss->nWindows; i++)
    {
	if (i == win)
	    continue;

	overlapX = overlapY = 0;
	xMax = MAX (ss->slots[i].x1, x1);
	xMin = MIN (ss->slots[i].x1 + WIN_W (ss->windows[i]) * as->scale, x2);
	if (xMax <= xMin)
	    overlapX = xMin - xMax;

	yMax = MAX (ss->slots[i].y1, y1);
	yMin = MIN (ss->slots[i].y1 + WIN_H (ss->windows[i]) * as->scale, y2);

	if (yMax <= yMin)
	    overlapY = yMin - yMax;

	result += (double)overlapX * overlapY;
    }

    return result;
}

static double
layoutOrganicFindBestHorizontalPosition (CompScreen *s,
					 int        win,
					 int        *bestX,
					 int        areaWidth)
{
    int    i, y1, y2, w;
    double bestOverlap = 1e31, overlap;

    SCALE_SCREEN ();
    ADDON_SCREEN ();

    y1 = ss->slots[win].y1;
    y2 = ss->slots[win].y1 + WIN_H (ss->windows[win]) * as->scale;

    w = WIN_W (ss->windows[win]) * as->scale;
    *bestX = ss->slots[win].x1;

    for (i = 0; i < ss->nWindows; i++)
    {
	CompWindow *lw = ss->windows[i];
	if (i == win)
	    continue;

	if (ss->slots[i].y1 < y2 &&
	    ss->slots[i].y1 + WIN_H (lw) * as->scale > y1)
	{
	    if (ss->slots[i].x1 - w >= 0)
	    {
		double overlap;
		
		overlap = layoutOrganicCalculateOverlap (s, win,
		 					 ss->slots[i].x1 - w,
							 y1);

		if (overlap < bestOverlap)
		{
		    *bestX = ss->slots[i].x1 - w;
		    bestOverlap = overlap;
		}
	    }
	    if (WIN_W (lw) * as->scale + ss->slots[i].x1 + w < areaWidth)
	    {
		double overlap;
		
		overlap = layoutOrganicCalculateOverlap (s, win,
		 					 ss->slots[i].x1 +
		 					 WIN_W (lw) * as->scale,
		 					 y1);

		if (overlap < bestOverlap)
		{
		    *bestX = ss->slots[i].x1 + WIN_W (lw) * as->scale;
		    bestOverlap = overlap;
		}
	    }
	}
    }

    overlap = layoutOrganicCalculateOverlap (s, win, 0, y1);
    if (overlap < bestOverlap)
    {
	*bestX = 0;
	bestOverlap = overlap;
    }

    overlap = layoutOrganicCalculateOverlap (s, win, areaWidth - w, y1);
    if (overlap < bestOverlap)
    {
	*bestX = areaWidth - w;
	bestOverlap = overlap;
    }

    return bestOverlap;
}

static double
layoutOrganicFindBestVerticalPosition (CompScreen *s,
				       int        win,
				       int        *bestY,
				       int        areaHeight)
{
    int    i, x1, x2, h;
    double bestOverlap = 1e31, overlap;

    SCALE_SCREEN ();
    ADDON_SCREEN ();

    x1 = ss->slots[win].x1;
    x2 = ss->slots[win].x1 + WIN_W (ss->windows[win]) * as->scale;
    h = WIN_H (ss->windows[win]) * as->scale;
    *bestY = ss->slots[win].y1;

    for (i = 0; i < ss->nWindows; i++)
    {
	CompWindow *w = ss->windows[i];

	if (i == win)
	    continue;

	if (ss->slots[i].x1 < x2 &&
	    ss->slots[i].x1 + WIN_W (w) * as->scale > x1)
	{
	    if (ss->slots[i].y1 - h >= 0 && ss->slots[i].y1 < areaHeight)
	    {
		double overlap;
		overlap = layoutOrganicCalculateOverlap (s, win, x1,
	 						 ss->slots[i].y1 - h);
		if (overlap < bestOverlap)
		{
		    *bestY = ss->slots[i].y1 - h;
		    bestOverlap = overlap;
		}
	    }
	    if (WIN_H (w) * as->scale + ss->slots[i].y1 > 0 &&
		WIN_H (w) * as->scale + h + ss->slots[i].y1 < areaHeight)
	    {
		double overlap;
		
		overlap = layoutOrganicCalculateOverlap (s, win, x1,
		 					 WIN_H (w) * as->scale +
							 ss->slots[i].y1);

		if (overlap < bestOverlap)
		{
		    *bestY = ss->slots[i].y1 + WIN_H(w) * as->scale;
		    bestOverlap = overlap;
		}
	    }
	}
    }

    overlap = layoutOrganicCalculateOverlap (s, win, x1, 0);
    if (overlap < bestOverlap)
    {
	*bestY = 0;
	bestOverlap = overlap;
    }

    overlap = layoutOrganicCalculateOverlap (s, win, x1, areaHeight - h);
    if (overlap < bestOverlap)
    {
	*bestY = areaHeight - h;
	bestOverlap = overlap;
    }

    return bestOverlap;
}

static bool
layoutOrganicLocalSearch (CompScreen *s,
			  int        areaWidth,
			  int        areaHeight)
{
    bool   improvement;
    int    i;
    double totalOverlap;

    SCALE_SCREEN ();

    do
    {
	improvement = false;
	for (i = 0; i < ss->nWindows; i++)
	{
	    bool improved;

	    do
	    {
		int    newX, newY;
		double oldOverlap, overlapH, overlapV;

		improved = false;
		oldOverlap = layoutOrganicCalculateOverlap (s, i,
 							    ss->slots[i].x1,
							    ss->slots[i].y1);

		overlapH = layoutOrganicFindBestHorizontalPosition (s, i,
								    &newX,
								    areaWidth);
		overlapV = layoutOrganicFindBestVerticalPosition (s, i,
								  &newY,
								  areaHeight);

		if (overlapH < oldOverlap - 0.1 ||
		    overlapV < oldOverlap - 0.1)
		{
		    improved = true;
		    improvement = true;
		    if (overlapV > overlapH)
			ss->slots[i].x1 = newX;
		    else
			ss->slots[i].y1 = newY;
		}
    	    }
	    while (improved);
	}
    }
    while (improvement);

    totalOverlap = 0.0;
    for (i = 0; i < ss->nWindows; i++)
    {
	totalOverlap += layoutOrganicCalculateOverlap (s, i,
						       ss->slots[i].x1,
						       ss->slots[i].y1);
    }

    return (totalOverlap > 0.1);
}

static void
layoutOrganicRemoveOverlap (CompScreen *s,
			    int        areaWidth,
			    int        areaHeight)
{
    int        i, spacing;
    CompWindow *w;

    SCALE_SCREEN ();
    ADDON_SCREEN ();

    spacing = ss->opt[SCALE_SCREEN_OPTION_SPACING].value.i;

    while (layoutOrganicLocalSearch (s, areaWidth, areaHeight))
    {
	for (i = 0; i < ss->nWindows; i++)
	{
	    int centerX, centerY;
	    int newX, newY, newWidth, newHeight;

	    w = ss->windows[i];

	    centerX = ss->slots[i].x1 + WIN_W (w) / 2;
	    centerY = ss->slots[i].y1 + WIN_H (w) / 2;

	    newWidth = (int)((1.0 - ORGANIC_STEP) *
			     (double)WIN_W (w)) - spacing / 2;
	    newHeight = (int)((1.0 - ORGANIC_STEP) *
			      (double)WIN_H (w)) - spacing / 2;
	    newX = centerX - (newWidth / 2);
	    newY = centerY - (newHeight / 2);

	    ss->slots[i].x1 = newX;
	    ss->slots[i].y1 = newY;
	    ss->slots[i].x2 = newX + WIN_W (w);
	    ss->slots[i].y2 = newY + WIN_H (w);
	}
	as->scale -= ORGANIC_STEP;
    }
}

static bool
layoutOrganicThumbs (CompScreen *s)
{
    CompWindow *w;
    int        i, moMode;
    XRectangle workArea;

    SCALE_SCREEN ();
    ADDON_SCREEN ();

    moMode = ss->opt[SCALE_SCREEN_OPTION_MULTIOUTPUT_MODE].value.i;

    switch (moMode) {
    case SCALE_MOMODE_ALL:
	workArea = s->workArea;
	break;
    case SCALE_MOMODE_CURRENT:
    default:
	workArea = s->outputDev[s->currentOutputDev].workArea;
	break;
    }

    as->scale = 1.0f;

    qsort (ss->windows, ss->nWindows, sizeof(CompWindow *),
	   organicCompareWindows);

    for (i = 0; i < ss->nWindows; i++)
    {
	w = ss->windows[i];
	SCALE_WINDOW (w);

	sWindow->slot = &ss->slots[i];
	ss->slots[i].x1 = WIN_X (w) - workArea.x;
	ss->slots[i].y1 = WIN_Y (w) - workArea.y;
	ss->slots[i].x2 = WIN_X (w) + WIN_W (w) - workArea.x;
	ss->slots[i].y2 = WIN_Y (w) + WIN_H (w) - workArea.y;

	if (ss->slots[i].x1 < 0)
	{
	    ss->slots[i].x2 += abs (ss->slots[i].x1);
	    ss->slots[i].x1 = 0;
	}
	if (ss->slots[i].x2 > workArea.width - workArea.x)
	{
	    ss->slots[i].x1 -= abs (ss->slots[i].x2 - workArea.width);
	    ss->slots[i].x2 = workArea.width - workArea.x;
	}

	if (ss->slots[i].y1 < 0)
	{
	    ss->slots[i].y2 += abs (ss->slots[i].y1);
	    ss->slots[i].y1 = 0;
	}
	if (ss->slots[i].y2 > workArea.height - workArea.y)
	{
	    ss->slots[i].y1 -= abs (ss->slots[i].y2 -
				    workArea.height - workArea.y);
	    ss->slots[i].y2 = workArea.height - workArea.y;
	}
    }

    ss->nSlots = ss->nWindows;

    layoutOrganicRemoveOverlap (s, workArea.width - workArea.x,
				workArea.height - workArea.y);
    for (i = 0; i < ss->nWindows; i++)
    {
	w = ss->windows[i];
	SCALE_WINDOW (w);

	if (ss->type == ScaleTypeGroup)
	    raiseWindow (ss->windows[i]);

	ss->slots[i].x1 += w->input.left + workArea.x;
	ss->slots[i].x2 += w->input.left + workArea.x;
	ss->slots[i].y1 += w->input.top + workArea.y;
	ss->slots[i].y2 += w->input.top + workArea.y;
	sWindow->adjust = true;
    }

    return true;
}

#endif

bool
ScaleAddonScreen::layoutSlotsAndAssignWindows ()
{
    bool status;

    switch (optionGetLayoutMode ())
    {
    case LayoutModeOrganicExperimental:
	//status = layoutOrganicThumbs ();
	break;
    case LayoutModeNormal:
    default:
	status = sScreen->layoutSlotsAndAssignWindows ();
	break;
    }

    return status;
}

void
ScaleAddonScreen::optionChanged (CompOption              *opt,
				 ScaleaddonOptions::Options num)
{
    switch (num)
    {
	case ScaleaddonOptions::WindowTitle:
	case ScaleaddonOptions::TitleBold:
	case ScaleaddonOptions::TitleSize:
	case ScaleaddonOptions::BorderSize:
	case ScaleaddonOptions::FontColor:
	case ScaleaddonOptions::BackColor:
	    {
		foreach (CompWindow *w, screen->windows ())
		{
		    ADDON_WINDOW (w);

		    if (textAvailable)
			aw->renderTitle ();
		}
	    }
	    break;
	default:
	    break;
    }
}

ScaleAddonScreen::ScaleAddonScreen (CompScreen *) :
    PluginClassHandler <ScaleAddonScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    sScreen (ScaleScreen::get (screen)),
    highlightedWindow (0),
    lastHighlightedWindow (0),
    lastState (ScaleScreen::Idle),
    scale (1.0f)
{
    ScreenInterface::setHandler (screen, true);
    CompositeScreenInterface::setHandler (cScreen, true);
    ScaleScreenInterface::setHandler (sScreen, true);

    optionSetCloseKeyInitiate (boost::bind (&ScaleAddonScreen::closeWindow, this,
					    _1, _2 ,_3));
    optionSetZoomKeyInitiate (boost::bind (&ScaleAddonScreen::zoomWindow, this,
					   _1, _2, _3));
    optionSetPullKeyInitiate (boost::bind (&ScaleAddonScreen::pullWindow, this,
					   _1, _2, _3));
    optionSetCloseButtonInitiate (boost::bind (&ScaleAddonScreen::closeWindow, this,
					       _1, _2, _3));
    optionSetZoomButtonInitiate (boost::bind (&ScaleAddonScreen::zoomWindow, this,
					      _1, _2, _3));
    optionSetPullButtonInitiate (boost::bind (&ScaleAddonScreen::pullWindow, this,
					      _1, _2, _3));

    optionSetWindowTitleNotify (boost::bind (&ScaleAddonScreen::optionChanged, this, _1, _2));
    optionSetTitleBoldNotify (boost::bind (&ScaleAddonScreen::optionChanged, this, _1, _2));
    optionSetTitleSizeNotify (boost::bind (&ScaleAddonScreen::optionChanged, this, _1, _2));
    optionSetBorderSizeNotify (boost::bind (&ScaleAddonScreen::optionChanged, this, _1, _2));
    optionSetFontColorNotify (boost::bind (&ScaleAddonScreen::optionChanged, this, _1, _2));
    optionSetBackColorNotify (boost::bind (&ScaleAddonScreen::optionChanged, this, _1, _2));
}


ScaleAddonWindow::ScaleAddonWindow (CompWindow *window) :
    PluginClassHandler <ScaleAddonWindow, CompWindow> (window),
    window (window),
    sWindow (ScaleWindow::get (window)),
    cWindow (CompositeWindow::get (window)),
    rescaled (false),
    oldAbove (NULL)
{
    ScaleWindowInterface::setHandler (sWindow);
}

bool
ScaleAddonPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("scale", COMPIZ_SCALE_ABI))
	return false;

    if (!CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI))
    {
	compLogMessage ("scaleaddon", CompLogLevelInfo, "Text Plugin not loaded, no text will be drawn\n");
	textAvailable = false;
    }
    else
	textAvailable = true;

    return true;
}

