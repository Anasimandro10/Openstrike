// Copyright (c) 2026 OpenStrike Project
// waypoints.c is part of OpenStrike.
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

#include "waypoints.h"

#include <float.h>    /* FLT_MAX */
#include <math.h>     /* sqrtf   */
#include <string.h>   /* memset  */

/* ─── Distancia euclidiana entre dos nodos ──────────────────────────────── */

static f32 dist_nodos(const WaypointGraph *g, i32 a, i32 b) {
    f32 dx = g->nodes[b].pos.x - g->nodes[a].pos.x;
    f32 dy = g->nodes[b].pos.y - g->nodes[a].pos.y;
    f32 dz = g->nodes[b].pos.z - g->nodes[a].pos.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* ─── wp_mas_cercano ─────────────────────────────────────────────────────── */

i32 wp_mas_cercano(const WaypointGraph *g, Vec3 pos) {
    if (g->count <= 0) return WP_NO_PATH;

    i32 mejor       = 0;
    f32 mejor_dist2 = FLT_MAX;

    for (i32 i = 0; i < g->count; i++) {
        f32 dx = pos.x - g->nodes[i].pos.x;
        f32 dy = pos.y - g->nodes[i].pos.y;
        f32 dz = pos.z - g->nodes[i].pos.z;
        f32 d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < mejor_dist2) {
            mejor_dist2 = d2;
            mejor       = i;
        }
    }
    return mejor;
}

/* ─── A* ─────────────────────────────────────────────────────────────────── */

/* Estado interno por nodo durante la búsqueda.
   Estático: no es reentrante, pero el juego es single-threaded. */
typedef struct {
    f32  g;            /* coste acumulado desde el inicio   */
    f32  f;            /* g + heurística hasta el destino   */
    i32  parent;       /* índice del nodo padre (-1 = raíz) */
    bool en_abierto;
    bool en_cerrado;
} AStarEstado;

static AStarEstado astar[WP_MAX_NODES];

i32 wp_buscar_camino(const WaypointGraph *g, i32 desde, i32 hasta,
                     i32 *out_path, i32 max_len) {
    if (!g || g->count <= 0 || max_len <= 0) return 0;
    if (desde < 0 || desde >= g->count)       return 0;
    if (hasta < 0 || hasta >= g->count)       return 0;

    /* Caso trivial */
    if (desde == hasta) {
        out_path[0] = desde;
        return 1;
    }

    /* Inicializar estado para todos los nodos en uso */
    for (i32 i = 0; i < g->count; i++) {
        astar[i].g          = FLT_MAX;
        astar[i].f          = FLT_MAX;
        astar[i].parent     = -1;
        astar[i].en_abierto = false;
        astar[i].en_cerrado = false;
    }

    astar[desde].g          = 0.0f;
    astar[desde].f          = dist_nodos(g, desde, hasta);
    astar[desde].en_abierto = true;
    i32 n_abiertos          = 1;

    while (n_abiertos > 0) {
        /* Extraer nodo con menor f del conjunto abierto — O(n), n <= 256 */
        i32 actual = -1;
        f32 min_f  = FLT_MAX;
        for (i32 i = 0; i < g->count; i++) {
            if (astar[i].en_abierto && astar[i].f < min_f) {
                min_f  = astar[i].f;
                actual = i;
            }
        }
        if (actual < 0) break;

        /* ¿Llegamos al destino? */
        if (actual == hasta) {
            /* Reconstruir camino: destino → inicio */
            i32 tmp[WP_MAX_NODES];
            i32 tmp_len = 0;
            i32 cur     = hasta;
            while (cur != -1 && tmp_len < WP_MAX_NODES) {
                tmp[tmp_len++] = cur;
                cur = astar[cur].parent;
            }
            /* Invertir en out_path */
            i32 out_count = tmp_len < max_len ? tmp_len : max_len;
            for (i32 i = 0; i < out_count; i++) {
                out_path[i] = tmp[tmp_len - 1 - i];
            }
            return out_count;
        }

        /* Mover de abierto a cerrado */
        astar[actual].en_abierto  = false;
        astar[actual].en_cerrado  = true;
        n_abiertos--;

        /* Expandir vecinos */
        const WaypointNode *nodo = &g->nodes[actual];
        for (i32 j = 0; j < nodo->conn_count; j++) {
            i32 v = nodo->conn[j];
            if (v < 0 || v >= g->count)  continue;
            if (astar[v].en_cerrado)       continue;

            f32 nuevo_g = astar[actual].g + dist_nodos(g, actual, v);

            if (!astar[v].en_abierto) {
                astar[v].en_abierto = true;
                n_abiertos++;
            } else if (nuevo_g >= astar[v].g) {
                continue; /* este camino no es mejor */
            }

            astar[v].parent = actual;
            astar[v].g      = nuevo_g;
            astar[v].f      = nuevo_g + dist_nodos(g, v, hasta);
        }
    }

    return 0; /* sin camino */
}
