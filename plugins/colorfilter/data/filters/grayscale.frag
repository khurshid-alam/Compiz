void grayscale_fragment () {
    float v = dot(gl_FragColor.rgb, vec3(0.33333, 0.33333, 0.33333));
    gl_FragColor.rgb = vec3(v, v, v);
}
