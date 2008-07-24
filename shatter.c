/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
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

void fxShatterInit(CompScreen * s, CompWindow * w)
{
    ANIM_WINDOW(w);
    ANIM_SCREEN(s);

    int screen_height = s->outputDev[outputDeviceForWindow(w)].height;

    fprintf(stderr, "height is %i\n", screen_height);

    if (!tessellateIntoGlass(w, 
                    animGetI(as, aw, ANIM_SCREEN_OPTION_EXPLODE3D_GRIDSIZE_X),
                    animGetI(as, aw, ANIM_SCREEN_OPTION_EXPLODE3D_GRIDSIZE_Y),
                    animGetF(as, aw, ANIM_SCREEN_OPTION_EXPLODE3D_THICKNESS)));

    PolygonSet *pset = aw->polygonSet;
    PolygonObject *p = pset->polygons;
    double sqrt2 = sqrt(2);

    int i, static_polygon;

    for (i = 0; i < pset->nPolygons; i++, p++)
    {
        p->rotAxis.x = 0;
        p->rotAxis.y = 0;
        p->rotAxis.z = 1;

        if ( RAND_FLOAT() < 0.7 )    
            static_polygon = 1;
        else
            static_polygon = 0;

        p->finalRelPos.x = 0;
        p->finalRelPos.y = static_polygon *
                            (-p->centerPosStart.y + screen_height);
        p->finalRelPos.z = 0;
        if (p->finalRelPos.y)
            p->finalRotAng = RAND_FLOAT() * 120;
    }
    
    
    
    pset->allFadeDuration = 0.3f;
    pset->doDepthTest = TRUE;
    pset->doLighting = TRUE;
    pset->correctPerspective = CorrectPerspectivePolygon;
    pset->backAndSidesFadeDur = 0.2f;

    aw->animTotalTime /= EXPLODE_PERCEIVED_T;
    aw->animRemainingTime = aw->animTotalTime;
}

