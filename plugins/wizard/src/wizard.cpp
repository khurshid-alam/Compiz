/*
 * Compiz wizard particle system plugin
 *
 * wizard.cpp
 *
 * Written by : Sebastian Kuhlen
 * E-mail     : DiCon@tankwar.de
 *
 * Ported to Compiz 0.9.x
 * Copyright : (c) 2010 Scott Moreau <oreaus@gmail.com>
 *
 * This plugin and parts of its code have been inspired by the showmouse plugin
 * by Dennis Kasprzyk
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
 */

#include <math.h>
#include <string.h>

#include "wizard.h"

/* 3 vertices per triangle, 2 triangles per particle */
const unsigned short CACHESIZE_FACTOR = 3 * 2;

/* 2 coordinates, x and y */
const unsigned short COORD_COMPONENTS = CACHESIZE_FACTOR * 2;

/* each vertex is stored as 3 GLfloats */
const unsigned short VERTEX_COMPONENTS = CACHESIZE_FACTOR * 3;

/* 4 colors, RGBA */
const unsigned short COLOR_COMPONENTS = CACHESIZE_FACTOR * 4;

static void
initParticles (int hardLimit, int softLimit, ParticleSystem * ps)
{
    ps->particles.clear ();

    ps->tex          = 0;
    ps->hardLimit    = hardLimit;
    ps->softLimit    = softLimit;
    ps->active       = false;
    ps->lastCount    = 0;

    // Initialize cache
    ps->vertices_cache.clear ();
    ps->coords_cache.clear ();
    ps->colors_cache.clear ();
    ps->dcolors_cache.clear ();

    for (int i = 0; i < hardLimit; i++)
    {
	Particle part;
	part.t = 0.0f;
	ps->particles.push_back (part);
    }
}

void
WizardScreen::loadGPoints (ParticleSystem *ps)
{
    if (!ps) //ps not yet initialized
	return;

    CompOption::Value::Vector GStrength = optionGetGStrength ();
    CompOption::Value::Vector GPosx = optionGetGPosx ();
    CompOption::Value::Vector GPosy = optionGetGPosy ();
    CompOption::Value::Vector GSpeed = optionGetGSpeed ();
    CompOption::Value::Vector GAngle = optionGetGAngle ();
    CompOption::Value::Vector GMovement = optionGetGMovement ();

    /* ensure that all properties have been updated  */
    unsigned int ng = GStrength.size ();
    if( ng != GPosx.size ()  ||
	ng != GPosy.size ()  ||
	ng != GSpeed.size () ||
	ng != GAngle.size () ||
	ng != GMovement.size ())
       return;

    ps->g.clear ();

    for (unsigned int i = 0; i < ng; i++)
    {
	GPoint gi;
	gi.strength = (float)GStrength.at (i).i ()/ 1000.;
	gi.x = (float)GPosx.at (i).i ();
	gi.y = (float)GPosy.at (i).i ();
	gi.espeed = (float)GSpeed.at (i).i () / 100.;
	gi.eangle = (float)GAngle.at (i).i () / 180.*M_PI;
	gi.movement = GMovement.at (i).i ();
	ps->g.push_back (gi);
    }
}

