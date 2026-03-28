// Copyright (c) 2026 OpenStrike Project
// pmove.c is part of OpenStrike.
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

#include <math.h>
#include <string.h>
#include "pmove.h"

/* ------------------------------------------------------------------ */
/* Funciones internas                                                  */
/* ------------------------------------------------------------------ */

static void aplicar_friccion(PhysPlayer *p, f32 dt) {
    f32 speed = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
    if (speed < 0.1f) {
        p->vel.x = 0.0f;
        p->vel.z = 0.0f;
        return;
    }
    f32 control   = (speed < SV_STOPSPEED) ? SV_STOPSPEED : speed;
    f32 drop      = control * SV_FRICTION * dt;
    f32 new_speed = speed - drop;
    if (new_speed < 0.0f) { new_speed = 0.0f; }
    f32 factor = new_speed / speed;
    p->vel.x *= factor;
    p->vel.z *= factor;
}

static void acelerar(PhysPlayer *p, Vec3 wish_dir,
                     f32 wish_speed, f32 accel, f32 dt) {
    f32 current = vec3_dot(p->vel, wish_dir);
    f32 add     = wish_speed - current;
    if (add <= 0.0f) { return; }
    f32 accel_speed = accel * wish_speed * dt;
    if (accel_speed > add) { accel_speed = add; }
    p->vel = vec3_add(p->vel, vec3_scale(wish_dir, accel_speed));
}

/* ------------------------------------------------------------------ */
/* API publica                                                         */
/* ------------------------------------------------------------------ */

void phys_player_init(PhysPlayer *p, Vec3 spawn_pos) {
    memset(p, 0, sizeof(*p));
    p->pos = spawn_pos;
}

void phys_tick(PhysPlayer *p, PhysInput input, f32 dt) {
    p->agachado  = input.agacharse;
    p->caminando = input.caminar;

    bool hay_wish = vec3_len(input.wish_dir) > 0.001f;

    if (p->en_suelo) {

        aplicar_friccion(p, dt);

        if (input.saltar) {
            p->vel.y    = JUMP_IMPULSE;
            p->en_suelo = false;
        }

        if (hay_wish) {
            f32 wish_speed = p->agachado  ? SPEED_DUCK :
                             p->caminando ? SPEED_WALK :
                                            SPEED_RUN;
            acelerar(p, input.wish_dir, wish_speed, SV_ACCELERATE, dt);
        }

    } else {

        /* Gravedad */
        p->vel.y -= SV_GRAVITY * dt;
        if (p->vel.y < -SV_MAXVELOCITY) { p->vel.y = -SV_MAXVELOCITY; }

        /* Aceleracion lateral en aire — air strafing estilo CS 1.6 */
        if (hay_wish) {
            acelerar(p, input.wish_dir, SPEED_RUN, SV_AIRACCELERATE, dt);
        }

        /* Cap de velocidad horizontal.
           Tras un salto con buen timing se mantiene ~320 u/s.
           Sin tecnica correcta la velocidad vuelve a 250 por friccion al aterrizar. */
        f32 xz = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
        if (xz > BHOP_MAX_SPEED) {
            f32 factor = BHOP_MAX_SPEED / xz;
            p->vel.x *= factor;
            p->vel.z *= factor;
        }
    }

    /* Integrar posicion */
    p->pos = vec3_add(p->pos, vec3_scale(p->vel, dt));

    /* Colision temporal con suelo — Sistema 4 reemplazara con swept AABB */
    if (p->pos.y <= 0.0f) {
        p->pos.y    = 0.0f;
        p->vel.y    = 0.0f;
        p->en_suelo = true;
    }
}

f32 phys_view_height(const PhysPlayer *p) {
    return p->agachado ? VIEW_HEIGHT_DUCK : VIEW_HEIGHT_STAND;
}
