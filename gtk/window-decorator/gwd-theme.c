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
#include "gtk-window-decorator.h"
#include "gwd-settings.h"
#include "gwd-theme.h"
#include "gwd-theme-cairo.h"

#ifdef USE_METACITY
#include "gwd-theme-metacity.h"
#endif

typedef struct
{
    GWDSettings          *settings;

    PangoFontDescription *titlebar_font;

    GtkWidget            *style_window;

    gulong                monitors_changed_id;
    gboolean              fixed_scale;
    gint                  scale;

    gulong                gtk_xft_dpi_id;
    gdouble               dpi;
} GWDThemePrivate;

enum
{
    PROP_0,

    PROP_SETTINGS,

    LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL };

G_DEFINE_TYPE_WITH_PRIVATE (GWDTheme, gwd_theme, G_TYPE_OBJECT)

static void
frames_update_pango_contexts (gpointer key,
                              gpointer value,
                              gpointer user_data)
{
    decor_frame_t *frame = (decor_frame_t *) value;
    GWDTheme *theme = GWD_THEME (user_data);
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    if (frame->pango_context == NULL)
        return;

    /* FIXME: PangoContext created by gtk_widget_create_pango_context is not
     * automatically updated. Resolution is not only thing that can change...
     */
    pango_cairo_context_set_resolution (frame->pango_context,
                                        priv->dpi * priv->scale);
}

static void
style_updated_cb (GtkWidget *widget,
                  GWDTheme  *theme)
{
    gwd_frames_foreach (frames_update_pango_contexts, theme);

    GWD_THEME_GET_CLASS (theme)->style_updated (theme);

    decorations_changed (wnck_screen_get_default ());
}

static void
create_style_window (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    GdkScreen *screen = gdk_screen_get_default ();
    GdkVisual *visual = gdk_screen_get_rgba_visual (screen);
    GtkWindow *window;

    priv->style_window = gtk_window_new (GTK_WINDOW_POPUP);
    window = GTK_WINDOW (priv->style_window);

    if (visual)
	    gtk_widget_set_visual (priv->style_window, visual);

    gtk_window_move (window, -100, -100);
    gtk_window_resize (window, 1, 1);

    gtk_widget_show (priv->style_window);

    g_signal_connect (priv->style_window, "style-updated",
                      G_CALLBACK (style_updated_cb), theme);
}

static void
monitors_changed_cb (GdkScreen *screen,
                     GWDTheme  *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    gint scale = gtk_widget_get_scale_factor (priv->style_window);

    if (priv->scale == scale)
        return;

    priv->scale = scale;

    GWD_THEME_GET_CLASS (theme)->scale_changed (theme);

    decorations_changed (wnck_screen_get_default ());
}

static void
track_window_scale (GWDTheme *theme)
{
    GdkScreen *screen = gdk_screen_get_default ();
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
 
    priv->monitors_changed_id = g_signal_connect (screen, "monitors-changed",
                                                  G_CALLBACK (monitors_changed_cb),
                                                  theme);

    priv->fixed_scale = g_getenv ("GDK_SCALE") != NULL;
    priv->scale = gtk_widget_get_scale_factor (priv->style_window);
}

static gdouble
get_unscaled_dpi (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    gint xft_dpi;
    gdouble dpi;

    g_object_get (gtk_settings_get_default (), "gtk-xft-dpi", &xft_dpi, NULL);

    if (xft_dpi > 0) {
        const gchar *scale_env;

        dpi = xft_dpi / 1024.0;

        /* When using automatic scaling gtk-xft-dpi value is unscaled, but
         * when it is disabled with GDK_SCALE or gdk_display_x11_set_scale then
         * this value will be already scaled.
         */
        if (priv->fixed_scale) {
            GdkScreen *screen = gdk_screen_get_default ();
            GValue value = G_VALUE_INIT;

            g_value_init (&value, G_TYPE_INT);

            if (gdk_screen_get_setting (screen, "gdk-window-scaling-factor", &value))
                dpi /= g_value_get_int (&value);
        }

        /* gtk-xft-dpi value is not scaled with GDK_DPI_SCALE, we need to do
         * that manually if we want GDK_DPI_SCALE support in decorations.
         */
        scale_env = g_getenv ("GDK_DPI_SCALE");
        if (scale_env) {
            gdouble scale = g_ascii_strtod (scale_env, NULL);

            if (scale != 0)
                dpi *= scale;
        }
    } else {
        dpi = 96.0;
    }

    return dpi;
}

static void
notify_gtk_xft_dpi_cb (GtkSettings *settings,
                       GParamSpec  *pspec,
                       GWDTheme    *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    gdouble dpi = get_unscaled_dpi (theme);

    if (priv->dpi == dpi)
        return;

    priv->dpi = dpi;

    gwd_frames_foreach (frames_update_pango_contexts, theme);

    GWD_THEME_GET_CLASS (theme)->dpi_changed (theme);

    decorations_changed (wnck_screen_get_default ());
}

