/*
 * Copyright © 2012 Canonical Ltd
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
#include <cstring>

#include <tr1/tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

#include <glib-object.h>

#include <gio/gio.h>

#include <glib_gslice_off_env.h>
#include <glib_gsettings_memory_backend_env.h>

#include <gtest_unspecified_bool_type_matcher.h>

#include "gwd-settings.h"

using ::testing::Eq;
using ::testing::Action;
using ::testing::Values;
using ::testing::_;
using ::testing::StrictMock;

MATCHER_P(IsShadowsEqual, element, "")
{
    return decor_shadow_options_cmp (&arg, &element);
}

MATCHER_P(IsStringsEqual, element, "")
{
    return g_strcmp0 (arg, element) == 0;
}

namespace testing_values
{
    const gdouble ACTIVE_SHADOW_OPACITY_VALUE = 1.0;
    const gdouble ACTIVE_SHADOW_RADIUS_VALUE = 2.0;
    const gdouble ACTIVE_SHADOW_OFFSET_X_VALUE = 3.0;
    const gint    ACTIVE_SHADOW_OFFSET_X_INT_VALUE = ACTIVE_SHADOW_OFFSET_X_VALUE;
    const gdouble ACTIVE_SHADOW_OFFSET_Y_VALUE = 4.0;
    const gint    ACTIVE_SHADOW_OFFSET_Y_INT_VALUE = ACTIVE_SHADOW_OFFSET_Y_VALUE;
    const std::string ACTIVE_SHADOW_COLOR_STR_VALUE ("#ffffffff");
    const gushort ACTIVE_SHADOW_COLOR_VALUE[] = { 255, 255, 255 };
    const gdouble INACTIVE_SHADOW_OPACITY_VALUE = 5.0;
    const gdouble INACTIVE_SHADOW_RADIUS_VALUE = 6.0;
    const gdouble INACTIVE_SHADOW_OFFSET_X_VALUE = 7.0;
    const gint    INACTIVE_SHADOW_OFFSET_X_INT_VALUE = INACTIVE_SHADOW_OFFSET_X_VALUE;
    const gdouble INACTIVE_SHADOW_OFFSET_Y_VALUE = 8.0;
    const gint    INACTIVE_SHADOW_OFFSET_Y_INT_VALUE = INACTIVE_SHADOW_OFFSET_Y_VALUE;
    const std::string INACTIVE_SHADOW_COLOR_STR_VALUE ("#00000000");
    const gushort INACTIVE_SHADOW_COLOR_VALUE[] = { 0, 0, 0 };
    const gboolean USE_TOOLTIPS_VALUE = !USE_TOOLTIPS_DEFAULT;
    const std::string BLUR_TYPE_TITLEBAR_VALUE ("titlebar");
    const gint BLUR_TYPE_TITLEBAR_INT_VALUE = BLUR_TYPE_TITLEBAR;
    const std::string BLUR_TYPE_ALL_VALUE ("all");
    const gint BLUR_TYPE_ALL_INT_VALUE = BLUR_TYPE_ALL;
    const std::string BLUR_TYPE_NONE_VALUE ("none");
    const gint BLUR_TYPE_NONE_INT_VALUE = BLUR_TYPE_NONE;
    const gboolean USE_METACITY_THEME_VALUE  = TRUE;
    const std::string METACITY_THEME_VALUE ("metacity_theme");
    const gdouble ACTIVE_OPACITY_VALUE = 0.9;
    const gdouble INACTIVE_OPACITY_VALUE = 0.8;
    const gboolean ACTIVE_SHADE_OPACITY_VALUE = !METACITY_ACTIVE_SHADE_OPACITY_DEFAULT;
    const gboolean INACTIVE_SHADE_OPACITY_VALUE = !METACITY_INACTIVE_SHADE_OPACITY_DEFAULT;
    const std::string BUTTON_LAYOUT_VALUE ("button_layout");
    const gboolean USE_SYSTEM_FONT_VALUE = TRUE;
    const gboolean NO_USE_SYSTEM_FONT_VALUE = FALSE;
    const std::string TITLEBAR_FONT_VALUE ("Ubuntu 12");
    const std::string TITLEBAR_ACTION_SHADE ("toggle_shade");
    const std::string TITLEBAR_ACTION_MAX_VERT ("toggle_maximize_vertically");
    const std::string TITLEBAR_ACTION_MAX_HORZ ("toggle_maximize_horizontally");
    const std::string TITLEBAR_ACTION_MAX ("toggle_maximize");
    const std::string TITLEBAR_ACTION_MINIMIZE ("minimize");
    const std::string TITLEBAR_ACTION_MENU ("menu");
    const std::string TITLEBAR_ACTION_LOWER ("lower");
    const std::string TITLEBAR_ACTION_NONE ("none");
    const std::string MOUSE_WHEEL_ACTION_SHADE ("shade");
}

class GWDSettingsTestCommon :
    public ::testing::Test
{
    public:
	virtual void SetUp ()
	{
	    env.SetUpEnv ();
	}
	virtual void TearDown ()
	{
	    env.TearDownEnv ();
	}
    private:

	CompizGLibGSliceOffEnv env;
};

namespace
{
    void gwd_settings_unref (GWDSettings *settings)
    {
	g_object_unref (G_OBJECT (settings));
    }
}

class GWDMockSettingsNotifiedGMock
{
    public:

        GWDMockSettingsNotifiedGMock (boost::shared_ptr <GWDSettings> settings)
        {
            g_signal_connect (settings.get (), "update-decorations",
                              G_CALLBACK (GWDMockSettingsNotifiedGMock::updateDecorationsCb), this);
            g_signal_connect (settings.get (), "update-frames",
                              G_CALLBACK (GWDMockSettingsNotifiedGMock::updateFramesCb), this);
            g_signal_connect (settings.get (), "update-metacity-theme",
                              G_CALLBACK (GWDMockSettingsNotifiedGMock::updateMetacityThemeCb), this);
            g_signal_connect (settings.get (), "update-metacity-button-layout",
                              G_CALLBACK (GWDMockSettingsNotifiedGMock::updateMetacityButtonLayoutCb), this);
        }

        MOCK_METHOD0 (updateDecorations, void ());
        MOCK_METHOD0 (updateFrames, void ());
        MOCK_METHOD0 (updateMetacityTheme, void ());
        MOCK_METHOD0 (updateMetacityButtonLayout, void ());

    private:

        static void updateDecorationsCb (GWDSettings                  *settings,
                                         GWDMockSettingsNotifiedGMock *gmock)
        {
            gmock->updateDecorations ();
        }

        static void updateFramesCb (GWDSettings                  *settings,
                                    GWDMockSettingsNotifiedGMock *gmock)
        {
            gmock->updateFrames ();
        }

        static void updateMetacityThemeCb (GWDSettings                  *settings,
                                           gint                          metacity_theme_type,
                                           const gchar                  *metacity_theme_name,
                                           GWDMockSettingsNotifiedGMock *gmock)
        {
            gmock->updateMetacityTheme ();
        }

        static void updateMetacityButtonLayoutCb (GWDSettings                  *settings,
                                                  const gchar                  *button_layout,
                                                  GWDMockSettingsNotifiedGMock *gmock)
        {
            gmock->updateMetacityButtonLayout ();
        }

};

class GWDSettingsTest :
    public GWDSettingsTestCommon
{
    public:

        virtual void SetUp ()
        {
            GWDSettingsTestCommon::SetUp ();

            mSettings.reset (gwd_settings_new (BLUR_TYPE_UNSET, NULL), boost::bind (gwd_settings_unref, _1));
            mGMockNotified.reset (new StrictMock <GWDMockSettingsNotifiedGMock> (mSettings));

            ExpectAllNotificationsOnce ();
        }

    protected:

        boost::shared_ptr <StrictMock <GWDMockSettingsNotifiedGMock> > mGMockNotified;
        boost::shared_ptr <GWDSettings> mSettings;

    private:

        void ExpectAllNotificationsOnce ()
        {
            EXPECT_CALL (*mGMockNotified, updateMetacityTheme ()).Times (1);
            EXPECT_CALL (*mGMockNotified, updateMetacityButtonLayout ()).Times (1);
            EXPECT_CALL (*mGMockNotified, updateFrames ()).Times (1);
            EXPECT_CALL (*mGMockNotified, updateDecorations ()).Times (1);

            gwd_settings_thaw_updates (mSettings.get ());
        }

};

TEST_F(GWDSettingsTest, TestGWDSettingsInstantiation)
{
}

/* Won't fail if the code in SetUp succeeds */
TEST_F(GWDSettingsTest, TestUpdateAllOnInstantiation)
{
}

