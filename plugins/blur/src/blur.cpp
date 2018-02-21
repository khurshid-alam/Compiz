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
#include <blur.h>
#include <sstream>
#include <opengl/framebufferobject.h>

/* Supporting independent texture fetch operations will require
 * GLVertexBuffer to support per-plugin attributes as texture co-ordinates
 * are two-component and not four-component
 */
#define INDEPENDENT_TEX_SUPPORTED 0

COMPIZ_PLUGIN_20090315 (blur, BlurPluginVTable)

/* pascal triangle based kernel generator */
namespace
{
int
blurCreateGaussianLinearKernel (int   radius,
				float strength,
				float *amp,
				float *pos,
				int   *optSize)
{
    float factor = 0.5f + (strength / 2.0f);
    float buffer1[BLUR_GAUSSIAN_RADIUS_MAX * 3];
    float buffer2[BLUR_GAUSSIAN_RADIUS_MAX * 3];
    float *ar1 = buffer1;
    float *ar2 = buffer2;
    float *tmp;
    float sum = 0;
    int   size = (radius * 2) + 1;
    int   mySize = ceil (radius / 2.0f);
    int   i, j;

    ar1[0] = 1.0;
    ar1[1] = 1.0;

    for (i = 3; i <= size; i++)
    {
	ar2[0] = 1;

	for (j = 1; j < i - 1; j++)
	    ar2[j] = (ar1[j - 1] + ar1[j]) * factor;

	ar2[i - 1] = 1;

	tmp = ar1;
	ar1 = ar2;
	ar2 = tmp;
    }

    /* normalize */
    for (i = 0; i < size; i++)
	sum += ar1[i];

    if (sum != 0.0f)
	sum = 1.0f / sum;

    for (i = 0; i < size; i++)
	ar1[i] *= sum;

    i = 0;
    j = 0;

    if (radius & 1)
    {
	pos[i] = radius;
	amp[i] = ar1[i];
	i = 1;
	j = 1;
    }

    for (; i < mySize; i++)
    {
	pos[i]  = radius - j;
	pos[i] -= ar1[j + 1] / (ar1[j] + ar1[j + 1]);
	amp[i]  = ar1[j] + ar1[j + 1];

	j += 2;
    }

    pos[mySize] = 0.0;
    amp[mySize] = ar1[radius];

    *optSize = mySize;

    return radius;
}
}

void
BlurScreen::updateFilterRadius ()
{
    switch (optionGetFilter ()) {
	case BlurOptions::Filter4xbilinear:
	    filterRadius = 2;
	    break;
	case BlurOptions::FilterGaussian: {
	    int   radius   = optionGetGaussianRadius ();
	    float strength = optionGetGaussianStrength ();

	    blurCreateGaussianLinearKernel (radius, strength, amp, pos,
					    &numTexop);

	    filterRadius = radius;
	} break;
	case BlurOptions::FilterMipmap: {
	    float lod = optionGetMipmapLod ();

	    filterRadius = powf (2.0f, ceilf (lod));
	} break;
    }
}


void
BlurScreen::blurReset ()
{
    updateFilterRadius ();

    srcBlurFunctions.clear ();
    dstBlurFunctions.clear ();

    program.reset ();
    texture.clear ();
}

static CompRegion
regionFromBoxes (std::vector<BlurBox> boxes,
		 int	 width,
		 int	 height)
{
    CompRegion region;
    int        x1, x2, y1, y2;

    foreach (BlurBox &box, boxes)
    {
	decor_apply_gravity (box.p1.gravity, box.p1.x, box.p1.y,
			     width, height,
			     &x1, &y1);


	decor_apply_gravity (box.p2.gravity, box.p2.x, box.p2.y,
			     width, height,
			     &x2, &y2);

	if (x2 > x1 && y2 > y1)
	    region += CompRect (x1, y1, x2 - x1, y2 - y1);
    }

    return region;
}

void
BlurWindow::updateRegion ()
{
    CompRegion region;

    if (state[BLUR_STATE_DECOR].threshold)
    {
	region += CompRect (-window->output ().left,
			    -window->output ().top,
			    window->width () + window->output ().right,
			    window->height () + window->output ().bottom);

	region -= CompRect (0, 0, window->width (), window->height ());

	state[BLUR_STATE_DECOR].clipped = false;

	if (!state[BLUR_STATE_DECOR].box.empty ())
	{
	    CompRegion q = regionFromBoxes (state[BLUR_STATE_DECOR].box,
					    window->width (),
					    window->height ());
	    if (!q.isEmpty ())
	    {
		q &= region;
		if (q != region)
		{
		    region = q;
		    state[BLUR_STATE_DECOR].clipped = true;
		}
	    }
	}
    }

    if (state[BLUR_STATE_CLIENT].threshold)
    {
	CompRegion r (0, 0, window->width (), window->height ());

	state[BLUR_STATE_CLIENT].clipped = false;

	if (!state[BLUR_STATE_CLIENT].box.empty ())
	{
	    CompRegion q = regionFromBoxes (state[BLUR_STATE_CLIENT].box,
					    window->width (),
					    window->height ());
	    if (!q.isEmpty ())
	    {
		q &= r;

		if (q != r)
		    state[BLUR_STATE_CLIENT].clipped = true;

		region += q;
	    }
	}
	else
	{
	    region += r;
	}
    }

    this->region = region;
    if (!region.isEmpty ())
	this->region.translate (window->x (), window->y ());
}

void
BlurWindow::setBlur (int                  state,
		     int                  threshold,
		     std::vector<BlurBox> box)
{
    this->state[state].threshold = threshold;
    this->state[state].box       = box;

    updateRegion ();

    cWindow->addDamage ();
}

void
BlurWindow::updateAlphaMatch ()
{
    if (!propSet[BLUR_STATE_CLIENT])
    {
	CompMatch *match;

	match = &bScreen->optionGetAlphaBlurMatch ();
	if (match->evaluate (window))
	{
	    if (!state[BLUR_STATE_CLIENT].threshold)
		setBlur (BLUR_STATE_CLIENT, 4, std::vector<BlurBox> ());
	}
	else
	{
	    if (state[BLUR_STATE_CLIENT].threshold)
		setBlur (BLUR_STATE_CLIENT, 0, std::vector<BlurBox> ());
	}
    }
}

void
BlurWindow::updateMatch ()
{
    CompMatch *match;
    bool      focus;

    updateAlphaMatch ();

    match = &bScreen->optionGetFocusBlurMatch ();

    focus = GL::shaders && match->evaluate (window);
    if (focus != focusBlur)
    {
	focusBlur = focus;
	cWindow->addDamage ();
    }
}



void
BlurWindow::update (int state)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *propData;
    int		  threshold = 0;
    std::vector<BlurBox> boxes;

    result = XGetWindowProperty (screen->dpy (), window->id (),
				 bScreen->blurAtom[state], 0L, 8192L, false,
				 XA_INTEGER, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	propSet[state] = true;

	if (n >= 2)
	{
	    long *data = (long *) propData;
	    BlurBox box;

	    threshold = data[0];

	    if ((n - 2) / 6)
	    {
		unsigned int i;

		data += 2;

		for (i = 0; i < (n - 2) / 6; i++)
		{
		    box.p1.gravity = *data++;
		    box.p1.x       = *data++;
		    box.p1.y       = *data++;
		    box.p2.gravity = *data++;
		    box.p2.x       = *data++;
		    box.p2.y       = *data++;

		    boxes.push_back (box);
		}

	    }
	}

	XFree (propData);
    }
    else
    {
	propSet[state] = false;
    }

    setBlur (state, threshold, boxes);

    updateAlphaMatch ();
}

