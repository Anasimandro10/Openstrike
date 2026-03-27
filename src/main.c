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

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>

#include "types.h"
#include "renderer/renderer.h"

/* ---- Constantes -------------------------------------------------------- */

#define TICK_RATE    64
#define TICK_MS      (1000.0 / TICK_RATE)
#define SENSIBILIDAD 0.15f
#define PITCH_MAX    89.0f

/* ---- Structs ----------------------------------------------------------- */

typedef struct {
    bool adelante;
    bool atras;
    bool izquierda;
    bool derecha;
    bool saltar;
    bool agacharse;
    bool caminar;
    f32  mouse_dx;
    f32  mouse_dy;
} InputState;

typedef struct {
    f32 yaw;
    f32 pitch;
} Camera;

/* ---- Globales ---------------------------------------------------------- */

static SDL_Window   *g_window  = NULL;
static SDL_GLContext g_gl_ctx  = NULL;
static bool          g_running = false;
static InputState    g_input   = {0};
static Camera        g_camara  = {0};

/* ---- Prototipos -------------------------------------------------------- */

static int  init(void);
static void shutdown(void);
static void process_events(void);
static void update(f32 dt);
static void render(f32 alpha);

/* ---- main -------------------------------------------------------------- */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_SetMainReady();

    if (!init()) {
        return 1;
    }

    u64 last_time   = SDL_GetTicks64();
    f64 accumulator = 0.0;
    g_running       = true;

    while (g_running) {
        process_events();

        u64 now      = SDL_GetTicks64();
        f64 frame_ms = (f64)(now - last_time);
        last_time    = now;

        if (frame_ms > 250.0) {
            frame_ms = 250.0;
        }
        accumulator += frame_ms;

        while (accumulator >= TICK_MS) {
            update((f32)(TICK_MS / 1000.0));
            g_input.mouse_dx  = 0.0f;
            g_input.mouse_dy  = 0.0f;
            accumulator      -= TICK_MS;
        }

        render((f32)(accumulator / TICK_MS));
        SDL_GL_SwapWindow(g_window);
    }

    shutdown();
    return 0;
}

/* ---- init -------------------------------------------------------------- */

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
        SDL_Quit();
        return 0;
    }

    g_gl_ctx = SDL_GL_CreateContext(g_window);
    if (!g_gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 0;
    }

    SDL_GL_SetSwapInterval(1);

    /* Setup GL basico (funciones OpenGL 1.x, sin cargar extensiones) */
    glViewport(0, 0, 1280, 720);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    /* Inicializar renderer (carga extensiones GL, compila shaders, sube VBO) */
    if (!renderer_init()) {
        SDL_GL_DeleteContext(g_gl_ctx);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 0;
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    fprintf(stdout, "OpenStrike iniciado: 1280x720 OpenGL 2.1 64Hz\n");
    fprintf(stdout, "WASD = girar camara | raton = mirar | ESC = salir\n");

    return 1;
}

/* ---- shutdown ---------------------------------------------------------- */

static void shutdown(void) {
    renderer_shutdown();
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_GL_DeleteContext(g_gl_ctx);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    fprintf(stdout, "OpenStrike cerrado.\n");
}

/* ---- process_events ---------------------------------------------------- */

static void process_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    g_running = false;
                }
                break;
            case SDL_MOUSEMOTION:
                g_input.mouse_dx += (f32)event.motion.xrel;
                g_input.mouse_dy += (f32)event.motion.yrel;
                break;
            default:
                break;
        }
    }

    const Uint8 *teclas = SDL_GetKeyboardState(NULL);
    g_input.adelante  = teclas[SDL_SCANCODE_W]      != 0;
    g_input.atras     = teclas[SDL_SCANCODE_S]      != 0;
    g_input.izquierda = teclas[SDL_SCANCODE_A]      != 0;
    g_input.derecha   = teclas[SDL_SCANCODE_D]      != 0;
    g_input.saltar    = teclas[SDL_SCANCODE_SPACE]  != 0;
    g_input.agacharse = teclas[SDL_SCANCODE_LCTRL]  != 0;
    g_input.caminar   = teclas[SDL_SCANCODE_LSHIFT] != 0;
}

/* ---- update ------------------------------------------------------------ */

static void update(f32 dt) {
    (void)dt;

    /* Actualizar angulos de la camara con el raton */
    g_camara.yaw   += g_input.mouse_dx * SENSIBILIDAD;
    g_camara.pitch -= g_input.mouse_dy * SENSIBILIDAD;

    if (g_camara.pitch >  PITCH_MAX) { g_camara.pitch =  PITCH_MAX; }
    if (g_camara.pitch < -PITCH_MAX) { g_camara.pitch = -PITCH_MAX; }

    /* Posicion fija en (0, 64, 0) hasta Sistema 3 (pmove).
       Sistema 3 pasara aqui pm.origin[] en lugar de (0,64,0). */
    renderer_set_camera(0.0f, 64.0f, 0.0f, g_camara.yaw, g_camara.pitch);
}

/* ---- render ------------------------------------------------------------ */

static void render(f32 alpha) {
    renderer_draw_frame(alpha);
}