void
WizardScreen::loadEmitters (ParticleSystem *ps)
{
    if (!ps) //ps not yet initialized
	return;

    CompOption::Value::Vector EActive = optionGetEActive ();
    CompOption::Value::Vector ETrigger = optionGetETrigger ();
    CompOption::Value::Vector EPosx = optionGetEPosx ();
    CompOption::Value::Vector EPosy = optionGetEPosy ();
    CompOption::Value::Vector ESpeed = optionGetESpeed ();
    CompOption::Value::Vector EAngle = optionGetEAngle ();
    CompOption::Value::Vector EMovement = optionGetEMovement ();
    CompOption::Value::Vector ECount = optionGetECount ();
    CompOption::Value::Vector EH = optionGetEH ();
    CompOption::Value::Vector EDh = optionGetEDh ();
    CompOption::Value::Vector EL = optionGetEL ();
    CompOption::Value::Vector EDl = optionGetEDl ();
    CompOption::Value::Vector EA = optionGetEA ();
    CompOption::Value::Vector EDa = optionGetEDa ();
    CompOption::Value::Vector EDx = optionGetEDx ();
    CompOption::Value::Vector EDy = optionGetEDy ();
    CompOption::Value::Vector EDcirc = optionGetEDcirc ();
    CompOption::Value::Vector EVx = optionGetEVx ();
    CompOption::Value::Vector EVy = optionGetEVy ();
    CompOption::Value::Vector EVt = optionGetEVt ();
    CompOption::Value::Vector EVphi = optionGetEVphi ();
    CompOption::Value::Vector EDvx = optionGetEDvx ();
    CompOption::Value::Vector EDvy = optionGetEDvy ();
    CompOption::Value::Vector EDvcirc = optionGetEDvcirc ();
    CompOption::Value::Vector EDvt = optionGetEDvt ();
    CompOption::Value::Vector EDvphi = optionGetEDvphi ();
    CompOption::Value::Vector ES = optionGetES ();
    CompOption::Value::Vector EDs = optionGetEDs ();
    CompOption::Value::Vector ESnew = optionGetESnew ();
    CompOption::Value::Vector EDsnew = optionGetEDsnew ();
    CompOption::Value::Vector EG = optionGetEG ();
    CompOption::Value::Vector EDg = optionGetEDg ();
    CompOption::Value::Vector EGp = optionGetEGp ();

    /* ensure that all properties have been updated  */
    unsigned int ne = EActive.size ();
    if( ne != ETrigger.size ()  ||
	ne != EPosx.size ()     ||
	ne != EPosy.size ()     ||
	ne != ESpeed.size ()    ||
	ne != EAngle.size ()    ||
	ne != EMovement.size () ||
	ne != ECount.size ()    ||
	ne != EH.size ()        ||
	ne != EDh.size ()       ||
	ne != EL.size ()        ||
	ne != EDl.size ()       ||
	ne != EA.size ()        ||
	ne != EDa.size ()       ||
	ne != EDx.size ()       ||
	ne != EDy.size ()       ||
	ne != EDcirc.size ()    ||
	ne != EVx.size ()       ||
	ne != EVy.size ()       ||
	ne != EVt.size ()       ||
	ne != EVphi.size ()     ||
	ne != EDvx.size ()      ||
	ne != EDvy.size ()      ||
	ne != EDvcirc.size ()   ||
	ne != EDvt.size ()      ||
	ne != EDvphi.size ()    ||
	ne != ES.size ()        ||
	ne != EDs.size ()       ||
	ne != ESnew.size ()     ||
	ne != EDsnew.size ()    ||
	ne != EG.size ()        ||
	ne != EDg.size ()       ||
	ne != EGp.size ())
       return;

    ps->e.clear ();

    for (unsigned int i = 0; i < ne; i++)
    {
	Emitter ei;
	ei.set_active = ei.active = EActive.at (i).b ();
	ei.trigger = ETrigger.at (i).i ();
	ei.x = (float)EPosx.at (i).i ();
	ei.y = (float)EPosy.at (i).i ();
	ei.espeed = (float)ESpeed.at (i).i () / 100.;
	ei.eangle = (float)EAngle.at (i).i () / 180.*M_PI;
	ei.movement = EMovement.at (i).i ();
	ei.count = (float)ECount.at (i).i ();
	ei.h = (float)EH.at (i).i () / 1000.;
	ei.dh = (float)EDh.at (i).i () / 1000.;
	ei.l = (float)EL.at (i).i () / 1000.;
	ei.dl = (float)EDl.at (i).i () / 1000.;
	ei.a = (float)EA.at (i).i () / 1000.;
	ei.da = (float)EDa.at (i).i () / 1000.;
	ei.dx = (float)EDx.at (i).i ();
	ei.dy = (float)EDy.at (i).i ();
	ei.dcirc = (float)EDcirc.at (i).i ();
	ei.vx = (float)EVx.at (i).i () / 1000.;
	ei.vy = (float)EVy.at (i).i () / 1000.;
	ei.vt = (float)EVt.at (i).i () / 10000.;
	ei.vphi = (float)EVphi.at (i).i () / 10000.;
	ei.dvx = (float)EDvx.at (i).i () / 1000.;
	ei.dvy = (float)EDvy.at (i).i () / 1000.;
	ei.dvcirc = (float)EDvcirc.at (i).i () / 1000.;
	ei.dvt = (float)EDvt.at (i).i () / 10000.;
	ei.dvphi = (float)EDvphi.at (i).i () / 10000.;
	ei.s = (float)ES.at (i).i ();
	ei.ds = (float)EDs.at (i).i ();
	ei.snew = (float)ESnew.at (i).i ();
	ei.dsnew = (float)EDsnew.at (i).i ();
	ei.g = (float)EG.at (i).i () / 1000.;
	ei.dg = (float)EDg.at (i).i () / 1000.;
	ei.gp = (float)EGp.at (i).i () / 10000.;
	ps->e.push_back (ei);
    }
}