void
BlurScreen::preparePaint (int msSinceLastPaint)
{
    if (moreBlur)
    {
	int	    steps;
	bool        focus = optionGetFocusBlur ();
	bool        focusBlur;

	steps = (msSinceLastPaint * 0xffff) / blurTime;
	if (steps < 12)
	    steps = 12;

	moreBlur = false;

	foreach (CompWindow *w, screen->windows ())
	{
	    BLUR_WINDOW (w);

	    focusBlur = bw->focusBlur && focus;

	    if (!bw->pulse &&
		(!focusBlur || w->id () == screen->activeWindow ()))
	    {
		if (bw->blur)
		{
		    bw->blur -= steps;
		    if (bw->blur > 0)
			moreBlur = true;
		    else
			bw->blur = 0;
		}
	    }
	    else
	    {
		if (bw->blur < 0xffff)
		{
		    if (bw->pulse)
		    {
			bw->blur += steps * 2;

			if (bw->blur >= 0xffff)
			{
			    bw->blur = 0xffff - 1;
			    bw->pulse = false;
			}

			moreBlur = true;
		    }
		    else
		    {
			bw->blur += steps;
			if (bw->blur < 0xffff)
			    moreBlur = true;
			else
			    bw->blur = 0xffff;
		    }
		}
	    }
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

bool
BlurScreen::markAreaDirty (const CompRegion &r)
{
    return allowAreaDirtyOnOwnDamageBuffer;
}

void
BlurScreen::damageCutoff ()
{
    if (alphaBlur)
    {
	this->output = &screen->fullscreenOutput ();

	/* We need to do a primary pass here with glPaint to check which
	 * regions of the backbuffer need to be updated.
	 *
	 * Because the backbuffer is effectively partially undefined, we are completely
	 * reliant on what core is telling us the repair regions of the
	 * backbuffer need to be - but obviously we can't completely rely
	 * on it because of a feedback effect, as we need to expand
	 * the repair region slightly so that the texture sampler
	 * doesn't go out of range of the damage region. If we were to
	 * simply expand the damage region, then that would feedback to us
	 * on the next frame, until the damage region eventually became infinite.
	 *
	 * As such, we have a tracker in core to track all damage except the effect
	 * of expanding the damage region that came back to us. We then use
	 * this tracker along with the frame age to determine what damage really
	 * needs to be expanded in order to determine the backbuffer update region,
	 * and then use the real damage in order to determine what blur regions
	 * should be updated
	 */
	backbufferUpdateRegionThisFrame &= CompRegion::empty ();
	CompRegion frameAgeDamage = damageQuery->damageForFrameAge (cScreen->getFrameAge ());
	foreach (CompWindow *w, screen->windows ())
	{
	    /* Skip windows that would not have glPaint called on them */
	    if (w->destroyed ())
		continue;

	    if (!w->shaded () && !w->isViewable ())
		continue;

	    BlurWindow *bw = BlurWindow::get (w);

	    if (!bw->cWindow->redirected ())
		continue;

	    if (!bw->projectedBlurRegion.isEmpty ())
		bw->projectedBlurRegion &= CompRegion::empty ();

	    GLMatrix screenSpace;
	    screenSpace.toScreenSpace (this->output, -DEFAULT_Z_CAMERA);

	    bw->gWindow->glPaint (bw->gWindow->paintAttrib (), screenSpace,
				  frameAgeDamage,
				  PAINT_WINDOW_NO_CORE_INSTANCE_MASK);

	    backbufferUpdateRegionThisFrame += bw->projectedBlurRegion;
	}

	allowAreaDirtyOnOwnDamageBuffer = false;
	cScreen->damageRegion (backbufferUpdateRegionThisFrame);
	allowAreaDirtyOnOwnDamageBuffer = true;
    }

    cScreen->damageCutoff ();
}

bool
BlurScreen::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
			   const GLMatrix &transform, const CompRegion &region,
			   CompOutput *output, unsigned int mask)
{
    if (alphaBlur)
    {
	stencilBox = region.boundingRect ();
	this->region = region;
    }

    if (!blurOcclusion)
    {
	occlusion = CompRegion ();

	foreach (CompWindow *w, screen->windows ())
	    BlurWindow::get (w)->clip = CompRegion ();
    }

    this->output = output;

    return gScreen->glPaintOutput (sAttrib, transform, region, output, mask);
}

void
BlurScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &sAttrib,
				      const GLMatrix &transform,
				      const CompRegion &region,
				      CompOutput *output, unsigned int mask)
{
    if (!blurOcclusion)
    {
	occlusion = CompRegion ();

	foreach (CompWindow *w, screen->windows ())
	    BlurWindow::get (w)->clip = CompRegion ();
    }

    gScreen->glPaintTransformedOutput (sAttrib, transform, region, output, mask);
}

void
BlurScreen::donePaint ()
{
    if (moreBlur)
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    BLUR_WINDOW (w);

	    if (bw->blur > 0 && bw->blur < 0xffff)
		bw->cWindow->addDamage ();
	}
    }

    cScreen->donePaint ();
}

void
BlurWindow::glTransformationComplete (const GLMatrix   &matrix,
				      const CompRegion &region,
				      unsigned int     mask)
{
    gWindow->glTransformationComplete (matrix, region, mask);

    int              clientThreshold;
    const CompRegion *reg = NULL;

    /* only care about client window blurring when it's translucent */
    if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
	clientThreshold = state[BLUR_STATE_CLIENT].threshold;
    else
	clientThreshold = 0;

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	reg = &CompRegion::infinite ();
    else
	reg = &region;

    bScreen->tmpRegion = this->region.intersected (*reg);

    if (state[BLUR_STATE_DECOR].threshold || clientThreshold)
    {
	determineBlurRegion (bScreen->optionGetFilter (),
			     matrix,
			     clientThreshold);
    }
}

bool
BlurWindow::glPaint (const GLWindowPaintAttrib &attrib,
		     const GLMatrix &transform,
		     const CompRegion &region, unsigned int mask)
{
    bool status = gWindow->glPaint (attrib, transform, region, mask);

    if (!bScreen->blurOcclusion &&
	(mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK))
    {
	clip = bScreen->occlusion;

	if (!(gWindow->lastMask () & PAINT_WINDOW_NO_CORE_INSTANCE_MASK) &&
	    !(gWindow->lastMask () & PAINT_WINDOW_TRANSFORMED_MASK) &&
	    !this->region.isEmpty ())
	    bScreen->occlusion += this->region;
    }

    return status;
}

namespace
{
struct SamplerInfo
{
    GLint      target;
    CompString func;
};

SamplerInfo
getSamplerInfoForSize (const CompSize &size)
{
    SamplerInfo info;

#ifdef USE_GLES
    info.target = GL_TEXTURE_2D;
    info.func = "texture2D";
#else
    if (GL::textureNonPowerOfTwo ||
	(POWER_OF_TWO (size.width ()) && POWER_OF_TWO (size.height ())))
    {
	info.target = GL_TEXTURE_2D;
	info.func = "texture2D";
    }
    else
    {
	info.target = GL_TEXTURE_RECTANGLE_NV;
	info.func = "textureRECT";
    }
#endif

    return info;
}
}

/* XXX: param is redundant */
const CompString &
BlurScreen::getSrcBlurFragmentFunction (GLTexture *texture)
{
    BlurFunction             function;
    std::stringstream        data (std::stringstream::out);

    SamplerInfo info (getSamplerInfoForSize (CompSize (texture->width (),
						       texture->height ())));

    foreach (const BlurFunction &bf, srcBlurFunctions)
	if (bf.target == info.target)
	    return bf.shader;

    data << "uniform vec4 focusblur_input_offset;\n"\
	    "\n"\
	    "void focusblur_fragment ()\n"\
	    "{\n";

    if (optionGetFilter () == BlurOptions::Filter4xbilinear)
	data << "    float blur_offset0, blur_offset1;\n"\
		"    vec4 blur_sum;\n"\
		"    vec4 offset0 = focusblur_input_offset.xyzw * vec4 (1.0, 1.0, 0.0, 0.0);\n"\
		"    vec4 offset1 = focusblur_input_offset.zwww * vec4 (1.0, 1.0, 0.0, 0.0);\n"
		"    vec4 output = texture2D (texture0, vTexCoord0 + offset0);\n"\
		"    blur_sum = output * 0.25;\n"\
		"    output = " << info.func << " (texture0, vTexCoord0 - offset0);\n"\
		"    blur_sum += output * 0.25;\n"\
		"    output = " << info.func << " (texture0, vTexCoord0 + offset1);\n"\
		"    blur_sum += output * 0.25;\n"\
		"    output = " << info.func << " (texture0, vTexCoord0 - offset1);\n"\
		"    output = output * 0.25 + blur_sum;\n"\
		"    gl_FragColor = output;\n";

    data << "}\n";

    function.shader = data.str ();
    function.target = info.target;

    srcBlurFunctions.push_back (function);

    return srcBlurFunctions.back ().shader;
}

/* This seems incomplete to me */

