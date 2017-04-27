void negative_fragment () {
    vec3 color = vec3(1.0, 1.0, 1.0) - gl_FragColor.rgb;
    gl_FragColor.rgb = color;
}