void
WizardScreen::drawParticles (ParticleSystem *ps,
			     const GLMatrix &transform)
{
    int i, j, k, l;

    /* Check that the cache is big enough */
    if (ps->vertices_cache.size () < (unsigned int)ps->hardLimit * VERTEX_COMPONENTS)
	ps->vertices_cache.resize (ps->hardLimit * VERTEX_COMPONENTS);

    if (ps->coords_cache.size () < (unsigned int)ps->hardLimit * COORD_COMPONENTS)
	ps->coords_cache.resize (ps->hardLimit * COORD_COMPONENTS);

    if (ps->colors_cache.size () < (unsigned int)ps->hardLimit * COLOR_COMPONENTS)
	ps->colors_cache.resize (ps->hardLimit * COLOR_COMPONENTS);

    if (ps->darken > 0)
	if (ps->dcolors_cache.size () < (unsigned int)ps->hardLimit * COLOR_COMPONENTS)
	    ps->dcolors_cache.resize (ps->hardLimit * COLOR_COMPONENTS);

    glEnable (GL_BLEND);

    if (ps->tex)
    {
	glBindTexture (GL_TEXTURE_2D, ps->tex);
	glEnable (GL_TEXTURE_2D);
    }

    i = j = k = l = 0;

    Particle *part = &ps->particles[0];
    int m;
    for (m = 0; m < ps->hardLimit; m++, part++)
    {
	if (part->t > 0.0f)
	{
	    float cOff = part->s / 2.;		//Corner offset from center

	    if (part->t > ps->tnew)		//New particles start larger
		cOff += (part->snew - part->s) * (part->t - ps->tnew)
			/ (1. - ps->tnew) / 2.;
	    else if (part->t < ps->told)	//Old particles shrink
		cOff -= part->s * (ps->told - part->t) / ps->told / 2.;

	    //Offsets after rotation of Texture
	    float offA = cOff * (cos (part->phi) - sin (part->phi));
	    float offB = cOff * (cos (part->phi) + sin (part->phi));

	    GLushort r, g, b, a, dark_a;

	    r = part->c[0] * 65535.0f;
	    g = part->c[1] * 65535.0f;
	    b = part->c[2] * 65535.0f;

	    if (part->t > ps->tnew)		//New particles start at a == 1
		a = part->a + (1. - part->a) * (part->t - ps->tnew)
			    / (1. - ps->tnew) * 65535.0f;
	    else if (part->t < ps->told)	//Old particles fade to a = 0
		a = part->a * part->t / ps->told * 65535.0f;
	    else				//The others have their own a
		a = part->a * 65535.0f;

	    dark_a = a * ps->darken;

	    //first triangle
	    ps->vertices_cache[i + 0] = part->x - offB;
	    ps->vertices_cache[i + 1] = part->y - offA;
	    ps->vertices_cache[i + 2] = 0;

	    ps->vertices_cache[i + 3] = part->x - offA;
	    ps->vertices_cache[i + 4] = part->y + offB;
	    ps->vertices_cache[i + 5] = 0;

	    ps->vertices_cache[i + 6] = part->x + offB;
	    ps->vertices_cache[i + 7] = part->y + offA;
	    ps->vertices_cache[i + 8] = 0;

	    //second triangle
	    ps->vertices_cache[i + 9] = part->x + offB;
	    ps->vertices_cache[i + 10] = part->y + offA;
	    ps->vertices_cache[i + 11] = 0;

	    ps->vertices_cache[i + 12] = part->x + offA;
	    ps->vertices_cache[i + 13] = part->y - offB;
	    ps->vertices_cache[i + 14] = 0;

	    ps->vertices_cache[i + 15] = part->x - offB;
	    ps->vertices_cache[i + 16] = part->y - offA;
	    ps->vertices_cache[i + 17] = 0;

	    i += 18;

	    ps->coords_cache[j + 0] = 0.0;
	    ps->coords_cache[j + 1] = 0.0;

	    ps->coords_cache[j + 2] = 0.0;
	    ps->coords_cache[j + 3] = 1.0;

	    ps->coords_cache[j + 4] = 1.0;
	    ps->coords_cache[j + 5] = 1.0;

	    //second
	    ps->coords_cache[j + 6] = 1.0;
	    ps->coords_cache[j + 7] = 1.0;

	    ps->coords_cache[j + 8] = 1.0;
	    ps->coords_cache[j + 9] = 0.0;

	    ps->coords_cache[j + 10] = 0.0;
	    ps->coords_cache[j + 11] = 0.0;

	    j += 12;

	    ps->colors_cache[k + 0] = r;
	    ps->colors_cache[k + 1] = g;
	    ps->colors_cache[k + 2] = b;
	    ps->colors_cache[k + 3] = a;

	    ps->colors_cache[k + 4] = r;
	    ps->colors_cache[k + 5] = g;
	    ps->colors_cache[k + 6] = b;
	    ps->colors_cache[k + 7] = a;

	    ps->colors_cache[k + 8] = r;
	    ps->colors_cache[k + 9] = g;
	    ps->colors_cache[k + 10] = b;
	    ps->colors_cache[k + 11] = a;

	    //second
	    ps->colors_cache[k + 12] = r;
	    ps->colors_cache[k + 13] = g;
	    ps->colors_cache[k + 14] = b;
	    ps->colors_cache[k + 15] = a;

	    ps->colors_cache[k + 16] = r;
	    ps->colors_cache[k + 17] = g;
	    ps->colors_cache[k + 18] = b;
	    ps->colors_cache[k + 19] = a;

	    ps->colors_cache[k + 20] = r;
	    ps->colors_cache[k + 21] = g;
	    ps->colors_cache[k + 22] = b;
	    ps->colors_cache[k + 23] = a;

	    k += 24;

	    if (ps->darken > 0)
	    {
		ps->dcolors_cache[l + 0] = r;
		ps->dcolors_cache[l + 1] = g;
		ps->dcolors_cache[l + 2] = b;
		ps->dcolors_cache[l + 3] = dark_a;

		ps->dcolors_cache[l + 4] = r;
		ps->dcolors_cache[l + 5] = g;
		ps->dcolors_cache[l + 6] = b;
		ps->dcolors_cache[l + 7] = dark_a;

		ps->dcolors_cache[l + 8] = r;
		ps->dcolors_cache[l + 9] = g;
		ps->dcolors_cache[l + 10] = b;
		ps->dcolors_cache[l + 11] = dark_a;

		//second
		ps->dcolors_cache[l + 12] = r;
		ps->dcolors_cache[l + 13] = g;
		ps->dcolors_cache[l + 14] = b;
		ps->dcolors_cache[l + 15] = dark_a;

		ps->dcolors_cache[l + 16] = r;
		ps->dcolors_cache[l + 17] = g;
		ps->dcolors_cache[l + 18] = b;
		ps->dcolors_cache[l + 19] = dark_a;

		ps->dcolors_cache[l + 20] = r;
		ps->dcolors_cache[l + 21] = g;
		ps->dcolors_cache[l + 22] = b;
		ps->dcolors_cache[l + 23] = dark_a;

		l += 24;
	    }
	}
    }

    GLVertexBuffer *stream = GLVertexBuffer::streamingBuffer ();

    if (ps->darken > 0)
    {
	glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	stream->begin (GL_TRIANGLES);
	stream->addVertices (i / 3, &ps->vertices_cache[0]);
	stream->addTexCoords (0, j / 2, &ps->coords_cache[0]);
	stream->addColors (l / 4, &ps->dcolors_cache[0]);

	if (stream->end ())
	    stream->render (transform);
    }

    /* draw particles */
    glBlendFunc (GL_SRC_ALPHA, ps->blendMode);
    stream->begin (GL_TRIANGLES);

    stream->addVertices (i / 3, &ps->vertices_cache[0]);
    stream->addTexCoords (0, j / 2, &ps->coords_cache[0]);
    stream->addColors (k / 4, &ps->colors_cache[0]);

    if (stream->end ())
	stream->render (transform);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_TEXTURE_2D);
    glDisable (GL_BLEND);
}

