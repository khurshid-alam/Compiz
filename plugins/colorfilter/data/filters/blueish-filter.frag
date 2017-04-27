void blueish_filter_fragment () {
    vec3 color = vec3(1.0, 1.0, 1.0) - gl_FragColor.rgb;
    color.b = clamp(0.8 + color.b, 0.0, 1.0);
    gl_FragColor.rgb = color;
}
