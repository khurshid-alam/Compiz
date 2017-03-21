void sepia_fragment () {
    vec3 color = gl_FragColor.rgb;
    color.r = dot(gl_FragColor.rgb, vec3(0.393, 0.769, 0.189));
    color.g = dot(gl_FragColor.rgb, vec3(0.349, 0.686, 0.168));
    color.b = dot(gl_FragColor.rgb, vec3(0.272, 0.534, 0.131));
    gl_FragColor.rgb = color.rgb;
}
