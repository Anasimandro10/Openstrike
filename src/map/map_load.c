// Copyright (c) 2026 OpenStrike Project
// map_load.c is part of OpenStrike.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

/* ═══════════════════════════════════════════════════════════════════════════
   Parser JSON mínimo — sin dependencias externas
   ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *s;   /* texto JSON completo          */
    int         i;   /* posición de lectura actual   */
    int         len; /* longitud total               */
    int         err; /* 1 tras el primer error       */
} JP;

/* jp_skip — avanza sobre espacios en blanco */
static void jp_skip(JP *p) {
    while (p->i < p->len) {
        char c = p->s[p->i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            p->i++;
        } else {
            break;
        }
    }
}

/* jp_peek — devuelve el siguiente carácter no-espacio sin consumirlo.
   EFECTO SECUNDARIO: llama jp_skip, modifica p->i.
   Retorna -1 si EOF. */
static int jp_peek(JP *p) {
    jp_skip(p);
    if (p->i >= p->len) return -1;
    return (unsigned char)p->s[p->i];
}

/* jp_eat — salta espacios y consume el carácter c. Marca err si no coincide. */
static void jp_eat(JP *p, char c) {
    jp_skip(p);
    if (p->i >= p->len || p->s[p->i] != c) {
        fprintf(stderr, "map: JSON parse error: esperaba '%c' en pos %d\n", c, p->i);
        p->err = 1;
        return;
    }
    p->i++;
}

/* jp_str — parsea string entre comillas; gestiona \\ de escape */
static void jp_str(JP *p, char *out, int max) {
    jp_eat(p, '"');
    if (p->err) return;
    int j = 0;
    while (!p->err && p->i < p->len && p->s[p->i] != '"') {
        if (p->s[p->i] == '\\') {
            p->i++;
            if (p->i < p->len && j < max - 1) {
                out[j++] = p->s[p->i++];
            }
        } else {
            if (j < max - 1) out[j++] = p->s[p->i];
            p->i++;
        }
    }
    if (j < max) out[j] = '\0';
    jp_eat(p, '"');
}

/* jp_num — parsea número JSON → f32 via atof */
static void jp_num(JP *p, f32 *out) {
    jp_skip(p);
    int start = p->i;
    if (p->i < p->len && (p->s[p->i] == '-' || p->s[p->i] == '+')) p->i++;
    while (p->i < p->len && p->s[p->i] >= '0' && p->s[p->i] <= '9') p->i++;
    if (p->i < p->len && p->s[p->i] == '.') {
        p->i++;
        while (p->i < p->len && p->s[p->i] >= '0' && p->s[p->i] <= '9') p->i++;
    }
    if (p->i < p->len && (p->s[p->i] == 'e' || p->s[p->i] == 'E')) {
        p->i++;
        if (p->i < p->len && (p->s[p->i] == '+' || p->s[p->i] == '-')) p->i++;
        while (p->i < p->len && p->s[p->i] >= '0' && p->s[p->i] <= '9') p->i++;
    }
    int len = p->i - start;
    if (len <= 0 || len >= 64) { p->err = 1; return; }
    char tmp[64];
    memcpy(tmp, p->s + start, (size_t)len);
    tmp[len] = '\0';
    *out = (f32)atof(tmp);
}

