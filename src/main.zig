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
const sdl = @cImport({
    @cInclude("SDL2/SDL.h");
});

/// Tick rate del servidor — 64Hz, igual que CS:GO
const TICK_RATE: f64 = 64.0;
/// Segundos por tick (1/64 = 0.015625s)
const TICK_S: f64 = 1.0 / TICK_RATE;
/// Ancho de ventana en píxeles
const WINDOW_W: c_int = 1280;
/// Alto de ventana en píxeles
const WINDOW_H: c_int = 720;
/// Grados de rotación de cámara por píxel de ratón
const MOUSE_SENSITIVITY: f32 = 0.15;

/// Estado de las teclas de movimiento en este tick
const InputState = struct {
    forward: bool,
    back: bool,
    left: bool,
    right: bool,
    jump: bool,
    duck: bool,
    walk: bool,
};

/// Orientación de la cámara FPS en grados
const Camera = struct {
    yaw: f32,
    pitch: f32,
};

/// Estado global del juego — todo en el stack, sin heap en el game loop
const GameState = struct {
    running: bool,
    input: InputState,
    camera: Camera,
    tickCount: u64,
};

pub fn main() !void {
    // Inicializar SDL2 con vídeo y eventos
    if (sdl.SDL_Init(sdl.SDL_INIT_VIDEO | sdl.SDL_INIT_EVENTS) != 0) {
        std.log.err("SDL_Init falló: {s}", .{sdl.SDL_GetError()});
        return error.SDLInitFailed;
    }
    defer sdl.SDL_Quit();

    // Pedir OpenGL 2.1 — ANTES de crear la ventana
    _ = sdl.SDL_GL_SetAttribute(sdl.SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    _ = sdl.SDL_GL_SetAttribute(sdl.SDL_GL_CONTEXT_MINOR_VERSION, 1);
    _ = sdl.SDL_GL_SetAttribute(sdl.SDL_GL_DOUBLEBUFFER, 1);
    _ = sdl.SDL_GL_SetAttribute(sdl.SDL_GL_DEPTH_SIZE, 24);

    // Crear la ventana 1280x720
    const window = sdl.SDL_CreateWindow(
        "OpenStrike",
        sdl.SDL_WINDOWPOS_CENTERED,
        sdl.SDL_WINDOWPOS_CENTERED,
        WINDOW_W,
        WINDOW_H,
        sdl.SDL_WINDOW_OPENGL | sdl.SDL_WINDOW_SHOWN,
    ) orelse {
        std.log.err("SDL_CreateWindow falló: {s}", .{sdl.SDL_GetError()});
        return error.WindowCreateFailed;
    };
    defer sdl.SDL_DestroyWindow(window);

    // Crear contexto OpenGL 2.1
    const glCtx = sdl.SDL_GL_CreateContext(window) orelse {
        std.log.err("SDL_GL_CreateContext falló: {s}", .{sdl.SDL_GetError()});
        return error.GLContextFailed;
    };
    defer sdl.SDL_GL_DeleteContext(glCtx);

    // Activar VSync — sincroniza con la pantalla
    if (sdl.SDL_GL_SetSwapInterval(1) != 0) {
        std.log.warn("VSync no disponible: {s}", .{sdl.SDL_GetError()});
    }

    // Capturar ratón — modo FPS, cursor invisible y relativo
    _ = sdl.SDL_SetRelativeMouseMode(sdl.SDL_TRUE);

    // Estado inicial del juego
    var state = GameState{
        .running  = true,
        .input    = InputState{
            .forward = false,
            .back    = false,
            .left    = false,
            .right   = false,
            .jump    = false,
            .duck    = false,
            .walk    = false,
        },
        .camera   = Camera{ .yaw = 0.0, .pitch = 0.0 },
        .tickCount = 0,
    };

    std.log.info("OpenStrike iniciado — game loop a 64Hz", .{});

    // ── Game loop ──────────────────────────────────────────────────────────────
    // Accumulator pattern: acumula el tiempo transcurrido y ejecuta ticks de
    // duración fija (TICK_S = 1/64s) hasta consumir el acumulador.
    // Esto garantiza física estable aunque el frame rate fluctúe.
    var lastTime = std.time.milliTimestamp();
    var accumulator: f64 = 0.0;

    while (state.running) {
        // Tiempo transcurrido desde el frame anterior
        const now = std.time.milliTimestamp();
        const frameMs: f64 = @as(f64, @floatFromInt(now - lastTime));
        lastTime = now;

        // Cap de 250ms: evita espiral de la muerte (pause en debugger, lag spike)
        const cappedMs = @min(frameMs, 250.0);
        accumulator += cappedMs / 1000.0;

        // Procesar eventos SDL (sin allocations)
        processEvents(&state);
        if (!state.running) {
            break;
        }

        // Ejecutar ticks de física a 64Hz exactos
        while (accumulator >= TICK_S) {
            state.tickCount += 1;
            tick(&state, @floatCast(TICK_S));
            accumulator -= TICK_S;
        }

        // Render placeholder — Sistema 2 añadirá OpenGL aquí
        sdl.SDL_GL_SwapWindow(window);
    }

    std.log.info("OpenStrike cerrado tras {d} ticks.", .{state.tickCount});
}

/// Procesa todos los eventos SDL pendientes en este frame — sin allocations
fn processEvents(state: *GameState) void {
    var event: sdl.SDL_Event = undefined;

    while (sdl.SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            sdl.SDL_QUIT => {
                state.running = false;
            },
            sdl.SDL_KEYDOWN => {
                if (event.key.keysym.sym == sdl.SDLK_ESCAPE) {
                    state.running = false;
                }
            },
            sdl.SDL_MOUSEMOTION => {
                const dx: f32 = @floatFromInt(event.motion.xrel);
                const dy: f32 = @floatFromInt(event.motion.yrel);
                // Yaw: girar a izquierda/derecha
                state.camera.yaw += dx * MOUSE_SENSITIVITY;
                // Pitch: mirar arriba/abajo — invertido porque Y de pantalla va hacia abajo
                state.camera.pitch -= dy * MOUSE_SENSITIVITY;
                // Clamp: no se puede mirar más allá de 89 grados arriba/abajo
                state.camera.pitch = std.math.clamp(state.camera.pitch, @as(f32, -89.0), @as(f32, 89.0));
            },
            else => {},
        }
    }

    // SDL_GetKeyboardState: polling de estado actual de teclas
    // Más fiable que eventos para teclas mantenidas (WASD durante movimiento)
    const keys = sdl.SDL_GetKeyboardState(null);
    state.input = InputState{
        .forward = keys[@intCast(sdl.SDL_SCANCODE_W)]      != 0,
        .back    = keys[@intCast(sdl.SDL_SCANCODE_S)]      != 0,
        .left    = keys[@intCast(sdl.SDL_SCANCODE_A)]      != 0,
        .right   = keys[@intCast(sdl.SDL_SCANCODE_D)]      != 0,
        .jump    = keys[@intCast(sdl.SDL_SCANCODE_SPACE)]  != 0,
        .duck    = keys[@intCast(sdl.SDL_SCANCODE_LCTRL)]  != 0,
        .walk    = keys[@intCast(sdl.SDL_SCANCODE_LSHIFT)] != 0,
    };
}

/// Tick de lógica de juego a tasa fija
/// dt es siempre TICK_S = 1/64 = 0.015625 segundos
fn tick(state: *GameState, dt: f32) void {
    _ = dt; // se usará en Sistema 3 — pmove

    // Una vez por segundo (cada 64 ticks): imprimir estado en el terminal
    if (state.tickCount % 64 == 0) {
        std.log.debug(
            "Tick {d:>6} | yaw={d:>7.1} pitch={d:>6.1} | W={d} A={d} S={d} D={d} Esp={d} Ctrl={d} Shift={d}",
            .{
                state.tickCount,
                state.camera.yaw,
                state.camera.pitch,
                @intFromBool(state.input.forward),
                @intFromBool(state.input.left),
                @intFromBool(state.input.back),
                @intFromBool(state.input.right),
                @intFromBool(state.input.jump),
                @intFromBool(state.input.duck),
                @intFromBool(state.input.walk),
            },
        );
    }
}
