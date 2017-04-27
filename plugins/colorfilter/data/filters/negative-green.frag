void negative_green_fragment () {
    gl_FragColor.rgb = vec3(0, 1.0 - gl_FragColor.g, 0);
}