/* We're just using use_tooltips here as an example */
TEST_F(GWDSettingsTest, TestFreezeUpdatesNoUpdates)
{
    gwd_settings_freeze_updates (mSettings.get ());
    EXPECT_THAT (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                    testing_values::USE_TOOLTIPS_VALUE), IsTrue ());
}

/* We're just using use_tooltips here as an example */
TEST_F(GWDSettingsTest, TestFreezeUpdatesNoUpdatesThawUpdatesAllUpdates)
{
    gwd_settings_freeze_updates (mSettings.get ());
    EXPECT_THAT (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                    testing_values::USE_TOOLTIPS_VALUE), IsTrue ());

    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    gwd_settings_thaw_updates (mSettings.get ());
}

/* We're just using use_tooltips here as an example */
TEST_F(GWDSettingsTest, TestFreezeUpdatesNoUpdatesThawUpdatesAllUpdatesNoDupes)
{
    gwd_settings_freeze_updates (mSettings.get ());
    EXPECT_THAT (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                    testing_values::USE_TOOLTIPS_VALUE), IsTrue ());
    EXPECT_THAT (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                    !testing_values::USE_TOOLTIPS_VALUE), IsTrue ());
    EXPECT_THAT (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                    testing_values::USE_TOOLTIPS_VALUE), IsTrue ());

    EXPECT_CALL (*mGMockNotified, updateDecorations ()).Times (1);
    gwd_settings_thaw_updates (mSettings.get ());
}

