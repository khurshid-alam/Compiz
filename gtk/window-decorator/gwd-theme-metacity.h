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
 
#ifndef GWD_THEME_METACITY_H
#define GWD_THEME_METACITY_H

#include <metacity-private/theme.h>

#include "gwd-theme.h"

G_BEGIN_DECLS

#define GWD_TYPE_THEME_METACITY gwd_theme_metacity_get_type ()
G_DECLARE_FINAL_TYPE (GWDThemeMetacity, gwd_theme_metacity,
                      GWD, THEME_METACITY, GWDTheme)

G_END_DECLS

#endif