const CompString &
BlurScreen::getDstBlurFragmentFunction (GLTexture *texture,
					int       unit,
				        int       numITC,
				        int       startTC)
{
    BlurFunction    function;
    std::stringstream data (std::stringstream::out);
    int             saturation = optionGetSaturation ();

    SamplerInfo info (getSamplerInfoForSize (CompSize (texture->width (),
						       texture->height ())));

    foreach (BlurFunction &function, dstBlurFunctions)
	if (function.target  == info.target &&
	    function.numITC  == numITC &&
	    function.startTC == startTC &&
	    function.saturation == saturation)
	    return function.shader;

    data << "uniform vec4 blur_translation;\n"\
	    "uniform vec4 blur_threshold;\n";

    int	          i, j;
    /* These are always initialized if running a gaussian shader */
    int               numIndirect = 0;
    int               numIndirectOp = 0;
    int               base, end, ITCbase;

    /* Set per-shader uniforms */
    switch (optionGetFilter ())
    {
	case BlurOptions::Filter4xbilinear:
	    data << "uniform vec4 blur_dxdy;\n";
	    break;
	default:
	    break;
    }

    data << "\n"\
	    "void blur_fragment ()\n"\
	    "{\n"\
	    "    vec4 blur_sum, blur_dst, blur_output;\n"\
	    "    vec2 blur_fCoord;\n"\
	    "    vec4 blur_mask;\n";

    if (saturation < 100)
	data << "    float blur_sat;\n";

    /* Define per-filter temporaries */
    switch (optionGetFilter ())
    {
	case BlurOptions::Filter4xbilinear:
	{
	    static const char *filterTemp[] = {
		"blur_t0", "blur_t1", "blur_t2", "blur_t3",
	    };

	    static const char *filterSampleTemp[] = {
		"blur_s0", "blur_s1", "blur_s2", "blur_s3"
	    };

	    for (i = 0;
		 (unsigned int) i < sizeof (filterTemp) / sizeof (filterTemp[0]);
		 ++i)
		data << "    vec2 " << filterTemp[i] << ";\n";

	    for (i = 0;
		 (unsigned int) i < sizeof (filterSampleTemp) / sizeof (filterSampleTemp[0]);
		 ++i)
		data << "    vec4 " << filterSampleTemp[i] << ";\n";
	}
	break;
	case BlurOptions::FilterGaussian:
	{
	    /* try to use only half of the available temporaries to keep
	       other plugins working */
	    if ((maxTemp / 2) - 4 >
		 (numTexop + (numTexop - numITC)) * 2)
	    {
		numIndirect   = 1;
		numIndirectOp = numTexop;
	    }
	    else
	    {
		i = MAX (((maxTemp / 2) - 4) / 4, 1);
		numIndirect = ceil ((float)numTexop / (float)i);
		numIndirectOp = ceil ((float)numTexop / (float)numIndirect);
	    }

	    /* we need to define all coordinate temporaries if we have
	       multiple indirection steps */
	    j = (numIndirect > 1) ? 0 : numITC;

	    for (i = 0; i < numIndirectOp * 2; i++)
		data << "    vec4 blur_pix_" << i << ";\n";

	    for (i = j * 2; i < numIndirectOp * 2; i++)
		data << "    vec2 blur_coord_" << i << ";\n";
	}
	break;
	case BlurOptions::FilterMipmap:
	{
	    data << "    float lod_bias;\n";
	}
	default:
	    break;
    }

    /*
     * blur_output: copy the original gl_FragColor determined by sampling
     * the window texture in the core shader
     * blur_fCoord: take the fragment co-ordinate in window co-ordinates
     * and normalize it (eg, blur_translation). Once normalized we can
     * sample texUnitN + 1 (for the copy of the backbuffer we wish to blur)
     * and texUnitN + 2 (scratch fbo for gaussian blurs)
     * blur_mask: take blur_output pixel alpha value, multiply by threshold value
     * and clamp between 0 - 1. We will use this later to mix the blur fragments
     * with the window fragments.
     */
    data << "\n"\
	    "    blur_output = gl_FragColor;\n"\
	    "    blur_fCoord = gl_FragCoord.st * blur_translation.st;\n"\
	    "    blur_mask = clamp (blur_output.a * blur_threshold, vec4 (0.0, 0.0, 0.0, 0.), vec4 (1.0, 1.0, 1.0, 1.0));\n"\
	    "\n";

    /* Define filter program */
    switch (optionGetFilter ())
    {
	case BlurOptions::Filter4xbilinear:
	{
	    data << "    blur_t0 = blur_fCoord + blur_dxdy.st;\n"\
		    "    blur_s0 = " << info.func << " (texture1, blur_t0);\n"\
		    "    blur_t1 = blur_fCoord - blur_dxdy.st;\n"\
		    "    blur_s1 = " << info.func << " (texture1, blur_t1);\n"\
		    "    blur_t2 = blur_fCoord + vec2 (-1.0, 1.0) * blur_dxdy.st;\n"\
		    "    blur_s2 = " << info.func << " (texture1, blur_t2);\n"\
		    "    blur_t3 = blur_fCoord + vec2 (1.0, -1.0) * blur_dxdy.st;\n"\
		    "    blur_s3 = " << info.func << " (texture1, blur_t3);\n"\
		    "    blur_sum = blur_s0 * 0.25;\n"\
		    "    blur_sum += blur_s1 * 0.25;\n"\
		    "    blur_sum += blur_s2 * 0.25;\n"\
		    "    blur_sum += blur_s3 * 0.25;\n";
	} break;
	case BlurOptions::FilterGaussian:
	{
	    /* Invert y */
	    data << "    blur_fCoord.y = 1.0 - blur_fCoord.y;\n"\
		    "    blur_sum = " << info.func << " (texture2, blur_fCoord);\n"\
		    "    blur_sum *= " << amp[numTexop] << ";\n";

	    for (j = 0; j < numIndirect; j++)
	    {
		base = j * numIndirectOp;
		end  = MIN ((j + 1) * numIndirectOp, numTexop) - base;

		ITCbase = MAX (numITC - base, 0);

		for (i = ITCbase; i < end; i++)
		{
		    data << "    blur_coord_" << i * 2 << " = blur_fCoord + vec2 (0.0, " << pos[base + i] * ty << ");\n"\
			    "    blur_coord_" << (i * 2) + 1 << " = blur_fCoord - vec2 (0.0, " << pos[base + i] * ty << ");\n";
		}
#if INDEPENDENT_TEX_SUPPORTED
		for (i = 0; i < ITCbase; i++)
		{
		    data << "    blur_pix_" + i * 2 + " vTexCoord"
		    data.addDataOp (
			"TXP pix_%d, fragment.texcoord[%d], texture[%d], %s;"
			"TXP pix_%d, fragment.texcoord[%d], texture[%d], %s;",
			i * 2, startTC + ((i + base) * 2),
			unit + 1, targetString,
			(i * 2) + 1, startTC + 1 + ((i + base) * 2),
			unit + 1, targetString);
		}
#endif
		for (i = ITCbase; i < end; i++)
		{
		    data << "    blur_pix_" << i * 2 << " = " << info.func << " (texture2, blur_coord_" << i * 2 << ");\n"\
			    "    blur_pix_" << (i * 2) + 1 << " = " << info.func << " (texture2, blur_coord_" << (i * 2) + 1 << ");\n";
		}

		for (i = 0; i < end * 2; i++)
		    data << "    blur_sum += blur_pix_" << i << " * " << amp[base + (i / 2)] << ";\n";
	    }

	} break;
	case BlurOptions::FilterMipmap:
	    data << "    lod_bias = blur_translation.w;\n"\
		    "    blur_sum = " << info.func << " (texture1, blur_fCoord, lod_bias);\n";

	    break;
    }

    if (saturation < 100)
    {
	data << "    blur_sat = blur_sum * vec4 (1.0, 1.0, 1.0, 0.0);\n"\
		"    blur_sat = dot (blur_sat, vec4 (" <<
		RED_SATURATION_WEIGHT << ", " << GREEN_SATURATION_WEIGHT << ", " <<
		BLUE_SATURATION_WEIGHT << ", 0.0f);\n"\
		"    blur_sum.xyz = mix (" << saturation / 100.f << ",  blur_sat);\n";
    }

    data << "    blur_dst = (blur_mask * -blur_output.a) + blur_mask;\n"\
	    "    blur_output.rgb = blur_sum.rgb * blur_dst.a + blur_output.rgb;\n"\
	    "    blur_output.a += blur_dst.a;\n"\
	    "    gl_FragColor = blur_output;\n"\
	    "}";

    function.shader  = data.str ();
    function.target  = texture->target ();
    function.numITC  = numITC;
    function.startTC = startTC;
    function.saturation = saturation;

    dstBlurFunctions.push_back (function);

    return dstBlurFunctions.back ().shader;
}

