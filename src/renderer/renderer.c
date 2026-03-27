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

#include "renderer.h"
#include "gl.h"

#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---- Vertex ---- */
/* Debe coincidir con los atributos del shader:
   a_position (vec3), a_texcoord (vec2), a_lightcolor (vec3) */

typedef struct {
    float pos[3];    /* offset  0 — 12 bytes */
    float uv[2];     /* offset 12 —  8 bytes */
    float light[3];  /* offset 20 — 12 bytes */
} Vertex;            /* total: 32 bytes */

/* ---- Estado del renderer ---- */

static GLuint g_shader     = 0;
static GLuint g_vbo        = 0;
static GLuint g_texture    = 0;
static int    g_vert_count = 0;

/* Locations de atributos y uniforms — inicializar a -1 (no encontrado) */
static GLint g_loc_pos     = -1;
static GLint g_loc_uv      = -1;
static GLint g_loc_light   = -1;
static GLint g_loc_view    = -1;
static GLint g_loc_proj    = -1;
static GLint g_loc_diffuse = -1;

/* Camara */
static Vec3 g_cam_pos   = { 0.0f, 64.0f, 0.0f };
static f32  g_cam_yaw   = 0.0f;
static f32  g_cam_pitch = 0.0f;

/* Proyeccion (fija 1280x720) */
static Mat4 g_proj;

/* ---- Utilidad: leer archivo completo ---- */

static char *leer_archivo(const char *ruta) {
    FILE *f = fopen(ruta, "rb");
    if (!f) {
        fprintf(stderr, "renderer: no se pudo abrir '%s'\n", ruta);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);
    char *buf = (char *)malloc((size_t)tam + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, (size_t)tam, f);
    buf[tam] = '\0';
    fclose(f);
    return buf;
}

/* ---- Compilar y enlazar shaders ---- */

static GLuint compilar_shader(const char *src, GLenum tipo) {
    GLuint s = gl_CreateShader(tipo);
    if (!s) { return 0; }
    gl_ShaderSource(s, 1, (const GLchar **)&src, NULL);
    gl_CompileShader(s);
    GLint ok;
    gl_GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        GLsizei len;
        gl_GetShaderInfoLog(s, (GLsizei)sizeof(log), &len, log);
        fprintf(stderr, "renderer: shader error:\n%.*s\n", (int)len, log);
        gl_DeleteShader(s);
        return 0;
    }
    return s;
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
    GLint ok;
    gl_GetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        GLsizei len;
        gl_GetProgramInfoLog(prog, (GLsizei)sizeof(log), &len, log);
        fprintf(stderr, "renderer: program error:\n%.*s\n", (int)len, log);
        gl_DeleteProgram(prog);
        return 0;
    }
    return prog;
}

/* ---- Textura procedural: checkerboard 64x64 ---- */
/* Se usa hasta que Sistema 5 cargue texturas reales del mapa. */

static GLuint crear_textura_checkerboard(void) {
    static uint8_t pixels[64 * 64 * 4];
    int x, y;
    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            int cell   = ((x >> 3) + (y >> 3)) & 1;
            uint8_t c  = cell ? 210 : 80;
            uint8_t *p = pixels + (y * 64 + x) * 4;
            p[0] = c;
            p[1] = (uint8_t)(c * 95 / 100);
            p[2] = (uint8_t)(c * 85 / 100);
            p[3] = 255;
        }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return tex;
}

/* ---- Geometria de la sala de prueba ---- */
/* Una sala de 512x512x192 unidades Hammer con camara en (0,64,0).
   Lightmaps bakeados en vertice — colores distintos por superficie.
   Todo se reemplazara en Sistema 5 con el formato de mapa JSON. */

#define MAX_VERTS_SALA 128

static int    s_n  = 0;
static Vertex s_verts[MAX_VERTS_SALA];

static void push_tri(
    float x0, float y0, float z0, float u0, float v0,
    float x1, float y1, float z1, float u1, float v1,
    float x2, float y2, float z2, float u2, float v2,
    float lr, float lg, float lb)
{
    if (s_n + 3 > MAX_VERTS_SALA) { return; }
    s_verts[s_n]   = (Vertex){{ x0, y0, z0 }, { u0, v0 }, { lr, lg, lb }};
    s_verts[s_n+1] = (Vertex){{ x1, y1, z1 }, { u1, v1 }, { lr, lg, lb }};
    s_verts[s_n+2] = (Vertex){{ x2, y2, z2 }, { u2, v2 }, { lr, lg, lb }};
    s_n += 3;
}

/* Quad como 2 triangulos (v0,v1,v2) y (v0,v2,v3).
   Winding: CCW cuando se mira desde el lado del normal deseado. */