static void
updateParticles (ParticleSystem * ps, float time)
{
    int i, j;
    int newCount = 0;
    Particle *part;
    GPoint *gi;
    float gdist, gangle;
    ps->active = false;

    part = &ps->particles[0];
    for (i = 0; i < ps->hardLimit; i++, part++)
    {
	if (part->t > 0.0f)
	{
	    // move particle
	    part->x += part->vx * time;
	    part->y += part->vy * time;

	    // Rotation
	    part->phi += part->vphi*time;

	    //Aging of particles
	    part->t += part->vt * time;
	    //Additional aging of particles increases if softLimit is exceeded
	    if (ps->lastCount > ps->softLimit)
		part->t += part->vt * time * (ps->lastCount - ps->softLimit)
			   / (ps->hardLimit - ps->softLimit);

	    //Global gravity
	    part->vx += ps->gx * time;
	    part->vy += ps->gy * time;

	    //GPoint gravity
	    gi = &ps->g[0];
	    for (j = 0; (unsigned int)j < ps->g.size (); j++, gi++)
	    {
		if (gi->strength != 0)
		{
		    gdist = sqrt ((part->x-gi->x)*(part->x-gi->x)
				  + (part->y-gi->y)*(part->y-gi->y));
		    if (gdist > 1)
		    {
			gangle = atan2 (gi->y-part->y, gi->x-part->x);
			part->vx += gi->strength / gdist * cos (gangle) * time;
			part->vy += gi->strength / gdist * sin (gangle) * time;
		    }
		}
	    }

	    ps->active  = true;
	    newCount++;
	}
    }
    ps->lastCount = newCount;

    //Particle gravity
    Particle *gpart;
    part = &ps->particles[0];
    for (i = 0; i < ps->hardLimit; i++, part++)
    {
	if (part->t > 0.0f && part->g != 0)
	{
	    gpart = &ps->particles[0];
	    for (j = 0; j < ps->hardLimit; j++, gpart++)
	    {
		if (gpart->t > 0.0f)
		{
		    gdist = sqrt ((part->x-gpart->x)*(part->x-gpart->x)
				  + (part->y-gpart->y)*(part->y-gpart->y));
		    if (gdist > 1)
		    {
			gangle = atan2 (part->y-gpart->y, part->x-gpart->x);
			gpart->vx += part->g/gdist* cos (gangle) * part->t*time;
			gpart->vy += part->g/gdist* sin (gangle) * part->t*time;
		    }
		}
	    }
	}
    }
}

