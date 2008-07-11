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
 * Helix and Blinds Effects by : Kevin DuBois
 * Email                       : kdub432@gmail.com
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

void fxHelixInit(CompScreen * s, CompWindow * w)
{
    ANIM_WINDOW (w);
    ANIM_SCREEN (s);

    int gridsizeY = animGetI(as, aw, ANIM_SCREEN_OPTION_HELIX_GRIDSIZE_Y);

    tessellateIntoRectangles(w, 
                             1,
                             gridsizeY,
                             animGetF(as, aw, ANIM_SCREEN_OPTION_HELIX_THICKNESS));

    PolygonSet *pset = aw->polygonSet;
    PolygonObject *p = pset->polygons;


    int i;
    for (i = 0; i < pset->nPolygons; i++, p++)
    {
        
    //rotate around y axis normally, or the z axis if the effect is in vertical mode
    p->rotAxis.x = 0;
    
    if (animGetB(as, aw, ANIM_SCREEN_OPTION_HELIX_DIRECTION))
    {
        p->rotAxis.y = 0;
        p->rotAxis.z = 1;
    } else {
        p->rotAxis.y = 1;
        p->rotAxis.z = 0;
    }
    
    //only move the pieces in a 'vertical' rotation
    p->finalRelPos.x = 0;   
    
    if (animGetB(as, aw, ANIM_SCREEN_OPTION_HELIX_DIRECTION))
        p->finalRelPos.y = -1 * ((w->height)/ gridsizeY) * (i -  gridsizeY/2);
    else 
        p->finalRelPos.y = 0;
    
         
    p->finalRelPos.z = 0;
    
    //determine how long, and what direction to spin
    int numberOfTwists = animGetI(as, aw, ANIM_SCREEN_OPTION_HELIX_NUM_TWISTS);
    int spin_dir =animGetI(as, aw, ANIM_SCREEN_OPTION_HELIX_SPIN_DIRECTION);
    
    if (spin_dir)
        p->finalRotAng = 270 - ( 2 * numberOfTwists * i); 
    else
        p->finalRotAng = ( 2 * numberOfTwists * i) - 270; 
        
       
    }
    
    
    pset->allFadeDuration = 0.4f;
    pset->backAndSidesFadeDur = 0.2f;
    pset->doDepthTest = TRUE;
    pset->doLighting = TRUE;
    pset->correctPerspective = CorrectPerspectivePolygon;

    aw->animTotalTime /= EXPLODE_PERCEIVED_T;
    aw->animRemainingTime = aw->animTotalTime;
}

