/*
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <iosfwd>
#include <boost/shared_ptr.hpp>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xatom.h>

#include "blur_options.h"

#include <composite/composite.h>
#include <opengl/opengl.h>
#include <opengl/program.h>
#include <decoration.h>


const unsigned short BLUR_GAUSSIAN_RADIUS_MAX = 15;

const unsigned short BLUR_STATE_CLIENT = 0;
const unsigned short BLUR_STATE_DECOR  = 1;
const unsigned short BLUR_STATE_NUM    = 2;

struct BlurFunction
{
    CompString shader;
    int target;
    int startTC;
    int numITC;
    int saturation;
};

struct BlurBox {
    decor_point_t p1;
    decor_point_t p2;
};

struct BlurState {
    int                  threshold;
    std::vector<BlurBox> box;
    bool                 active;
    bool                 clipped;
};

class BlurScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public PluginClassHandler<BlurScreen,CompScreen>,
    public BlurOptions
{
    public:
	BlurScreen (CompScreen *screen);
	~BlurScreen ();

	bool setOption (const CompString &name, CompOption::Value &value);

	void handleEvent (XEvent *);

	virtual void matchExpHandlerChanged ();
	virtual void matchPropertyChanged (CompWindow *window);

	void preparePaint (int);
	void donePaint ();

	void damageCutoff ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, const CompRegion &,
			    CompOutput *, unsigned int);
	void glPaintTransformedOutput (const GLScreenPaintAttrib &,
				       const GLMatrix &,
				       const CompRegion &,
				       CompOutput *, unsigned int);

	void updateFilterRadius ();
        void blurReset ();

	const CompString & getSrcBlurFragmentFunction (GLTexture *);
	const CompString & getDstBlurFragmentFunction (GLTexture *texture,
						       int       unit,
						       int       numITC,
						       int       startTC);

	bool projectVertices (CompOutput     *output,
			      const GLMatrix &transform,
			      const float    *object,
			      float          *scr,
			      int            n);

	bool loadFragmentProgram (boost::shared_ptr <GLProgram> &program,
				  const char                    *vertex,
				  const char                    *fragment);

	bool loadFilterProgram (int numITC);

	bool fboPrologue ();
	void fboEpilogue ();
	bool fboUpdate (BoxPtr pBox, int nBox);


    public:
	GLScreen        *gScreen;
	CompositeScreen *cScreen;

	Atom blurAtom[BLUR_STATE_NUM];

	bool alphaBlur;

	int  blurTime;
	bool moreBlur;

	bool blurOcclusion;

	int filterRadius;

	std::vector<BlurFunction> srcBlurFunctions;
	std::vector<BlurFunction> dstBlurFunctions;

	CompRegion region;
	CompRegion tmpRegion;
	CompRegion tmpRegion2;
	CompRegion tmpRegion3;
	CompRegion occlusion;

	CompRect stencilBox;
	GLint    stencilBits;

	CompOutput *output;
	int count;

	GLTexture::List texture;

	float  tx;
	float  ty;
	int    width;
	int    height;

	boost::shared_ptr <GLProgram> program;
	int    maxTemp;
	boost::shared_ptr <GLFramebufferObject> fbo;

	GLFramebufferObject *oldDrawFramebuffer;

	float amp[BLUR_GAUSSIAN_RADIUS_MAX];
	float pos[BLUR_GAUSSIAN_RADIUS_MAX];
	int   numTexop;

	GLMatrix mvp;

	bool     determineProjectedBlurRegionsPass;
	bool     allowAreaDirtyOnOwnDamageBuffer;
	CompRegion backbufferUpdateRegionThisFrame;

	compiz::composite::buffertracking::AgeDamageQuery::Ptr damageQuery;

    private:

	bool markAreaDirty (const CompRegion &r);
};

class BlurWindow :
    public WindowInterface,
    public GLWindowInterface,
    public PluginClassHandler<BlurWindow,CompWindow>
{

    public:

	BlurWindow (CompWindow *window);
	~BlurWindow ();

	void resizeNotify (int dx, int dy, int dwidth, int dheight);
	void moveNotify (int dx, int dy, bool immediate);

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);
	void glTransformationComplete (const GLMatrix   &matrix,
				       const CompRegion &region,
				       unsigned int     mask);

	bool glDraw (const GLMatrix &, const GLWindowPaintAttrib &,
		     const CompRegion &, unsigned int);
	void glDrawTexture (GLTexture *texture,
			    const GLMatrix &,
			    const GLWindowPaintAttrib &,
			    unsigned int);
	void updateRegion ();

	void setBlur (int                  state,
		      int                  threshold,
		      std::vector<BlurBox> box);

	void updateAlphaMatch ();
	void updateMatch ();
	void update (int state);

	void projectRegion (CompOutput     *output,
			    const GLMatrix &transform);

	void determineBlurRegion (int            filter,
				  const GLMatrix &transform,
				  int            clientThreshold);

	bool updateDstTexture (const GLMatrix &transform,
			       CompRect       *pExtents,
			       unsigned int mask);

    public:
	CompWindow      *window;
	CompositeWindow *cWindow;
	GLWindow        *gWindow;
	BlurScreen      *bScreen;

	int  blur;
	bool pulse;
	bool focusBlur;

	BlurState state[BLUR_STATE_NUM];
	bool      propSet[BLUR_STATE_NUM];

	CompRegion region;
	CompRegion clip;
	CompRegion projectedBlurRegion;
};

#define BLUR_SCREEN(s) \
    BlurScreen *bs = BlurScreen::get (s)

#define BLUR_WINDOW(w) \
    BlurWindow *bw = BlurWindow::get (w)

class BlurPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<BlurScreen,BlurWindow>
{
    public:

	bool init ();
};
