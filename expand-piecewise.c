/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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

#include "animationsim.h"

// =====================  Effect: ExpandPW  =========================

float
fxExpandPWAnimProgress (CompWindow *w)
{
    ANIMSIM_DISPLAY (w->screen->display);
    float forwardProgress = (*ad->animBaseFunc->defaultAnimProgress) (w);

    return (*ad->animBaseFunc->decelerateProgress) (forwardProgress);
}

static void
applyExpandPWTransform (CompWindow *w)
{
    ANIMSIM_WINDOW (w);

    CompTransform *transform = &aw->com->transform;

    float forwardProgress = 1.0f - fxExpandPWAnimProgress (w);

    float initialXScale = animGetI (w, ANIMSIM_SCREEN_OPTION_EXPANDPW_INITIAL_HORIZ) / (float) w->width;
    float initialYScale = animGetI (w, ANIMSIM_SCREEN_OPTION_EXPANDPW_INITIAL_VERT)  / (float) w->height;
    
    // animation movement
    matrixTranslate (transform, WIN_X (w) + WIN_W (w) / 2.0f,
				WIN_Y (w) + WIN_H (w) / 2.0f,
				0.0f);
	
    float switchPoint;
    float xScale;
    float yScale;
    float delay = animGetF (w, ANIMSIM_SCREEN_OPTION_EXPANDPW_DELAY);
    
    if(animGetB (w, ANIMSIM_SCREEN_OPTION_EXPANDPW_HORIZ_FIRST))
    {
        switchPoint = w->width / (float) (w->width + w->height);
        xScale = initialXScale + (1.0f - initialXScale) * (forwardProgress < (switchPoint-delay/2) ? 1.0f - (switchPoint-delay/2 - forwardProgress)/(switchPoint-delay/2) : 1.0f);
        yScale = initialYScale + (1.0f - initialYScale) * (forwardProgress > (switchPoint+delay/2) ? (forwardProgress - (switchPoint+delay/2))/(1.0f-switchPoint-delay/2) : 0.0f);
    }
    else
    {
        switchPoint = w->height / (float) (w->width + w->height);
        xScale = initialXScale + (1.0f - initialXScale) * (forwardProgress > switchPoint ? (forwardProgress - switchPoint)/(1.0f-switchPoint) : 0.0f);
        yScale = initialYScale + (1.0f - initialYScale) * (forwardProgress < switchPoint ? 1.0f - (switchPoint - forwardProgress)/switchPoint : 1.0f);    
    }

    matrixScale (transform, xScale, yScale, 0.0f);

    matrixTranslate (transform, -(WIN_X (w) + WIN_W (w) / 2.0f),
				-(WIN_Y (w) + WIN_H (w) / 2.0f),
				0.0f);

}

void
fxExpandPWAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyExpandPWTransform (w);
}

void
fxExpandPWUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
}

void
fxExpandPWUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxExpandPWInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}
