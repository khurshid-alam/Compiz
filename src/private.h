#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <animation/animation.h>
#include <animationjc/animationjc.h>

#include "animationjc_options.h"

extern AnimEffect AnimEffectBlackHole;
extern AnimEffect AnimEffectRaindrop;

#define NUM_EFFECTS 2

// This must have the value of the first "effect setting" above
// in AnimJCScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

class ExtensionPluginAnimJC : public ExtensionPluginInfo
{
public:
    ExtensionPluginAnimJC (const CompString &name,
			      unsigned int nEffects,
			      AnimEffect *effects,
			      CompOption::Vector *effectOptions,
			      unsigned int firstEffectOptionIndex) :
	ExtensionPluginInfo (name, nEffects, effects, effectOptions,
			     firstEffectOptionIndex) {}
    ~ExtensionPluginAnimJC () {}

    //void prePaintOutput (CompOutput *output);
    const CompOutput *output () { return mOutput; }

private:
    const CompOutput *mOutput;
};

class PrivateAnimJCScreen :
    public AnimationjcOptions
{
    friend class AnimJCScreen;

public:
    PrivateAnimJCScreen (CompScreen *);
    ~PrivateAnimJCScreen ();

protected:
    void initAnimationList ();

    CompOutput &mOutput;
};

class AnimJCWindow :
    public PluginClassHandler<AnimJCWindow, CompWindow>
{
public:
    AnimJCWindow (CompWindow *);
    ~AnimJCWindow ();

protected:
    CompWindow *mWindow;    ///< Window being animated.
    AnimWindow *aWindow;
};

class BlackHoleAnim :
    public GridTransformAnim
{
public:
    BlackHoleAnim (CompWindow *w,
                   WindowEvent curWindowEvent,
                   float duration,
                   const AnimEffect info,
                   const CompRect &icon);

    float getBlackHoleProgress () { return progressLinear (); }

    void initGrid ();
    inline bool using3D () { return false; }
    void step ();
};

class RaindropAnim :
    public GridTransformAnim
{
public:
    RaindropAnim (CompWindow *w,
                  WindowEvent curWindowEvent,
                  float duration,
                  const AnimEffect info,
                  const CompRect &icon);

    float getRaindropProgress () { return progressLinear (); }

    void initGrid ();

    inline bool using3D () { return true; }

    void step ();
};

