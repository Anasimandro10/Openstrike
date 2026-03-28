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
   Parser JSON mínimo — diseñado solo para el formato de mapas de OpenStrike.
   Campos desconocidos se saltan sin error (compatibilidad futura con 4c/4d).
   ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *s;    /* fuente completa */
    int         i;    /* posición actual */
    int         len;  /* longitud total */
    int         err;  /* 1 tras primer error */
} JP;

/* Salta espacios, tabs y saltos de línea */
static void jp_skip(JP *p) {
    while (p->i < p->len &&
           (p->s[p->i] == ' '  || p->s[p->i] == '\t' ||
            p->s[p->i] == '\n' || p->s[p->i] == '\r'))
        p->i++;
}

/* Devuelve el siguiente carácter no-espacio sin consumirlo, -1 si EOF */
static int jp_peek(JP *p) {
    jp_skip(p);
    return (p->i < p->len) ? (unsigned char)p->s[p->i] : -1;
}

/* Consume el carácter esperado; marca error si no coincide */
static int jp_eat(JP *p, char c) {
    jp_skip(p);
    if (p->i >= p->len || p->s[p->i] != c) {
        if (!p->err) {
            fprintf(stderr,
                    "map_load: esperaba '%c' en pos %d (encontre '%c')\n",
                    c, p->i,
                    (p->i < p->len) ? p->s[p->i] : '?');
        }
        p->err = 1;
        return 0;
    }
    p->i++;
    return 1;
}

/* Parsea un string JSON entre comillas → out (max incluye el null terminator) */
static int jp_str(JP *p, char *out, int max) {
    jp_skip(p);
    if (!jp_eat(p, '"')) return 0;
    int j = 0;
    while (p->i < p->len && p->s[p->i] != '"') {
        if (p->s[p->i] == '\\') {
            p->i++;
            if (p->i >= p->len) break;
        }
        if (j < max - 1) out[j++] = p->s[p->i];
        p->i++;
    }
    out[j] = '\0';
    return jp_eat(p, '"');
}

/* Parsea un número JSON (entero o decimal, con signo) → f32 */
static int jp_num(JP *p, f32 *out) {
    jp_skip(p);
    if (p->i >= p->len) { p->err = 1; return 0; }

    char buf[64];
    int  j = 0;

    if (p->s[p->i] == '-') buf[j++] = p->s[p->i++];

    while (p->i < p->len && p->s[p->i] >= '0' && p->s[p->i] <= '9')
        buf[j++] = p->s[p->i++];

    if (p->i < p->len && p->s[p->i] == '.') {
        buf[j++] = p->s[p->i++];
        while (p->i < p->len && p->s[p->i] >= '0' && p->s[p->i] <= '9')
            buf[j++] = p->s[p->i++];
    }

    if (p->i < p->len && (p->s[p->i] == 'e' || p->s[p->i] == 'E')) {
        buf[j++] = p->s[p->i++];
        if (p->i < p->len && (p->s[p->i] == '+' || p->s[p->i] == '-'))
            buf[j++] = p->s[p->i++];
        while (p->i < p->len && p->s[p->i] >= '0' && p->s[p->i] <= '9')
            buf[j++] = p->s[p->i++];
    }

    if (j == 0) { p->err = 1; return 0; }
    buf[j] = '\0';
    *out = (f32)atof(buf);
    return 1;
}

/* Salta cualquier valor JSON sin interpretarlo.
   Soporta: strings, objetos {}, arrays [], números, booleans, null. */
