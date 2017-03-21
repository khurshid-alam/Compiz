void contrast_fragment () {
    /* FIXME: does CMP compare components AND or OR?
     * was:
     *
     * SUB output.rgb, input, 0.51;
     * CMP output.rgb, output, dark, bright;
     */
    if (gl_FragColor.r < 0.51 && gl_FragColor.g < 0.51 && gl_FragColor.b < 0.51) {
      gl_FragColor.rgb -= vec3(0.3, 0.3, 0.3);
    } else {
      gl_FragColor.rgb += vec3(0.3, 0.3, 0.3);
    }
}
