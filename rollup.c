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

#include "animation-internal.h"

// =====================  Effect: Roll Up  =========================

void
fxRollUpInitGrid(AnimScreen * as, AnimWindow *aw,
		 int *gridWidth, int *gridHeight)
{
    *gridWidth = 2;
    if (aw->curWindowEvent == WindowEventShade ||
	aw->curWindowEvent == WindowEventUnshade)
	*gridHeight = 4;
    else
	*gridHeight = 2;
}

static void
fxRollUpModelStepObject(CompWindow * w,
			Model * model,
			Object * object,
			float forwardProgress, Bool fixedInterior)
{
    ANIM_WINDOW(w);

    float origx = WIN_X(w) + WIN_W(w) * object->gridPosition.x;

    if (aw->curWindowEvent == WindowEventShade ||
	aw->curWindowEvent == WindowEventUnshade)
    {
	// Execute shade mode

	// find position in window contents
	// (window contents correspond to 0.0-1.0 range)
	float relPosInWinContents =
	    (object->gridPosition.y * WIN_H(w) -
	     model->topHeight) / w->height;

	if (object->gridPosition.y == 0)
	{
	    object->position.x = origx;
	    object->position.y = WIN_Y(w);
	}
	else if (object->gridPosition.y == 1)
	{
	    object->position.x = origx;
	    object->position.y =
		(1 - forwardProgress) *
		(WIN_Y(w) +
		 WIN_H(w) * object->gridPosition.y) +
		forwardProgress * (WIN_Y(w) +
				   model->topHeight +
				   model->bottomHeight);
	}
	else
	{
	    object->position.x = origx;

	    if (relPosInWinContents > forwardProgress)
	    {
		object->position.y =
		    (1 - forwardProgress) *
		    (WIN_Y(w) +
		     WIN_H(w) * object->gridPosition.y) +
		    forwardProgress * (WIN_Y(w) + model->topHeight);

		if (fixedInterior)
		    object->offsetTexCoordForQuadBefore.y =
			-forwardProgress * w->height;
	    }
	    else
	    {
		object->position.y = WIN_Y(w) + model->topHeight;
		if (!fixedInterior)
		    object->offsetTexCoordForQuadAfter.
			y =
			(forwardProgress -
			 relPosInWinContents) * w->height;
	    }
	}
    }
}

void
fxRollUpModelStep (CompScreen *s, CompWindow *w, float time)
{
    defaultAnimStep (s, w, time);

    ANIM_SCREEN(s);
    ANIM_WINDOW(w);

    Model *model = aw->model;

    float forwardProgress = sigmoidAnimProgress(aw);

    int i;
    for (i = 0; i < model->numObjects; i++)
	fxRollUpModelStepObject
	    (w, 
	     model,
	     &model->objects[i],
	     forwardProgress,
	     animGetB(as, aw,
		      ANIM_SCREEN_OPTION_ROLLUP_FIXED_INTERIOR));
}

void fxRollUpAnimInit(CompScreen * s, CompWindow * w)
{
    ANIM_WINDOW(w);

    aw->animTotalTime /= ROLLUP_PERCEIVED_T;
    aw->animRemainingTime = aw->animTotalTime;
}