namespace
{
bool
project (float objx, float objy, float objz,
	 const float modelview[16], const float projection[16],
	 const GLint viewport[4],
	 float *winx, float *winy, float *winz)
{
    unsigned int i;
    float in[4];
    float out[4];

    in[0] = objx;
    in[1] = objy;
    in[2] = objz;
    in[3] = 1.0;

    for (i = 0; i < 4; i++) {
	out[i] =
	    in[0] * modelview[i] +
	    in[1] * modelview[4  + i] +
	    in[2] * modelview[8  + i] +
	    in[3] * modelview[12 + i];
    }

    for (i = 0; i < 4; i++) {
	in[i] =
	    out[0] * projection[i] +
	    out[1] * projection[4  + i] +
	    out[2] * projection[8  + i] +
	    out[3] * projection[12 + i];
    }

    if (in[3] == 0.0)
	return false;

    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];
    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;

    /* Map x,y to viewport */
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];

    *winx = in[0];
    *winy = in[1];
    *winz = in[2];
    return true;
}
}

bool
BlurScreen::projectVertices (CompOutput     *output,
			     const GLMatrix &transform,
			     const float    *object,
			     float          *scr,
			     int            n)
{
    GLfloat dProjection[16];
    GLfloat dModel[16];
    GLint   viewport[4];
    float   x, y, z;
    int	    i;

    viewport[0] = output->x1 ();
    viewport[1] = screen->height () - output->y2 ();
    viewport[2] = output->width ();
    viewport[3] = output->height ();

    for (i = 0; i < 16; i++)
    {
	dModel[i]      = transform.getMatrix ()[i];
	dProjection[i] = gScreen->projectionMatrix ()->getMatrix ()[i];
    }

    while (n--)
    {
	if (!project (object[0], object[1], object[2],
		      dModel, dProjection, viewport,
		      &x, &y, &z))
	    return false;

	scr[0] = x;
	scr[1] = y;

	object += 3;
	scr += 2;
    }

    return true;
}

bool
BlurScreen::loadFragmentProgram (boost::shared_ptr <GLProgram> &program,
				 const char                    *vertex,
				 const char                    *fragment)
{
    if (!program)
	program.reset (new GLProgram (CompString (vertex),
				      CompString (fragment)));

    if (program && program->valid ())
	return true;
    else
    {
	program.reset ();
	compLogMessage ("blur", CompLogLevelError,
			"Failed to load blur program %s", fragment);
	return false;
    }
}

bool
BlurScreen::loadFilterProgram (int numITC)
{
    std::stringstream svtx;

    /* A simple pass-thru vertex shader */
    svtx << "#ifdef GL_ES\n"\
	    "precision mediump float;\n"\
	    "#endif\n"\
	    "uniform mat4 modelview;\n"\
	    "uniform mat4 projection;\n"\
	    "attribute vec4 position;\n"\
	    "attribute vec2 texCoord0;\n"\
	    "varying vec2 vTexCoord0;\n"\
	    "\n"\
	    "void main ()\n"\
	    "{\n"\
	    "    vTexCoord0 = texCoord0;\n"\
	    "    gl_Position = projection * modelview * position;\n"\
	    "}";

    std::stringstream str;
    int   i, j;
    int   numIndirect;
    int   numIndirectOp;
    int   base, end, ITCbase;

    SamplerInfo info (getSamplerInfoForSize (*screen));

    str << "varying vec2 vTexCoord0;\n"\
	   "uniform sampler2D texture0;\n";

    if (maxTemp - 1 > (numTexop + (numTexop - numITC)) * 2)
    {
	numIndirect   = 1;
	numIndirectOp = numTexop;
    }
    else
    {
	i = (maxTemp - 1) / 4;
	numIndirect = ceil ((float)numTexop / (float)i);
	numIndirectOp = ceil ((float)numTexop / (float)numIndirect);
    }

    /* we need to define all coordinate temporaries if we have
       multiple indirection steps */
    j = (numIndirect > 1) ? 0 : numITC;

    str << "\n"\
	   "void main ()\n"\
	   "{\n";

    for (i = 0; i < numIndirectOp; i++)
	str << "    vec4 blur_pix_" << i * 2 << ", blur_pix_" << (i * 2) + 1 << ";\n";

    for (i = j; i < numIndirectOp; i++)
	str << "    vec2 blur_coord_" << i * 2 << ", blur_coord_" << (i * 2) + 1 << ";\n";

    str << "    vec4 blur_sum;\n";
    str << "    blur_sum = " << info.func << " (texture0, vTexCoord0);\n"\
	   "    blur_sum = blur_sum * " << amp[numTexop] << ";\n";

    for (j = 0; j < numIndirect; j++)
    {
	base = j * numIndirectOp;
	end  = MIN ((j + 1) * numIndirectOp, numTexop) - base;

	ITCbase = MAX (numITC - base, 0);

	for (i = ITCbase; i < end; i++)
	    str << "    blur_coord_" << i * 2 << " = vTexCoord0 + vec2 (" << pos[base + i] * tx << ", 0.0);\n"\
		   "    blur_coord_" << (i * 2) + 1 << " = vTexCoord0 - vec2 (" << pos[base + i] * tx << ", 0.0);\n";
#if INDEPENDENT_TEX_SUPPORTED
	for (i = 0; i < ITCbase; i++)
	    str << sprintf (str,
		"TEX pix_%d, fragment.texcoord[%d], texture[0], %s;"
		"TEX pix_%d, fragment.texcoord[%d], texture[0], %s;",
		i * 2, ((i + base) * 2) + 1, targetString,
		(i * 2) + 1, ((i + base) * 2) + 2, targetString);
#endif
	for (i = ITCbase; i < end; i++)
	    str << "    blur_pix_" << (i * 2) << " = " << info.func << " (texture0, blur_coord_" << i * 2 << ");\n"\
		   "    blur_pix_" << (i * 2) + 1 << " = " << info.func << " (texture0, blur_coord_" << (i * 2) + 1 << ");\n";

	for (i = 0; i < end * 2; i++)
	    str << "    blur_sum += blur_pix_" << i << " * " << amp[base + (i / 2)] << ";\n";
    }

    str << "    gl_FragColor = blur_sum;\n"\
	   "}";

    return loadFragmentProgram (program,
				svtx.str ().c_str (),
				str.str ().c_str ());
}

bool
BlurScreen::fboPrologue ()
{
    if (!fbo)
	return false;

    oldDrawFramebuffer = fbo->bind ();

    return true;
}

void
BlurScreen::fboEpilogue ()
{
    oldDrawFramebuffer->bind ();

    fbo->tex ()->enable (GLTexture::Good);
    //GL::generateMipmap (fbo->tex ()->target ());

    fbo->tex ()->disable ();
}

bool
BlurScreen::fboUpdate (BoxPtr pBox,
		       int    nBox)
{
    float  iTC = 0;
    bool wasCulled = glIsEnabled (GL_CULL_FACE);

    if (GL::maxTextureUnits && optionGetIndependentTex () && false)
	iTC = MIN ((GL::maxTextureUnits - 1) / 2, numTexop);

    if (!program)
	if (!loadFilterProgram (iTC))
	    return false;

    if (!fboPrologue ())
	return false;

    glDisable (GL_CULL_FACE);
    GL::activeTexture (GL_TEXTURE0);
    texture[0]->enable (GLTexture::Good);

    GLVertexBuffer *streamingBuffer = GLVertexBuffer::streamingBuffer ();

    while (nBox--)
    {
	float x1 = pBox->x1;
	float x2 = pBox->x2;
	float y1 = screen->height () - pBox->y2;
	float y2 = screen->height () - pBox->y1;

	GLfloat texCoords[] =
	{
	    tx * x1, ty * y1,
	    tx * x1, ty * y2,
	    tx * x2, ty * y1,
	    tx * x2, ty * y2
	};

	GLfloat vertices[] =
	{
	    x1, y1, 0,
	    x1, y2, 0,
	    x2, y1, 0,
	    x2, y2, 0
	};

	GLMatrix mv;
	mv.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	streamingBuffer->begin (GL_TRIANGLE_STRIP);
	streamingBuffer->setProgram (program.get ());
	streamingBuffer->addTexCoords (0, 4, texCoords);
	streamingBuffer->addVertices (4, vertices);

	if (streamingBuffer->end ())
	    streamingBuffer->render (mv);

	/* Unset program */
	streamingBuffer->setProgram (NULL);

	pBox++;
    }

    if (wasCulled)
	glEnable (GL_CULL_FACE);

    fboEpilogue ();

    return true;
}

