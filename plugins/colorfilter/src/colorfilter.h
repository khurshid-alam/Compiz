/*
 * Compiz/Fusion color filtering plugin
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * Ported to Compiz 0.9 by:
 * Copyright (c) 2009 Sam Spilsbury <smspillaz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _COMPIZ_COLORFILTER_H
#define _COMPIZ_COLORFILTER_H

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "colorfilter_options.h"

class ColorfilterFunction
{
    public:

	CompString	shader;
	CompString	name;

	ColorfilterFunction (const CompString &name);

	bool load (CompString fname);
	bool loaded () const { return !shader.empty (); }

    private:

	void programCleanName (CompString &name);
};

class ColorfilterScreen :
    public PluginClassHandler <ColorfilterScreen, CompScreen>,
    public ColorfilterOptions
{
    public:

	ColorfilterScreen (CompScreen *);
	~ColorfilterScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	bool isFiltered;
	int  currentFilter; /* 0 : cumulative mode
			       0 < c <= count : single mode */

	std::vector <std::shared_ptr <ColorfilterFunction> > filtersFunctions;

	void
	toggle ();

	void
	switchFilter ();

	bool
	toggleWindow (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector options);

	bool
	toggleScreen (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector options);

	bool
	filterSwitch (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector options);

	void
	unloadFilters ();

	int
	loadFilters ();

	void
	matchsChanged (CompOption		      *opt,
		       ColorfilterOptions::Options num);

	void
	excludeMatchsChanged (CompOption		      *opt,
			      ColorfilterOptions::Options num);

	void
	filtersChanged (CompOption		       *opt,
			ColorfilterOptions::Options num);

	void
	damageDecorations (CompOption		  *opt,
			   ColorfilterOptions::Options num);
};

#define FILTER_SCREEN(s)						       \
     ColorfilterScreen *cfs = ColorfilterScreen::get (s)

class ColorfilterWindow :
    public PluginClassHandler <ColorfilterWindow, CompWindow>,
    public GLWindowInterface
{
    public:

	ColorfilterWindow (CompWindow *);

	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

	bool		isFiltered;

	void
	glDrawTexture (GLTexture                 *texture,
		       const GLMatrix            &transform,
		       const GLWindowPaintAttrib &attrib,
		       unsigned int              mask);

	void
	toggle ();
};

#define FILTER_WINDOW(w)						       \
     ColorfilterWindow *cfw = ColorfilterWindow::get (w)

class ColorfilterPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <ColorfilterScreen,
						 ColorfilterWindow>
{
    public:

	bool init ();
};

#endif
