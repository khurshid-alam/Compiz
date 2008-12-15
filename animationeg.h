#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-animation.h>
#include <compiz-animationaddon.h>

extern int animDisplayPrivateIndex;
extern CompMetadata animMetadata;

extern AnimEffect AnimEffectExample1;
extern AnimEffect AnimEffectExample2;

#define NUM_EFFECTS 2

typedef enum
{
    // Effect settings
    ANIMEG_SCREEN_OPTION_EXPLODE_THICKNESS,
    ANIMEG_SCREEN_OPTION_EXPLODE_GRIDSIZE_X,
    ANIMEG_SCREEN_OPTION_EXPLODE_GRIDSIZE_Y,
    ANIMEG_SCREEN_OPTION_EXPLODE_TESS,

    ANIMEG_SCREEN_OPTION_NUM
} AnimEgScreenOptions;

// This must have the value of the first "effect setting" above
// in AnimEgScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

typedef enum _AnimEgDisplayOptions
{
    ANIMEG_DISPLAY_OPTION_ABI = 0,
    ANIMEG_DISPLAY_OPTION_INDEX,
    ANIMEG_DISPLAY_OPTION_NUM
} AnimEgDisplayOptions;

typedef struct _AnimEgDisplay
{
    int screenPrivateIndex;
    AnimBaseFunctions *animBaseFunc;
    AnimAddonFunctions *animAddonFunc;

    CompOption opt[ANIMEG_DISPLAY_OPTION_NUM];
} AnimEgDisplay;

typedef struct _AnimEgScreen
{
    int windowPrivateIndex;

    CompOutput *output;

    CompOption opt[ANIMEG_SCREEN_OPTION_NUM];
} AnimEgScreen;

typedef struct _AnimEgWindow
{
    AnimWindowCommon *com;
    AnimWindowEngineData *eng;

} AnimEgWindow;

#define GET_ANIMEG_DISPLAY(d)						\
    ((AnimEgDisplay *) (d)->base.privates[animDisplayPrivateIndex].ptr)

#define ANIMEG_DISPLAY(d)				\
    AnimEgDisplay *ad = GET_ANIMEG_DISPLAY (d)

#define GET_ANIMEG_SCREEN(s, ad)						\
    ((AnimEgScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANIMEG_SCREEN(s)							\
    AnimEgScreen *as = GET_ANIMEG_SCREEN (s, GET_ANIMEG_DISPLAY (s->display))

#define GET_ANIMEG_WINDOW(w, as)						\
    ((AnimEgWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANIMEG_WINDOW(w)					     \
    AnimEgWindow *aw = GET_ANIMEG_WINDOW (w,                     \
		     GET_ANIMEG_SCREEN (w->screen,             \
		     GET_ANIMEG_DISPLAY (w->screen->display)))

// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#define EXPLODE_PERCEIVED_T 0.7f

/*
 * Function prototypes
 *
 */

OPTION_GETTERS_HDR

/* explode3d.c */

Bool
fxExplodeInit (CompWindow *w);

