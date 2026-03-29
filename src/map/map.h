// Copyright (c) 2026 OpenStrike Project
// map.h is part of OpenStrike.
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

#ifndef OPENSTRIKE_MAP_H
#define OPENSTRIKE_MAP_H

#include "types.h"
#include "renderer/math.h"
#include "waypoints.h"        /* WaypointGraph — añadido en 4d */

/* ─── Constantes ────────────────────────────────────────────────────────── */

#define MAP_MAX_VERTICES    65536   /* 65536 × 32 B = 2 MB                  */
#define MAP_MAX_FACES       4096    /* caras del mapa                        */
#define MAP_MAX_TEX_NAME    64      /* longitud máx. nombre de textura        */
#define MAP_NOMBRE_MAX      64      /* longitud máx. nombre del mapa          */
#define MAP_FORMAT_VERSION  1       /* versión actual del formato JSON        */
#define MAP_MAX_SPAWNS_CT   10      /* máx. spawns CT por mapa               */
#define MAP_MAX_SPAWNS_T    10      /* máx. spawns T por mapa                */

/* ─── Vértice — layout idéntico al Vertex interno de renderer.c ──────────
   pos[3] @ offset 0   (12 B)
   uv[2]  @ offset 12  ( 8 B)
   light  @ offset 20  (12 B)
   total: 32 bytes — stride del VBO                                          */
typedef struct {
    f32 pos[3];
    f32 uv[2];
    f32 light[3];
} MapVertex;

/* ─── Cara ───────────────────────────────────────────────────────────────── */
typedef struct {
    u32  vertex_start;                /* índice base en Map.vertices          */
    u32  vertex_count;                /* siempre múltiplo de 3 (triangulado)  */
    char textura[MAP_MAX_TEX_NAME];   /* nombre sin extensión ni ruta         */
} MapFace;

/* ─── Punto de spawn ─────────────────────────────────────────────────────── */
typedef struct {
    Vec3 pos;   /* Y = suelo del jugador en unidades Hammer */
} SpawnPoint;

/* ─── Zona AABB ──────────────────────────────────────────────────────────── */
typedef struct {
    Vec3 mins;
    Vec3 maxs;
} ZoneAABB;

/* ─── Mapa completo — declarar siempre como static global (~2.3 MB) ──────── */
typedef struct {
    char      nombre[MAP_NOMBRE_MAX];
    i32       version;

    MapVertex vertices[MAP_MAX_VERTICES];
    i32       vertex_count;

    MapFace   faces[MAP_MAX_FACES];
    i32       face_count;

    bool      cargado;

    /* Spawns (añadidos en 4c) */
    SpawnPoint spawns_ct[MAP_MAX_SPAWNS_CT];
    i32        spawn_ct_count;
    SpawnPoint spawns_t[MAP_MAX_SPAWNS_T];
    i32        spawn_t_count;

    /* Zonas (añadidas en 4c) */
    ZoneAABB bomb_site_a;
    ZoneAABB bomb_site_b;
    ZoneAABB buy_zone_ct;
    ZoneAABB buy_zone_t;

    bool tiene_bomb_site_a;
    bool tiene_bomb_site_b;
    bool tiene_buy_zone_ct;
    bool tiene_buy_zone_t;

    /* Grafo de waypoints (añadido en 4d — ~12 KB adicionales) */
    WaypointGraph waypoints;
} Map;

/* ─── API pública ────────────────────────────────────────────────────────── */

/*
 * map_cargar — lee ruta JSON, parsea todo el mapa en *map.
 * Retorna 1 OK, 0 fallo (mensaje en stderr).
 * DEBE llamarse desde la raíz del repositorio (rutas relativas).
 */
int  map_cargar(Map *map, const char *ruta);

/*
 * map_liberar — memset a cero; equivale a descargar el mapa.
 */
void map_liberar(Map *map);

#endif /* OPENSTRIKE_MAP_H */
