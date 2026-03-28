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

/* Límites de mapa — suficientes para de_dust2 y variantes */
#define MAP_MAX_VERTICES   65536   /* 65536 * 32 bytes = 2 MB */
#define MAP_MAX_FACES       4096
#define MAP_MAX_TEX_NAME      64   /* nombre de textura sin extensión ni ruta */
#define MAP_NOMBRE_MAX        64
#define MAP_FORMAT_VERSION     1

/* Layout de vértice idéntico a Vertex en renderer.c.
   Permite subir Map.vertices directamente a un VBO sin conversión. */
typedef struct {
    f32 pos[3];    /* posición XYZ en unidades Hammer */
    f32 uv[2];     /* coordenadas de textura */
    f32 light[3];  /* color de luz bakeado en vértice (RGB 0.0-1.0) */
} MapVertex;       /* 32 bytes exactos — stride del VBO */

typedef struct {
    u32  vertex_start;               /* índice base en Map.vertices */
    u32  vertex_count;               /* nº de vértices (siempre múltiplo de 3) */
    char textura[MAP_MAX_TEX_NAME];  /* nombre de textura, sin extensión */
} MapFace;

/* Struct principal del mapa.
   IMPORTANTE: no declarar como variable local — ocupa ~2.3 MB.
   Declarar como global estático en main.c:  static Map g_map; */
typedef struct {
    char      nombre[MAP_NOMBRE_MAX];
    i32       version;
    MapVertex vertices[MAP_MAX_VERTICES];
    i32       vertex_count;
    MapFace   faces[MAP_MAX_FACES];
    i32       face_count;
    bool      cargado;
} Map;

/* Carga el mapa desde un archivo JSON.
   Imprime mensajes en stderr si hay error.
   Retorna 1 si OK, 0 si fallo. */
int  map_cargar(Map *map, const char *ruta);

/* Resetea la estructura a cero (equivale a descargar el mapa). */
void map_liberar(Map *map);

#endif /* OPENSTRIKE_MAP_H */
