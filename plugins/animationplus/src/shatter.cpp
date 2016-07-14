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

const float ShatterAnim::kDurationFactor = 1.43;

ShatterAnim::ShatterAnim (CompWindow *w,
			  WindowEvent curWindowEvent,
			  float	      duration,
			  const AnimEffect info,
			  const CompRect   &icon) :
	Animation::Animation (w, curWindowEvent, kDurationFactor * duration, info,
			      icon),
	PolygonAnim::PolygonAnim (w, curWindowEvent, kDurationFactor * duration,
				  info, icon)
{
    mAllFadeDuration = 0.4f;
    mBackAndSidesFadeDur = 0.2f;
    mDoDepthTest = true;
    mDoLighting = true;
    mCorrectPerspective = CorrectPerspectivePolygon;
}

void
ShatterAnim::init ()
{
    ANIMPLUS_SCREEN (screen);
    int static_polygon;
    int screen_height = screen->outputDevs ().at (mWindow->outputDevice ()).height ();

    tessellateIntoGlass (as->optionGetShatterNumSpokes (),
			 as->optionGetShatterNumTiers (),
			 1); //can't really see how thick it is...

    foreach (PolygonObject *p, mPolygons)
    {
	p->rotAxis.set (0, 0, 1);
	static_polygon = 1;

	p->finalRelPos.set (0,
			    static_polygon * (-p->centerPosStart.y () + screen_height),
			    0);
        if (p->finalRelPos.y ())
            p->finalRotAng = RAND_FLOAT() * 120 * ( RAND_FLOAT() < 0.5 ? -1 : 1 );
    }
}