static void
track_xft_dpi (GWDTheme *theme)
{
    GtkSettings *settings = gtk_settings_get_default ();
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    priv->gtk_xft_dpi_id = g_signal_connect (settings, "notify::gtk-xft-dpi",
                                             G_CALLBACK (notify_gtk_xft_dpi_cb),
                                             theme);

    priv->dpi = get_unscaled_dpi (theme);
}

static void
gwd_theme_constructed (GObject *object)
{
    GWDTheme *theme = GWD_THEME (object);

    G_OBJECT_CLASS (gwd_theme_parent_class)->constructed (object);

    create_style_window (theme);
    track_window_scale (theme);
    track_xft_dpi (theme);
}

static void
gwd_theme_dispose (GObject *object)
{
    GWDTheme *theme = GWD_THEME (object);
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    g_clear_object (&priv->settings);

    pango_font_description_free (priv->titlebar_font);
    priv->titlebar_font = NULL;

    g_clear_pointer (&priv->style_window, gtk_widget_destroy);

    if (priv->monitors_changed_id != 0) {
        GdkScreen *screen = gdk_screen_get_default ();

        g_signal_handler_disconnect (screen, priv->monitors_changed_id);
        priv->monitors_changed_id = 0;
    }

    if (priv->gtk_xft_dpi_id != 0) {
        GtkSettings *settings = gtk_settings_get_default ();

        g_signal_handler_disconnect (settings, priv->gtk_xft_dpi_id);
        priv->gtk_xft_dpi_id = 0;
    }

    G_OBJECT_CLASS (gwd_theme_parent_class)->dispose (object);
}