static void
finiParticles (ParticleSystem * ps)
{
    if (ps->tex)
	glDeleteTextures (1, &ps->tex);

}


static void
genNewParticles (ParticleSystem *ps, Emitter *e)
{

    float q, p, t = 0, h, l;
    int count = e->count;

    Particle *part = &ps->particles[0];
    int i, j;

    for (i = 0; i < ps->hardLimit && count > 0; i++, part++)
    {
	if (part->t <= 0.0f)
	{
	    //Position
	    part->x = rRange (e->x, e->dx);		// X Position
	    part->y = rRange (e->y, e->dy);		// Y Position
	    if ((q = rRange (e->dcirc/2.,e->dcirc/2.)) > 0)
	    {
		p = rRange (0, M_PI);
		part->x += q * cos (p);
		part->y += q * sin (p);
	    }

	    //Speed
	    part->vx = rRange (e->vx, e->dvx);		// X Speed
	    part->vy = rRange (e->vy, e->dvy);		// Y Speed
	    if ((q = rRange (e->dvcirc/2.,e->dvcirc/2.)) > 0)
	    {
		p = rRange (0, M_PI);
		part->vx += q * cos (p);
		part->vy += q * sin (p);
	    }
	    part->vt = rRange (e->vt, e->dvt);		// Aging speed
	    if (part->vt > -0.0001)
		part->vt = -0.0001;

	    //Size, Gravity and Rotation
	    part->s = rRange (e->s, e->ds);		// Particle size
	    part->snew = rRange (e->snew, e->dsnew);	// Particle start size
	    if (e->gp > (float)(random () & 0xffff) / 65535.)
		part->g = rRange (e->g, e->dg);		// Particle gravity
	    else
		part->g = 0.;
	    part->phi = rRange (0, M_PI);		// Random orientation
	    part->vphi = rRange (e->vphi, e->dvphi);	// Rotation speed

	    //Alpha
	    part->a = rRange (e->a, e->da);		// Alpha
	    if (part->a > 1)
		part->a = 1.;
	    else if (part->a < 0)
		part->a = 0.;

	    //HSL to RGB conversion from Wikipedia simplified by S = 1
	    h = rRange (e->h, e->dh); //Random hue within range
	    if (h < 0)
		h += 1.;
	    else if (t > 1)
		h -= 1.;
	    l = rRange (e->l, e->dl); //Random lightness ...
	    if (l > 1)
		l = 1.;
	    else if (l < 0)
		l = 0.;
	    q = e->l * 2;
	    if (q > 1)
		q = 1.;
	    p = 2 * e->l - q;
	    for (j = 0; j < 3; j++)
	    {
		t = h + (1-j)/3.;
		if (t < 0)
		    t += 1.;
		else if (t > 1)
		    t -= 1.;
		if (t < 1/6.)
		    part->c[j] = p + ((q-p)*6*t);
		else if (t < .5)
		    part->c[j] = q;
		else if (t < 2/3.)
		    part->c[j] = p + ((q-p)*6*(2/3.-t));
		else
		    part->c[j] = p;
	    }

	    // give new life
	    part->t = 1.;

	    ps->active = true;
	    count -= 1;
	}
    }
}

