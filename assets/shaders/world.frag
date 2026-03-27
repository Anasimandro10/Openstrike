#version 120

varying vec2 v_texcoord;
varying vec3 v_lightcolor;

uniform sampler2D u_diffuse;

void main() {
    vec4 texColor = texture2D(u_diffuse, v_texcoord);
    vec3 final = texColor.rgb * v_lightcolor;
    gl_FragColor = vec4(final, texColor.a);
}