static void jp_skip_value(JP *p) {
    jp_skip(p);
    if (p->i >= p->len) return;
    char c = p->s[p->i];

    if (c == '"') {
        char dummy[256];
        jp_str(p, dummy, sizeof(dummy));
        return;
    }

    if (c == '{' || c == '[') {
        char open = c, close = (c == '{') ? '}' : ']';
        p->i++;
        int depth = 1;
        while (p->i < p->len && depth > 0) {
            char ch = p->s[p->i++];
            if      (ch == open)  depth++;
            else if (ch == close) depth--;
            else if (ch == '"') {
                /* salta string literal para ignorar llaves dentro de strings */
                while (p->i < p->len && p->s[p->i] != '"') {
                    if (p->s[p->i] == '\\') p->i++;
                    p->i++;
                }
                if (p->i < p->len) p->i++;
            }
        }
        return;
    }

    /* número, bool o null: avanza hasta separador */
    while (p->i < p->len) {
        char ch = p->s[p->i];
        if (ch == ',' || ch == '}' || ch == ']' ||
            ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') break;
        p->i++;
    }
}

/* Parsea [n0, n1, ..., n_{count-1}] → array de f32 */
static int jp_float_arr(JP *p, f32 *out, int count) {
    if (!jp_eat(p, '[')) return 0;
    for (int k = 0; k < count && !p->err; k++) {
        if (!jp_num(p, &out[k])) return 0;
        jp_skip(p);
        if (k < count - 1 && !jp_eat(p, ',')) return 0;
    }
    jp_skip(p);
    return jp_eat(p, ']');
}

/* Pre-escaneo de un objeto de cara para extraer "light" independientemente
   del orden de campos. Opera sobre una COPIA del parser — no toca p.
   Si no encuentra "light", deja light = {1, 1, 1}. */
static void jp_prescan_light(const JP *src, f32 light[3]) {
    JP   scan = *src;
    char key[32];

    light[0] = light[1] = light[2] = 1.0f;

    jp_eat(&scan, '{');
    while (!scan.err) {
        jp_skip(&scan);
        if (jp_peek(&scan) == (int)'}') break;
        if (!jp_str(&scan, key, sizeof(key))) break;
        if (!jp_eat(&scan, ':'))             break;
        if (strcmp(key, "light") == 0) {
            jp_float_arr(&scan, light, 3);
            return;
        }
        jp_skip_value(&scan);
        jp_skip(&scan);
        if (jp_peek(&scan) == (int)',') scan.i++;
    }
}

/* Parsea {"pos":[x,y,z], "uv":[u,v]} → MapVertex.
   light ya se conoce del pre-escaneo de la cara. */
static int jp_parse_vertex(JP *p, MapVertex *v, const f32 light[3]) {
    if (!jp_eat(p, '{')) return 0;

    v->light[0] = light[0];
    v->light[1] = light[1];
    v->light[2] = light[2];

    int  has_pos = 0, has_uv = 0;
    char key[32];

    while (!p->err) {
        jp_skip(p);
        if (jp_peek(p) == (int)'}') break;
        if (!jp_str(p, key, sizeof(key))) return 0;
        if (!jp_eat(p, ':'))             return 0;

        if (strcmp(key, "pos") == 0) {
            if (!jp_float_arr(p, v->pos, 3)) return 0;
            has_pos = 1;
        } else if (strcmp(key, "uv") == 0) {
            if (!jp_float_arr(p, v->uv, 2)) return 0;
            has_uv = 1;
        } else {
            jp_skip_value(p);
        }

        jp_skip(p);
        if (jp_peek(p) == (int)',') p->i++;
    }

    if (!jp_eat(p, '}')) return 0;

    if (!has_pos || !has_uv) {
        fprintf(stderr, "map_load: vertice sin 'pos' o 'uv'\n");
        p->err = 1;
        return 0;
    }
    return 1;
}

/* Parsea un objeto de cara completo y añade vértices + MapFace a map */
static int jp_parse_face(JP *p, Map *map) {
    /* pre-escaneo para color de luz (independiente del orden de campos) */
    f32 light[3];
    jp_prescan_light(p, light);

    if (!jp_eat(p, '{')) return 0;

    char textura[MAP_MAX_TEX_NAME];
    strncpy(textura, "sin_textura", sizeof(textura) - 1);
    textura[sizeof(textura) - 1] = '\0';

    int  face_v_start = map->vertex_count;
    int  face_v_count = 0;
    char key[32];

    while (!p->err) {
        jp_skip(p);
        if (jp_peek(p) == (int)'}') break;
        if (!jp_str(p, key, sizeof(key))) return 0;
        if (!jp_eat(p, ':'))             return 0;

        if (strcmp(key, "textura") == 0) {
            jp_str(p, textura, MAP_MAX_TEX_NAME);

        } else if (strcmp(key, "light") == 0) {
            /* ya leído en pre-escaneo — saltar sin procesar */
            jp_skip_value(p);

        } else if (strcmp(key, "vertices") == 0) {
            if (!jp_eat(p, '[')) return 0;
            while (!p->err) {
                jp_skip(p);
                if (jp_peek(p) == (int)']') break;

                if (map->vertex_count >= MAP_MAX_VERTICES) {
                    fprintf(stderr,
                            "map_load: limite de vertices alcanzado (%d)\n",
                            MAP_MAX_VERTICES);
                    p->err = 1;
                    return 0;
                }

                MapVertex *v = &map->vertices[map->vertex_count];
                if (!jp_parse_vertex(p, v, light)) return 0;
                map->vertex_count++;
                face_v_count++;

                jp_skip(p);
                if (jp_peek(p) == (int)',') p->i++;
            }
            if (!jp_eat(p, ']')) return 0;

        } else {
            /* campo desconocido (spawns, bomb sites, waypoints...) — ignorar */
            jp_skip_value(p);
        }

        jp_skip(p);
        if (jp_peek(p) == (int)',') p->i++;
    }

    if (!jp_eat(p, '}')) return 0;

    /* recortar si el número de vértices no es múltiplo de 3 */
    if (face_v_count % 3 != 0) {
        fprintf(stderr,
                "map_load: cara con %d vertices (no multiplo de 3), recortando\n",
                face_v_count);
        int excess     = face_v_count % 3;
        map->vertex_count -= excess;
        face_v_count      -= excess;
    }

    if (face_v_count == 0) return 1;  /* cara vacía — ignorar sin error */

    if (map->face_count >= MAP_MAX_FACES) {
        fprintf(stderr, "map_load: limite de caras alcanzado (%d)\n",
                MAP_MAX_FACES);
        p->err = 1;
        return 0;
    }

    MapFace *face       = &map->faces[map->face_count];
    face->vertex_start  = (u32)face_v_start;
    face->vertex_count  = (u32)face_v_count;
    strncpy(face->textura, textura, MAP_MAX_TEX_NAME - 1);
    face->textura[MAP_MAX_TEX_NAME - 1] = '\0';
    map->face_count++;

    return 1;
}

/* Parsea el objeto JSON raíz del mapa */
static int jp_parse_map(JP *p, Map *map) {
    if (!jp_eat(p, '{')) return 0;

    char key[32];

    while (!p->err) {
        jp_skip(p);
        if (jp_peek(p) == (int)'}') break;
        if (!jp_str(p, key, sizeof(key))) return 0;
        if (!jp_eat(p, ':'))             return 0;

        if (strcmp(key, "nombre") == 0) {
            jp_str(p, map->nombre, MAP_NOMBRE_MAX);

        } else if (strcmp(key, "version") == 0) {
            f32 v = 0.0f;
            jp_num(p, &v);
            map->version = (i32)v;

        } else if (strcmp(key, "faces") == 0) {
            if (!jp_eat(p, '[')) return 0;
            while (!p->err) {
                jp_skip(p);
                if (jp_peek(p) == (int)']') break;
                if (!jp_parse_face(p, map)) return 0;
                jp_skip(p);
                if (jp_peek(p) == (int)',') p->i++;
            }
            if (!jp_eat(p, ']')) return 0;

        } else {
            jp_skip_value(p);
        }

        jp_skip(p);
        if (jp_peek(p) == (int)',') p->i++;
    }

    if (!jp_eat(p, '}')) return 0;
    return !p->err;
}

/* ═══════════════════════════════════════════════════════════════════════════
   API pública
   ═══════════════════════════════════════════════════════════════════════════ */

int map_cargar(Map *map, const char *ruta) {
    memset(map, 0, sizeof(*map));

    FILE *f = fopen(ruta, "rb");
    if (!f) {
        fprintf(stderr, "map_load: no se pudo abrir '%s'\n", ruta);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);

    if (tam <= 0) {
        fprintf(stderr, "map_load: archivo vacio o error leyendo '%s'\n", ruta);
        fclose(f);
        return 0;
    }

    char *buf = (char *)malloc((size_t)tam + 1);
    if (!buf) {
        fprintf(stderr, "map_load: sin memoria para '%s'\n", ruta);
        fclose(f);
        return 0;
    }

    fread(buf, 1, (size_t)tam, f);
    buf[tam] = '\0';
    fclose(f);

    JP  p  = { buf, 0, (int)tam, 0 };
    int ok = jp_parse_map(&p, map);
    free(buf);

    if (!ok || p.err) {
        fprintf(stderr, "map_load: error al parsear '%s'\n", ruta);
        memset(map, 0, sizeof(*map));
        return 0;
    }

    map->cargado = true;
    fprintf(stdout,
            "map_load: '%s' cargado — %d vertices, %d caras\n",
            map->nombre, map->vertex_count, map->face_count);
    return 1;
}

void map_liberar(Map *map) {
    memset(map, 0, sizeof(*map));
}