/* jp_skip_value — salta cualquier valor JSON manteniendo depth correcto */
static void jp_skip_value(JP *p) {
    int c = jp_peek(p);
    if (c < 0) { p->err = 1; return; }

    if (c == '"') {
        char buf[256];
        jp_str(p, buf, sizeof(buf));
    } else if (c == '{') {
        p->i++;          /* consume '{' */
        int depth = 1;
        while (!p->err && p->i < p->len && depth > 0) {
            int ch = (unsigned char)p->s[p->i++];
            if      (ch == '{') depth++;
            else if (ch == '}') depth--;
            else if (ch == '"') {
                /* saltar string para no confundir llaves dentro de él */
                while (!p->err && p->i < p->len) {
                    int q = (unsigned char)p->s[p->i++];
                    if (q == '\\') { if (p->i < p->len) p->i++; }
                    else if (q == '"') break;
                }
            }
        }
    } else if (c == '[') {
        p->i++;          /* consume '[' */
        int depth = 1;
        while (!p->err && p->i < p->len && depth > 0) {
            int ch = (unsigned char)p->s[p->i++];
            if      (ch == '[') depth++;
            else if (ch == ']') depth--;
            else if (ch == '"') {
                while (!p->err && p->i < p->len) {
                    int q = (unsigned char)p->s[p->i++];
                    if (q == '\\') { if (p->i < p->len) p->i++; }
                    else if (q == '"') break;
                }
            }
        }
    } else {
        /* número, bool, null */
        while (p->i < p->len) {
            int ch = (unsigned char)p->s[p->i];
            if (ch == ',' || ch == '}' || ch == ']' ||
                ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') break;
            p->i++;
        }
    }
}

/* jp_float_arr — parsea [n0, n1, ..., n_{count-1}] → array f32 */
static void jp_float_arr(JP *p, f32 *out, int count) {
    jp_eat(p, '[');
    for (int i = 0; i < count && !p->err; i++) {
        jp_num(p, &out[i]);
        if (i < count - 1) jp_eat(p, ',');
    }
    jp_eat(p, ']');
}

/* jp_prescan_light — opera sobre COPIA del parser; busca "light" en la cara.
   Garantiza que el color está disponible independientemente del orden de campos. */
static void jp_prescan_light(JP src, f32 light[3]) {
    light[0] = light[1] = light[2] = 1.0f;  /* blanco por defecto */
    jp_eat(&src, '{');
    while (!src.err && jp_peek(&src) != '}') {
        char key[32];
        jp_str(&src, key, sizeof(key));
        jp_eat(&src, ':');
        if (strcmp(key, "light") == 0) {
            jp_float_arr(&src, light, 3);
            return;
        }
        jp_skip_value(&src);
        if (jp_peek(&src) == ',') jp_eat(&src, ',');
    }
}

/* jp_parse_vertex — parsea {"pos":[x,y,z],"uv":[u,v]} → MapVertex */
static void jp_parse_vertex(JP *p, MapVertex *v, f32 light[3]) {
    jp_eat(p, '{');
    while (!p->err && jp_peek(p) != '}') {
        char key[32];
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');
        if (strcmp(key, "pos") == 0) {
            jp_float_arr(p, v->pos, 3);
        } else if (strcmp(key, "uv") == 0) {
            jp_float_arr(p, v->uv, 2);
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') jp_eat(p, ',');
    }
    jp_eat(p, '}');
    v->light[0] = light[0];
    v->light[1] = light[1];
    v->light[2] = light[2];
}

/* jp_parse_face — parsea objeto de cara completo y lo añade al mapa */
static void jp_parse_face(JP *p, Map *map) {
    if (map->face_count >= MAP_MAX_FACES) {
        fprintf(stderr, "map: demasiadas caras (max %d)\n", MAP_MAX_FACES);
        jp_skip_value(p);
        return;
    }

    /* Pre-escanear luz desde copia del parser */
    JP   scan = *p;
    f32  light[3];
    jp_prescan_light(scan, light);

    MapFace face;
    memset(&face, 0, sizeof(face));
    face.vertex_start = (u32)map->vertex_count;
    int face_v_count  = 0;

    jp_eat(p, '{');
    while (!p->err && jp_peek(p) != '}') {
        char key[64];
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');

        if (strcmp(key, "textura") == 0) {
            jp_str(p, face.textura, MAP_MAX_TEX_NAME);
        } else if (strcmp(key, "light") == 0) {
            jp_skip_value(p);   /* ya leído por prescan */
        } else if (strcmp(key, "vertices") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                if (map->vertex_count >= MAP_MAX_VERTICES) {
                    fprintf(stderr, "map: demasiados vertices\n");
                    p->err = 1;
                    break;
                }
                jp_parse_vertex(p, &map->vertices[map->vertex_count], light);
                if (!p->err) { map->vertex_count++; face_v_count++; }
                if (jp_peek(p) == ',') jp_eat(p, ',');
            }
            jp_eat(p, ']');
        } else {
            jp_skip_value(p);
        }

        if (jp_peek(p) == ',') jp_eat(p, ',');
    }
    jp_eat(p, '}');

    /* Recortar si no es múltiplo de 3 */
    if (face_v_count % 3 != 0) {
        int extra = face_v_count % 3;
        map->vertex_count -= extra;
        face_v_count      -= extra;
    }
    if (face_v_count == 0) return;

    face.vertex_count        = (u32)face_v_count;
    map->faces[map->face_count++] = face;
}

/* jp_parse_spawn — parsea {"pos":[x,y,z]} → SpawnPoint */
static void jp_parse_spawn(JP *p, SpawnPoint *sp) {
    jp_eat(p, '{');
    while (!p->err && jp_peek(p) != '}') {
        char key[32];
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');
        if (strcmp(key, "pos") == 0) {
            f32 arr[3];
            jp_float_arr(p, arr, 3);
            sp->pos.x = arr[0]; sp->pos.y = arr[1]; sp->pos.z = arr[2];
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') jp_eat(p, ',');
    }
    jp_eat(p, '}');
}

/* jp_parse_zone — parsea {"mins":[…],"maxs":[…]} → ZoneAABB */
static void jp_parse_zone(JP *p, ZoneAABB *z) {
    jp_eat(p, '{');
    while (!p->err && jp_peek(p) != '}') {
        char key[32];
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');
        if (strcmp(key, "mins") == 0) {
            f32 arr[3];
            jp_float_arr(p, arr, 3);
            z->mins.x = arr[0]; z->mins.y = arr[1]; z->mins.z = arr[2];
        } else if (strcmp(key, "maxs") == 0) {
            f32 arr[3];
            jp_float_arr(p, arr, 3);
            z->maxs.x = arr[0]; z->maxs.y = arr[1]; z->maxs.z = arr[2];
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') jp_eat(p, ',');
    }
    jp_eat(p, '}');
}

/* jp_parse_waypoint — parsea {"pos":[x,y,z],"conn":[i0,i1,...]} → WaypointNode
   Añadido en Sistema 4d. */
static void jp_parse_waypoint(JP *p, WaypointGraph *wg) {
    if (wg->count >= WP_MAX_NODES) {
        fprintf(stderr, "map: demasiados waypoints (max %d)\n", WP_MAX_NODES);
        jp_skip_value(p);
        return;
    }

    WaypointNode *node = &wg->nodes[wg->count];
    memset(node, 0, sizeof(*node));

    jp_eat(p, '{');
    while (!p->err && jp_peek(p) != '}') {
        char key[32];
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');

        if (strcmp(key, "pos") == 0) {
            f32 arr[3];
            jp_float_arr(p, arr, 3);
            node->pos.x = arr[0];
            node->pos.y = arr[1];
            node->pos.z = arr[2];
        } else if (strcmp(key, "conn") == 0) {
            jp_eat(p, '[');
            node->conn_count = 0;
            while (!p->err && jp_peek(p) != ']') {
                f32 idx_f;
                jp_num(p, &idx_f);
                if (node->conn_count < WP_MAX_CONNECTIONS) {
                    node->conn[node->conn_count++] = (i32)idx_f;
                }
                if (jp_peek(p) == ',') jp_eat(p, ',');
            }
            jp_eat(p, ']');
        } else {
            jp_skip_value(p);
        }

        if (jp_peek(p) == ',') jp_eat(p, ',');
    }
    jp_eat(p, '}');

    if (!p->err) wg->count++;
}

/* jp_parse_map — parsea el objeto JSON raíz completo */
static void jp_parse_map(JP *p, Map *map) {
    jp_eat(p, '{');
    while (!p->err && jp_peek(p) != '}') {
        char key[64];
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');

        if (strcmp(key, "nombre") == 0) {
            jp_str(p, map->nombre, MAP_NOMBRE_MAX);

        } else if (strcmp(key, "version") == 0) {
            f32 v;
            jp_num(p, &v);
            map->version = (i32)v;

        } else if (strcmp(key, "faces") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                jp_parse_face(p, map);
                if (jp_peek(p) == ',') jp_eat(p, ',');
            }
            jp_eat(p, ']');

        } else if (strcmp(key, "spawns_ct") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                if (map->spawn_ct_count < MAP_MAX_SPAWNS_CT) {
                    jp_parse_spawn(p, &map->spawns_ct[map->spawn_ct_count++]);
                } else {
                    jp_skip_value(p);
                }
                if (jp_peek(p) == ',') jp_eat(p, ',');
            }
            jp_eat(p, ']');

        } else if (strcmp(key, "spawns_t") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                if (map->spawn_t_count < MAP_MAX_SPAWNS_T) {
                    jp_parse_spawn(p, &map->spawns_t[map->spawn_t_count++]);
                } else {
                    jp_skip_value(p);
                }
                if (jp_peek(p) == ',') jp_eat(p, ',');
            }
            jp_eat(p, ']');

        } else if (strcmp(key, "bomb_site_a") == 0) {
            jp_parse_zone(p, &map->bomb_site_a);
            map->tiene_bomb_site_a = true;

        } else if (strcmp(key, "bomb_site_b") == 0) {
            jp_parse_zone(p, &map->bomb_site_b);
            map->tiene_bomb_site_b = true;

        } else if (strcmp(key, "buy_zone_ct") == 0) {
            jp_parse_zone(p, &map->buy_zone_ct);
            map->tiene_buy_zone_ct = true;

        } else if (strcmp(key, "buy_zone_t") == 0) {
            jp_parse_zone(p, &map->buy_zone_t);
            map->tiene_buy_zone_t = true;

        } else if (strcmp(key, "waypoints") == 0) {
            /* Sistema 4d */
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                jp_parse_waypoint(p, &map->waypoints);
                if (jp_peek(p) == ',') jp_eat(p, ',');
            }
            jp_eat(p, ']');

        } else {
            jp_skip_value(p);   /* campo desconocido — compatibilidad futura */
        }

        if (jp_peek(p) == ',') jp_eat(p, ',');
    }
    jp_eat(p, '}');
}

/* ═══════════════════════════════════════════════════════════════════════════
   API pública
   ═══════════════════════════════════════════════════════════════════════════ */

int map_cargar(Map *map, const char *ruta) {
    memset(map, 0, sizeof(*map));

    FILE *f = fopen(ruta, "rb");
    if (!f) {
        fprintf(stderr, "map: no se pudo abrir %s\n", ruta);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)tam + 1);
    if (!buf) {
        fprintf(stderr, "map: OOM al cargar %s\n", ruta);
        fclose(f);
        return 0;
    }

    fread(buf, 1, (size_t)tam, f);
    buf[tam] = '\0';
    fclose(f);

    JP p = { buf, 0, (int)tam, 0 };
    jp_parse_map(&p, map);
    free(buf);

    if (p.err) {
        fprintf(stderr, "map: error al parsear %s\n", ruta);
        memset(map, 0, sizeof(*map));
        return 0;
    }

    map->cargado = true;
    printf("map: '%s' — %d verts, %d caras, %d waypoints\n",
           map->nombre, map->vertex_count, map->face_count,
           map->waypoints.count);
    return 1;
}

void map_liberar(Map *map) {
    memset(map, 0, sizeof(*map));
}
