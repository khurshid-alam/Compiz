/*
 * Copyright (C) 2016 Alberts Muktupāvels
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GWD_THEME_CAIRO_H
#define GWD_THEME_CAIRO_H

#include "gwd-theme.h"

G_BEGIN_DECLS

#define GWD_TYPE_THEME_CAIRO gwd_theme_cairo_get_type ()
G_DECLARE_FINAL_TYPE (GWDThemeCairo, gwd_theme_cairo,
                      GWD, THEME_CAIRO, GWDTheme)

G_END_DECLS

#endif