static void
gwd_theme_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    GWDTheme *theme = GWD_THEME (object);
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    switch (property_id) {
        case PROP_SETTINGS:
            g_value_set_object (value, priv->settings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_theme_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    GWDTheme *theme = GWD_THEME (object);
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    switch (property_id) {
        case PROP_SETTINGS:
            priv->settings = g_value_dup_object (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
gwd_theme_real_dpi_changed (GWDTheme *theme)
{
}

static void
gwd_theme_real_scale_changed (GWDTheme *theme)
{
}

static void
gwd_theme_real_style_updated (GWDTheme *theme)
{
}

static void
gwd_theme_real_get_shadow (GWDTheme               *theme,
                           decor_frame_t          *frame,
                           decor_shadow_options_t *options,
                           gboolean                active)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    decor_shadow_options_t shadow;

    if (active)
        shadow = gwd_settings_get_active_shadow (priv->settings);
    else
        shadow = gwd_settings_get_inactive_shadow (priv->settings);

    memcpy (options, &shadow, sizeof (decor_shadow_options_t));
}

static void
gwd_theme_real_draw_window_decoration (GWDTheme *theme,
                                       decor_t  *decor)
{
}

static gboolean
gwd_theme_real_calc_decoration_size (GWDTheme *theme,
                                     decor_t  *decor,
                                     gint      w,
                                     gint      h,
                                     gint      name_width,
                                     gint     *width,
                                     gint     *height)
{
    return FALSE;
}

static void
gwd_theme_real_update_border_extents (GWDTheme      *theme,
                                      decor_frame_t *frame)
{
}

static void
gwd_theme_real_get_event_window_position (GWDTheme *theme,
                                          decor_t  *decor,
                                          gint      i,
                                          gint      j,
                                          gint      width,
                                          gint      height,
                                          gint     *x,
                                          gint     *y,
                                          gint     *w,
                                          gint     *h)
{
}

static gboolean
gwd_theme_real_get_button_position (GWDTheme *theme,
                                    decor_t  *decor,
                                    gint      i,
                                    gint      width,
                                    gint      height,
                                    gint     *x,
                                    gint     *y,
                                    gint     *w,
                                    gint     *h)
{
    return FALSE;
}

static void
gwd_theme_real_update_titlebar_font (GWDTheme                   *theme,
                                     const PangoFontDescription *titlebar_font)
{
}

static void
gwd_theme_class_init (GWDThemeClass *theme_class)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (theme_class);

    object_class->constructed = gwd_theme_constructed;
    object_class->dispose = gwd_theme_dispose;
    object_class->get_property = gwd_theme_get_property;
    object_class->set_property = gwd_theme_set_property;

    theme_class->dpi_changed = gwd_theme_real_dpi_changed;
    theme_class->scale_changed = gwd_theme_real_scale_changed;
    theme_class->style_updated = gwd_theme_real_style_updated;
    theme_class->get_shadow = gwd_theme_real_get_shadow;
    theme_class->draw_window_decoration = gwd_theme_real_draw_window_decoration;
    theme_class->calc_decoration_size = gwd_theme_real_calc_decoration_size;
    theme_class->update_border_extents = gwd_theme_real_update_border_extents;
    theme_class->get_event_window_position = gwd_theme_real_get_event_window_position;
    theme_class->get_button_position = gwd_theme_real_get_button_position;
    theme_class->update_titlebar_font = gwd_theme_real_update_titlebar_font;

    properties[PROP_SETTINGS] =
        g_param_spec_object ("settings", "GWDSettings", "GWDSettings",
                             GWD_TYPE_SETTINGS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gwd_theme_init (GWDTheme *theme)
{
}

/**
 * gwd_theme_new:
 * @type: a #GWDThemeType
 * @settings: a #GWDSettings
 *
 * Creates a new #GWDTheme. If requested @type can not be created then a
 * #GWDThemeCairo will be created as fallback. Thus this function will always
 * return valid theme.
 *
 * Returns: (transfer full): a newly created #GWDTheme
 */
GWDTheme *
gwd_theme_new (GWDThemeType  type,
               GWDSettings  *settings)
{
#ifdef USE_METACITY
    if (type == GWD_THEME_TYPE_METACITY) {
        GWDTheme *theme = gwd_theme_metacity_new (settings);

        /* gwd_theme_metacity_new may return NULL if meta_theme_load fails
         * to load Metacity theme. In such case we must fallback to Cairo
         * theme.
         */
        if (theme != NULL)
            return theme;
    }
#endif

    return g_object_new (GWD_TYPE_THEME_CAIRO,
                         "settings", settings,
                         NULL);
}

GWDSettings *
gwd_theme_get_settings (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    return priv->settings;
}

gdouble
gwd_theme_get_dpi (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    return priv->dpi;
}

gint
gwd_theme_get_scale (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    return priv->scale;
}

GtkWidget *
gwd_theme_get_style_window (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);

    return priv->style_window;
}

void
gwd_theme_get_shadow (GWDTheme               *theme,
                      decor_frame_t          *frame,
                      decor_shadow_options_t *options,
                      gboolean                active)
{
    GWD_THEME_GET_CLASS (theme)->get_shadow (theme, frame, options, active);
}

void
gwd_theme_draw_window_decoration (GWDTheme *theme,
                                  decor_t  *decor)
{
    GWD_THEME_GET_CLASS (theme)->draw_window_decoration (theme, decor);
}

gboolean
gwd_theme_calc_decoration_size (GWDTheme *theme,
                                decor_t  *decor,
                                gint      w,
                                gint      h,
                                gint      name_width,
                                gint     *width,
                                gint     *height)
{
    return GWD_THEME_GET_CLASS (theme)->calc_decoration_size (theme, decor,
                                                              w, h, name_width,
                                                              width, height);
}

void
gwd_theme_update_border_extents (GWDTheme      *theme,
                                 decor_frame_t *frame)
{
    GWD_THEME_GET_CLASS (theme)->update_border_extents (theme, frame);
}

void
gwd_theme_get_event_window_position (GWDTheme *theme,
                                     decor_t  *decor,
                                     gint      i,
                                     gint      j,
                                     gint      width,
                                     gint      height,
                                     gint     *x,
                                     gint     *y,
                                     gint     *w,
                                     gint     *h)
{
    GWD_THEME_GET_CLASS (theme)->get_event_window_position (theme, decor, i, j,
                                                            width, height,
                                                            x, y, w, h);
}

gboolean
gwd_theme_get_button_position (GWDTheme *theme,
                               decor_t  *decor,
                               gint      i,
                               gint      width,
                               gint      height,
                               gint     *x,
                               gint     *y,
                               gint     *w,
                               gint     *h)
{
    return GWD_THEME_GET_CLASS (theme)->get_button_position (theme, decor, i,
                                                             width, height,
                                                             x, y, w, h);
}

void
gwd_theme_update_titlebar_font (GWDTheme *theme)
{
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    const gchar *titlebar_font = gwd_settings_get_titlebar_font (priv->settings);

    pango_font_description_free (priv->titlebar_font);
    priv->titlebar_font = NULL;

    if (titlebar_font != NULL)
        priv->titlebar_font = pango_font_description_from_string (titlebar_font);

    GWD_THEME_GET_CLASS (theme)->update_titlebar_font (theme, priv->titlebar_font);
}

PangoFontDescription *
gwd_theme_get_titlebar_font (GWDTheme      *theme,
                             decor_frame_t *frame)
{
    PangoFontDescription *font_desc = NULL;
    GWDThemePrivate *priv = gwd_theme_get_instance_private (theme);
    GtkStyleContext *context = gtk_widget_get_style_context (priv->style_window);

    /* Check if non-system font is in use */
    if (priv->titlebar_font)
        return pango_font_description_copy (priv->titlebar_font);

    /* Use system titlebar font */
    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_get (context, GTK_STATE_FLAG_NORMAL, "font", &font_desc, NULL);
    gtk_style_context_restore (context);

    return font_desc;
}
