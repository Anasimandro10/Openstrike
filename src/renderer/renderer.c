// Copyright (c) 2026 OpenStrike Project
// renderer.c is part of OpenStrike.
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>   /* offsetof */

#include "types.h"
#include "renderer/gl.h"
#include "renderer/math.h"
#include "renderer/renderer.h"

/* MapVertex (de map/map.h via renderer.h) tiene layout identico al
   antiguo Vertex interno:
     pos[3]   offset  0  (12 bytes)  -> a_position
     uv[2]    offset 12  ( 8 bytes)  -> a_texcoord
     light[3] offset 20  (12 bytes)  -> a_lightcolor
   Total 32 bytes, stride 32.
   Los datos de Map.vertices se pasan directamente a gl_BufferData. */

/* ── Estado estatico del renderer ─────────────────────────────────── */
static GLuint g_shader      = 0;
static GLuint g_vbo         = 0;
static GLuint g_texture     = 0;
static int    g_vert_count  = 0;
static GLint  g_loc_pos     = -1;
static GLint  g_loc_uv      = -1;
static GLint  g_loc_light   = -1;
static GLint  g_loc_view    = -1;
static GLint  g_loc_proj    = -1;
static GLint  g_loc_diffuse = -1;
static Vec3   g_cam_pos     = {0.0f, 64.0f, 0.0f};
static f32    g_cam_yaw     = 0.0f;
static f32    g_cam_pitch   = 0.0f;
static Mat4   g_proj;

/* ── Utilidades internas ───────────────────────────────────────────── */

static char *leer_archivo(const char *ruta, size_t *out_tam) {
    FILE *f = fopen(ruta, "rb");
    if (!f) {
        fprintf(stderr, "leer_archivo: no se pudo abrir '%s'\n", ruta);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t tam = (size_t)ftell(f);
    rewind(f);
    char *buf = malloc(tam + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, tam, f);
    buf[tam] = '\0';
    fclose(f);
    if (out_tam) { *out_tam = tam; }
    return buf;
}

static GLuint compilar_shader(const char *src, GLenum tipo) {
    GLuint shader = gl_CreateShader(tipo);
    if (!shader) { return 0; }
    gl_ShaderSource(shader, 1, &src, NULL);
    gl_CompileShader(shader);
    GLint status;
    gl_GetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        GLsizei log_len;
        gl_GetShaderInfoLog(shader, (GLsizei)sizeof(log), &log_len, log);
        fprintf(stderr, "Shader error: %.*s\n", (int)log_len, log);
        gl_DeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint crear_programa(const char *vert_src, const char *frag_src) {
    GLuint vs = compilar_shader(vert_src, GL_VERTEX_SHADER);
    GLuint fs = compilar_shader(frag_src, GL_FRAGMENT_SHADER);
    if (!vs || !fs) {
        if (vs) { gl_DeleteShader(vs); }
        if (fs) { gl_DeleteShader(fs); }
        return 0;
    }
    GLuint prog = gl_CreateProgram();
    gl_AttachShader(prog, vs);
    gl_AttachShader(prog, fs);
    gl_LinkProgram(prog);
    gl_DeleteShader(vs);
    gl_DeleteShader(fs);
    GLint status;
    gl_GetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        GLsizei log_len;
        gl_GetProgramInfoLog(prog, (GLsizei)sizeof(log), &log_len, log);
        fprintf(stderr, "Program link error: %.*s\n", (int)log_len, log);
        gl_DeleteProgram(prog);
        return 0;
    }
    return prog;
}

/* Textura checkerboard procedural 64x64 RGBA (placeholder hasta Sistema 5). */
static GLuint crear_textura_checkerboard(void) {
    unsigned char pixeles[64 * 64 * 4];
    int x, y;
    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            int idx   = (y * 64 + x) * 4;
            int celda = ((x / 8) + (y / 8)) % 2;
            if (celda == 0) {
                pixeles[idx + 0] = 210; pixeles[idx + 1] = 200;
                pixeles[idx + 2] = 179; pixeles[idx + 3] = 255;
            } else {
                pixeles[idx + 0] = 80;  pixeles[idx + 1] = 76;
                pixeles[idx + 2] = 68;  pixeles[idx + 3] = 255;
            }
        }
    }
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixeles);
    return tex_id;
}

/* ── API publica ───────────────────────────────────────────────────── */

