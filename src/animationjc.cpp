#include "private.h"

class AnimJCPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<AnimJCScreen, AnimJCWindow>
{
public:
    bool init ();
    void fini ();
};

COMPIZ_PLUGIN_20090315 (animationjc, AnimJCPluginVTable);

AnimEffect animEffects[NUM_EFFECTS];

ExtensionPluginAnimJC animJCExtPluginInfo (CompString ("animationjc"),
						 NUM_EFFECTS, animEffects, NULL,
                                                 NUM_NONEFFECT_OPTIONS);

AnimEffect AnimEffectBlackHole;
AnimEffect AnimEffectRaindrop;

ExtensionPluginInfo *
BaseAddonAnim::getExtensionPluginInfo ()
{
    return &animJCExtPluginInfo;
}

BaseAddonAnim::BaseAddonAnim (CompWindow *w,
			      WindowEvent curWindowEvent,
			      float duration,
			      const AnimEffect info,
			      const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    mIntenseTimeStep (AnimJCScreen::get (::screen)->getIntenseTimeStep ()),
    mCScreen (CompositeScreen::get (::screen)),
    mGScreen (GLScreen::get (::screen)),
    mDoDepthTest (false)
{
}

void
PrivateAnimJCScreen::initAnimationList ()
{
    int i = 0;

    animEffects[i++] = AnimEffectBlackHole =
        new AnimEffectInfo ("animationjc:Black Hole",
                            true, true, true, false, false,
                            &createAnimation<BlackHoleAnim>);
    animEffects[i++] = AnimEffectRaindrop =
        new AnimEffectInfo ("animationjc:Raindrop",
                            true, true, true, false, true,
                            &createAnimation<RaindropAnim>);

    animJCExtPluginInfo.effectOptions = &getOptions ();

    AnimScreen *as = AnimScreen::get (::screen);

    // Extends animation plugin with this set of animation effects.
    as->addExtension (&animJCExtPluginInfo);
}

PrivateAnimJCScreen::PrivateAnimJCScreen (CompScreen *s) :
    //cScreen (CompositeScreen::get (s)),
    //gScreen (GLScreen::get (s)),
    //aScreen (as),
    mOutput (s->fullscreenOutput ())
{
    initAnimationList ();
}

PrivateAnimJCScreen::~PrivateAnimJCScreen ()
{
    AnimScreen *as = AnimScreen::get (::screen);

    as->removeExtension (&animJCExtPluginInfo);

    for (int i = 0; i < NUM_EFFECTS; i++)
    {
	delete animEffects[i];
	animEffects[i] = NULL;
    }
}

AnimJCScreen::AnimJCScreen (CompScreen *s) :
    PluginClassHandler<AnimJCScreen, CompScreen, ANIMATIONADDON_ABI> (s),
    priv (new PrivateAnimJCScreen (s))
{
}

AnimJCScreen::~AnimJCScreen ()
{
    delete priv;
}

CompOption::Vector &
AnimJCScreen::getOptions ()
{
    return priv->getOptions ();
}

bool
AnimJCScreen::setOption (const CompString  &name,
                            CompOption::Value &value)
{
    return priv->setOption (name, value);
}

AnimJCWindow::AnimJCWindow (CompWindow *w) :
    PluginClassHandler<AnimJCWindow, CompWindow> (w),
    mWindow (w),
    aWindow (AnimWindow::get (w))
{
}

AnimJCWindow::~AnimJCWindow ()
{
    Animation *curAnim = aWindow->curAnimation ();

    if (!curAnim)
	return;

    // We need to interrupt and clean up the animation currently being played
    // by animationaddon for this window (if any)
    if (curAnim->remainingTime () > 0 &&
	curAnim->getExtensionPluginInfo ()->name ==
	    CompString ("animationjc"))
    {
	aWindow->postAnimationCleanUp ();
    }
}

bool
AnimJCPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) |
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) |
        !CompPlugin::checkPluginABI ("animation", ANIMATION_ABI))
	 return false;

    CompPrivate p;
    p.uval = ANIMATIONADDON_ABI;
    ::screen->storeValue ("animationaddon_ABI", p);

    return true;
}

void
AnimJCPluginVTable::fini ()
{
    ::screen->eraseValue ("animationaddon_ABI");
}
