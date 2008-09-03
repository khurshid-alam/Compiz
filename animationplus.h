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
#include "compiz-animationplus.h"

extern int animDisplayPrivateIndex;
extern CompMetadata animMetadata;


extern AnimEffect AnimEffectBlinds;
extern AnimEffect AnimEffectHelix;
extern AnimEffect AnimEffectShatter;

#define NUM_EFFECTS 2

typedef enum
{
    // Misc. settings
    ANIMPLUS_SCREEN_OPTION_TIME_STEP_INTENSE = 0,
    // Effect settings
    ANIMPLUS_SCREEN_OPTION_BLINDS_HALFTWISTS,
    ANIMPLUS_SCREEN_OPTION_BLINDS_GRIDX,
    ANIMPLUS_SCREEN_OPTION_BLINDS_THICKNESS,
    ANIMPLUS_SCREEN_OPTION_HELIX_NUM_TWISTS,
    ANIMPLUS_SCREEN_OPTION_HELIX_GRIDSIZE_Y,
    ANIMPLUS_SCREEN_OPTION_HELIX_THICKNESS,
    ANIMPLUS_SCREEN_OPTION_HELIX_DIRECTION,
    ANIMPLUS_SCREEN_OPTION_HELIX_SPIN_DIRECTION,

    ANIMPLUS_SCREEN_OPTION_NUM
} AnimPlusScreenOptions;

// This must have the value of the first "effect setting" above
// in AnimPlusScreenOptions
#define NUM_NONEFFECT_OPTIONS ANIMPLUS_SCREEN_OPTION_HELIX_NUM_TWISTS

typedef enum _AnimPlusDisplayOptions
{
    ANIMPLUS_DISPLAY_OPTION_ABI = 0,
    ANIMPLUS_DISPLAY_OPTION_INDEX,
    ANIMPLUS_DISPLAY_OPTION_NUM
} AnimPlusDisplayOptions;

typedef struct _AnimPlusDisplay
{
    int screenPrivateIndex;
    AnimBaseFunctions *animBaseFunctions;

    CompOption opt[ANIMPLUS_DISPLAY_OPTION_NUM];
} AnimPlusDisplay;

typedef struct _AnimPlusScreen
{
    int windowPrivateIndex;

    CompOutput *output;

    CompOption opt[ANIMPLUS_SCREEN_OPTION_NUM];
} AnimPlusScreen;

typedef struct _AnimPlusWindow
{
    AnimWindowCommon *com;
    AnimWindowEngineData eng;

    // for polygon engine
    int nDrawGeometryCalls;
    Bool deceleratingMotion;	// For effects that have decel. motion
    int nClipsPassed;	        /* # of clips passed to animAddWindowGeometry so far
				   in this draw step */
    Bool clipsUpdated;          // whether stored clips are updated in this anim step
} AnimPlusWindow;

#define GET_ANIMPLUS_DISPLAY(d)						\
    ((AnimPlusDisplay *) (d)->base.privates[animDisplayPrivateIndex].ptr)

#define ANIMPLUS_DISPLAY(d)				\
    AnimPlusDisplay *ad = GET_ANIMPLUS_DISPLAY (d)

#define GET_ANIMPLUS_SCREEN(s, ad)						\
    ((AnimPlusScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANIMPLUS_SCREEN(s)							\
    AnimPlusScreen *as = GET_ANIMPLUS_SCREEN (s, GET_ANIMPLUS_DISPLAY (s->display))

#define GET_ANIMPLUS_WINDOW(w, as)						\
    ((AnimPlusWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANIMPLUS_WINDOW(w)					     \
    AnimPlusWindow *aw = GET_ANIMPLUS_WINDOW (w,                     \
		     GET_ANIMPLUS_SCREEN (w->screen,             \
		     GET_ANIMPLUS_DISPLAY (w->screen->display)))

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

/* blinds.c */

Bool 
fxBlindsInit( CompWindow *w );

/* helix.c */

Bool 
fxHelixInit( CompWindow *w ); 

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
freePolygonSet (AnimPlusWindow * aw);

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
