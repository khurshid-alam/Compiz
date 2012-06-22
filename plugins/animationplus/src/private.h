#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#ifdef USE_LIBRSVG
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#endif

#include <core/core.h>
#include <opengl/opengl.h>
#include <composite/composite.h>
#include <animation/animation.h>
#include <animationaddon/animationaddon.h>

#include "animationplus_options.h"

extern AnimEffect AnimEffectBlinds;
extern AnimEffect AnimEffectBonanza;
extern AnimEffect AnimEffectHelix;
extern AnimEffect AnimEffectShatter;

#define NUM_EFFECTS 4

// This must have the value of the first "effect setting" above
// in AnimEgScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

#define WIN_X(w) ((w)->x () - (w)->input ().left)
#define WIN_Y(w) ((w)->y () - (w)->input ().top)
#define WIN_W(w) ((w)->width () + (w)->input ().left + (w)->input ().right)
#define WIN_H(w) ((w)->height () + (w)->input ().top + (w)->input ().bottom)

class ExtensionPluginAnimPlus : public ExtensionPluginInfo
{
    public:

	ExtensionPluginAnimPlus (const CompString &name,
				 unsigned int	  nEffects,
				 AnimEffect	  *effects,
				 CompOption::Vector *effectOptions,
				 unsigned int	  firstEffectOptionIndex) :
		ExtensionPluginInfo (name, nEffects, effects, effectOptions,
				      firstEffectOptionIndex) {}
	~ExtensionPluginAnimPlus () {}

	const CompOutput *output () { return mOutput; }
    private:

	const CompOutput *mOutput;
};

class BasePlusAnim :
    virtual public Animation
{
    public:

	BasePlusAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float	 duration,
		      const AnimEffect info,
		      const CompRect   &icon);

	~BasePlusAnim () {}

    protected:
	// Gets info about the extension plugin that implements this animation.
	ExtensionPluginInfo* getExtensionPluginInfo ();

	CompositeScreen *mCScreen;
	GLScreen	*mGScreen;
};

class AnimPlusScreen :
    public PluginClassHandler <AnimPlusScreen, CompScreen>,
    public AnimationplusOptions
{
    public:

	AnimPlusScreen (CompScreen *);
	~AnimPlusScreen ();

    protected:

	void initAnimationList ();

	CompOutput &mOutput;
};

class AnimPlusWindow :
    public PluginClassHandler <AnimPlusWindow, CompWindow>
{
    public:

	AnimPlusWindow (CompWindow *);
	~AnimPlusWindow ();

    protected:

	CompWindow *mWindow;
	AnimWindow *aWindow;

};

#define ANIMPLUS_SCREEN(s)					    \
    AnimPlusScreen *as = AnimPlusScreen::get (s);

#define ANIMPLUS_WINDOW(w)					    \
    AnimPlusWindow *aw = AnimPlusWindow::get (w);

// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#define EXPLODE_PERCEIVED_T 0.7f

/*
 * Function prototypes
 *
 */

class BlindsAnim : public PolygonAnim
{
    public:

	BlindsAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float	    duration,
		    const	    AnimEffect info,
		    const	    CompRect   &icon);

	void init ();
    protected:
	static const float kDurationFactor;
};

class HelixAnim : public PolygonAnim
{
    public:
	HelixAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float       duration,
		   const       AnimEffect info,
		   const       CompRect   &icon);

	void init ();
    protected:
	static const float kDurationFactor;
};

class BonanzaAnim : public ParticleAnim
{
    public:
	BonanzaAnim (CompWindow *w,
		     WindowEvent curWindowEvent,
		     float	 duration,
		     const	 AnimEffect info,
		     const	 CompRect   &icon);

	void
	genFire (int x,
		 int y,
		 int radius,
		 float size,
		float time);

	void step (float);
    protected:

	int  mAnimFireDirection;
	unsigned int mFirePDId;
};

class ShatterAnim : public PolygonAnim
{
    public:
	ShatterAnim (CompWindow *w,
		     WindowEvent curWindowEvent,
		     float	    duration,
		     const	    AnimEffect info,
		     const	    CompRect   &icon);

	void init ();
    protected:
	static const float kDurationFactor;
};

class AnimPlusPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <AnimPlusScreen, AnimPlusWindow>
{
    public:

	bool init ();
};
