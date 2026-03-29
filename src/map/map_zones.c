// Copyright (c) 2026 OpenStrike Project
// map_zones.c is part of OpenStrike.
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

#include "map_zones.h"

/* ── Primitiva AABB ──────────────────────────────────────────────────── */

bool zone_contiene(const ZoneAABB *z, Vec3 pos) {
    return pos.x >= z->mins.x && pos.x <= z->maxs.x &&
           pos.y >= z->mins.y && pos.y <= z->maxs.y &&
           pos.z >= z->mins.z && pos.z <= z->maxs.z;
}

/* ── Bomb sites ──────────────────────────────────────────────────────── */

bool zone_es_bomb_site_a(const Map *map, Vec3 pos) {
    if (!map->tiene_bomb_site_a) return false;
    return zone_contiene(&map->bomb_site_a, pos);
}

bool zone_es_bomb_site_b(const Map *map, Vec3 pos) {
    if (!map->tiene_bomb_site_b) return false;
    return zone_contiene(&map->bomb_site_b, pos);
}

/* ── Buy zones ───────────────────────────────────────────────────────── */

bool zone_puede_comprar_ct(const Map *map, Vec3 pos) {
    if (!map->tiene_buy_zone_ct) return false;
    return zone_contiene(&map->buy_zone_ct, pos);
}

bool zone_puede_comprar_t(const Map *map, Vec3 pos) {
    if (!map->tiene_buy_zone_t) return false;
    return zone_contiene(&map->buy_zone_t, pos);
}

/* ── Spawns ──────────────────────────────────────────────────────────── */

Vec3 zone_spawn_ct(const Map *map, int indice) {
    if (indice < 0 || indice >= map->spawn_ct_count) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    return map->spawns_ct[indice].pos;
}

Vec3 zone_spawn_t(const Map *map, int indice) {
    if (indice < 0 || indice >= map->spawn_t_count) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    return map->spawns_t[indice].pos;
}