void
WizardScreen::positionUpdate (const CompPoint &pos)
{
    mx = pos.x ();
    my = pos.y ();

    if (ps && active)
    {
	Emitter *ei = &ps->e[0];
	GPoint  *gi = &ps->g[0];

	for (unsigned int i = 0; i < ps->g.size (); i++, gi++)
	{
	    if (gi->movement == MOVEMENT_MOUSEPOSITION)
	    {
		gi->x = pos.x ();
		gi->y = pos.y ();
	    }
	}

	for (unsigned int i = 0; i < ps->e.size (); i++, ei++)
	{
	    if (ei->movement == MOVEMENT_MOUSEPOSITION)
	    {
		ei->x = pos.x ();
		ei->y = pos.y ();
	    }
	    if (ei->active && ei->trigger == TRIGGER_MOUSEMOVEMENT)
		genNewParticles (ps, ei);
	}
    }
}

void
WizardScreen::preparePaint (int time)
{
    if (active && !pollHandle.active ())
	pollHandle.start ();

    if (active && !ps)
    {
	ps = (ParticleSystem*) calloc(1, sizeof (ParticleSystem));
	if (!ps)
	{
	    cScreen->preparePaint (time);
	    return;
	}
	loadGPoints (ps);
	loadEmitters (ps);
	initParticles (optionGetHardLimit (), optionGetSoftLimit (), ps);
	ps->darken = optionGetDarken ();
	ps->blendMode = (optionGetBlend ()) ? GL_ONE :
					      GL_ONE_MINUS_SRC_ALPHA;
	ps->tnew = optionGetTnew ();
	ps->told = optionGetTold ();
	ps->gx = optionGetGx ();
	ps->gy = optionGetGy ();

	glGenTextures (1, &ps->tex);
	glBindTexture (GL_TEXTURE_2D, ps->tex);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0,
		      GL_RGBA, GL_UNSIGNED_BYTE, particleTex);
	glBindTexture (GL_TEXTURE_2D, 0);
    }

    if (ps && active)
    {
	Emitter *ei = &ps->e[0];
	GPoint *gi = &ps->g[0];

	for (unsigned int i = 0; i < ps->g.size (); i++, gi++)
	{
	    if (gi->movement==MOVEMENT_BOUNCE || gi->movement==MOVEMENT_WRAP)
	    {
		gi->x += gi->espeed * cos (gi->eangle) * time;
		gi->y += gi->espeed * sin (gi->eangle) * time;
		if (gi->x >= screen->width ())
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->x = 2*screen->width () - gi->x - 1;
			gi->eangle = M_PI - gi->eangle;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->x -= screen->width ();
		}
		else if (gi->x < 0)
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->x *= -1;
			gi->eangle = M_PI - gi->eangle;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->x += screen->width ();
		}
		if (gi->y >= screen->height ())
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->y = 2*screen->height () - gi->y - 1;
			gi->eangle *= -1;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->y -= screen->height ();
		}
		else if (gi->y < 0)
		{
		    if (gi->movement==MOVEMENT_BOUNCE)
		    {
			gi->y *= -1;
			gi->eangle *= -1;
		    }
		    else if (gi->movement==MOVEMENT_WRAP)
			gi->y += screen->height ();
		}
	    }
	    if (gi->movement==MOVEMENT_FOLLOWMOUSE
		&& (my!=gi->y||mx!=gi->x))
	    {
		gi->eangle = atan2(my-gi->y, mx-gi->x);
		gi->x += gi->espeed * cos(gi->eangle) * time;
		gi->y += gi->espeed * sin(gi->eangle) * time;
	    }
	}

	for (unsigned int i = 0; i < ps->e.size (); i++, ei++)
	{
	    if (ei->movement==MOVEMENT_BOUNCE || ei->movement==MOVEMENT_WRAP)
	    {
		ei->x += ei->espeed * cos (ei->eangle) * time;
		ei->y += ei->espeed * sin (ei->eangle) * time;
		if (ei->x >= screen->width ())
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->x = 2*screen->width () - ei->x - 1;
			ei->eangle = M_PI - ei->eangle;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->x -= screen->width ();
		}
		else if (ei->x < 0)
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->x *= -1;
			ei->eangle = M_PI - ei->eangle;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->x += screen->width ();
		}
		if (ei->y >= screen->height ())
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->y = 2*screen->height () - ei->y - 1;
			ei->eangle *= -1;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->y -= screen->height ();
		}
		else if (ei->y < 0)
		{
		    if (ei->movement==MOVEMENT_BOUNCE)
		    {
			ei->y *= -1;
			ei->eangle *= -1;
		    }
		    else if (ei->movement==MOVEMENT_WRAP)
			ei->y += screen->height ();
		}
	    }
	    if (ei->movement==MOVEMENT_FOLLOWMOUSE
		&& (my!=ei->y||mx!=ei->x))
	    {
		ei->eangle = atan2 (my-ei->y, mx-ei->x);
		ei->x += ei->espeed * cos (ei->eangle) * time;
		ei->y += ei->espeed * sin (ei->eangle) * time;
	    }
	    if (ei->trigger == TRIGGER_RANDOMPERIOD
		&& ei->set_active && !((int)random ()&0xff))
		ei->active = !ei->active;
	    if (ei->active && (
		    (ei->trigger == TRIGGER_PERSISTENT) ||
		    (ei->trigger == TRIGGER_RANDOMSHOT && !((int)random()&0xff)) ||
		    (ei->trigger == TRIGGER_RANDOMPERIOD)
		    ))
		genNewParticles (ps, ei);
	}
    }

    if (ps && ps->active)
    {
	updateParticles (ps, time);
	cScreen->damageScreen ();
    }

    cScreen->preparePaint (time);
}

