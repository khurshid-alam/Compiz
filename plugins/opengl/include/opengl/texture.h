/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 * Copyright 2015 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

#ifndef _GLTEXTURE_H
#define _GLTEXTURE_H

#include "core/region.h"
#include "core/string.h"

#include <X11/Xlib-xcb.h>

#ifdef USE_GLES
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

#include <boost/function.hpp>

#include <vector>

#include <opengl/pixmapsource.h>


#define POWER_OF_TWO(v) ((v & (v - 1)) == 0)

/**
 * Returns a 2D matrix adjusted texture co-ordinate x
 */
#define COMP_TEX_COORD_X(m, vx) ((m).xx * (vx) + (m).x0)
/**
 * Returns a 2D matrix adjusted texture co-ordinate y
 */
#define COMP_TEX_COORD_Y(m, vy) ((m).yy * (vy) + (m).y0)

/**
 * Returns a 2D matrix adjusted texture co-ordinate xy
 */
#define COMP_TEX_COORD_XY(m, vx, vy)		\
    ((m).xx * (vx) + (m).xy * (vy) + (m).x0)
/**
 * Returns a 2D matrix adjusted texture co-ordinate yx
 */
#define COMP_TEX_COORD_YX(m, vx, vy)		\
    ((m).yx * (vx) + (m).yy * (vy) + (m).y0)

class PrivateTexture;

/**
 * An abstract representation of an OpenGL texture.
 *
 * Enabling and disabling the texture, and drawing it with OpenGL
 * --------------------------------------------------------------
 *
 * Because there are different texture binding backends, it is not safe to
 * enable the texture target using OpenGL directly. Instead, you should use the
 * ::enable () and ::disable () member functions. The ::enable () member
 * function also allows you to specifiy a level of texture filtering, either
 * Fast or Good. If you are drawing the texture at a smaller size then the
 * render size, then it should be safe to use the Fast filter.
 *
 * Getting OpenGL information about the texture
 * --------------------------------------------
 *
 * It is possible to get the ::name () and ::target () of the texture to use
 * that data for OpenGL purposes.
 *
 * Mipmapping
 * ----------
 *
 * Compiz also supports a mipmapping filter to make textures appear smoother
 * when viewed at angles. To enable this you can use the ::setMipmap and
 * ::mipmap functions in order to enable this. Mipmapping is not available on
 * certain hardware. In this case, the function will simply fail silently.
 *
 * Overloading the binding of textures
 * -----------------------------------
 *
 * For whatever reason, you may wish to write your own texture binding method
 * which overloads the standard GLX_ext_texture_from_pixmap method. It is
 * possible to do this by overloading the ::enable () and ::disable () methods
 * of the GLTexture through your own class. Then on your plugin's init function
 * you should use GLTexture::BindPixmapHandle hnd = GLScreen::get
 * (screen)->registerBindPixmap (YourPixmap::bindPixmapToTexture) and on fini
 * you must also call GLScreen::get (screen)->unregistrerBindPixmap (hnd).
 *
 * A sample implementation can be found in the Copy To Texture plugin.
 */
class GLTexture : public CompRect {
    public:

	typedef enum {
	    Fast,
	    Good
	} Filter;

	/**
	 * a texture matrix
	 *
	 * Texture Matrix manipulation
	 *
	 * Since all textures are 2D, it is possible to perform basic 2D
	 * Manipulation operations on the texture.
	 *
	 * Member variable | Function
	 * ----------------|----------
	 *          .x0    | X Position on the screen
	 *          .y0    | Y Position on the screen
	 *          .xx    | X Scale Factor
	 *          .yy    | Y Scale Factor
	 *          .xy    | X Shear Factor
	 *          .yx    | Y Shear Factor
	 *
	 * The COMP_TEX_COORD functions return matrix-adjusted values for
	 * texture positions. The "real" value of your co-ordinate on the
	 * texture itself should be inputted into these functions, and these
	 * functions will return the appropriate number to feed to functions
	 * such as glTexCoord2f.
	 */
	typedef struct {
	    float xx; float yx;
	    float xy; float yy;
	    float x0; float y0;
	} Matrix;

	typedef std::vector<Matrix> MatrixList;

