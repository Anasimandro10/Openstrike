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

#include "pmove.h"
#include <math.h>
#include <string.h>

/* ── Funciones internas ───────────────────────────────────────────────────── */

/* Aplica fricción sobre el plano XZ cuando el jugador está en suelo. */
static void aplicar_friccion(PhysPlayer *p, f32 dt)
{
    f32 speed = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
    if (speed < 0.1f) { p->vel.x = 0.0f; p->vel.z = 0.0f; return; }

    f32 control   = (speed < SV_STOPSPEED) ? SV_STOPSPEED : speed;
    f32 drop      = control * SV_FRICTION * dt;
    f32 new_speed = speed - drop;
    if (new_speed < 0.0f) { new_speed = 0.0f; }

    f32 factor = new_speed / speed;
    p->vel.x *= factor;
    p->vel.z *= factor;
}

/* Acelera en wish_dir hasta wish_speed con el coeficiente accel dado.
 * Usada tanto para suelo como para aire. */
static void acelerar(PhysPlayer *p, Vec3 wish_dir,
                     f32 wish_speed, f32 accel, f32 dt)
{
    f32 current   = vec3_dot(p->vel, wish_dir);
    f32 add_speed = wish_speed - current;
    if (add_speed <= 0.0f) { return; }

    f32 accel_speed = accel * wish_speed * dt;
    if (accel_speed > add_speed) { accel_speed = add_speed; }

    p->vel = vec3_add(p->vel, vec3_scale(wish_dir, accel_speed));
}

/* ── API pública ──────────────────────────────────────────────────────────── */

void phys_player_init(PhysPlayer *p, Vec3 spawn_pos)
{
    memset(p, 0, sizeof(*p));
    p->pos = spawn_pos;
}

void phys_tick(PhysPlayer *p, PhysInput input, const Map *map, f32 dt)
{
    p->agachado  = input.agacharse;
    p->caminando = input.caminar;

    if (p->en_suelo) {
        /* ── En suelo: fricción + movimiento ──────────────────────────── */
        aplicar_friccion(p, dt);

        if (input.saltar) {
            /* Impulso de salto */
            p->vel.y    = JUMP_IMPULSE;
            p->en_suelo = false;

            /* Air strafing inmediato en el frame del salto */
            if (vec3_len(input.wish_dir) > 0.001f) {
                acelerar(p, input.wish_dir, SPEED_RUN, SV_AIRACCELERATE, dt);
            }

            /* Cap XZ — el bhop mantiene velocidad pero no se acumula */
            f32 xz = sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z);
            if (xz > BHOP_MAX_SPEED) {
                f32 scale = BHOP_MAX_SPEED / xz;
                p->vel.x *= scale;
                p->vel.z *= scale;
            }
        } else {
            /* Aceleración en suelo según estado */
            f32 wish_speed = p->agachado  ? SPEED_DUCK :
                             p->caminando ? SPEED_WALK :
                                            SPEED_RUN;
            if (vec3_len(input.wish_dir) > 0.001f) {
                acelerar(p, input.wish_dir, wish_speed, SV_ACCELERATE, dt);
            }
        }

    } else {
        /* ── En aire: gravedad + air strafing ─────────────────────────── */
        p->vel.y -= SV_GRAVITY * dt;
        if (p->vel.y < -SV_MAXVELOCITY) { p->vel.y = -SV_MAXVELOCITY; }

        if (vec3_len(input.wish_dir) > 0.001f) {
            acelerar(p, input.wish_dir, SPEED_RUN, SV_AIRACCELERATE, dt);
        }
    }

    /* ── Integrar posición ────────────────────────────────────────────── */
    p->pos = vec3_add(p->pos, vec3_scale(p->vel, dt));

    /* ── Detectar suelo real contra geometría del mapa ───────────────── *
     * Reemplaza el suelo temporal "if (pos.y <= 0)".                    *
     * Condiciones para aterrizar:                                        *
     *   - Hay geometría de suelo en rango                                *
     *   - Los pies están dentro de 0.5 u por encima del suelo            *
     *   - La velocidad vertical es 0 o negativa (no estamos subiendo)    */
    FloorHit fh = collision_detectar_suelo(map, p->pos);
    if (fh.hit && p->pos.y <= fh.y + 0.5f && p->vel.y <= 0.0f) {
        p->pos.y    = fh.y;
        p->vel.y    = 0.0f;
        p->en_suelo = true;
    } else {
        p->en_suelo = false;
    }
}

f32 phys_view_height(const PhysPlayer *p)
{
    return p->agachado ? VIEW_HEIGHT_DUCK : VIEW_HEIGHT_STAND;
}
