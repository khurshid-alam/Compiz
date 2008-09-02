/*
 * Animation plugin for compiz/beryl
 *
 * blinds.c
 *
 * Copyright : (C) 2008 Kevin DuBois
 * E-mail    : kdub432@gmail.com
 *
 * Based on other animations by
 *           : Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 * 
 * Which were based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
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

void fxBlindsInit(CompScreen * s, CompWindow * w)
{
    ANIM_WINDOW (w);
    ANIM_SCREEN (s);

tessellateIntoRectangles(w, 
                      animGetI(as, aw, ANIM_SCREEN_OPTION_BLINDS_GRIDX),
                      1,
                      animGetF(as, aw, ANIM_SCREEN_OPTION_BLINDS_THICKNESS));

    PolygonSet *pset = aw->polygonSet;
    PolygonObject *p = pset->polygons;

    int i;

    for (i = 0; i < pset->nPolygons; i++, p++)
    {
    //rotate around y axis
    p->rotAxis.x = 0;
    p->rotAxis.y = 1;
    p->rotAxis.z = 0;

    //dont translate the pieces
    p->finalRelPos.x = 0;
    p->finalRelPos.y = 0;
    p->finalRelPos.z = 0;
    
    int numberOfHalfTwists = animGetI(as, aw, ANIM_SCREEN_OPTION_BLINDS_HALFTWISTS);
    p->finalRotAng = 180 * numberOfHalfTwists ;
    }
    
    
    pset->allFadeDuration = 0.4f;
    pset->backAndSidesFadeDur = 0.2f;
    pset->doDepthTest = TRUE;
    pset->doLighting = TRUE;
    pset->correctPerspective = CorrectPerspectivePolygon;

    aw->animTotalTime /= EXPLODE_PERCEIVED_T;
    aw->animRemainingTime = aw->animTotalTime;
}

