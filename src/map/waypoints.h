// Copyright (c) 2026 OpenStrike Project
// waypoints.h is part of OpenStrike.
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

#ifndef OPENSTRIKE_WAYPOINTS_H
#define OPENSTRIKE_WAYPOINTS_H

#include "types.h"
#include "renderer/math.h"

/* Límites del grafo — valores suficientes para todos los mapas previstos */
#define WP_MAX_NODES        256   /* máximo de nodos por mapa               */
#define WP_MAX_CONNECTIONS  8     /* máximo de aristas por nodo              */
#define WP_NO_PATH          -1    /* retornado cuando no hay camino/nodo     */

/* Nodo del grafo de navegación */
typedef struct {
    Vec3 pos;                          /* posición en world space (unidades Hammer) */
    i32  conn[WP_MAX_CONNECTIONS];     /* índices de nodos conectados                */
    i32  conn_count;                   /* cuántas conexiones hay en uso              */
} WaypointNode;

/* Grafo completo — almacenado dentro de Map, sin malloc */
typedef struct {
    WaypointNode nodes[WP_MAX_NODES];  /* array fijo de nodos                        */
    i32          count;                /* cuántos nodos están en uso                 */
} WaypointGraph;

/*
 * wp_mas_cercano — devuelve el índice del nodo más cercano a pos.
 * Retorna WP_NO_PATH si el grafo está vacío.
 * Complejidad O(n). Llamar una vez al teleportar/spawnear un bot,
 * NO cada tick.
 */
i32 wp_mas_cercano(const WaypointGraph *g, Vec3 pos);

/*
 * wp_buscar_camino — A* desde el nodo 'desde' hasta el nodo 'hasta'.
 * Escribe los índices del camino en out_path (inicio incluido, fin incluido).
 * Retorna el número de nodos en el camino, 0 si no hay camino.
 * Llamar cuando el bot necesita una nueva ruta, NO cada tick.
 * No es reentrante — el juego es single-threaded, así que es seguro.
 */
i32 wp_buscar_camino(const WaypointGraph *g, i32 desde, i32 hasta,
                     i32 *out_path, i32 max_len);

#endif /* OPENSTRIKE_WAYPOINTS_H */
