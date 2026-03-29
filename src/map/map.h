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
#include "renderer/math.h"  /* Vec3 */

/* ── Constantes de geometría ──────────────────────────────────────────── */
#define MAP_MAX_VERTICES    65536   /* 65536 x 32 B = 2 MB               */
#define MAP_MAX_FACES        4096
#define MAP_MAX_TEX_NAME       64   /* longitud máx. nombre de textura    */
#define MAP_NOMBRE_MAX         64   /* longitud máx. nombre del mapa      */
#define MAP_FORMAT_VERSION      1   /* versión actual del formato JSON    */

/* ── Constantes de zonas ──────────────────────────────────────────────── */
#define MAP_MAX_SPAWNS_CT      10   /* máximo de spawns CT por mapa       */
#define MAP_MAX_SPAWNS_T       10   /* máximo de spawns T  por mapa       */

/* ── Tipos de geometría ───────────────────────────────────────────────── */

/* Layout idéntico al Vertex interno de renderer.c — upload directo a VBO */
typedef struct {
    f32 pos[3];     /* XYZ en unidades Hammer  — offset  0, 12 bytes */
    f32 uv[2];      /* coordenadas de textura  — offset 12,  8 bytes */
    f32 light[3];   /* luz bakeada RGB 0-1     — offset 20, 12 bytes */
} MapVertex;        /* total 32 bytes exactos, stride = 32            */

typedef struct {
    u32  vertex_start;              /* índice base en Map.vertices        */
    u32  vertex_count;              /* siempre múltiplo de 3              */
    char textura[MAP_MAX_TEX_NAME]; /* nombre sin extensión ni ruta       */
} MapFace;

/* ── Tipos de zona ────────────────────────────────────────────────────── */

/* Punto de aparición de un jugador */
typedef struct {
    Vec3 pos;   /* posición en unidades Hammer (Y = suelo del jugador)  */
} SpawnPoint;

/* Zona AABB — caja alineada con los ejes (spawns, bomb sites, buy zones) */
typedef struct {
    Vec3 mins;  /* esquina menor (x_min, y_min, z_min)                  */
    Vec3 maxs;  /* esquina mayor (x_max, y_max, z_max)                  */
} ZoneAABB;

/* ── Struct principal ─────────────────────────────────────────────────── */

/* CRÍTICO: declarar siempre como global estático — ocupa ~2.3 MB
   static Map g_map;   (va a BSS, zero-initialized al arrancar)
   Nunca como variable local de función.                                  */
typedef struct {
    /* Metadatos */
    char      nombre[MAP_NOMBRE_MAX];
    i32       version;

    /* Geometría */
    MapVertex vertices[MAP_MAX_VERTICES];
    i32       vertex_count;
    MapFace   faces[MAP_MAX_FACES];
    i32       face_count;

    /* Spawns */
    SpawnPoint spawns_ct[MAP_MAX_SPAWNS_CT];
    i32        spawn_ct_count;
    SpawnPoint spawns_t[MAP_MAX_SPAWNS_T];
    i32        spawn_t_count;

    /* Zonas de juego */
    ZoneAABB   bomb_site_a;
    ZoneAABB   bomb_site_b;
    ZoneAABB   buy_zone_ct;
    ZoneAABB   buy_zone_t;

    /* Flags de presencia (false = zona no definida en el JSON) */
    bool       tiene_bomb_site_a;
    bool       tiene_bomb_site_b;
    bool       tiene_buy_zone_ct;
    bool       tiene_buy_zone_t;

    bool       cargado;
} Map;

/* ── API pública ──────────────────────────────────────────────────────── */

/* Lee el JSON, parsea geometría y zonas, rellena map.
   Retorna 1 OK, 0 fallo (mensaje en stderr).
   DEBE llamarse desde la raíz del repositorio (rutas relativas).        */
int  map_cargar(Map *map, const char *ruta);

/* memset a cero — equivale a descargar el mapa.                         */
void map_liberar(Map *map);

#endif /* OPENSTRIKE_MAP_H */