void
WizardScreen::donePaint ()
{
    if (active || (ps && ps->active))
	cScreen->damageScreen ();

    if (!active && pollHandle.active ())
	pollHandle.stop ();

    if (!active && ps && !ps->active)
    {
	finiParticles (ps);
	free (ps);
	ps = NULL;
	toggleFunctions (false);
    }

    cScreen->donePaint ();
}

bool
WizardScreen::glPaintOutput (const GLScreenPaintAttrib	&sa,
			     const GLMatrix		&transform,
			     const CompRegion		&region,
			     CompOutput			*output,
			     unsigned int		mask)
{
    bool status = gScreen->glPaintOutput (sa, transform, region, output, mask);
    GLMatrix sTransform = transform;

    if (!ps || !ps->active)
	return status;

    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    drawParticles (ps, sTransform);

    return status;
}

bool
WizardScreen::toggle ()
{
    active = !active;
    if (active)
	toggleFunctions (true);

    cScreen->damageScreen ();
    return true;
}

void
WizardScreen::toggleFunctions(bool enabled)
{
    cScreen->preparePaintSetEnabled (this, enabled);
    cScreen->donePaintSetEnabled (this, enabled);
    gScreen->glPaintOutputSetEnabled (this, enabled);
}

void
WizardScreen::optionChanged (CompOption	      *opt,
			     WizardOptions::Options num)
{
    loadGPoints (ps);
    loadEmitters (ps);
}

