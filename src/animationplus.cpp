/**
 * Example Animation extension plugin for compiz
 *
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
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <GL/glu.h>
#include "private.h"

COMPIZ_PLUGIN_20090315 (animationplus, AnimPlusPluginVTable);

AnimEffect animEffects[NUM_EFFECTS];

ExtensionPluginAnimPlus animPlusExtPluginInfo (CompString ("animationplus"),
						 NUM_EFFECTS, animEffects, NULL,
                                                 NUM_NONEFFECT_OPTIONS);

ExtensionPluginInfo *
BasePlusAnim::getExtensionPluginInfo ()
{
    return &animPlusExtPluginInfo;
}

BasePlusAnim::BasePlusAnim (CompWindow *w,
			    WindowEvent curWindowEvent,
			    float	duration,
			    const AnimEffect info,
			    const CompRect   &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    mCScreen (CompositeScreen::get (::screen)),
    mGScreen (GLScreen::get (::screen))
{
}

/*
// For effects with custom polygon step functions:
AnimExtEffectProperties fxAirplaneExtraProp = {
    .animStepPolygonFunc = fxAirplaneLinearAnimStepPolygon};
*/

AnimEffect AnimEffectBlinds;
AnimEffect AnimEffectHelix;
AnimEffect AnimEffectShatter;
AnimEffect AnimEffectBonanza;

void
AnimPlusScreen::initAnimationList ()
{
    int i = 0;

    animEffects[i++] = AnimEffectBlinds = 
	new AnimEffectInfo ("animationplus:Blinds",
			     true, true, true, false, false,
			     &createAnimation <BlindsAnim>);

    /* Currently broken */

    animEffects[i++] = AnimEffectBonanza =
	new AnimEffectInfo ("animationplus:Bonanza",
			    true, true, true, false, false,
			    &createAnimation <BonanzaAnim>);

    animEffects[i++] = AnimEffectHelix =
	new AnimEffectInfo ("animationplus:Helix",
			   true, true, true, false, false,
			   &createAnimation <HelixAnim>);

    animEffects[i++] = AnimEffectShatter =
	new AnimEffectInfo ("animationplus:Shatter",
			   true, true, true, false, false,
			   &createAnimation <ShatterAnim>);

    animPlusExtPluginInfo.effectOptions = &getOptions ();

    AnimScreen *as = AnimScreen::get (::screen);

    as->addExtension (&animPlusExtPluginInfo);
}

AnimPlusScreen::AnimPlusScreen (CompScreen *s) :
    //cScreen (CompositeScreen::get (s)),
    //gScreen (GLScreen::get (s)),
    //aScreen (as),
    PluginClassHandler <AnimPlusScreen, CompScreen> (s),
    mOutput (s->fullscreenOutput ())
{
    initAnimationList ();
}

AnimPlusScreen::~AnimPlusScreen ()
{
    AnimScreen *as = AnimScreen::get (::screen);

    as->removeExtension (&animPlusExtPluginInfo);

    for (int i = 0; i < NUM_EFFECTS; i++)
    {
	delete animEffects[i];
	animEffects[i] = NULL;
    }
}

AnimPlusWindow::AnimPlusWindow (CompWindow *w) :
    PluginClassHandler<AnimPlusWindow, CompWindow> (w),
    mWindow (w),
    aWindow (AnimWindow::get (w))
{
}

AnimPlusWindow::~AnimPlusWindow ()
{
    Animation *curAnim = aWindow->curAnimation ();

    if (!curAnim)
	return;

    // We need to interrupt and clean up the animation currently being played
    // by animationsim for this window (if any)
    if (curAnim->remainingTime () > 0 &&
	curAnim->getExtensionPluginInfo ()->name ==
	    CompString ("animationplus"))
    {
	aWindow->postAnimationCleanUp ();
    }
}

bool
AnimPlusPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
        !CompPlugin::checkPluginABI ("animation", ANIMATION_ABI) ||
        !CompPlugin::checkPluginABI ("animationaddon", ANIMATIONADDON_ABI))
	 return false;

    return true;
}