static const unsigned short MAX_VERTEX_PROJECT_COUNT = 20;

void
BlurWindow::projectRegion (CompOutput     *output,
			   const GLMatrix &transform)
{
    float      scrv[MAX_VERTEX_PROJECT_COUNT * 2];
    float      vertices[MAX_VERTEX_PROJECT_COUNT * 3];
    int	       nVertices, nQuadCombine;
    int        i, j, stride;
    float      *v, *vert;
    float      minX, maxX, minY, maxY, minZ, maxZ;
    float      *scr;

    GLTexture::MatrixList ml;

    gWindow->vertexBuffer ()->begin ();
    gWindow->glAddGeometry (ml, bScreen->tmpRegion2, CompRegion::infinite ());

    if (!gWindow->vertexBuffer ()->end ())
	return;

    GLVertexBuffer        *vb = gWindow->vertexBuffer ();

    nVertices    = vb->countVertices ();
    nQuadCombine = 1;

    stride = vb->getVertexStride ();
    vert = vb->getVertices () + (stride - 3);

    /* construct quads from bounding vertices */
    minX = screen->width ();
    maxX = 0;
    minY = screen->height ();
    maxY = 0;
    minZ = 1000000;
    maxZ = -1000000;

    for (i = 0; i < vb->countVertices (); i++)
    {
	v = vert + (stride * i);

	if (v[0] < minX)
	    minX = v[0];

	if (v[0] > maxX)
	    maxX = v[0];

	if (v[1] < minY)
	    minY = v[1];

	if (v[1] > maxY)
	    maxY = v[1];

	if (v[2] < minZ)
	    minZ = v[2];

	if (v[2] > maxZ)
	    maxZ = v[2];
    }

    vertices[0] = vertices[9]  = minX;
    vertices[1] = vertices[4]  = minY;
    vertices[3] = vertices[6]  = maxX;
    vertices[7] = vertices[10] = maxY;
    vertices[2] = vertices[5]  = maxZ;
    vertices[8] = vertices[11] = maxZ;

    nVertices = 4;

    if (maxZ != minZ)
    {
	vertices[12] = vertices[21] = minX;
	vertices[13] = vertices[16] = minY;
	vertices[15] = vertices[18] = maxX;
	vertices[19] = vertices[22] = maxY;
	vertices[14] = vertices[17] = minZ;
	vertices[20] = vertices[23] = minZ;
	nQuadCombine = 2;
    }

    if (!bScreen->projectVertices (output, transform, vertices, scrv,
				   nVertices * nQuadCombine))
	return;

    for (i = 0; i < nVertices / 4; i++)
    {
	scr = scrv + (i * 4 * 2);

	minX = screen->width ();
	maxX = 0;
	minY = screen->height ();
	maxY = 0;

	for (j = 0; j < 8 * nQuadCombine; j += 2)
	{
	    if (scr[j] < minX)
		minX = scr[j];

	    if (scr[j] > maxX)
		maxX = scr[j];

	    if (scr[j + 1] < minY)
		minY = scr[j + 1];

	    if (scr[j + 1] > maxY)
		maxY = scr[j + 1];
	}

	int x1, y1, x2, y2;

	x1 = minX - bScreen->filterRadius - 0.5;
	y1 = screen->height () - maxY - bScreen->filterRadius - 0.5;
	x2 = maxX + bScreen->filterRadius + 0.5;
	y2 = screen->height () - minY + bScreen->filterRadius + 0.5;


        bScreen->tmpRegion3 += CompRect (x1, y1, x2 - x1, y2 - y1);

    }
}

void
BlurWindow::determineBlurRegion (int            filter,
				 const GLMatrix &transform,
				 int            clientThreshold)
{
    bScreen->tmpRegion3 = CompRegion ();

    if (filter == BlurOptions::FilterGaussian)
    {
	if (state[BLUR_STATE_DECOR].threshold)
	{
	    int  xx, yy, ww, hh;
	    // top
	    xx = window->x () - window->output ().left;
	    yy = window->y () - window->output ().top;
	    ww = window->width () + window->output ().left +
		 window->output ().right;
	    hh = window->output ().top;

	    bScreen->tmpRegion2 = bScreen->tmpRegion.intersected (
		CompRect (xx, yy, ww, hh));

	    if (!bScreen->tmpRegion2.isEmpty ())
		projectRegion (bScreen->output, transform);

	    // bottom
	    xx = window->x () - window->output ().left;
	    yy = window->y () + window->height ();
	    ww = window->width () + window->output ().left +
		 window->output ().right;
	    hh = window->output ().bottom;

	    bScreen->tmpRegion2 = bScreen->tmpRegion.intersected (
		CompRect (xx, yy, ww, hh));

	    if (!bScreen->tmpRegion2.isEmpty ())
		projectRegion (bScreen->output, transform);

	    // left
	    xx = window->x () - window->output ().left;
	    yy = window->y ();
	    ww = window->output ().left;
	    hh = window->height ();

	    bScreen->tmpRegion2 = bScreen->tmpRegion.intersected (
		CompRect (xx, yy, ww, hh));

	    if (!bScreen->tmpRegion2.isEmpty ())
		projectRegion (bScreen->output, transform);

	    // right
	    xx = window->x () + window->width ();
	    yy = window->y ();
	    ww = window->output ().right;
	    hh = window->height ();

	    bScreen->tmpRegion2 = bScreen->tmpRegion.intersected (
		CompRect (xx, yy, ww, hh));

	    if (!bScreen->tmpRegion2.isEmpty ())
		projectRegion (bScreen->output, transform);
	}

	if (clientThreshold)
	{
	    // center
	    bScreen->tmpRegion2 = bScreen->tmpRegion.intersected (
		CompRect (window->x (),
			  window->y (),
			  window->width (),
			  window->height ()));

	    if (!bScreen->tmpRegion2.isEmpty ())
		projectRegion (bScreen->output, transform);
	}
    }
    else
    {
	// center
	bScreen->tmpRegion2 = bScreen->tmpRegion;

	if (!bScreen->tmpRegion2.isEmpty ())
	    projectRegion (bScreen->output, transform);
    }

    projectedBlurRegion = bScreen->tmpRegion3;
}

