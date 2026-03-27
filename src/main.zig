// Copyright (c) 2026 OpenStrike Project
// main.zig is part of OpenStrike.
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

const std = @import("std");

const c = @cImport({
    @cInclude("SDL2/SDL.h");
    @cInclude("SDL2/SDL_opengl.h");
});

const gl          = @import("renderer/gl.zig");
const Renderer    = @import("renderer/renderer.zig").Renderer;
const CameraInput = @import("renderer/renderer.zig").CameraInput;

// ─── Constantes del loop ─────────────────────────────────────────────────────

const TICK_RATE: f64 = 64.0;
const TICK_S: f64    = 1.0 / TICK_RATE; // 0.015625s por tick

// ─── Estado de input ─────────────────────────────────────────────────────────

const InputState = struct {
    adelante:  bool = false,
    atras:     bool = false,
    izquierda: bool = false,
    derecha:   bool = false,
    saltar:    bool = false,
    agacharse: bool = false,
    caminar:   bool = false,
    mouse_dx:  f32  = 0,
    mouse_dy:  f32  = 0,
};

// ─── Variables globales ───────────────────────────────────────────────────────

var running:     bool       = true;
var input_state: InputState = .{};

// ─── Main ────────────────────────────────────────────────────────────────────

pub fn main() !void {
    if (c.SDL_Init(c.SDL_INIT_VIDEO | c.SDL_INIT_EVENTS) != 0) {
        std.log.err("SDL_Init fallo: {s}", .{c.SDL_GetError()});
        return error.SDLInitFallo;
    }
    defer c.SDL_Quit();

    _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 1);
    _ = c.SDL_GL_SetAttribute(c.SDL_GL_DOUBLEBUFFER, 1);
    _ = c.SDL_GL_SetAttribute(c.SDL_GL_DEPTH_SIZE, 24);

    const window = c.SDL_CreateWindow(
        "OpenStrike",
        c.SDL_WINDOWPOS_CENTERED,
        c.SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        c.SDL_WINDOW_OPENGL | c.SDL_WINDOW_SHOWN,
    ) orelse {
        std.log.err("SDL_CreateWindow fallo: {s}", .{c.SDL_GetError()});
        return error.VentanaFallo;
    };
    defer c.SDL_DestroyWindow(window);

    const gl_ctx = c.SDL_GL_CreateContext(window) orelse {
        std.log.err("SDL_GL_CreateContext fallo: {s}", .{c.SDL_GetError()});
        return error.GLContextFallo;
    };
    defer c.SDL_GL_DeleteContext(gl_ctx);

    _ = c.SDL_GL_SetSwapInterval(1);

    // Cargar funciones GL 2.1 — OBLIGATORIO antes de cualquier VBO o shader
    try gl.cargarFuncionesGL();

    c.glEnable(c.GL_DEPTH_TEST);
    c.glEnable(c.GL_CULL_FACE);
    c.glCullFace(c.GL_BACK);
    c.glFrontFace(c.GL_CCW);
    c.glClearColor(0.08, 0.08, 0.10, 1.0);

    _ = c.SDL_SetRelativeMouseMode(c.SDL_TRUE);

    var ren = try Renderer.init();
    defer ren.deinit();

    std.log.info("OpenStrike iniciado — 1280x720 OpenGL 2.1 64Hz", .{});
    std.log.info("WASD = mover | raton = mirar | ESC = salir", .{});

    // ─── Game loop — accumulator pattern a 64Hz ───────────────────────────
    var last_time   = std.time.milliTimestamp();
    var accumulator: f64 = 0.0;

    while (running) {
        const now     = std.time.milliTimestamp();
        var frame_ms: f64 = @as(f64, @floatFromInt(now - last_time));
        last_time = now;
        if (frame_ms > 250.0) frame_ms = 250.0;
        accumulator += frame_ms / 1000.0;

        procesarEventos();

        while (accumulator >= TICK_S) {
            ren.update(.{
                .adelante  = input_state.adelante,
                .atras     = input_state.atras,
                .izquierda = input_state.izquierda,
                .derecha   = input_state.derecha,
                .mouse_dx  = input_state.mouse_dx,
                .mouse_dy  = input_state.mouse_dy,
            }, @floatCast(TICK_S));

            accumulator          -= TICK_S;
            input_state.mouse_dx  = 0;
            input_state.mouse_dy  = 0;
        }

        c.glClear(c.GL_COLOR_BUFFER_BIT | c.GL_DEPTH_BUFFER_BIT);
        ren.render();
        c.SDL_GL_SwapWindow(window);
    }

    std.log.info("OpenStrike cerrado limpiamente.", .{});
}

// ─── Eventos ─────────────────────────────────────────────────────────────────

fn procesarEventos() void {
    var event: c.SDL_Event = undefined;
    while (c.SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            c.SDL_QUIT => {
                running = false;
            },
            c.SDL_KEYDOWN => {
                if (event.key.keysym.sym == c.SDLK_ESCAPE) {
                    running = false;
                }
            },
            c.SDL_MOUSEMOTION => {
                input_state.mouse_dx += @as(f32, @floatFromInt(event.motion.xrel));
                input_state.mouse_dy += @as(f32, @floatFromInt(event.motion.yrel));
            },
            else => {},
        }
    }

    const teclas          = c.SDL_GetKeyboardState(null);
    input_state.adelante  = teclas[c.SDL_SCANCODE_W] != 0;
    input_state.atras     = teclas[c.SDL_SCANCODE_S] != 0;
    input_state.izquierda = teclas[c.SDL_SCANCODE_A] != 0;
    input_state.derecha   = teclas[c.SDL_SCANCODE_D] != 0;
    input_state.saltar    = teclas[c.SDL_SCANCODE_SPACE] != 0;
    input_state.agacharse = teclas[c.SDL_SCANCODE_LCTRL] != 0;
    input_state.caminar   = teclas[c.SDL_SCANCODE_LSHIFT] != 0;
}