int renderer_init(void) {
    if (!gl_cargar()) {
        fprintf(stderr, "renderer_init: gl_cargar fallo\n");
        return 0;
    }

    char *vert_src = leer_archivo("assets/shaders/world.vert", NULL);
    char *frag_src = leer_archivo("assets/shaders/world.frag", NULL);
    if (!vert_src || !frag_src) {
        fprintf(stderr, "renderer_init: no se pudieron leer los shaders\n");
        free(vert_src);
        free(frag_src);
        return 0;
    }

    g_shader = crear_programa(vert_src, frag_src);
    free(vert_src);
    free(frag_src);
    if (!g_shader) {
        fprintf(stderr, "renderer_init: fallo crear_programa\n");
        return 0;
    }

    g_loc_pos     = gl_GetAttribLocation(g_shader,  "a_position");
    g_loc_uv      = gl_GetAttribLocation(g_shader,  "a_texcoord");
    g_loc_light   = gl_GetAttribLocation(g_shader,  "a_lightcolor");
    g_loc_view    = gl_GetUniformLocation(g_shader, "u_view");
    g_loc_proj    = gl_GetUniformLocation(g_shader, "u_proj");
    g_loc_diffuse = gl_GetUniformLocation(g_shader, "u_diffuse");

    g_texture = crear_textura_checkerboard();
    if (!g_texture) {
        fprintf(stderr, "renderer_init: fallo crear_textura_checkerboard\n");
        return 0;
    }

    /* Proyeccion fija: FOV 90, 1280/720, near=4, far=4096 */
    g_proj = mat4_perspective(90.0f, 1280.0f / 720.0f, 4.0f, 4096.0f);

    /* El VBO se crea en renderer_cargar_mapa(), no aqui */
    return 1;
}

int renderer_cargar_mapa(const Map *map) {
    if (!map || !map->cargado || map->vertex_count == 0) {
        fprintf(stderr, "renderer_cargar_mapa: mapa invalido o sin vertices\n");
        return 0;
    }

    /* Crear VBO si todavia no existe */
    if (g_vbo == 0) {
        gl_GenBuffers(1, &g_vbo);
        if (g_vbo == 0) {
            fprintf(stderr, "renderer_cargar_mapa: gl_GenBuffers fallo\n");
            return 0;
        }
    }

    /* MapVertex tiene el mismo layout de 32 bytes que el antiguo Vertex
       interno — se puede subir directamente sin conversion. */
    gl_BindBuffer(GL_ARRAY_BUFFER, g_vbo);
    gl_BufferData(GL_ARRAY_BUFFER,
                  (GLsizeiptr)((size_t)map->vertex_count * sizeof(MapVertex)),
                  map->vertices,
                  GL_STATIC_DRAW);
    gl_BindBuffer(GL_ARRAY_BUFFER, 0);

    g_vert_count = map->vertex_count;
    printf("renderer_cargar_mapa: %d vertices cargados al VBO\n", g_vert_count);
    return 1;
}

void renderer_shutdown(void) {
    if (g_vbo)     { gl_DeleteBuffers(1, &g_vbo);     g_vbo     = 0; }
    if (g_shader)  { gl_DeleteProgram(g_shader);      g_shader  = 0; }
    if (g_texture) { glDeleteTextures(1, &g_texture); g_texture = 0; }
    g_vert_count = 0;
}

void renderer_set_camera(float x, float y, float z, float yaw, float pitch) {
    g_cam_pos   = (Vec3){ x, y, z };
    g_cam_yaw   = yaw;
    g_cam_pitch = pitch;
}

void renderer_draw_frame(float alpha) {
    (void)alpha;   /* reservado para interpolacion futura */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Sin mapa cargado solo limpiamos la pantalla */
    if (g_vert_count == 0 || g_vbo == 0) { return; }

    Mat4 view = mat4_fps_view(g_cam_pos, g_cam_yaw, g_cam_pitch);

    gl_UseProgram(g_shader);
    gl_UniformMatrix4fv(g_loc_view, 1, GL_FALSE, view.m);
    gl_UniformMatrix4fv(g_loc_proj, 1, GL_FALSE, g_proj.m);

    gl_ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    gl_Uniform1i(g_loc_diffuse, 0);

    gl_BindBuffer(GL_ARRAY_BUFFER, g_vbo);

    /* Configurar atributos usando offsetof — sin numeros magicos */
    if (g_loc_pos >= 0) {
        gl_VertexAttribPointer((GLuint)g_loc_pos, 3, GL_FLOAT, GL_FALSE,
                               (GLsizei)sizeof(MapVertex),
                               (void *)offsetof(MapVertex, pos));
        gl_EnableVertexAttribArray((GLuint)g_loc_pos);
    }
    if (g_loc_uv >= 0) {
        gl_VertexAttribPointer((GLuint)g_loc_uv, 2, GL_FLOAT, GL_FALSE,
                               (GLsizei)sizeof(MapVertex),
                               (void *)offsetof(MapVertex, uv));
        gl_EnableVertexAttribArray((GLuint)g_loc_uv);
    }
    if (g_loc_light >= 0) {
        gl_VertexAttribPointer((GLuint)g_loc_light, 3, GL_FLOAT, GL_FALSE,
                               (GLsizei)sizeof(MapVertex),
                               (void *)offsetof(MapVertex, light));
        gl_EnableVertexAttribArray((GLuint)g_loc_light);
    }

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)g_vert_count);

    if (g_loc_pos   >= 0) { gl_DisableVertexAttribArray((GLuint)g_loc_pos); }
    if (g_loc_uv    >= 0) { gl_DisableVertexAttribArray((GLuint)g_loc_uv); }
    if (g_loc_light >= 0) { gl_DisableVertexAttribArray((GLuint)g_loc_light); }

    gl_BindBuffer(GL_ARRAY_BUFFER, 0);
    gl_UseProgram(0);
}
