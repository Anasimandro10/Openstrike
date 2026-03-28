// Copyright (c) 2026 OpenStrike Project
// main.c is part of OpenStrike.
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
#include <math.h>
#include <stdio.h>

#include "types.h"
#include "renderer/renderer.h"
#include "physics/pmove.h"

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */

#define TICK_RATE    64
#define TICK_MS      (1000.0 / TICK_RATE)   /* 15.625ms por tick */
#define SENSIBILIDAD 0.15f
#define PITCH_MAX    89.0f

/* ------------------------------------------------------------------ */
/* Structs locales                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    bool adelante, atras, izquierda, derecha;
    bool saltar, agacharse, caminar;
    f32  mouse_dx, mouse_dy;
} InputState;

typedef struct {
    f32 yaw;    /* grados — 0 = mirando -Z */
    f32 pitch;  /* grados — +90 arriba, -90 abajo */
} Camera;

/* ------------------------------------------------------------------ */
/* Variables globales                                                  */
/* ------------------------------------------------------------------ */

static SDL_Window   *g_window           = NULL;
static SDL_GLContext g_gl_ctx           = NULL;
static bool          g_running          = false;
static InputState    g_input            = {0};
static Camera        g_camara           = {0};
static PhysPlayer    g_player;
static bool          g_espacio_anterior = false;  /* detectar pulsacion nueva de SPACE */

/* ------------------------------------------------------------------ */
/* Inicializacion                                                      */
/* ------------------------------------------------------------------ */

static int init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    g_window = SDL_CreateWindow(
        "OpenStrike",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        return 0;
    }

    g_gl_ctx = SDL_GL_CreateContext(g_window);
    if (!g_gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_GL_SetSwapInterval(1);

    glViewport(0, 0, 1280, 720);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    if (!renderer_init()) {
        fprintf(stderr, "renderer_init fallo\n");
        return 0;
    }

    Vec3 spawn = {0.0f, 0.0f, 0.0f};
    phys_player_init(&g_player, spawn);
    g_player.en_suelo = true;

    SDL_SetRelativeMouseMode(SDL_TRUE);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Eventos e input                                                     */
/* ------------------------------------------------------------------ */

static void process_events(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                g_running = false;
                break;
            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    g_running = false;
                }
                break;
            case SDL_MOUSEMOTION:
                g_input.mouse_dx += (f32)ev.motion.xrel;
                g_input.mouse_dy += (f32)ev.motion.yrel;
                break;
            default:
                break;
        }
    }

    const Uint8 *teclas = SDL_GetKeyboardState(NULL);

    g_input.adelante  = (bool)teclas[SDL_SCANCODE_W];
    g_input.atras     = (bool)teclas[SDL_SCANCODE_S];
    g_input.izquierda = (bool)teclas[SDL_SCANCODE_A];
    g_input.derecha   = (bool)teclas[SDL_SCANCODE_D];
    g_input.agacharse = (bool)teclas[SDL_SCANCODE_LCTRL];
    g_input.caminar   = (bool)teclas[SDL_SCANCODE_LSHIFT];

    /* Salto: solo activo en el frame en que se pulsa SPACE por primera vez.
       Mantener pulsado no genera saltos repetidos — hay que soltar y volver
       a pulsar. Esto es lo que hace que el bhop requiera timing real.      */
    bool espacio_ahora     = (bool)teclas[SDL_SCANCODE_SPACE];
    g_input.saltar         = espacio_ahora && !g_espacio_anterior;
    g_espacio_anterior     = espacio_ahora;
}

/* ------------------------------------------------------------------ */
/* Update (llamado a 64Hz exacto)                                     */
/* ------------------------------------------------------------------ */

static void update(f32 dt) {
    /* Camara */
    g_camara.yaw   += g_input.mouse_dx * SENSIBILIDAD;
    g_camara.pitch -= g_input.mouse_dy * SENSIBILIDAD;
    if (g_camara.pitch >  PITCH_MAX) { g_camara.pitch =  PITCH_MAX; }
    if (g_camara.pitch < -PITCH_MAX) { g_camara.pitch = -PITCH_MAX; }
    g_input.mouse_dx = 0.0f;
    g_input.mouse_dy = 0.0f;

    /* Direccion de movimiento desde yaw */
    f32 yaw_rad = g_camara.yaw * (3.14159265f / 180.0f);
    Vec3 forward = { sinf(yaw_rad), 0.0f, -cosf(yaw_rad) };
    Vec3 right   = { cosf(yaw_rad), 0.0f,  sinf(yaw_rad) };

    Vec3 wish = {0.0f, 0.0f, 0.0f};
    if (g_input.adelante)  { wish = vec3_add(wish, forward); }
    if (g_input.atras)     { wish = vec3_sub(wish, forward); }
    if (g_input.derecha)   { wish = vec3_add(wish, right);   }
    if (g_input.izquierda) { wish = vec3_sub(wish, right);   }
    if (vec3_len(wish) > 0.001f) { wish = vec3_norm(wish); }

    PhysInput phys_input;
    phys_input.wish_dir  = wish;
    phys_input.saltar    = g_input.saltar;
    phys_input.agacharse = g_input.agacharse;
    phys_input.caminar   = g_input.caminar;

    phys_tick(&g_player, phys_input, dt);

    /* Consumir saltar — si el accumulator es alto y update() se llama dos
       veces en el mismo frame, el segundo tick no intenta saltar de nuevo. */
    g_input.saltar = false;

    f32 view_y = g_player.pos.y + phys_view_height(&g_player);
    renderer_set_camera(
        g_player.pos.x, view_y, g_player.pos.z,
        g_camara.yaw, g_camara.pitch
    );
}

/* ------------------------------------------------------------------ */
/* Render                                                              */
/* ------------------------------------------------------------------ */

static void render(f32 alpha) {
    renderer_draw_frame(alpha);
    SDL_GL_SwapWindow(g_window);
}

/* ------------------------------------------------------------------ */
/* Shutdown                                                            */
/* ------------------------------------------------------------------ */

static void shutdown(void) {
    renderer_shutdown();
    if (g_gl_ctx) { SDL_GL_DeleteContext(g_gl_ctx); }
    if (g_window)  { SDL_DestroyWindow(g_window);    }
    SDL_Quit();
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!init()) { return 1; }
    g_running = true;

    u64  last_time   = SDL_GetTicks64();
    f64  accumulator = 0.0;

    while (g_running) {
        process_events();

        u64 now      = SDL_GetTicks64();
        f64 frame_ms = (f64)(now - last_time);
        last_time    = now;
        if (frame_ms > 250.0) { frame_ms = 250.0; }
        accumulator += frame_ms;

        while (accumulator >= TICK_MS) {
            update((f32)(TICK_MS / 1000.0));
            accumulator -= TICK_MS;
        }

        render((f32)(accumulator / TICK_MS));
    }

    shutdown();
    return 0;
}