WizardScreen::WizardScreen (CompScreen *screen) :
    PluginClassHandler <WizardScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    active (false),
    ps (NULL)
{
    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

#define optionNotify(name)						       \
    optionSet##name##Notify (boost::bind (&WizardScreen::optionChanged,	       \
    this, _1, _2))

    optionNotify (GStrength);
    optionNotify (GPosx);
    optionNotify (GPosy);
    optionNotify (GSpeed);
    optionNotify (GAngle);
    optionNotify (GMovement);
    optionNotify (EActive);
    optionNotify (EName);
    optionNotify (ETrigger);
    optionNotify (EPosx);
    optionNotify (EPosy);
    optionNotify (ESpeed);
    optionNotify (EAngle);
    optionNotify (GMovement);
    optionNotify (ECount);
    optionNotify (EH);
    optionNotify (EDh);
    optionNotify (EL);
    optionNotify (EDl);
    optionNotify (EA);
    optionNotify (EDa);
    optionNotify (EDx);
    optionNotify (EDy);
    optionNotify (EDcirc);
    optionNotify (EVx);
    optionNotify (EVy);
    optionNotify (EVt);
    optionNotify (EVphi);
    optionNotify (EDvx);
    optionNotify (EDvy);
    optionNotify (EDvcirc);
    optionNotify (EDvt);
    optionNotify (EDvphi);
    optionNotify (ES);
    optionNotify (EDs);
    optionNotify (ESnew);
    optionNotify (EDsnew);
    optionNotify (EG);
    optionNotify (EDg);
    optionNotify (EGp);

#undef optionNotify

    pollHandle.setCallback (boost::bind (&WizardScreen::positionUpdate, this, _1));

    optionSetToggleInitiate (boost::bind (&WizardScreen::toggle, this));
}

WizardScreen::~WizardScreen ()
{
    if (pollHandle.active ())
	pollHandle.stop ();

    if (ps && ps->active)
	cScreen->damageScreen ();
}

bool
WizardPluginVTable::init ()
{
    if (CompPlugin::checkPluginABI ("core", CORE_ABIVERSION)		&&
	CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI)	&&
	CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI)	&&
	CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return true;

    return false;
}
