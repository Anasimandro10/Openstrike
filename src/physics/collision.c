// Copyright (c) 2026 OpenStrike Project
// collision.c is part of OpenStrike.
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

#include "collision.h"
#include <math.h>
#include <string.h>

/* ── constantes internas ─────────────────────────────────────────────────── */

/* Offset del origen del rayo sobre los pies del jugador (evita self-hit) */
#define RAY_ORIG_OFFSET   1.0f

/* Distancia máxima de búsqueda = SV_STEPSIZE (18) + margen de 2 unidades */
#define SUELO_MAX_DIST   20.0f

/* ── Möller–Trumbore: intersección rayo–triángulo ────────────────────────── *
 * Retorna true si el rayo (orig, dir) intersecta el triángulo (v0,v1,v2).    *
 * Solo acepta triángulos con normal Y > 0 (cara de suelo, no techo/pared).  *
 * Escribe la distancia t en *t_out. t > 0 siempre al retornar true.         */
static bool ray_triangulo(Vec3 orig, Vec3 dir,
                           Vec3 v0, Vec3 v1, Vec3 v2,
                           f32 *t_out)
{
    Vec3 e1 = vec3_sub(v1, v0);
    Vec3 e2 = vec3_sub(v2, v0);

    /* Descartar caras no-suelo antes del cálculo completo */
    Vec3 face_normal = vec3_cross(e1, e2);
    if (face_normal.y <= 0.0f) { return false; }

    /* Test de paralelismo */
    Vec3 h = vec3_cross(dir, e2);
    f32  a = vec3_dot(e1, h);
    if (a > -1e-6f && a < 1e-6f) { return false; }

    f32  f = 1.0f / a;
    Vec3 s = vec3_sub(orig, v0);
    f32  u = f * vec3_dot(s, h);
    if (u < 0.0f || u > 1.0f) { return false; }

    Vec3 q = vec3_cross(s, e1);
    f32  v = f * vec3_dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) { return false; }

    f32 t = f * vec3_dot(e2, q);
    if (t < 1e-6f) { return false; }   /* intersección detrás del origen */

    *t_out = t;
    return true;
}

/* ── API pública ──────────────────────────────────────────────────────────── */

FloorHit collision_detectar_suelo(const Map *map, Vec3 pos)
{
    FloorHit result;
    memset(&result, 0, sizeof(result));
    result.hit = false;

    if (!map || !map->cargado || map->face_count == 0) {
        return result;
    }

    /* Rayo hacia abajo: origen ligeramente por encima de los pies */
    Vec3 orig = { pos.x, pos.y + RAY_ORIG_OFFSET, pos.z };
    Vec3 dir  = { 0.0f, -1.0f, 0.0f };

    /* Buscamos el hit más cercano dentro del rango permitido */
    f32 best_t = SUELO_MAX_DIST + RAY_ORIG_OFFSET;

    for (i32 fi = 0; fi < map->face_count; fi++) {
        const MapFace *face  = &map->faces[fi];
        u32            base  = face->vertex_start;
        u32            count = face->vertex_count;

        /* Cada 3 vértices = 1 triángulo (el formato garantiza múltiplo de 3) */
        for (u32 ti = 0; ti + 2 < count; ti += 3) {
            const MapVertex *va = &map->vertices[base + ti + 0];
            const MapVertex *vb = &map->vertices[base + ti + 1];
            const MapVertex *vc = &map->vertices[base + ti + 2];

            Vec3 v0 = { va->pos[0], va->pos[1], va->pos[2] };
            Vec3 v1 = { vb->pos[0], vb->pos[1], vb->pos[2] };
            Vec3 v2 = { vc->pos[0], vc->pos[1], vc->pos[2] };

            f32 t;
            if (ray_triangulo(orig, dir, v0, v1, v2, &t)) {
                if (t < best_t) {
                    best_t = t;
                    result.hit = true;
                    /* Y del suelo = Y del origen - distancia recorrida hacia abajo */
                    result.y = orig.y - t;
                    /* Normal normalizada de la cara */
                    Vec3 e1 = vec3_sub(v1, v0);
                    Vec3 e2 = vec3_sub(v2, v0);
                    result.normal = vec3_norm(vec3_cross(e1, e2));
                }
            }
        }
    }

    return result;
}