TEST_F(GWDSettingsTest, TestShadowPropertyChanged)
{
    decor_shadow_options_t activeShadow;
    decor_shadow_options_t inactiveShadow;

    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_shadow_property_changed (mSettings.get (),
                                                       testing_values::ACTIVE_SHADOW_OPACITY_VALUE,
                                                       testing_values::ACTIVE_SHADOW_RADIUS_VALUE,
                                                       testing_values::ACTIVE_SHADOW_OFFSET_X_VALUE,
                                                       testing_values::ACTIVE_SHADOW_OFFSET_Y_VALUE,
                                                       testing_values::ACTIVE_SHADOW_COLOR_STR_VALUE.c_str (),
                                                       testing_values::INACTIVE_SHADOW_OPACITY_VALUE,
                                                       testing_values::INACTIVE_SHADOW_RADIUS_VALUE,
                                                       testing_values::INACTIVE_SHADOW_OFFSET_X_VALUE,
                                                       testing_values::INACTIVE_SHADOW_OFFSET_Y_VALUE,
                                                       testing_values::INACTIVE_SHADOW_COLOR_STR_VALUE.c_str ()));

    activeShadow.shadow_opacity = testing_values::ACTIVE_SHADOW_OPACITY_VALUE;
    activeShadow.shadow_radius = testing_values::ACTIVE_SHADOW_RADIUS_VALUE;
    activeShadow.shadow_offset_x = testing_values::ACTIVE_SHADOW_OFFSET_X_INT_VALUE;
    activeShadow.shadow_offset_y = testing_values::ACTIVE_SHADOW_OFFSET_Y_INT_VALUE;
    activeShadow.shadow_color[0] = testing_values::ACTIVE_SHADOW_COLOR_VALUE[0];
    activeShadow.shadow_color[1] = testing_values::ACTIVE_SHADOW_COLOR_VALUE[1];
    activeShadow.shadow_color[2] = testing_values::ACTIVE_SHADOW_COLOR_VALUE[2];

    inactiveShadow.shadow_opacity = testing_values::INACTIVE_SHADOW_OPACITY_VALUE;
    inactiveShadow.shadow_radius = testing_values::INACTIVE_SHADOW_RADIUS_VALUE;
    inactiveShadow.shadow_offset_x = testing_values::INACTIVE_SHADOW_OFFSET_X_INT_VALUE;
    inactiveShadow.shadow_offset_y = testing_values::INACTIVE_SHADOW_OFFSET_Y_INT_VALUE;
    inactiveShadow.shadow_color[0] = testing_values::INACTIVE_SHADOW_COLOR_VALUE[0];
    inactiveShadow.shadow_color[1] = testing_values::INACTIVE_SHADOW_COLOR_VALUE[1];
    inactiveShadow.shadow_color[2] = testing_values::INACTIVE_SHADOW_COLOR_VALUE[2];

    EXPECT_THAT (gwd_settings_get_active_shadow (mSettings.get ()),
                 IsShadowsEqual (activeShadow));

    EXPECT_THAT (gwd_settings_get_inactive_shadow (mSettings.get ()),
                 IsShadowsEqual (inactiveShadow));
}

