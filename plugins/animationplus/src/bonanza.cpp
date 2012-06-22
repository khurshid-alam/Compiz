/*
 * Animation plugin for compiz/beryl
 *
 * bonanza.c
 *
 * Copyright : (C) 2008 Kevin DuBois
 * E-mail    : kdub423@gmail.com
 *
 * Based on animations system by: (C) 2006 Erkin Bahceci
 * E-mail                       : erkinbah@gmail.com
 *
 * Based on particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                            : onestone@beryl-project.org
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
#include "animation_tex.h"

// =====================  Effect: Burn  =========================

BonanzaAnim::BonanzaAnim (CompWindow *w,
			  WindowEvent curWindowEvent,
			  float	    duration,
			  const AnimEffect info,
			  const CompRect   &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    ParticleAnim::ParticleAnim (w, curWindowEvent, duration, info, icon)
{
    ANIMPLUS_SCREEN (screen);

    // Light Particles are for extras, dark particles unused?
    initLightDarkParticles (as->optionGetBonanzaParticles (),
			    as->optionGetBonanzaParticles () / 10,
			    0.125,
			    0.5);

    mAnimFireDirection = 0;
}

void
BonanzaAnim::genFire (int x,
		      int y,
		      int radius,
		      float size,
		      float time)                
{
    ANIMPLUS_SCREEN (screen);

    ParticleSystem &ps = mParticleSystems[0];

    float fireLife = as->optionGetBonanzaLife ();
    float fireLifeNeg = 1 - fireLife;
    float fadeExtra = 0.2f * (1.01 - fireLife);
    float max_new = ps.particles ().size () * (time / 50) * (1.05 - fireLife);
    float numParticles = ps.particles ().size ();

    unsigned short *c =	as->optionGetBonanzaColor ();
    float colr1 = (float)c[0] / 0xffff;
    float colg1 = (float)c[1] / 0xffff;
    float colb1 = (float)c[2] / 0xffff;
    float colr2 = 1 / 1.7 * (float)c[0] / 0xffff;
    float colg2 = 1 / 1.7 * (float)c[1] / 0xffff;
    float colb2 = 1 / 1.7 * (float)c[2] / 0xffff;
    float cola = (float)c[3] / 0xffff;
    float rVal;

    Particle *part = &(ps.particles ()[0]);

    float deg = 0;
    float inc = 2.0 * 3.1415 / numParticles;
    float partw = 5.00;
    float parth = partw * 1.5;
    bool mysticalFire = as->optionGetBonanzaMystical ();

    for (unsigned int i = 0; i < numParticles && max_new > 0; i++, part++)
    {
        deg += inc; 

        if (part->life <= 0.0f)
        {
            // give gt new life
            rVal = (float)(random() & 0xff) / 255.0;
            part->life = 1.0f;
            part->fade = rVal * fireLifeNeg + fadeExtra; // Random Fade Value

            // set size
            part->width = partw;
            part->height = parth;
            rVal = (float)(random() & 0xff) / 255.0;
            part->w_mod = part->h_mod = size * rVal;

            part->x = (float)x + (float) radius * cosf(deg);
            part->y = (float)y + (float) radius * sinf(deg);

            //clip
            if (part->x <= 0)
            part->x = 0;
            if (part->x >= 2 * x)
                part->x = 2*x;
        
            if (part->y <= 0)
            part->y = 0;
            if (part->y >= 2 * y)
                part->y = 2*y;

            part->z = 0.0;

            part->xo = part->x;
            part->yo = part->y;
            part->zo = 0.0f;

            // set speed and direction
            rVal = (float)(random() & 0xff) / 255.0;
            part->xi = ((rVal * 20.0) - 10.0f);
            rVal = (float)(random() & 0xff) / 255.0;
            part->yi = ((rVal * 20.0) - 15.0f);
            part->zi = 0.0f;

            if (mysticalFire)
            {
                // Random colors! (aka Mystical Fire)
                rVal = (float)(random() & 0xff) / 255.0;
                part->r = rVal;
                rVal = (float)(random() & 0xff) / 255.0;
                part->g = rVal;
                rVal = (float)(random() & 0xff) / 255.0;
                part->b = rVal;
            }
            else
            {
                rVal = (float)(random() & 0xff) / 255.0;
                part->r = colr1 - rVal * colr2;
                part->g = colg1 - rVal * colg2;
                part->b = colb1 - rVal * colb2;
            }
            // set transparancy
            part->a = cola;

            // set gravity
            part->xg = (part->x < part->xo) ? 1.0 : -1.0;
            part->yg = -3.0f;
            part->zg = 0.0f;

            ps.activate ();
            max_new -= 1;
        }
        else
        {
            part->xg = (part->x < part->xo) ? 1.0 : -1.0;
        }

    }

}

void
BonanzaAnim::step (float time)
{
    float timestep = 2.0;
    float old = 1 - (mRemainingTime) / (mTotalTime - timestep);
    float stepSize;
    CompRect rect = mWindow->outputRect ();

    mRemainingTime -= timestep;
    if (mRemainingTime <= 0)
	    mRemainingTime = 0;	// avoid sub-zero values
    float new_f = 1 - (mRemainingTime) / (mTotalTime - timestep);

    stepSize = new_f - old;

    if (mCurWindowEvent == WindowEventOpen ||
	mCurWindowEvent == WindowEventUnminimize ||
	mCurWindowEvent == WindowEventUnshade)
    {
	new_f = 1 - new_f;
    }
    
    mUseDrawRegion = true;
    mDrawRegion = CompRegion ();


    /* define an expanding circle as a union of rectangular X regions. */
    float radius;
    if (mRemainingTime > 0)
    {        
        CompPoint::vector pts;
	pts.resize (20);
        int i;
        float two_pi = 3.14159 * 2.0;
        int centerX = rect.centerX () + rect.x ();
        int centerY = rect.centerY () + rect.y ();
        float corner_dist = sqrt( powf(rect.centerX ()/2,2) + powf(rect.centerY (),2));
        radius = new_f * corner_dist;
        for (i = 0; i < 20; i++)
        {
            pts[i].set (centerX + (int)(radius * cosf( (float) i/20.0 * two_pi )),
			centerY + (int)(radius * sinf( (float) i/20.0 * two_pi )));
        }
        
        CompRegion r1 (pts), r2 (emptyRegion.united (rect));

	mDrawRegion = r1 - r2;
        

    }
    else
    {
    	mDrawRegion = emptyRegion;
    }


    mUseDrawRegion = (fabs (new_f) > 1e-5);

    genFire (rect.centerX (),
	     rect.centerY (),
             radius,
             WIN_W(mWindow) / 40.0,
             time);

    if (mRemainingTime <= 0 && mParticleSystems.size () && mParticleSystems.at (0).active ())
    {
        mRemainingTime = 0;
    }

    if (mParticleSystems.empty () || !mParticleSystems.at (0).active ())
    {
	mParticleSystems.clear ();
	// Abort animation
	compLogMessage ("animationaddon", CompLogLevelError, "Couldn't do bonanza animation\n");
	mRemainingTime = 0;
    }
}