static void push_quad(
    float x0, float y0, float z0, float u0, float v0,
    float x1, float y1, float z1, float u1, float v1,
    float x2, float y2, float z2, float u2, float v2,
    float x3, float y3, float z3, float u3, float v3,
    float lr, float lg, float lb)
{
    push_tri(x0,y0,z0,u0,v0, x1,y1,z1,u1,v1, x2,y2,z2,u2,v2, lr,lg,lb);
    push_tri(x0,y0,z0,u0,v0, x2,y2,z2,u2,v2, x3,y3,z3,u3,v3, lr,lg,lb);
}

static GLuint crear_vbo_sala(int *out_count) {
    /* Escala UV: 64 unidades = 1 tile de textura */
    const float S = 1.0f / 64.0f;

    s_n = 0;

    /* Suelo (y=0, normal +Y)
       Winding CCW visto desde arriba: v0(-,-),v1(-,+),v2(+,+),v3(+,-) en XZ */
    push_quad(
        -256.0f, 0.0f, -256.0f,   0.0f,    0.0f,
        -256.0f, 0.0f,  256.0f,   0.0f,    512*S,
         256.0f, 0.0f,  256.0f,   512*S,   512*S,
         256.0f, 0.0f, -256.0f,   512*S,   0.0f,
        0.45f, 0.41f, 0.35f
    );

    /* Techo (y=192, normal -Y)
       Winding CCW visto desde abajo: v0(-,-),v1(+,-),v2(+,+),v3(-,+) en XZ */
    push_quad(
        -256.0f, 192.0f, -256.0f,  0.0f,   0.0f,
         256.0f, 192.0f, -256.0f,  512*S,  0.0f,
         256.0f, 192.0f,  256.0f,  512*S,  512*S,
        -256.0f, 192.0f,  256.0f,  0.0f,   512*S,
        0.26f, 0.26f, 0.30f
    );

    /* Pared norte (z=-256, normal +Z) — la que se ve al iniciar (yaw=0) */
    push_quad(
        -256.0f,   0.0f, -256.0f,  0.0f,   0.0f,
         256.0f,   0.0f, -256.0f,  512*S,  0.0f,
         256.0f, 192.0f, -256.0f,  512*S,  192*S,
        -256.0f, 192.0f, -256.0f,  0.0f,   192*S,
        0.55f, 0.50f, 0.43f
    );

    /* Pared sur (z=+256, normal -Z) */
    push_quad(
         256.0f,   0.0f,  256.0f,  0.0f,   0.0f,
        -256.0f,   0.0f,  256.0f,  512*S,  0.0f,
        -256.0f, 192.0f,  256.0f,  512*S,  192*S,
         256.0f, 192.0f,  256.0f,  0.0f,   192*S,
        0.48f, 0.44f, 0.38f
    );

    /* Pared este (x=+256, normal -X) */
    push_quad(
         256.0f,   0.0f, -256.0f,  0.0f,   0.0f,
         256.0f,   0.0f,  256.0f,  512*S,  0.0f,
         256.0f, 192.0f,  256.0f,  512*S,  192*S,
         256.0f, 192.0f, -256.0f,  0.0f,   192*S,
        0.48f, 0.44f, 0.38f
    );

    /* Pared oeste (x=-256, normal +X) */
    push_quad(
        -256.0f,   0.0f,  256.0f,  0.0f,   0.0f,
        -256.0f,   0.0f, -256.0f,  512*S,  0.0f,
        -256.0f, 192.0f, -256.0f,  512*S,  192*S,
        -256.0f, 192.0f,  256.0f,  0.0f,   192*S,
        0.55f, 0.50f, 0.43f
    );

    GLuint vbo;
    gl_GenBuffers(1, &vbo);
    gl_BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl_BufferData(GL_ARRAY_BUFFER,
                  (GLsizeiptr)(s_n * (int)sizeof(Vertex)),
                  s_verts,
                  GL_STATIC_DRAW);
    gl_BindBuffer(GL_ARRAY_BUFFER, 0);

    *out_count = s_n;
    return vbo;
}

/* ---- renderer_init ---- */