TEST_F(GWDSettingsTest, TestShadowPropertyChangedIsDefault)
{
    EXPECT_FALSE (gwd_settings_shadow_property_changed (mSettings.get (),
                                                        ACTIVE_SHADOW_RADIUS_DEFAULT,
                                                        ACTIVE_SHADOW_OPACITY_DEFAULT,
                                                        ACTIVE_SHADOW_OFFSET_X_DEFAULT,
                                                        ACTIVE_SHADOW_OFFSET_Y_DEFAULT,
                                                        ACTIVE_SHADOW_COLOR_DEFAULT,
                                                        INACTIVE_SHADOW_RADIUS_DEFAULT,
                                                        INACTIVE_SHADOW_OPACITY_DEFAULT,
                                                        INACTIVE_SHADOW_OFFSET_X_DEFAULT,
                                                        INACTIVE_SHADOW_OFFSET_Y_DEFAULT,
                                                        INACTIVE_SHADOW_COLOR_DEFAULT));
}

TEST_F(GWDSettingsTest, TestUseTooltipsChanged)
{
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                    testing_values::USE_TOOLTIPS_VALUE));

    EXPECT_THAT (gwd_settings_get_use_tooltips (mSettings.get ()),
                 Eq (testing_values::USE_TOOLTIPS_VALUE));
}

TEST_F(GWDSettingsTest, TestUseTooltipsChangedIsDefault)
{
    EXPECT_FALSE (gwd_settings_use_tooltips_changed (mSettings.get (),
                                                     USE_TOOLTIPS_DEFAULT));
}

TEST_F(GWDSettingsTest, TestBlurChangedTitlebar)
{
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_blur_changed (mSettings.get (),
                                            testing_values::BLUR_TYPE_TITLEBAR_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_blur_type (mSettings.get ()),
                 Eq (testing_values::BLUR_TYPE_TITLEBAR_INT_VALUE));
}

TEST_F(GWDSettingsTest, TestBlurChangedAll)
{
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_blur_changed (mSettings.get (),
                                            testing_values::BLUR_TYPE_ALL_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_blur_type (mSettings.get ()),
                 Eq (testing_values::BLUR_TYPE_ALL_INT_VALUE));
}

