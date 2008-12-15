#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-animation.h>
#include <compiz-animationaddon.h>

extern int animDisplayPrivateIndex;
extern CompMetadata animMetadata;

extern AnimEffect AnimEffectFlyIn;
extern AnimEffect AnimEffectBounce;
extern AnimEffect AnimEffectRotateIn;
extern AnimEffect AnimEffectSheet;

#define NUM_EFFECTS 2

typedef enum
{
    // Effect settings
    ANIMSIM_SCREEN_OPTION_EXPLODE_THICKNESS,
    ANIMSIM_SCREEN_OPTION_EXPLODE_GRIDSIZE_X,
    ANIMSIM_SCREEN_OPTION_EXPLODE_GRIDSIZE_Y,
    ANIMSIM_SCREEN_OPTION_EXPLODE_TESS,

    ANIMSIM_SCREEN_OPTION_NUM
} AnimSimScreenOptions;

// This must have the value of the first "effect setting" above
// in AnimSimScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

typedef enum _AnimSimDisplayOptions
{
    ANIMSIM_DISPLAY_OPTION_ABI = 0,
    ANIMSIM_DISPLAY_OPTION_INDEX,
    ANIMSIM_DISPLAY_OPTION_NUM
} AnimSimDisplayOptions;

typedef struct _AnimSimDisplay
{
    int screenPrivateIndex;
    AnimBaseFunctions *animBaseFunc;
    AnimAddonFunctions *animAddonFunc;

    CompOption opt[ANIMSIM_DISPLAY_OPTION_NUM];
} AnimSimDisplay;

typedef struct _AnimSimScreen
{
    int windowPrivateIndex;

    CompOutput *output;

    CompOption opt[ANIMSIM_SCREEN_OPTION_NUM];
} AnimSimScreen;

typedef struct _AnimSimWindow
{
    AnimWindowCommon *com;
    AnimWindowEngineData *eng;

} AnimSimWindow;

#define GET_ANIMSIM_DISPLAY(d)						\
    ((AnimSimDisplay *) (d)->base.privates[animDisplayPrivateIndex].ptr)

#define ANIMSIM_DISPLAY(d)				\
    AnimSimDisplay *ad = GET_ANIMSIM_DISPLAY (d)

#define GET_ANIMSIM_SCREEN(s, ad)						\
    ((AnimSimScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)

#define ANIMSIM_SCREEN(s)							\
    AnimSimScreen *as = GET_ANIMSIM_SCREEN (s, GET_ANIMSIM_DISPLAY (s->display))

#define GET_ANIMSIM_WINDOW(w, as)						\
    ((AnimSimWindow *) (w)->base.privates[(as)->windowPrivateIndex].ptr)

#define ANIMSIM_WINDOW(w)					     \
    AnimSimWindow *aw = GET_ANIMSIM_WINDOW (w,                     \
		     GET_ANIMSIM_SCREEN (w->screen,             \
		     GET_ANIMSIM_DISPLAY (w->screen->display)))

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

