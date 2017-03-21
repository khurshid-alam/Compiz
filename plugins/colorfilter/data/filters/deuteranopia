void deuteranopia_fragment () {
    vec3 color = gl_FragColor.rgb;
    vec4 ab = vec4(-0.0615003999429, 0.058589396688,
		   0.0827700165681, -0.0793451999536);
    vec3 ci = vec3(-0.01320014122, 0.013289415272, 0.57638336864);

    color.r = pow(color.r, 0.4761904);
    color.g = pow(color.g, 0.5);
    color.b = pow(color.b, 0.4761904);

    vec3 temp1;
    temp1.r = dot(vec3(0.05059983, 0.08585369, 0.00952420), color);
    temp1.g = dot(vec3(0.01893033, 0.08925308, 0.01370054), color);
    temp1.b = dot(vec3(0.00292202, 0.00975732, 0.07145979), color);

    if ((1.0/temp1.r) * temp1.b - ci.b < 0.0) {
	temp1.g = (ab.r * temp1.r + ci.r * temp1.b) * (1.0/-ab.b);
    } else {
	temp1.g = (ab.g * temp1.r + ci.g * temp1.b) * (1.0/-ab.a);
    }

    color.r = dot(vec3(30.830854, -29.832659, 1.610474), temp1);
    color.g = dot(vec3(-6.481468, 17.715578, -2.532642), temp1);
    color.b = dot(vec3(-0.375690, -1.199062, 14.273846), temp1);

    color.r = pow(color.r, 2.1);
    color.g = pow(color.g, 2.0);
    color.b = pow(color.b, 2.1);

    gl_FragColor.rgb = color;
}