TEST_F(GWDSettingsTest, TestBlurChangedNone)
{
    EXPECT_FALSE (gwd_settings_blur_changed (mSettings.get (),
                                             testing_values::BLUR_TYPE_NONE_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_blur_type (mSettings.get ()),
                 Eq (testing_values::BLUR_TYPE_NONE_INT_VALUE));
}

TEST_F(GWDSettingsTest, TestBlurSetCommandLine)
{
    gint blurType = testing_values::BLUR_TYPE_ALL_INT_VALUE;

    mSettings.reset (gwd_settings_new (blurType, NULL),
                     boost::bind (gwd_settings_unref, _1));

    EXPECT_FALSE (gwd_settings_blur_changed (mSettings.get (),
                                             testing_values::BLUR_TYPE_NONE_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_blur_type (mSettings.get ()), Eq (blurType));
}

TEST_F(GWDSettingsTest, TestMetacityThemeChanged)
{
    EXPECT_CALL (*mGMockNotified, updateMetacityTheme ());
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_metacity_theme_changed (mSettings.get (),
                                                      testing_values::USE_METACITY_THEME_VALUE,
                                                      METACITY_THEME_TYPE_DEFAULT,
                                                      testing_values::METACITY_THEME_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_metacity_theme_name (mSettings.get ()),
                 IsStringsEqual (testing_values::METACITY_THEME_VALUE.c_str ()));
}

TEST_F(GWDSettingsTest, TestMetacityThemeChangedNoUseMetacityTheme)
{
    const gchar *metacityTheme = NULL;

    EXPECT_CALL (*mGMockNotified, updateMetacityTheme ());
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_metacity_theme_changed (mSettings.get (), FALSE,
                                                      METACITY_THEME_TYPE_DEFAULT,
                                                      METACITY_THEME_NAME_DEFAULT));

    EXPECT_THAT (gwd_settings_get_metacity_theme_name (mSettings.get ()),
                 IsStringsEqual (metacityTheme));
}

TEST_F(GWDSettingsTest, TestMetacityThemeChangedIsDefault)
{
    EXPECT_FALSE (gwd_settings_metacity_theme_changed (mSettings.get (),
                                                       testing_values::USE_METACITY_THEME_VALUE,
                                                       METACITY_THEME_TYPE_DEFAULT,
                                                       METACITY_THEME_NAME_DEFAULT));
}

TEST_F(GWDSettingsTest, TestMetacityThemeSetCommandLine)
{
    const gchar *metacityTheme = "Ambiance";

    mSettings.reset (gwd_settings_new (BLUR_TYPE_UNSET, metacityTheme),
                     boost::bind (gwd_settings_unref, _1));

    EXPECT_FALSE (gwd_settings_metacity_theme_changed (mSettings.get (),
                                                       testing_values::USE_METACITY_THEME_VALUE,
                                                       METACITY_THEME_TYPE_DEFAULT,
                                                       testing_values::METACITY_THEME_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_metacity_theme_name (mSettings.get ()),
                 IsStringsEqual (metacityTheme));
}

TEST_F(GWDSettingsTest, TestMetacityOpacityChanged)
{
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_opacity_changed (mSettings.get (),
                                               testing_values::ACTIVE_OPACITY_VALUE,
                                               testing_values::INACTIVE_OPACITY_VALUE,
                                               testing_values::ACTIVE_SHADE_OPACITY_VALUE,
                                               testing_values::INACTIVE_SHADE_OPACITY_VALUE));

    EXPECT_THAT (gwd_settings_get_metacity_inactive_opacity (mSettings.get ()),
                 Eq (testing_values::INACTIVE_OPACITY_VALUE));

    EXPECT_THAT (gwd_settings_get_metacity_active_opacity (mSettings.get ()),
                 Eq (testing_values::ACTIVE_OPACITY_VALUE));

    EXPECT_THAT (gwd_settings_get_metacity_inactive_shade_opacity (mSettings.get ()),
                 Eq (testing_values::INACTIVE_SHADE_OPACITY_VALUE));

    EXPECT_THAT (gwd_settings_get_metacity_active_shade_opacity (mSettings.get ()),
                 Eq (testing_values::ACTIVE_SHADE_OPACITY_VALUE));
}

TEST_F(GWDSettingsTest, TestMetacityOpacityChangedIsDefault)
{
    EXPECT_FALSE (gwd_settings_opacity_changed (mSettings.get (),
                                                METACITY_ACTIVE_OPACITY_DEFAULT,
                                                METACITY_INACTIVE_OPACITY_DEFAULT,
                                                METACITY_ACTIVE_SHADE_OPACITY_DEFAULT,
                                                METACITY_INACTIVE_SHADE_OPACITY_DEFAULT));
}

TEST_F(GWDSettingsTest, TestButtonLayoutChanged)
{
    EXPECT_CALL (*mGMockNotified, updateMetacityButtonLayout ());
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_button_layout_changed (mSettings.get (),
                                                     testing_values::BUTTON_LAYOUT_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_metacity_button_layout (mSettings.get ()),
                 IsStringsEqual (testing_values::BUTTON_LAYOUT_VALUE.c_str ()));
}

TEST_F(GWDSettingsTest, TestButtonLayoutChangedIsDefault)
{
    EXPECT_FALSE (gwd_settings_button_layout_changed (mSettings.get (),
                                                      METACITY_BUTTON_LAYOUT_DEFAULT));
}

TEST_F(GWDSettingsTest, TestTitlebarFontChanged)
{
    EXPECT_CALL (*mGMockNotified, updateFrames ());
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_font_changed (mSettings.get (),
                                            testing_values::NO_USE_SYSTEM_FONT_VALUE,
                                            testing_values::TITLEBAR_FONT_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_titlebar_font (mSettings.get ()),
                 IsStringsEqual (testing_values::TITLEBAR_FONT_VALUE.c_str ()));
}

TEST_F(GWDSettingsTest, TestTitlebarFontChangedUseSystemFont)
{
    const gchar *titlebarFont = NULL;

    EXPECT_CALL (*mGMockNotified, updateFrames ());
    EXPECT_CALL (*mGMockNotified, updateDecorations ());
    EXPECT_TRUE (gwd_settings_font_changed (mSettings.get (),
                                            testing_values::USE_SYSTEM_FONT_VALUE,
                                            testing_values::TITLEBAR_FONT_VALUE.c_str ()));

    EXPECT_THAT (gwd_settings_get_titlebar_font (mSettings.get ()),
                 IsStringsEqual (titlebarFont));
}

TEST_F(GWDSettingsTest, TestTitlebarFontChangedIsDefault)
{
    EXPECT_FALSE (gwd_settings_font_changed (mSettings.get (),
                                             testing_values::NO_USE_SYSTEM_FONT_VALUE,
                                             TITLEBAR_FONT_DEFAULT));
}

namespace
{
    class GWDTitlebarActionInfo
    {
	public:

	    GWDTitlebarActionInfo (const std::string &titlebarAction,
				   const std::string &mouseWheelAction,
				   const gint	     titlebarActionId,
				   const gint	     mouseWheelActionId) :
		mTitlebarAction (titlebarAction),
		mMouseWheelAction (mouseWheelAction),
		mTitlebarActionId (titlebarActionId),
		mMouseWheelActionId (mouseWheelActionId)
	    {
	    }

	    const std::string & titlebarAction () const { return mTitlebarAction; }
	    const std::string & mouseWheelAction () const { return mMouseWheelAction; }
	    const gint	      & titlebarActionId () const { return mTitlebarActionId; }
	    const gint	      & mouseWheelActionId () const { return mMouseWheelActionId; }

	private:

	    std::string mTitlebarAction;
	    std::string mMouseWheelAction;
	    gint	mTitlebarActionId;
	    gint	mMouseWheelActionId;
    };
}

class GWDSettingsTestClickActions :
    public GWDSettingsTestCommon,
    public ::testing::WithParamInterface <GWDTitlebarActionInfo>
{
    public:

	virtual void SetUp ()
	{
	    GWDSettingsTestCommon::SetUp ();
	    mSettings.reset (gwd_settings_new (BLUR_TYPE_UNSET, NULL),
			     boost::bind (gwd_settings_unref, _1));
	}

    protected:

	boost::shared_ptr <GWDSettings> mSettings;
};

TEST_P(GWDSettingsTestClickActions, TestClickActionsAndMouseActions)
{
    gwd_settings_titlebar_actions_changed (mSettings.get (),
                                           GetParam ().titlebarAction ().c_str (),
                                           GetParam ().titlebarAction ().c_str (),
                                           GetParam ().titlebarAction ().c_str (),
                                           GetParam ().mouseWheelAction ().c_str ());

    EXPECT_THAT (gwd_settings_get_titlebar_double_click_action (mSettings.get ()),
                 Eq (GetParam ().titlebarActionId ()));

    EXPECT_THAT (gwd_settings_get_titlebar_middle_click_action (mSettings.get ()),
                 Eq (GetParam ().titlebarActionId ()));

    EXPECT_THAT (gwd_settings_get_titlebar_right_click_action (mSettings.get ()),
                 Eq (GetParam ().titlebarActionId ()));

    EXPECT_THAT (gwd_settings_get_mouse_wheel_action (mSettings.get ()),
                 Eq (GetParam ().mouseWheelActionId ()));
}

INSTANTIATE_TEST_CASE_P (MouseActions, GWDSettingsTestClickActions,
			 ::testing::Values (GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_NONE,
								   testing_values::TITLEBAR_ACTION_NONE,
								   CLICK_ACTION_NONE,
								   WHEEL_ACTION_NONE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_SHADE,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_SHADE,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_MAX,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_MAXIMIZE,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_MINIMIZE,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_MINIMIZE,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_LOWER,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_LOWER,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_MENU,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_MENU,
								   WHEEL_ACTION_SHADE)));
