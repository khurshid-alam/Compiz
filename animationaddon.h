#define _GNU_SOURCE
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

#include <compiz-core.h>
#include <compiz-animation.h>
#include "compiz-animationaddon.h"

extern int animDisplayPrivateIndex;
extern CompMetadata animMetadata;


extern AnimEffect AnimEffectBlinds;
extern AnimEffect AnimEffectHelix;
extern AnimEffect AnimEffectShatter;

#define NUM_EFFECTS 1

typedef enum
{
    // Misc. settings
    ANIMADDON_SCREEN_OPTION_TIME_STEP_INTENSE = 0,
    // Effect settings
    ANIMADDON_SCREEN_OPTION_HELIX_NUM_TWISTS,
    ANIMADDON_SCREEN_OPTION_HELIX_THICKNESS,
    ANIMADDON_SCREEN_OPTION_HELIX_GRIDSIZE_Y,
    ANIMADDON_SCREEN_OPTION_HELIX_DIRECTION,
    ANIMADDON_SCREEN_OPTION_HELIX_SPIN_DIRECTION,

    ANIMADDON_SCREEN_OPTION_NUM
} AnimAddonScreenOptions;

// This must have the value of the first "effect setting" above
// in AnimAddonScreenOptions
#define NUM_NONEFFECT_OPTIONS ANIMADDON_SCREEN_OPTION_HELIX_NUM_TWISTS

typedef enum _AnimAddonDisplayOptions
{
    ANIMADDON_DISPLAY_OPTION_ABI = 0,
    ANIMADDON_DISPLAY_OPTION_INDEX,
    ANIMADDON_DISPLAY_OPTION_NUM
} AnimAddonDisplayOptions;

typedef struct _AnimAddonDisplay
{
    int screenPrivateIndex;
    AnimBaseFunctions *animBaseFunctions;

    CompOption opt[ANIMADDON_DISPLAY_OPTION_NUM];
} AnimAddonDisplay;

typedef struct _AnimAddonScreen
{
    int windowPrivateIndex;

    CompOutput *output;

    CompOption opt[ANIMADDON_SCREEN_OPTION_NUM];
} AnimAddonScreen;

typedef struct _AnimAddonWindow
{
    AnimWindowCommon *com;
    AnimWindowEngineData eng;

    // for polygon engine
    int nDrawGeometryCalls;
    Bool deceleratingMotion;	// For effects that have decel. motion
    int nClipsPassed;	        /* # of clips passed to animAddWindowGeometry so far
				   in this draw step */
    Bool clipsUpdated;          // whether stored clips are updated in this anim step
} AnimAddonWindow;

#define GET_ANIMADDON_DISPLAY(d)						\
    ((AnimAddonDisplay *) (d)->base.privates[animDisplayPrivateIndex].ptr)

#define ANIMADDON_DISPLAY(d)				\
    AnimAddonDisplay *ad = GET_ANIMADDON_DISPLAY (d)

#define GET_ANIMADDON_SCREEN(s, ad)						\
    ((AnimAddonScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANIMADDON_SCREEN(s)							\
    AnimAddonScreen *as = GET_ANIMADDON_SCREEN (s, GET_ANIMADDON_DISPLAY (s->display))

#define GET_ANIMADDON_WINDOW(w, as)						\
    ((AnimAddonWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANIMADDON_WINDOW(w)					     \
    AnimAddonWindow *aw = GET_ANIMADDON_WINDOW (w,                     \
		     GET_ANIMADDON_SCREEN (w->screen,             \
		     GET_ANIMADDON_DISPLAY (w->screen->display)))

// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#define EXPLODE_PERCEIVED_T 0.7f

/*
 * Function prototypes
 *
 */

OPTION_GETTERS_HDR

int
getIntenseTimeStep (CompScreen *s);

/* helix.c */

void 
fxHelixInit( CompScreen *s, CompWindow *w);


/* particle.c */

void
initParticles (int numParticles,
               ParticleSystem * ps);

void
drawParticles (CompWindow * w,
               ParticleSystem * ps);

void
updateParticles (ParticleSystem * ps,
                 float time);

void
finiParticles (ParticleSystem * ps);

void
drawParticleSystems (CompWindow *w);

void
particlesUpdateBB (CompOutput *output,
                   CompWindow * w,
                   Box *BB);

void
particlesCleanup (CompWindow * w);

Bool
particlesPrePrepPaintScreen (CompWindow * w,
                             int msSinceLastPaint);

/* polygon.c */

Bool
tessellateIntoRectangles (CompWindow * w,
			  int gridSizeX,
			  int gridSizeY,
			  float thickness);
 
Bool
tessellateIntoHexagons (CompWindow * w,
			int gridSizeX,
			int gridSizeY,
			float thickness);

void
polygonsStoreClips (CompWindow * w,
		    int nClip, BoxPtr pClip,
		    int nMatrix, CompMatrix * matrix);
 
void
polygonsDrawCustomGeometry (CompWindow * w);

void
polygonsPrePaintWindow (CompWindow * w);
 
void
polygonsPostPaintWindow (CompWindow * w);

void
freePolygonSet (AnimAddonWindow * aw);

void
freePolygonObjects (PolygonSet * pset);
 
void
polygonsLinearAnimStepPolygon (CompWindow * w,
			       PolygonObject *p,
			       float forwardProgress);

void
polygonsDeceleratingAnimStepPolygon (CompWindow * w,
				     PolygonObject *p,
				     float forwardProgress);

void
polygonsUpdateBB (CompOutput *output,
		  CompWindow * w,
		  Box *BB);

void
polygonsAnimStep (CompWindow *w,
		  float time);

Bool
polygonsPrePreparePaintScreen (CompWindow *w,
			       int msSinceLastPaint);

void
polygonsCleanup (CompWindow *w);

Bool
polygonsAnimInit (CompWindow *w);

void
polygonsPrePaintOutput (CompScreen *s, CompOutput *output);

void
polygonsRefresh (CompWindow *w,
		 Bool animInitialized);