bool
BlurWindow::updateDstTexture (const GLMatrix &transform,
			      CompRect       *pExtents,
			      unsigned int   mask)
{
    bool       ret = false;
    int        filter = bScreen->optionGetFilter ();

    /* Paint region n projected region */
    bScreen->tmpRegion = bScreen->region.intersected (projectedBlurRegion);

    if (!bScreen->blurOcclusion &&
	!(mask & PAINT_WINDOW_TRANSFORMED_MASK))
	bScreen->tmpRegion -= clip;

    if (bScreen->tmpRegion.isEmpty ())
	return false;

    CompRect br (bScreen->tmpRegion.boundingRect ());

    if (bScreen->texture.empty () ||
	CompSize (bScreen->texture[0]->width (),
		  bScreen->texture[0]->height ()) !=
	    static_cast <const CompSize &> (*screen))
    {
	bScreen->texture = GLTexture::imageDataToTexture (NULL,
							  *screen,
							  GL_RGB,
#if IMAGE_BYTE_ORDER == MSBFirst
							  GL_UNSIGNED_INT_8_8_8_8_REV);
#else
							  GL_UNSIGNED_BYTE);
#endif

	if (bScreen->texture[0]->target () == GL_TEXTURE_2D)
	{
	    bScreen->tx = 1.0f / bScreen->texture[0]->width ();
	    bScreen->ty = 1.0f / bScreen->texture[0]->height ();
	}
	else
	{
	    bScreen->tx = 1;
	    bScreen->ty = 1;
	}

	if (filter == BlurOptions::FilterGaussian)
	{
	    bScreen->fbo->allocate (*screen,
				    NULL,
				    GL_BGRA);

	    /* We have to bind it in order to get a status */
	    GLFramebufferObject *old = bScreen->fbo->bind();
	    bool status = bScreen->fbo->checkStatus ();
	    old->bind();

	    if (!status)
		compLogMessage ("blur", CompLogLevelError,
				"Failed to create framebuffer object");

	    else
	    {
		unsigned int filter = (bScreen->gScreen->driverHasBrokenFBOMipmaps () ?
				       GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
		bScreen->gScreen->setTextureFilter (filter);

		bScreen->fbo->tex ()->enable (GLTexture::Good);

		const CompRect &r (*bScreen->fbo->tex ());

		glCopyTexSubImage2D (bScreen->fbo->tex ()->target (),
				     0, 0, 0, 0, 0,
				     r.width (),
				     r.height ());

		if (!bScreen->gScreen->driverHasBrokenFBOMipmaps ())
		    GL::generateMipmap (bScreen->fbo->tex ()->target ());

		bScreen->fbo->tex ()->disable ();
	    }
	}

	/* Set update region to entire screen */
	br.setGeometry (0, 0,
			screen->width (),
			screen->height ());
    }

    *pExtents = br;

    CompRegion *updateRegion = NULL;
    updateRegion = &bScreen->tmpRegion;

    foreach (GLTexture *tex, bScreen->texture)
    {
	/* We need to set the active texture filter to GL_LINEAR_MIPMAP_LINEAR */
	if (filter == BlurOptions::FilterMipmap &&
	    !bScreen->gScreen->driverHasBrokenFBOMipmaps ())
	    bScreen->gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

        tex->enable (GLTexture::Good);

        CompRect::vector rects (updateRegion->rects ());

	foreach (const CompRect &r, rects)
	{
	    int y = screen->height () - r.y2 ();

	    glCopyTexSubImage2D (bScreen->texture[0]->target (), 0,
				 r.x1 (), y,
				 r.x1 (), y,
				 r.width (),
				 r.height ());
	}

	/* Force mipmap regeneration, because GLTexture assumes static
	 * textures and won't do it for us */
	if (filter == BlurOptions::FilterMipmap &&
	    !bScreen->gScreen->driverHasBrokenFBOMipmaps ())
	    GL::generateMipmap (tex->target ());

	if (filter == BlurOptions::FilterGaussian)
	    ret |=  bScreen->fboUpdate (updateRegion->handle ()->rects,
					updateRegion->numRects ());
	else
	    ret = true;

	tex->disable ();
    }

    return ret;
}

bool
BlurWindow::glDraw (const GLMatrix      &transform,
		    const GLWindowPaintAttrib &attrib,
		    const CompRegion    &region,
		    unsigned int        mask)
{
    bool       status;

    if (bScreen->alphaBlur && !region.isEmpty ())
    {
	int clientThreshold;

	/* only care about client window blurring when it's translucent */
	if (mask & PAINT_WINDOW_TRANSLUCENT_MASK)
	    clientThreshold = state[BLUR_STATE_CLIENT].threshold;
	else
	    clientThreshold = 0;

	if (state[BLUR_STATE_DECOR].threshold || clientThreshold)
	{
	    bool       clipped = false;
	    CompRect   box (0, 0, 0, 0);

	    bScreen->mvp = *(bScreen->gScreen->projectionMatrix ());
	    bScreen->mvp *= transform;

	    if (updateDstTexture (transform, &box, mask))
	    {
		if (clientThreshold)
		{
		    if (state[BLUR_STATE_CLIENT].clipped)
		    {
			if (bScreen->stencilBits)
			{
			    state[BLUR_STATE_CLIENT].active = true;
			    clipped = true;
			}
		    }
		    else
		    {
			state[BLUR_STATE_CLIENT].active = true;
		    }
		}

		if (state[BLUR_STATE_DECOR].threshold)
		{
		    if (state[BLUR_STATE_DECOR].clipped)
		    {
			if (bScreen->stencilBits)
			{
			    state[BLUR_STATE_DECOR].active = true;
			    clipped = true;
			}
		    }
		    else
		    {
			state[BLUR_STATE_DECOR].active = true;
		    }
		}

		if (!bScreen->blurOcclusion && !clip.isEmpty ())
		    clipped = true;
	    }

	    if (!bScreen->blurOcclusion)
		bScreen->tmpRegion = this->region - clip;
	    else
		bScreen->tmpRegion = this->region;

	    if (!clientThreshold)
	    {
		bScreen->tmpRegion -= CompRect (window->x (),
						window->x () + window->width (),
						window->y (),
						window->y () + window->height ());
	    }

	    if (clipped)
	    {
		GLTexture::MatrixList ml;
		const CompRegion      *reg = NULL;

		if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
		    reg = &CompRegion::infinite ();
		else
		    reg = &region;

		gWindow->vertexBuffer ()->begin ();
		gWindow->glAddGeometry (ml, bScreen->tmpRegion, *reg);
		gWindow->vertexBuffer ()->color4f (1.0, 1.0, 1.0, 1.0);
		if (gWindow->vertexBuffer ()->end ())
		{
		    CompRect clearBox = bScreen->stencilBox;

		    bScreen->stencilBox = box;

		    glEnable (GL_STENCIL_TEST);
		    glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		    glStencilMask (1);

		    if (clearBox.x2 () > clearBox.x1 () &&
			clearBox.y2 () > clearBox.y1 ())
		    {
			/* We might have a previous scissor region, so we need
			 * to fetch it, however slow that might be .. */
			GLint scissorBox[4];
			GLboolean scissorEnabled;

			scissorEnabled = glIsEnabled (GL_SCISSOR_TEST);
			glGetIntegerv (GL_SCISSOR_BOX, scissorBox);

			if (!scissorEnabled)
			    glEnable (GL_SCISSOR_TEST);

			glScissor (clearBox.x1 (),
				   screen->height () - clearBox.y2 (),
				   clearBox.width (),
				   clearBox.height ());
			glClearStencil (0);
			glClear (GL_STENCIL_BUFFER_BIT);
			
			if (!scissorEnabled)
			    glDisable (GL_SCISSOR_TEST);

			glScissor (scissorBox[0], scissorBox[1],
				   scissorBox[2], scissorBox[3]);
		    }

		    glStencilFunc (GL_ALWAYS, 1, 1);
		    glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);

		    /* Render a white polygon where the window is */
		    gWindow->vertexBuffer ()->render (transform);

		    glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		    glStencilMask (0);
		    glDisable (GL_STENCIL_TEST);
		}
	    }
	}
    }

    status = gWindow->glDraw (transform, attrib, region, mask);

    state[BLUR_STATE_CLIENT].active = false;
    state[BLUR_STATE_DECOR].active  = false;

    return status;
}

