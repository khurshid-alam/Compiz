/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

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

#include "config.h"
#include "gwd-theme-cairo.h"

struct _GWDThemeCairo
{
    GObject parent;
};

G_DEFINE_TYPE (GWDThemeCairo, gwd_theme_cairo, GWD_TYPE_THEME)

static void
gwd_theme_cairo_class_init (GWDThemeCairoClass *cairo_class)
{
}

static void
gwd_theme_cairo_init (GWDThemeCairo *cairo)
{
}
