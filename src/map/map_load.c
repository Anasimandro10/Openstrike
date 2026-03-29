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

/* ═══════════════════════════════════════════════════════════════════════
   Parser JSON minimo — sin dependencias externas (~270 lineas)
   ═══════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *s;   /* texto JSON completo                              */
    int         i;   /* posicion de lectura actual                       */
    int         len; /* longitud total del texto                         */
    int         err; /* 1 tras el primer error; impide mas parsing       */
} JP;

/* ── Utilidades basicas ──────────────────────────────────────────────── */

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

/* Efecto secundario: llama jp_skip modificando p->i */
static int jp_peek(JP *p) {
    jp_skip(p);
    if (p->i >= p->len) return -1;
    return (unsigned char)p->s[p->i];
}

static void jp_eat(JP *p, char c) {
    if (p->err) return;
    jp_skip(p);
    if (p->i >= p->len || p->s[p->i] != c) {
        fprintf(stderr, "JSON: esperaba '%c', encontre '%c' en pos %d\n",
                c, (p->i < p->len ? p->s[p->i] : '?'), p->i);
        p->err = 1;
        return;
    }
    p->i++;
}

static void jp_str(JP *p, char *out, int max) {
    if (p->err) return;
    jp_eat(p, '"');
    if (p->err) return;
    int n = 0;
    while (p->i < p->len && p->s[p->i] != '"') {
        if (p->s[p->i] == '\\') {
            p->i++;
            if (p->i < p->len) {
                if (n < max - 1) out[n++] = p->s[p->i];
                p->i++;
            }
        } else {
            if (n < max - 1) out[n++] = p->s[p->i];
            p->i++;
        }
    }
    out[n] = '\0';
    jp_eat(p, '"');
}

static void jp_num(JP *p, f32 *out) {
    if (p->err) return;
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
    if (p->i == start) { p->err = 1; return; }
    char buf[64];
    int  blen = p->i - start;
    if (blen >= 64) blen = 63;
    memcpy(buf, p->s + start, (size_t)blen);
    buf[blen] = '\0';
    *out = (f32)atof(buf);
}

