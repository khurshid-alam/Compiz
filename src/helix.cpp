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

#include "private.h"

const float HelixAnim::kDurationFactor = 1.43;

HelixAnim::HelixAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float	    duration,
		      const AnimEffect info,
		      const CompRect   &icon) :
    Animation::Animation (w, curWindowEvent, kDurationFactor * duration, info, icon),
    PolygonAnim::PolygonAnim (w, curWindowEvent, kDurationFactor * duration, info, icon)
{
    mAllFadeDuration = 0.4f;
    mBackAndSidesFadeDur = 0.2f;
    mDoDepthTest = TRUE;
    mDoLighting = TRUE;
    mCorrectPerspective = CorrectPerspectivePolygon;
}

void
HelixAnim::init ()
{
    ANIMPLUS_SCREEN (screen);

    int gridsizeY = as->optionGetHelixGridy ();
    int count = 0;

    tessellateIntoRectangles (1, gridsizeY, as->optionGetHelixThickness ());

    foreach (PolygonObject *p, mPolygons)
    {
        
	//rotate around y axis normally, or the z axis if the effect is in vertical mode
	if (as->optionGetHelixDirection ())
	    p->rotAxis.set (0, 0, 1);
	else
	    p->rotAxis.set (0, 1, 0);

	//only move the pieces in a 'vertical' rotation
	if (as->optionGetHelixDirection ())
	    p->finalRelPos.set (0,
				-1 * ((mWindow->height () / gridsizeY) * (count - gridsizeY/2)),
				0);
	else
	    p->finalRelPos.set (0,
				0,
				0);

	//determine how long, and what direction to spin
	int numberOfTwists = as->optionGetHelixNumTwists ();
	int spin_dir = as->optionGetHelixSpinDirection ();

	if (spin_dir)
	    p->finalRotAng = 270 - ( 2 * numberOfTwists * count); 
	else
	    p->finalRotAng = ( 2 * numberOfTwists * count) - 270; 

	count++;        
       
    }
}

