// Copyright (c) 2026 OpenStrike Project
// pmove.h is part of OpenStrike.
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
#pragma once

#include "types.h"
#include "renderer/math.h"

/* ── Constantes de movimiento — exactas CS:GO ─────────────────────────────── */
#define SV_GRAVITY        800.0f
#define SV_MAXVELOCITY   3500.0f
#define SV_FRICTION         5.5f
#define SV_STOPSPEED       80.0f
#define SV_ACCELERATE       5.5f
#define SV_AIRACCELERATE   12.0f   /* sin cap post-salto = bunny hop CS 1.6 */
#define SV_STEPSIZE        18.0f
#define SPEED_RUN         250.0f
#define SPEED_WALK        130.0f
#define SPEED_DUCK         85.0f
#define JUMP_IMPULSE  301.993012f
#define VIEW_HEIGHT_STAND  64.0f   /* altura del ojo de pie  */
#define VIEW_HEIGHT_DUCK   28.0f   /* altura del ojo agachado */

/* Input de física — construido en main.c a partir de g_input + g_camara.yaw */
typedef struct {
    Vec3 wish_dir;   /* dirección de movimiento normalizada (plano XZ) */
    bool saltar;
    bool agacharse;
    bool caminar;
} PhysInput;

/* Estado del jugador para el sistema de física */
typedef struct {
    Vec3 pos;
    Vec3 vel;
    bool en_suelo;
    bool agachado;
    bool caminando;
} PhysPlayer;

void phys_player_init(PhysPlayer *p, Vec3 spawn_pos);
void phys_tick(PhysPlayer *p, PhysInput input, f32 dt);
f32  phys_view_height(const PhysPlayer *p);
