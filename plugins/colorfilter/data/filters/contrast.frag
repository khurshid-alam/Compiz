void contrast_fragment () {
    vec3 d;

    d.r = gl_FragColor.r < 0.51 ? -0.3 : +0.3;
    d.g = gl_FragColor.g < 0.51 ? -0.3 : +0.3;
    d.b = gl_FragColor.b < 0.51 ? -0.3 : +0.3;

    gl_FragColor.rgb += d;
}
