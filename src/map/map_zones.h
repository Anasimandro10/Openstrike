// Copyright (c) 2026 OpenStrike Project
// map_zones.h is part of OpenStrike.
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

#ifndef OPENSTRIKE_MAP_ZONES_H
#define OPENSTRIKE_MAP_ZONES_H

#include "map.h"

/* ── Consulta de zonas ────────────────────────────────────────────────────
   Todas las funciones son O(1). Seguras a llamar en el game loop.
   ────────────────────────────────────────────────────────────────────── */

/* true si pos esta dentro de la caja AABB z (inclusive en bordes)        */
bool zone_contiene(const ZoneAABB *z, Vec3 pos);

/* ── Bomb sites ──────────────────────────────────────────────────────── */

/* true si pos esta en bomb site A (y el mapa lo tiene definido)          */
bool zone_es_bomb_site_a(const Map *map, Vec3 pos);

/* true si pos esta en bomb site B (y el mapa lo tiene definido)          */
bool zone_es_bomb_site_b(const Map *map, Vec3 pos);

/* ── Buy zones ───────────────────────────────────────────────────────── */

/* true si pos esta en la zona de compra CT                               */
bool zone_puede_comprar_ct(const Map *map, Vec3 pos);

/* true si pos esta en la zona de compra T                                */
bool zone_puede_comprar_t(const Map *map, Vec3 pos);

/* ── Spawns ──────────────────────────────────────────────────────────── */

/* Retorna el spawn CT numero 'indice' (0-based).
   Si indice esta fuera de rango retorna (0, 0, 0).                       */
Vec3 zone_spawn_ct(const Map *map, int indice);

/* Retorna el spawn T numero 'indice' (0-based).
   Si indice esta fuera de rango retorna (0, 0, 0).                       */
Vec3 zone_spawn_t(const Map *map, int indice);

#endif /* OPENSTRIKE_MAP_ZONES_H */
