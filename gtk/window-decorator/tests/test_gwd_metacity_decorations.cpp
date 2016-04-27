/*
 * Copyright © 2010 Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gtest_unspecified_bool_type_matcher.h"
#include "gwd-metacity-window-decoration-util.h"

using ::testing::IsNull;
using ::testing::NotNull;

class GWDMetacityDecorationUtilTest :
    public ::testing::Test
{
};

namespace
{
    const MetaThemeType themeType = META_THEME_TYPE_METACITY;

    const std::string emptyTheme ("");
    const std::string realTheme ("Adwaita");
    const std::string badTheme ("Clearlooks");
}

TEST (GWDMetacityDecorationUtilTest, TestNULLDecorationRevertsToCairo)
{
    EXPECT_THAT (gwd_metacity_window_decoration_update_meta_theme (themeType, NULL), IsNull ());
}

TEST (GWDMetacityDecorationUtilTest, TestEmptyStringDecorationRevertsToCairo)
{
    EXPECT_THAT (gwd_metacity_window_decoration_update_meta_theme (themeType, emptyTheme.c_str ()), IsNull ());
}

TEST (GWDMetacityDecorationUtilTest, TestBadThemeStringDecorationRevertsToCairo)
{
    EXPECT_THAT (gwd_metacity_window_decoration_update_meta_theme (themeType, badTheme.c_str ()), IsNull ());
}

TEST (GWDMetacityDecorationUtilTest, TestGoodThemeStringDecorationUsesMetacity)
{
    EXPECT_THAT (gwd_metacity_window_decoration_update_meta_theme (themeType, realTheme.c_str ()), NotNull ());
}