int renderer_init(void) {
    /* 1. Cargar extensiones GL 2.1 */
    if (!gl_cargar()) {
        fprintf(stderr, "renderer: fallo al cargar funciones GL\n");
        return 0;
    }
    fprintf(stdout, "Renderer: funciones GL cargadas OK\n");

    /* 2. Compilar shader world desde archivos */
    char *vert_src = leer_archivo("assets/shaders/world.vert");
    char *frag_src = leer_archivo("assets/shaders/world.frag");
    if (!vert_src || !frag_src) {
        free(vert_src);
        free(frag_src);
        fprintf(stderr, "renderer: no se pudieron leer los shaders de 'assets/shaders/'\n");
        fprintf(stderr, "renderer: ejecuta el binario desde la raiz del repositorio.\n");
        return 0;
    }
    g_shader = crear_programa(vert_src, frag_src);
    free(vert_src);
    free(frag_src);
    if (!g_shader) {
        fprintf(stderr, "renderer: fallo al compilar/enlazar shaders\n");
        return 0;
    }
    fprintf(stdout, "Renderer: shader world compilado OK\n");

    /* 3. Locations */
    g_loc_pos     = gl_GetAttribLocation (g_shader, "a_position");
    g_loc_uv      = gl_GetAttribLocation (g_shader, "a_texcoord");
    g_loc_light   = gl_GetAttribLocation (g_shader, "a_lightcolor");
    g_loc_view    = gl_GetUniformLocation(g_shader, "u_view");
    g_loc_proj    = gl_GetUniformLocation(g_shader, "u_proj");
    g_loc_diffuse = gl_GetUniformLocation(g_shader, "u_diffuse");

    /* 4. VBO sala de prueba */
    g_vbo = crear_vbo_sala(&g_vert_count);
    fprintf(stdout, "Renderer: VBO sala test (%d vertices) OK\n", g_vert_count);

    /* 5. Textura checkerboard placeholder */
    g_texture = crear_textura_checkerboard();
    fprintf(stdout, "Renderer: textura checkerboard 64x64 OK\n");

    /* 6. Proyeccion (fija para 1280x720, FOV 90) */
    g_proj = mat4_perspective(90.0f, 1280.0f / 720.0f, 4.0f, 4096.0f);

    return 1;
}

/* ---- renderer_shutdown ---- */

void renderer_shutdown(void) {
    if (g_vbo     != 0) { gl_DeleteBuffers(1, &g_vbo);      g_vbo     = 0; }
    if (g_shader  != 0) { gl_DeleteProgram(g_shader);        g_shader  = 0; }
    if (g_texture != 0) { glDeleteTextures(1, &g_texture);   g_texture = 0; }
    fprintf(stdout, "Renderer: apagado limpio\n");
}

/* ---- renderer_set_camera ---- */

void renderer_set_camera(float x, float y, float z, float yaw, float pitch) {
    g_cam_pos.x = x;
    g_cam_pos.y = y;
    g_cam_pos.z = z;
    g_cam_yaw   = yaw;
    g_cam_pitch = pitch;
}

/* ---- renderer_draw_frame ---- */

void renderer_draw_frame(float alpha) {
    (void)alpha; /* reservado para interpolacion Sistema 3+ */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!g_shader || !g_vbo || !g_texture) { return; }

    /* Calcular matriz view desde la camara actual */
    Mat4 view = mat4_fps_view(g_cam_pos, g_cam_yaw, g_cam_pitch);

    /* Usar shader y subir uniforms */
    gl_UseProgram(g_shader);
    gl_UniformMatrix4fv(g_loc_view,    1, GL_FALSE, view.m);
    gl_UniformMatrix4fv(g_loc_proj,    1, GL_FALSE, g_proj.m);
    gl_ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    gl_Uniform1i(g_loc_diffuse, 0);

    /* Configurar VBO y punteros de atributo
       Stride = sizeof(Vertex) = 32 bytes
       Offsets: pos=0, uv=12, light=20 */
    gl_BindBuffer(GL_ARRAY_BUFFER, g_vbo);

    if (g_loc_pos >= 0) {
        gl_EnableVertexAttribArray((GLuint)g_loc_pos);
        gl_VertexAttribPointer((GLuint)g_loc_pos, 3, GL_FLOAT, GL_FALSE,
                               (GLsizei)sizeof(Vertex), (void *)0);
    }
    if (g_loc_uv >= 0) {
        gl_EnableVertexAttribArray((GLuint)g_loc_uv);
        gl_VertexAttribPointer((GLuint)g_loc_uv, 2, GL_FLOAT, GL_FALSE,
                               (GLsizei)sizeof(Vertex), (void *)(3 * sizeof(float)));
    }
    if (g_loc_light >= 0) {
        gl_EnableVertexAttribArray((GLuint)g_loc_light);
        gl_VertexAttribPointer((GLuint)g_loc_light, 3, GL_FLOAT, GL_FALSE,
                               (GLsizei)sizeof(Vertex), (void *)(5 * sizeof(float)));
    }

    glDrawArrays(GL_TRIANGLES, 0, g_vert_count);

    /* Limpiar estado */
    if (g_loc_pos   >= 0) { gl_DisableVertexAttribArray((GLuint)g_loc_pos);   }
    if (g_loc_uv    >= 0) { gl_DisableVertexAttribArray((GLuint)g_loc_uv);    }
    if (g_loc_light >= 0) { gl_DisableVertexAttribArray((GLuint)g_loc_light); }

    gl_BindBuffer(GL_ARRAY_BUFFER, 0);
    gl_UseProgram(0);
}