namespace
{
void setupShadersAndUniformsForDstBlur (GLTexture  *texture,
					GLWindow   *gWindow,
					BlurScreen *bScreen,
					int        &unit,
					int        &iTC,
					float      threshold)
{
    GLfloat	       dx, dy;

    switch (bScreen->optionGetFilter ())
    {
	case BlurOptions::Filter4xbilinear:
	{
	    dx = bScreen->tx / 2.1f;
	    dy = bScreen->ty / 2.1f;

	    unit  = 1; // FIXME!!

	    const CompString &function (
		bScreen->getDstBlurFragmentFunction (
		texture, unit, 0, 0));

	    if (!function.empty ())
	    {
		(*GL::activeTexture) (GL_TEXTURE0 + unit);
		bScreen->texture[0]->enable (GLTexture::Good);
		gWindow->vertexBuffer ()->addTexCoords (unit, 0, NULL);
		(*GL::activeTexture) (GL_TEXTURE0);

		gWindow->addShaders ("blur",
				     "",
				     function);

		gWindow->vertexBuffer ()->addUniform4f ("blur_translation",
							bScreen->tx, bScreen->ty,
							0.0f, 0.0f);

		gWindow->vertexBuffer ()->addUniform4f ("blur_threshold",
							threshold,
							threshold,
							threshold,
							threshold);

		gWindow->vertexBuffer ()->addUniform4f ("blur_dxdy",
							dx, dy, 0.0, 0.0);
	    }
	}
	break;
	case BlurOptions::FilterGaussian:
	{
#if INDEPENDENT_TEX_SUPPORTED
	    if (bScreen->optionGetIndependentTex ())
	    {
		/* leave one free texture unit for fragment position */
		iTC = MAX (0, GL::maxTextureUnits -
			   (gWindow->geometry ().texUnits + 1));
		if (iTC)
		    iTC = MIN (iTC / 2, bScreen->numTexop);
	    }
#endif
	    unit  = 1;

	    const CompString &function (
		bScreen->getDstBlurFragmentFunction (
		    texture, unit, iTC,
		    gWindow->vertexBuffer ()->countTextures ()));

	    if (!function.empty ())
	    {
		gWindow->addShaders ("blur",
				     "",
				     function);

		(*GL::activeTexture) (GL_TEXTURE0 + unit);
		bScreen->texture[0]->enable (GLTexture::Good);
		gWindow->vertexBuffer ()->addTexCoords (unit, 0, NULL);
		(*GL::activeTexture) (GL_TEXTURE0 + unit + 1);
		bScreen->fbo->tex ()->enable (GLTexture::Good);
		gWindow->vertexBuffer ()->addTexCoords (unit + 1, 0, NULL);
		(*GL::activeTexture) (GL_TEXTURE0);

		gWindow->vertexBuffer ()->addUniform4f ("blur_translation",
							bScreen->tx, bScreen->ty,
							0.0f, 0.0f);

		gWindow->vertexBuffer ()->addUniform4f ("blur_threshold",
							threshold,
							threshold,
							threshold,
							threshold);
#if INDEPENDENT_TEX_SUPPORTED
		if (iTC)
		{
		    GLMatrix tm, rm;
		    float s_gen[4], t_gen[4], q_gen[4];

		    for (unsigned int i = 0; i < 16; i++)
			tm[i] = 0;
		    tm[0] = (bScreen->output->width () / 2.0) *
			    bScreen->tx;
		    tm[5] = (bScreen->output->height () / 2.0) *
			    bScreen->ty;
		    tm[10] = 1;

		    tm[12] = (bScreen->output->width () / 2.0 +
			     bScreen->output->x1 ()) * bScreen->tx;
		    tm[13] = (bScreen->output->height () / 2.0 +
			     screen->height () -
			     bScreen->output->y2 ()) * bScreen->ty;
		    tm[14] = 1;
		    tm[15] = 1;

		    tm *= bScreen->mvp;

		    for (int i = 0; i < iTC; i++)
		    {
			(*GL::activeTexture) (GL_TEXTURE0 +
			    gWindow->geometry ().texUnits + (i * 2));

			rm.reset ();
			rm[13] = bScreen->ty * bScreen->pos[i];
			rm *= tm;

			s_gen[0] = rm[0];
			s_gen[1] = rm[4];
			s_gen[2] = rm[8];
			s_gen[3] = rm[12];
			t_gen[0] = rm[1];
			t_gen[1] = rm[5];
			t_gen[2] = rm[9];
			t_gen[3] = rm[13];
			q_gen[0] = rm[3];
			q_gen[1] = rm[7];
			q_gen[2] = rm[11];
			q_gen[3] = rm[15];

			glTexGenfv (GL_T, GL_OBJECT_PLANE, t_gen);
			glTexGenfv (GL_S, GL_OBJECT_PLANE, s_gen);
			glTexGenfv (GL_Q, GL_OBJECT_PLANE, q_gen);

			glTexGeni (GL_S, GL_TEXTURE_GEN_MODE,
				   GL_OBJECT_LINEAR);
			glTexGeni (GL_T, GL_TEXTURE_GEN_MODE,
				   GL_OBJECT_LINEAR);
			glTexGeni (GL_Q, GL_TEXTURE_GEN_MODE,
				   GL_OBJECT_LINEAR);

			glEnable (GL_TEXTURE_GEN_S);
			glEnable (GL_TEXTURE_GEN_T);
			glEnable (GL_TEXTURE_GEN_Q);

			(*GL::activeTexture) (GL_TEXTURE0 +
			    gWindow->geometry ().texUnits +
			    1 + (i * 2));

			rm.reset ();

			rm[13] = -bScreen->ty * bScreen->pos[i];
			rm *= tm;

			s_gen[0] = rm[0];
			s_gen[1] = rm[4];
			s_gen[2] = rm[8];
			s_gen[3] = rm[12];
			t_gen[0] = rm[1];
			t_gen[1] = rm[5];
			t_gen[2] = rm[9];
			t_gen[3] = rm[13];
			q_gen[0] = rm[3];
			q_gen[1] = rm[7];
			q_gen[2] = rm[11];
			q_gen[3] = rm[15];

			glTexGenfv (GL_T, GL_OBJECT_PLANE, t_gen);
			glTexGenfv (GL_S, GL_OBJECT_PLANE, s_gen);
			glTexGenfv (GL_Q, GL_OBJECT_PLANE, q_gen);

			glTexGeni (GL_S, GL_TEXTURE_GEN_MODE,
				   GL_OBJECT_LINEAR);
			glTexGeni (GL_T, GL_TEXTURE_GEN_MODE,
				   GL_OBJECT_LINEAR);
			glTexGeni (GL_Q, GL_TEXTURE_GEN_MODE,
				   GL_OBJECT_LINEAR);

			glEnable (GL_TEXTURE_GEN_S);
			glEnable (GL_TEXTURE_GEN_T);
			glEnable (GL_TEXTURE_GEN_Q);
		    }

		    (*GL::activeTexture) (GL_TEXTURE0);
		}
#endif
	    }
	}
	break;
	case BlurOptions::FilterMipmap:
	{
	    unit  = 1; // FIXME!!

	    const CompString &function (
		bScreen->getDstBlurFragmentFunction (texture,
						     unit,
						     0,
						     0));

	    if (!function.empty ())
	    {
		float lod =
		    bScreen->optionGetMipmapLod ();

		gWindow->addShaders ("blur",
				     "",
				     function);

		(*GL::activeTexture) (GL_TEXTURE0 + unit);
		bScreen->texture[0]->enable (GLTexture::Good);
		gWindow->vertexBuffer ()->addTexCoords (unit, 0, NULL);
		(*GL::activeTexture) (GL_TEXTURE0);

		gWindow->vertexBuffer ()->addUniform4f ("blur_translation",
							bScreen->tx, bScreen->ty,
							0.0f, lod);

		gWindow->vertexBuffer ()->addUniform4f ("blur_threshold",
							threshold,
							threshold,
							threshold,
							threshold);
	    }
	}
	break;
    }
}
}

void
BlurWindow::glDrawTexture (GLTexture                 *texture,
			   const GLMatrix            &matrix,
			   const GLWindowPaintAttrib &attrib,
			   unsigned int              mask)
{
    int state = BLUR_STATE_DECOR;

    foreach (GLTexture *tex, gWindow->textures ())
	if (texture == tex)
	    state = BLUR_STATE_CLIENT;

    if (blur || this->state[state].active)
    {
	int	           unit = 0;
	int                iTC = 0;

	if (blur)
	{
	    GLfloat	           dx, dy;

	    const CompString &function (
		bScreen->getSrcBlurFragmentFunction (texture));

	    if (!function.empty ())
	    {
		gWindow->addShaders ("focusblur",
				     "",
				     function);

		dx = ((texture->matrix ().xx / 2.1f) * blur) / 65535.0f;
		dy = ((texture->matrix ().yy / 2.1f) * blur) / 65535.0f;

		gWindow->vertexBuffer ()->addUniform4f ("focusblur_input_offset",
							dx, dy, dx, -dy);

		/* bi-linear filtering is required */
		mask |= PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK;
	    }
	}

	/* We are drawing the scraped texture, blur it */
	if (this->state[state].active)
	{
	    setupShadersAndUniformsForDstBlur (texture,
					       gWindow,
					       bScreen,
					       unit,
					       iTC,
					       this->state[state].threshold);

	    if (this->state[state].clipped ||
		(!bScreen->blurOcclusion && !clip.isEmpty ()))
	    {
		glEnable (GL_STENCIL_TEST);

		/* Don't touch the stencil buffer */
		glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

		/* Draw region with blur only where the stencil test passes */
		glStencilFunc (GL_EQUAL, 1, 1);

		/* draw region with destination blur - we are relying
		 * on a fairly awkward side-effect here which clears our
		 * active shader once this call is complete */
		gWindow->glDrawTexture (texture, matrix, attrib, mask);

		/* Allow drawing in everywhere but where we just drew */
		glStencilFunc (GL_EQUAL, 0, 1);

		/* draw region without destination blur */
		gWindow->glDrawTexture (texture, matrix, attrib, mask);

		glDisable (GL_STENCIL_TEST);
	    }
	    else
	    {
		/* draw with destination blur */
		gWindow->glDrawTexture (texture, matrix, attrib, mask);
	    }
	}
	else
	{
	    gWindow->glDrawTexture (texture, matrix, attrib, mask);
	}

	if (unit)
	{
	    (*GL::activeTexture) (GL_TEXTURE0 + unit);
	    bScreen->texture[0]->disable ();
	    (*GL::activeTexture) (GL_TEXTURE0 + unit + 1);
	    if (bScreen->fbo &&
		bScreen->fbo->tex ())
		bScreen->fbo->tex ()->disable ();
	    (*GL::activeTexture) (GL_TEXTURE0);
	}

#if INDEPENDENT_TEX_SUPPORTED
	if (iTC)
	{
	    int i;
	    for (i = gWindow->geometry ().texUnits;
		 i < gWindow->geometry ().texUnits + (2 * iTC); i++)
	    {
		(*GL::activeTexture) (GL_TEXTURE0 + i);
		glDisable (GL_TEXTURE_GEN_S);
		glDisable (GL_TEXTURE_GEN_T);
		glDisable (GL_TEXTURE_GEN_Q);
	    }
	    (*GL::activeTexture) (GL_TEXTURE0);
	}
#endif
    }
    else
    {
	gWindow->glDrawTexture (texture, matrix, attrib, mask);
    }
}

