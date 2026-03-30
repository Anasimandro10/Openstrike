// Copyright (c) 2026 OpenStrike Project
// collision.h is part of OpenStrike.
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

#ifndef OPENSTRIKE_COLLISION_H
#define OPENSTRIKE_COLLISION_H

#include "types.h"
#include "renderer/math.h"
#include "map/map.h"

/* ────────────────────────────────────────────────────────────────────────── *
 * Resultado de una consulta de suelo.                                        *
 * ────────────────────────────────────────────────────────────────────────── */
typedef struct {
    bool hit;      /* true si hay geometría de suelo bajo el jugador         */
    f32  y;        /* altura del suelo — posición Y donde apoyar los pies    */
    Vec3 normal;   /* normal de la superficie golpeada (apunta hacia arriba)  */
} FloorHit;

/* Lanza un rayo hacia abajo desde 'pos' y busca la superficie de suelo más
 * cercana dentro de SV_STEPSIZE + 2 unidades.
 * Solo detecta caras cuya normal apunta hacia arriba (Y > 0) — ignora techos
 * y paredes.
 * Seguro llamarlo cada tick: sin malloc, O(vértices del mapa). */
FloorHit collision_detectar_suelo(const Map *map, Vec3 pos);

#endif /* OPENSTRIKE_COLLISION_H */
