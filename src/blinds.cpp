/*
 * Animation plugin for compiz/beryl
 *
 * blinds.cpp
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
 *const float ExplodeAnim::kDurationFactor = 1.43;
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

const float BlindsAnim::kDurationFactor = 1.43;


BlindsAnim::BlindsAnim (CompWindow *w,
			WindowEvent curWindowEvent,
			float	    duration,
			const AnimEffect info,
			const CompRect   &icon) :
    Animation::Animation (w, curWindowEvent, kDurationFactor * duration, info, icon),
    PolygonAnim::PolygonAnim (w, curWindowEvent, kDurationFactor * duration, info, icon)
{
    mAllFadeDuration = 0.3f;
    mDoDepthTest = true;
    mDoLighting = true;
    mCorrectPerspective = CorrectPerspectivePolygon;
    mBackAndSidesFadeDur = 0.2f;
}

void
BlindsAnim::init ()
{
    ANIMPLUS_SCREEN (screen);

    tessellateIntoRectangles (as->optionGetBlindsGridx (), 1,
			      as->optionGetBlindsThickness ());

    foreach (PolygonObject *p, mPolygons)
    {
	//rotate around y axis
	p->rotAxis.set (0, 1, 0);
	p->finalRelPos.set (0, 0, 0);

	int numberOfHalfTwists = as->optionGetBlindsNumHalftwists ();
	p->finalRotAng = 180 * numberOfHalfTwists ;
    }
}