	/**
	 * a list of OpenGL textures, usually used for texture tiling
	 *
	 * In order to overcome hardware maximum texture size limitations,
	 * Compiz supports tiled texturing. As such, all texture generation
	 * functions (such as bindPixmapTotexture, imageDataToTexture,
	 * readImageToTexture, etc) will all return a GLTexture:List. When
	 * rendering these textures you must loop through each texture in the
	 * list in order that you render each part of the texture that the user
	 * sees.
	 *
	 * On especially small textures, it is ok to directly access the first
	 * item in the list for the sake of optimization, however whenever it is
	 * not clear what the size of the texture is, you should always loop the
	 * list.
	 */
	class List : public std::vector <GLTexture *> {

	    public:
		List ();
		List (unsigned int);
		List (const List &);
		~List ();

		List & operator= (const List &);

		void clear ();
	};

	typedef boost::function<List (Pixmap, int, int, int, compiz::opengl::PixmapSource)> BindPixmapProc;
	typedef unsigned int BindPixmapHandle;

    public:

	/**
	 * Returns the OpenGL texture name.
	 */
	GLuint name () const;

	/**
	 * Returns the OpenGL texture target.
	 */
	GLenum target () const;

	/**
	 * Returns the openGL texture filter.
	 */
	GLenum filter () const;

	/**
	 * Returns a 2D 2x3 matrix describing the transformation of
	 * this texture.
	 */
	const Matrix & matrix () const;

	/**
	 * Establishes the texture as the current drawing texture
	 * in the openGL context
	 *
	 * @param filter Defines what kind of filtering level this
	 * texture should be drawn with
	 */
	virtual void enable (Filter filter);

	/**
	 * Stops the textures from being the current drawing texture
	 * in the openGL context
	 */
	virtual void disable ();

	/**
	 * Returns true if this texture is MipMapped.
	 */
	bool mipmap () const;

	/**
	 * Sets if this texture should be MipMapped.
	 */
	void setMipmap (bool);

	/**
	 * Sets the openGL filter which should be used on this texture.
	 */
	void setFilter (GLenum);
	void setWrap (GLenum);

	/**
	 * Increases the reference count of a texture.
	 */
	static void incRef (GLTexture *);

	/**
	 * Decreases the reference count of a texture.
	 */
	static void decRef (GLTexture *);

	/**
	 * Creates a collection of GLTextures from an X11 pixmap.
	 * @param pixmap  pixmap data which should be converted
	 *                into texture data
	 * @param width   width of the texture
	 * @param height  height of the texture
	 * @param depth   color depth of the texture
	 * @param source  whether the pixmap lifecycle is managed externally
	 *
	 * @returns a GLTexture::List with the contents of the pixmap.
	 */
	static List bindPixmapToTexture (Pixmap                       pixmap,
					 int                          width,
					 int                          height,
					 int                          depth,
					 compiz::opengl::PixmapSource source
					     = compiz::opengl::InternallyManaged);

	/**
	 * Creates a collection of GLTextures from a raw image buffer.
	 * @param image  a raw image buffer which should be converted
	 *               into texture data
	 * @param size   size of this new texture
	 *
	 * @returns a GLTexture::List with the contents of of
	 * a raw image buffer.
	 */
	static List imageBufferToTexture (const char *image, CompSize size);

	/**
	 * Creates a collection of GLTextures from raw pixel data.
	 * @param image   raw pixel data
	 * @param size    size of the raw pixel data
	 * @param format  GL format of the raw pixel data (eg. GL_RGBA)
	 * @param type    GL data type of the raw pixel data (eg. GL_FLOAT)
	 */
	static List imageDataToTexture (const char *image,
					CompSize   size,
					GLenum     format,
					GLenum     type);

	/**
	 * Uses image loading plugins to read an image from the disk and
	 * return a GLTexture::List with its contents
	 *
	 * @param imageFileName  filename of the image
	 * @param pluginName     name of the plugin, used to find the default
	 *                       image path
	 * @param size           size of this new texture
	 */
	static List readImageToTexture (CompString &imageFileName,
					CompString &pluginName,
					CompSize   &size);

	friend class PrivateTexture;

    protected:
	/**
	 * Consrtucts a defauly empty GLTexture.
	 *
	 * @deprecated This function is only for ABI backwards compatibility and
	 * will be removed in the future.
	 */
	GLTexture ();

	/** Constructs an OpenGL texture. */
	GLTexture (int width, int height, GLenum target, Matrix const& m, bool mipmap);
	virtual ~GLTexture ();

	/** 
	 * Initializer.
	 *
	 * @deprecated This function is only for ABI backwards compatibility and
	 * will be removed in the future.
	 */
	void setData (GLenum target, Matrix const& m, bool mipmap);

    private:
	PrivateTexture *priv;
};

#endif
