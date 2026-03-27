// Copyright (c) 2026 OpenStrike Project
// math.h is part of OpenStrike.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef OPENSTRIKE_MATH_H
#define OPENSTRIKE_MATH_H

#include <math.h>
#include "../types.h"

/* ---- Vec3 ---- */

typedef struct { f32 x, y, z; } Vec3;

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}
static inline Vec3 vec3_scale(Vec3 a, f32 s) {
    return (Vec3){ a.x * s, a.y * s, a.z * s };
}
static inline f32 vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
static inline Vec3 vec3_norm(Vec3 a) {
    f32 inv = 1.0f / sqrtf(vec3_dot(a, a));
    return vec3_scale(a, inv);
}
static inline f32 vec3_len(Vec3 a) {
    return sqrtf(vec3_dot(a, a));
}

/* ---- Mat4 (column-major, convencion OpenGL) ---- */

typedef struct { f32 m[16]; } Mat4;

static inline Mat4 mat4_identity(void) {
    Mat4 r = {{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }};
    return r;
}

static inline Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r;
    int col, row, k;
    for (col = 0; col < 4; col++) {
        for (row = 0; row < 4; row++) {
            f32 sum = 0.0f;
            for (k = 0; k < 4; k++) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

/* Proyeccion perspectiva (right-hand, -Z hacia adelante) */
static inline Mat4 mat4_perspective(f32 fov_deg, f32 aspect, f32 near_z, f32 far_z) {
    f32 f = 1.0f / tanf(fov_deg * (3.14159265f / 180.0f) * 0.5f);
    Mat4 r = {{ 0 }};
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (far_z + near_z) / (near_z - far_z);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far_z * near_z) / (near_z - far_z);
    return r;
}

/* Matriz view look-at */
static inline Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_norm(vec3_sub(center, eye));
    Vec3 s = vec3_norm(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);
    Mat4 r;
    r.m[0]  =  s.x;  r.m[4]  =  s.y;  r.m[8]  =  s.z;  r.m[12] = -vec3_dot(s, eye);
    r.m[1]  =  u.x;  r.m[5]  =  u.y;  r.m[9]  =  u.z;  r.m[13] = -vec3_dot(u, eye);
    r.m[2]  = -f.x;  r.m[6]  = -f.y;  r.m[10] = -f.z;  r.m[14] =  vec3_dot(f, eye);
    r.m[3]  =  0.0f; r.m[7]  =  0.0f; r.m[11] =  0.0f; r.m[15] =  1.0f;
    return r;
}

/* View matrix FPS: posicion + yaw + pitch.
   yaw=0, pitch=0  -> mirando a -Z (convencion OpenGL).
   yaw aumenta     -> gira a la derecha.
   pitch positivo  -> mira hacia arriba. */
static inline Mat4 mat4_fps_view(Vec3 pos, f32 yaw_deg, f32 pitch_deg) {
    f32  yaw   = yaw_deg   * (3.14159265f / 180.0f);
    f32  pitch = pitch_deg * (3.14159265f / 180.0f);
    Vec3 fwd   = {
         sinf(yaw) * cosf(pitch),
         sinf(pitch),
        -cosf(yaw) * cosf(pitch)
    };
    Vec3 center = vec3_add(pos, fwd);
    Vec3 up     = { 0.0f, 1.0f, 0.0f };
    return mat4_look_at(pos, center, up);
}

#endif /* OPENSTRIKE_MATH_H */
