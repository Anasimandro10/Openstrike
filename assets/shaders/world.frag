#version 120

varying vec2 v_texcoord;
varying vec3 v_lightcolor;

uniform sampler2D u_diffuse;

void main() {
    vec3 color   = texture2D(u_diffuse, v_texcoord).rgb;
    gl_FragColor = vec4(color * v_lightcolor, 1.0);
}
