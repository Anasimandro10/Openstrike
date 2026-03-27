#version 120

attribute vec3 a_position;
attribute vec2 a_texcoord;
attribute vec3 a_lightcolor;

varying vec2 v_texcoord;
varying vec3 v_lightcolor;

uniform mat4 u_view;
uniform mat4 u_proj;

void main() {
    v_texcoord   = a_texcoord;
    v_lightcolor = a_lightcolor;
    gl_Position  = u_proj * u_view * vec4(a_position, 1.0);
}