void
BlurScreen::handleEvent (XEvent *event)
{
    Window activeWindow = screen->activeWindow ();

    screen->handleEvent (event);

    if (screen->activeWindow () != activeWindow)
    {
	CompWindow *w;

	w = screen->findWindow (activeWindow);
	if (w)
	{
	    if (optionGetFocusBlur ())
	    {
		CompositeWindow::get (w)->addDamage ();
		moreBlur = true;
	    }
	}

	w = screen->findWindow (screen->activeWindow ());
	if (w)
	{
	    if (optionGetFocusBlur ())
	    {
		CompositeWindow::get (w)->addDamage ();
		moreBlur = true;
	    }
	}
    }

    if (event->type == PropertyNotify)
    {
	int i;

	for (i = 0; i < BLUR_STATE_NUM; i++)
	{
	    if (event->xproperty.atom == blurAtom[i])
	    {
		CompWindow *w;

		w = screen->findWindow (event->xproperty.window);
		if (w)
		    BlurWindow::get (w)->update (i);
	    }
	}
    }
}

void
BlurWindow::resizeNotify (int dx,
			  int dy,
			  int dwidth,
			  int dheight)
{
    if (bScreen->alphaBlur)
    {
	if (state[BLUR_STATE_CLIENT].threshold ||
	    state[BLUR_STATE_DECOR].threshold)
	    updateRegion ();
    }

    window->resizeNotify (dx, dy, dwidth, dheight);

}

void
BlurWindow::moveNotify (int  dx,
		        int  dy,
		        bool immediate)
{
    if (!region.isEmpty ())
	region.translate (dx, dy);

    window->moveNotify (dx, dy, immediate);
}

static bool
blurPulse (CompAction         *action,
	   CompAction::State  state,
	   CompOption::Vector &options)
{
    CompWindow *w;
    int	       xid;

    xid = CompOption::getIntOptionNamed (options, "window",
					 screen->activeWindow ());

    w = screen->findWindow (xid);
    if (w && GL::shaders)
    {
	BLUR_SCREEN (screen);
	BLUR_WINDOW (w);

	bw->pulse    = true;
	bs->moreBlur = true;

	bw->cWindow->addDamage ();
    }

    return false;
}

void
BlurScreen::matchExpHandlerChanged ()
{
    screen->matchExpHandlerChanged ();

    /* match options are up to date after the call to matchExpHandlerChanged */
    foreach (CompWindow *w, screen->windows ())
	BlurWindow::get (w)->updateMatch ();

}

void
BlurScreen::matchPropertyChanged (CompWindow *w)
{
    BlurWindow::get (w)->updateMatch ();

    screen->matchPropertyChanged (w);
}

bool
BlurScreen::setOption (const CompString &name, CompOption::Value &value)
{
    unsigned int index;

    bool rv = BlurOptions::setOption (name, value);

    if (!rv || !CompOption::findOption (getOptions (), name, &index))
	return false;

    switch (index) {
	case BlurOptions::BlurSpeed:
	    blurTime = 1000.0f / optionGetBlurSpeed ();
	    break;
	case BlurOptions::FocusBlurMatch:
	case BlurOptions::AlphaBlurMatch:
	    foreach (CompWindow *w, screen->windows ())
		BlurWindow::get (w)->updateMatch ();

	    moreBlur = true;
	    cScreen->damageScreen ();
	    break;
	case BlurOptions::FocusBlur:
	    moreBlur = true;
	    cScreen->damageScreen ();
	    break;
	case BlurOptions::AlphaBlur:
	    if (GL::shaders && optionGetAlphaBlur ())
		alphaBlur = true;
	    else
		alphaBlur = false;

	    cScreen->damageScreen ();
	    break;
	case BlurOptions::Filter:
	    blurReset ();
	    cScreen->damageScreen ();
	    break;
	case BlurOptions::GaussianRadius:
	case BlurOptions::GaussianStrength:
	case BlurOptions::IndependentTex:
	    if (optionGetFilter () == BlurOptions::FilterGaussian)
	    {
		blurReset ();
		cScreen->damageScreen ();
	    }
	    break;
	case BlurOptions::MipmapLod:
	    if (optionGetFilter () == BlurOptions::FilterMipmap)
	    {
		blurReset ();
		cScreen->damageScreen ();
	    }
	    break;
	case BlurOptions::Saturation:
	    blurReset ();
	    cScreen->damageScreen ();
	    break;
	case BlurOptions::Occlusion:
	    blurOcclusion = optionGetOcclusion ();
	    blurReset ();
	    cScreen->damageScreen ();
	    break;
	default:
	    break;
    }

    return rv;
}

BlurScreen::BlurScreen (CompScreen *screen) :
    PluginClassHandler<BlurScreen,CompScreen> (screen),
    gScreen (GLScreen::get (screen)),
    cScreen (CompositeScreen::get (screen)),
    moreBlur (false),
    filterRadius (0),
    srcBlurFunctions (0),
    dstBlurFunctions (0),
    output (NULL),
    count (0),
    maxTemp (32),
    fbo (new GLFramebufferObject ()),
    oldDrawFramebuffer (NULL),
    determineProjectedBlurRegionsPass (false),
    damageQuery (cScreen->getDamageQuery (boost::bind (
					      &BlurScreen::markAreaDirty,
					      this,
					      _1)))
{
    blurAtom[BLUR_STATE_CLIENT] =
	XInternAtom (screen->dpy (), "_COMPIZ_WM_WINDOW_BLUR", 0);
    blurAtom[BLUR_STATE_DECOR] =
	XInternAtom (screen->dpy (), DECOR_BLUR_ATOM_NAME, 0);

    blurTime = 1000.0f / optionGetBlurSpeed ();
    blurOcclusion = optionGetOcclusion ();

    glGetIntegerv (GL_STENCIL_BITS, &stencilBits);
    if (!stencilBits)
	compLogMessage ("blur", CompLogLevelWarn,
			"No stencil buffer. Region based blur disabled");

    /* We need GL_ARB_shading_language_100 for blur */
    if (GL::shaders)
	alphaBlur = optionGetAlphaBlur ();
    else
	alphaBlur = false;

    if (GL::shaders)
    {
	maxTemp = 1024; // FIXME!!!!!!!
    }

    updateFilterRadius ();

    optionSetPulseInitiate (blurPulse);

    ScreenInterface::setHandler (screen, true);
    CompositeScreenInterface::setHandler (cScreen, true);
    GLScreenInterface::setHandler (gScreen, true);
}


BlurScreen::~BlurScreen ()
{
    cScreen->damageScreen ();
}

BlurWindow::BlurWindow (CompWindow *w) :
    PluginClassHandler<BlurWindow,CompWindow> (w),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    bScreen (BlurScreen::get (screen)),
    blur (0),
    pulse (false),
    focusBlur (false)
{
    for (int i = 0; i < BLUR_STATE_NUM; i++)
    {
	state[i].threshold = 0;
	state[i].clipped   = false;
	state[i].active    = false;

	propSet[i] = false;
    }

    update (BLUR_STATE_CLIENT);
    update (BLUR_STATE_DECOR);

    updateMatch ();


    WindowInterface::setHandler (window, true);
    GLWindowInterface::setHandler (gWindow, true);
}

BlurWindow::~BlurWindow ()
{
}

bool
BlurPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) |
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    return true;
}