/* Salta cualquier valor JSON — mantiene depth correcto para anidados     */
static void jp_skip_value(JP *p) {
    if (p->err) return;
    jp_skip(p);
    if (p->i >= p->len) { p->err = 1; return; }
    char c = p->s[p->i];
    if (c == '"') {
        p->i++;
        while (p->i < p->len) {
            if (p->s[p->i] == '\\') { p->i += 2; continue; }
            if (p->s[p->i] == '"') { p->i++; return; }
            p->i++;
        }
        p->err = 1;
    } else if (c == '{' || c == '[') {
        char open  = c;
        char close = (c == '{') ? '}' : ']';
        int  depth = 1;
        p->i++;
        while (p->i < p->len && depth > 0) {
            char ch = p->s[p->i];
            if (ch == '"') {
                p->i++;
                while (p->i < p->len) {
                    if (p->s[p->i] == '\\') { p->i += 2; continue; }
                    if (p->s[p->i] == '"') { p->i++; break; }
                    p->i++;
                }
                continue;
            }
            if (ch == open)  depth++;
            else if (ch == close) depth--;
            p->i++;
        }
        if (depth != 0) p->err = 1;
    } else {
        /* numero, true, false, null */
        while (p->i < p->len) {
            char ch = p->s[p->i];
            if (ch == ',' || ch == '}' || ch == ']' ||
                ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') break;
            p->i++;
        }
    }
}

/* Parsea [n0, n1, ..., n_{count-1}] → array de f32                      */
static void jp_float_arr(JP *p, f32 *out, int count) {
    if (p->err) return;
    jp_eat(p, '[');
    for (int i = 0; i < count; i++) {
        jp_num(p, &out[i]);
        if (i < count - 1) jp_eat(p, ',');
    }
    jp_eat(p, ']');
}

/* ── Parseo de vertice y cara ────────────────────────────────────────── */

/* Pre-escaneo: opera sobre COPIA de src para encontrar "light" sin
   consumir el parser original. Garantiza que el color esta disponible
   antes de parsear vertices, sin importar el orden de campos en el JSON.  */
static void jp_prescan_light(const JP *src, f32 light[3]) {
    light[0] = light[1] = light[2] = 1.0f;  /* defecto: blanco          */
    JP p = *src;
    jp_eat(&p, '{');
    while (!p.err) {
        if (jp_peek(&p) == '}') break;
        char key[64] = {0};
        jp_str(&p, key, sizeof(key));
        jp_eat(&p, ':');
        if (strcmp(key, "light") == 0) {
            jp_float_arr(&p, light, 3);
            return;
        }
        jp_skip_value(&p);
        if (jp_peek(&p) == ',') { p.i++; }
    }
}

static void jp_parse_vertex(JP *p, MapVertex *v, const f32 light[3]) {
    if (p->err) return;
    jp_eat(p, '{');
    while (!p->err) {
        if (jp_peek(p) == '}') break;
        char key[64] = {0};
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');
        if (strcmp(key, "pos") == 0) {
            jp_float_arr(p, v->pos, 3);
        } else if (strcmp(key, "uv") == 0) {
            jp_float_arr(p, v->uv, 2);
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') { p->i++; }
    }
    jp_eat(p, '}');
    v->light[0] = light[0];
    v->light[1] = light[1];
    v->light[2] = light[2];
}

static void jp_parse_face(JP *p, Map *map) {
    if (p->err) return;

    f32 light[3];
    jp_prescan_light(p, light);

    char textura[MAP_MAX_TEX_NAME] = {0};
    int  face_v_start = map->vertex_count;
    int  face_v_count = 0;

    jp_eat(p, '{');
    while (!p->err) {
        if (jp_peek(p) == '}') break;
        char key[64] = {0};
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');

        if (strcmp(key, "textura") == 0) {
            jp_str(p, textura, MAP_MAX_TEX_NAME);
        } else if (strcmp(key, "light") == 0) {
            jp_skip_value(p);  /* ya leido por prescan */
        } else if (strcmp(key, "vertices") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                if (map->vertex_count < MAP_MAX_VERTICES) {
                    jp_parse_vertex(p, &map->vertices[map->vertex_count], light);
                    map->vertex_count++;
                    face_v_count++;
                } else {
                    jp_skip_value(p);
                }
                if (jp_peek(p) == ',') { p->i++; }
            }
            jp_eat(p, ']');
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') { p->i++; }
    }
    jp_eat(p, '}');

    /* Recortar si no es multiplo de 3 */
    if (face_v_count % 3 != 0) {
        int trim = face_v_count % 3;
        map->vertex_count -= trim;
        face_v_count      -= trim;
    }

    if (face_v_count == 0) return;  /* cara vacia — no es error */

    if (map->face_count < MAP_MAX_FACES) {
        MapFace *face       = &map->faces[map->face_count];
        face->vertex_start  = (u32)face_v_start;
        face->vertex_count  = (u32)face_v_count;
        strncpy(face->textura, textura, MAP_MAX_TEX_NAME - 1);
        face->textura[MAP_MAX_TEX_NAME - 1] = '\0';
        map->face_count++;
    }
}

/* ── Parseo de zonas (4c) ────────────────────────────────────────────── */

/* Parsea {"pos": [x, y, z]} → SpawnPoint                                */
static void jp_parse_spawn(JP *p, SpawnPoint *sp) {
    if (p->err) return;
    f32 arr[3] = {0};
    jp_eat(p, '{');
    while (!p->err) {
        if (jp_peek(p) == '}') break;
        char key[64] = {0};
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');
        if (strcmp(key, "pos") == 0) {
            jp_float_arr(p, arr, 3);
            sp->pos.x = arr[0];
            sp->pos.y = arr[1];
            sp->pos.z = arr[2];
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') { p->i++; }
    }
    jp_eat(p, '}');
}

/* Parsea {"mins": [x,y,z], "maxs": [x,y,z]} → ZoneAABB                 */
static void jp_parse_zone(JP *p, ZoneAABB *z) {
    if (p->err) return;
    f32 arr[3] = {0};
    jp_eat(p, '{');
    while (!p->err) {
        if (jp_peek(p) == '}') break;
        char key[64] = {0};
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');
        if (strcmp(key, "mins") == 0) {
            jp_float_arr(p, arr, 3);
            z->mins.x = arr[0]; z->mins.y = arr[1]; z->mins.z = arr[2];
        } else if (strcmp(key, "maxs") == 0) {
            jp_float_arr(p, arr, 3);
            z->maxs.x = arr[0]; z->maxs.y = arr[1]; z->maxs.z = arr[2];
        } else {
            jp_skip_value(p);
        }
        if (jp_peek(p) == ',') { p->i++; }
    }
    jp_eat(p, '}');
}

/* ── Parseo raiz ─────────────────────────────────────────────────────── */

static void jp_parse_map(JP *p, Map *map) {
    if (p->err) return;
    jp_eat(p, '{');
    while (!p->err) {
        if (jp_peek(p) == '}') break;
        char key[64] = {0};
        jp_str(p, key, sizeof(key));
        jp_eat(p, ':');

        if (strcmp(key, "nombre") == 0) {
            jp_str(p, map->nombre, MAP_NOMBRE_MAX);

        } else if (strcmp(key, "version") == 0) {
            f32 v = 0.0f;
            jp_num(p, &v);
            map->version = (i32)v;

        } else if (strcmp(key, "faces") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                jp_parse_face(p, map);
                if (jp_peek(p) == ',') { p->i++; }
            }
            jp_eat(p, ']');

        } else if (strcmp(key, "spawns_ct") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                if (map->spawn_ct_count < MAP_MAX_SPAWNS_CT) {
                    jp_parse_spawn(p, &map->spawns_ct[map->spawn_ct_count]);
                    map->spawn_ct_count++;
                } else {
                    jp_skip_value(p);
                }
                if (jp_peek(p) == ',') { p->i++; }
            }
            jp_eat(p, ']');

        } else if (strcmp(key, "spawns_t") == 0) {
            jp_eat(p, '[');
            while (!p->err && jp_peek(p) != ']') {
                if (map->spawn_t_count < MAP_MAX_SPAWNS_T) {
                    jp_parse_spawn(p, &map->spawns_t[map->spawn_t_count]);
                    map->spawn_t_count++;
                } else {
                    jp_skip_value(p);
                }
                if (jp_peek(p) == ',') { p->i++; }
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

        } else {
            jp_skip_value(p);  /* campo desconocido — compatibilidad futura */
        }

        if (jp_peek(p) == ',') { p->i++; }
    }
    jp_eat(p, '}');
}

/* ═══════════════════════════════════════════════════════════════════════
   API publica
   ═══════════════════════════════════════════════════════════════════════ */

int map_cargar(Map *map, const char *ruta) {
    memset(map, 0, sizeof(*map));

    FILE *f = fopen(ruta, "rb");
    if (!f) {
        fprintf(stderr, "map_cargar: no se pudo abrir '%s'\n", ruta);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t tam = (size_t)ftell(f);
    rewind(f);

    char *buf = (char *)malloc(tam + 1);
    if (!buf) {
        fprintf(stderr, "map_cargar: OOM (%zu bytes)\n", tam + 1);
        fclose(f);
        return 0;
    }

    fread(buf, 1, tam, f);
    buf[tam] = '\0';
    fclose(f);

    JP p = { buf, 0, (int)tam, 0 };
    jp_parse_map(&p, map);
    free(buf);

    if (p.err) {
        fprintf(stderr, "map_cargar: error parseando '%s'\n", ruta);
        memset(map, 0, sizeof(*map));
        return 0;
    }

    map->cargado = true;
    fprintf(stdout,
            "Mapa '%s' cargado: %d vertices, %d caras | "
            "CT spawns: %d, T spawns: %d | sites: %s%s | "
            "buy: CT=%s T=%s\n",
            map->nombre, map->vertex_count, map->face_count,
            map->spawn_ct_count, map->spawn_t_count,
            map->tiene_bomb_site_a ? "A" : "-",
            map->tiene_bomb_site_b ? "B" : "-",
            map->tiene_buy_zone_ct ? "si" : "no",
            map->tiene_buy_zone_t  ? "si" : "no");
    return 1;
}

void map_liberar(Map *map) {
    memset(map, 0, sizeof(*map));
}
