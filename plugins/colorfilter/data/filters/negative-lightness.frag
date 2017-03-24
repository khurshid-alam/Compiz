/* HSL/RGB conversion taken off GTK's _gtk_hsla_init_from_rgba() and
 * _gdk_rgba_init_from_hsla(), GPLv2+,
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org> */

/* hsl: .r = H, .g = S, .b = L */
vec3 negative_lightness_rgb_to_hsl(vec3 rgb) {
    vec3 hsl;
    float min;
    float max;

    max = max(max(rgb.r, rgb.g), rgb.b);
    min = min(min(rgb.r, rgb.g), rgb.b);

    hsl.r = 0.0; // H
    hsl.g = 0.0; // S
    hsl.b = (max + min) / 2.0; // L

    if (max != min)
    {
	if (hsl.b <= 0.5)
	    hsl.g = (max - min) / (max + min);
	else
	    hsl.g = (max - min) / (2.0 - max - min);

	float delta = max - min;
	if (rgb.r == max)
	    hsl.r = (rgb.g - rgb.b) / delta;
	else if (rgb.g == max)
	    hsl.r = 2.0 + (rgb.b - rgb.r) / delta;
	else if (rgb.b == max)
	    hsl.r = 4.0 + (rgb.r - rgb.g) / delta;

	hsl.r *= 60.0;
	if (hsl.r < 0.0)
	    hsl.r += 360.0;
    }

    return hsl;
}

/* hsl: .r = H, .g = S, .b = L */
vec3 negative_lightness_hsl_to_rgb(vec3 hsl) {
    vec3 rgb;
    float m1, m2;

    if (hsl.b <= 0.5) // H
	m2 = hsl.b * (1.0 + hsl.g);
    else
	m2 = hsl.b + hsl.g - hsl.b * hsl.g;
    m1 = 2.0 * hsl.b - m2;

    if (hsl.g == 0.0)
	rgb = vec3(hsl.b, hsl.b, hsl.b);
    else
    {
	float hue;

	hue = hsl.r + 120.0;
	while (hue > 360.0)
	    hue -= 360.0;
	while (hue < 0.0)
	    hue += 360.0;

	if (hue < 60.0)
	    rgb.r = m1 + (m2 - m1) * hue / 60.0;
	else if (hue < 180.0)
	    rgb.r = m2;
	else if (hue < 240.0)
	    rgb.r = m1 + (m2 - m1) * (240.0 - hue) / 60.0;
	else
	    rgb.r = m1;

	hue = hsl.r;
	while (hue > 360.0)
	    hue -= 360.0;
	while (hue < 0.0)
	    hue += 360.0;

	if (hue < 60.0)
	    rgb.g = m1 + (m2 - m1) * hue / 60.0;
	else if (hue < 180.0)
	    rgb.g = m2;
	else if (hue < 240.0)
	    rgb.g = m1 + (m2 - m1) * (240.0 - hue) / 60.0;
	else
	    rgb.g = m1;

	hue = hsl.r - 120.0;
	while (hue > 360.0)
	    hue -= 360.0;
	while (hue < 0.0)
	    hue += 360.0;

	if (hue < 60.0)
	    rgb.b = m1 + (m2 - m1) * hue / 60.0;
	else if (hue < 180.0)
	    rgb.b = m2;
	else if (hue < 240.0)
	    rgb.b = m1 + (m2 - m1) * (240.0 - hue) / 60.0;
	else
	    rgb.b = m1;
    }

    return rgb;
}

void negative_lightness_fragment() {
    vec3 hsl = negative_lightness_rgb_to_hsl(gl_FragColor.rgb);
    hsl.b = 1.0 - hsl.b; // invert L
    gl_FragColor.rgb = negative_lightness_hsl_to_rgb(hsl);
}
