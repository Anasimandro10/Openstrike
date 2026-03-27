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
const gl = @import("renderer/gl.zig");
const cam = @import("renderer/camera.zig");

const c = @cImport({
    @cInclude("SDL2/SDL.h");
    @cInclude("SDL2/SDL_opengl.h");
});

// ─── Constantes del loop ─────────────────────────────────────────────────────

const TICK_RATE: f64 = 64.0;
const TICK_S: f64 = 1.0 / TICK_RATE;

// ─── Vertex de la escena de prueba ───────────────────────────────────────────

const Vertex = packed struct {
    x: f32,
    y: f32,
    z: f32,
    u: f32,
    v: f32,
    lr: f32,
    lg: f32,
    lb: f32,
};

const SUELO_VERTICES = [_]Vertex{
    .{ .x = -512, .y = 0, .z = -512, .u = 0, .v = 0, .lr = 0.72, .lg = 0.64, .lb = 0.52 },
    .{ .x =  512, .y = 0, .z = -512, .u = 8, .v = 0, .lr = 0.72, .lg = 0.64, .lb = 0.52 },
    .{ .x =  512, .y = 0, .z =  512, .u = 8, .v = 8, .lr = 0.68, .lg = 0.60, .lb = 0.48 },
    .{ .x = -512, .y = 0, .z = -512, .u = 0, .v = 0, .lr = 0.72, .lg = 0.64, .lb = 0.52 },
    .{ .x =  512, .y = 0, .z =  512, .u = 8, .v = 8, .lr = 0.68, .lg = 0.60, .lb = 0.48 },
    .{ .x = -512, .y = 0, .z =  512, .u = 0, .v = 8, .lr = 0.68, .lg = 0.60, .lb = 0.48 },
};

fn buildCubo() [36]Vertex {
    const L: f32 = 20.0;
    const cx: f32 = 0;
    const cy: f32 = 20;
    const cz: f32 = -80;

    const lf = [3]f32{ 0.90, 0.82, 0.68 };
    const lb = [3]f32{ 0.40, 0.36, 0.28 };
    const lr = [3]f32{ 0.65, 0.58, 0.45 };
    const ll = [3]f32{ 0.50, 0.44, 0.34 };
    const lt = [3]f32{ 0.95, 0.90, 0.80 };
    const lb2 = [3]f32{ 0.20, 0.18, 0.14 };

    return [36]Vertex{
        .{ .x = cx - L, .y = cy - L, .z = cz + L, .u = 0, .v = 0, .lr = lf[0], .lg = lf[1], .lb = lf[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz + L, .u = 1, .v = 0, .lr = lf[0], .lg = lf[1], .lb = lf[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz + L, .u = 1, .v = 1, .lr = lf[0], .lg = lf[1], .lb = lf[2] },
        .{ .x = cx - L, .y = cy - L, .z = cz + L, .u = 0, .v = 0, .lr = lf[0], .lg = lf[1], .lb = lf[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz + L, .u = 1, .v = 1, .lr = lf[0], .lg = lf[1], .lb = lf[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz + L, .u = 0, .v = 1, .lr = lf[0], .lg = lf[1], .lb = lf[2] },

        .{ .x = cx + L, .y = cy - L, .z = cz - L, .u = 0, .v = 0, .lr = lb[0], .lg = lb[1], .lb = lb[2] },
        .{ .x = cx - L, .y = cy - L, .z = cz - L, .u = 1, .v = 0, .lr = lb[0], .lg = lb[1], .lb = lb[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz - L, .u = 1, .v = 1, .lr = lb[0], .lg = lb[1], .lb = lb[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz - L, .u = 0, .v = 0, .lr = lb[0], .lg = lb[1], .lb = lb[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz - L, .u = 1, .v = 1, .lr = lb[0], .lg = lb[1], .lb = lb[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz - L, .u = 0, .v = 1, .lr = lb[0], .lg = lb[1], .lb = lb[2] },

        .{ .x = cx + L, .y = cy - L, .z = cz + L, .u = 0, .v = 0, .lr = lr[0], .lg = lr[1], .lb = lr[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz - L, .u = 1, .v = 0, .lr = lr[0], .lg = lr[1], .lb = lr[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz - L, .u = 1, .v = 1, .lr = lr[0], .lg = lr[1], .lb = lr[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz + L, .u = 0, .v = 0, .lr = lr[0], .lg = lr[1], .lb = lr[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz - L, .u = 1, .v = 1, .lr = lr[0], .lg = lr[1], .lb = lr[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz + L, .u = 0, .v = 1, .lr = lr[0], .lg = lr[1], .lb = lr[2] },

        .{ .x = cx - L, .y = cy - L, .z = cz - L, .u = 0, .v = 0, .lr = ll[0], .lg = ll[1], .lb = ll[2] },
        .{ .x = cx - L, .y = cy - L, .z = cz + L, .u = 1, .v = 0, .lr = ll[0], .lg = ll[1], .lb = ll[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz + L, .u = 1, .v = 1, .lr = ll[0], .lg = ll[1], .lb = ll[2] },
        .{ .x = cx - L, .y = cy - L, .z = cz - L, .u = 0, .v = 0, .lr = ll[0], .lg = ll[1], .lb = ll[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz + L, .u = 1, .v = 1, .lr = ll[0], .lg = ll[1], .lb = ll[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz - L, .u = 0, .v = 1, .lr = ll[0], .lg = ll[1], .lb = ll[2] },

        .{ .x = cx - L, .y = cy + L, .z = cz + L, .u = 0, .v = 0, .lr = lt[0], .lg = lt[1], .lb = lt[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz + L, .u = 1, .v = 0, .lr = lt[0], .lg = lt[1], .lb = lt[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz - L, .u = 1, .v = 1, .lr = lt[0], .lg = lt[1], .lb = lt[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz + L, .u = 0, .v = 0, .lr = lt[0], .lg = lt[1], .lb = lt[2] },
        .{ .x = cx + L, .y = cy + L, .z = cz - L, .u = 1, .v = 1, .lr = lt[0], .lg = lt[1], .lb = lt[2] },
        .{ .x = cx - L, .y = cy + L, .z = cz - L, .u = 0, .v = 1, .lr = lt[0], .lg = lt[1], .lb = lt[2] },

        .{ .x = cx - L, .y = cy - L, .z = cz - L, .u = 0, .v = 0, .lr = lb2[0], .lg = lb2[1], .lb = lb2[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz - L, .u = 1, .v = 0, .lr = lb2[0], .lg = lb2[1], .lb = lb2[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz + L, .u = 1, .v = 1, .lr = lb2[0], .lg = lb2[1], .lb = lb2[2] },
        .{ .x = cx - L, .y = cy - L, .z = cz - L, .u = 0, .v = 0, .lr = lb2[0], .lg = lb2[1], .lb = lb2[2] },
        .{ .x = cx + L, .y = cy - L, .z = cz + L, .u = 1, .v = 1, .lr = lb2[0], .lg = lb2[1], .lb = lb2[2] },
        .{ .x = cx - L, .y = cy - L, .z = cz + L, .u = 0, .v = 1, .lr = lb2[0], .lg = lb2[1], .lb = lb2[2] },
    };
}

// ─── Shaders embedded (fallback si no hay archivos en assets/shaders/) ────────

const VERT_FALLBACK =
    \\#version 120
    \\attribute vec3 a_position;
    \\attribute vec2 a_texcoord;
    \\attribute vec3 a_lightcolor;
    \\varying vec2 v_texcoord;
    \\varying vec3 v_lightcolor;
    \\uniform mat4 u_view;
    \\uniform mat4 u_proj;
    \\void main() {
    \\    v_texcoord   = a_texcoord;
    \\    v_lightcolor = a_lightcolor;
    \\    gl_Position  = u_proj * u_view * vec4(a_position, 1.0);
    \\}
;

const FRAG_FALLBACK =
    \\#version 120
    \\varying vec2 v_texcoord;
    \\varying vec3 v_lightcolor;
    \\uniform sampler2D u_diffuse;
    \\void main() {
    \\    vec4 texColor = texture2D(u_diffuse, v_texcoord);
    \\    vec3 final = texColor.rgb * v_lightcolor;
    \\    gl_FragColor = vec4(final, texColor.a);
    \\}
;

// ─── Estado de render ─────────────────────────────────────────────────────────

const RenderState = struct {
    programa: c.GLuint = 0,
    vbo_suelo: c.GLuint = 0,
    vbo_cubo: c.GLuint = 0,
    tex_suelo: c.GLuint = 0,
    tex_cubo: c.GLuint = 0,
    loc_view: c.GLint = -1,
    loc_proj: c.GLint = -1,
    loc_diffuse: c.GLint = -1,
    attr_pos: c.GLuint = 0,
    attr_uv: c.GLuint = 0,
    attr_light: c.GLuint = 0,
};

// ─── Estado de input ─────────────────────────────────────────────────────────

const InputState = struct {
    adelante: bool = false,
    atras: bool = false,
    izquierda: bool = false,
    derecha: bool = false,
    saltar: bool = false,
    agacharse: bool = false,
    caminar: bool = false,
    mouse_dx: f32 = 0,
    mouse_dy: f32 = 0,
};

// ─── Variables globales ───────────────────────────────────────────────────────

var running: bool = true;
var input_state: InputState = .{};
var camara: cam.Camera = .{};
var render_state: RenderState = .{};
var mat_proj: cam.Mat4 = undefined;

// ─── Main ────────────────────────────────────────────────────────────────────

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

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
        "OpenStrike — Sistema 2 Renderer",
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
    _ = c.SDL_SetRelativeMouseMode(c.SDL_TRUE);

    try gl.cargarFuncionesGL();

    c.glEnable(c.GL_DEPTH_TEST);
    c.glEnable(c.GL_CULL_FACE);
    c.glCullFace(c.GL_BACK);
    c.glViewport(0, 0, 1280, 720);
    c.glClearColor(0.55, 0.62, 0.68, 1.0);

    const programa = cargarShadersConFallback(allocator) catch |err| {
        std.log.err("Error cargando shaders: {}", .{err});
        return err;
    };
    render_state.programa = programa;

    render_state.loc_view = gl.glGetUniformLocation(programa, "u_view");
    render_state.loc_proj = gl.glGetUniformLocation(programa, "u_proj");
    render_state.loc_diffuse = gl.glGetUniformLocation(programa, "u_diffuse");

    const attr_pos_i = gl.glGetAttribLocation(programa, "a_position");
    const attr_uv_i = gl.glGetAttribLocation(programa, "a_texcoord");
    const attr_light_i = gl.glGetAttribLocation(programa, "a_lightcolor");

    if (attr_pos_i < 0 or attr_uv_i < 0 or attr_light_i < 0) {
        std.log.err("GL: atributo no encontrado en shader (pos={d} uv={d} light={d})", .{
            attr_pos_i, attr_uv_i, attr_light_i,
        });
        return error.AtributoFaltante;
    }
    render_state.attr_pos = @intCast(attr_pos_i);
    render_state.attr_uv = @intCast(attr_uv_i);
    render_state.attr_light = @intCast(attr_light_i);

    render_state.vbo_suelo = try gl.subirVBO(Vertex, &SUELO_VERTICES);
    const cubo_verts = buildCubo();
    render_state.vbo_cubo = try gl.subirVBO(Vertex, &cubo_verts);

    render_state.tex_suelo = try gl.texturaSolida(0xB8, 0xA8, 0x88, 0xFF);
    render_state.tex_cubo = try gl.texturaSolida(0x90, 0x88, 0x78, 0xFF);

    mat_proj = cam.Mat4.perspectiva(90.0, 1280.0 / 720.0, 1.0, 4096.0);

    camara.pos = .{ .x = 0, .y = 40, .z = 80 };
    camara.yaw = 180.0;

    std.log.info("OpenStrike — Sistema 2 renderer listo", .{});
    std.log.info("WASD moverse | raton mirar | ESC salir", .{});

    var last_time = std.time.milliTimestamp();
    var accumulator: f64 = 0.0;
    var tick_count: u64 = 0;

    while (running) {
        const now = std.time.milliTimestamp();
        var frame_ms: f64 = @as(f64, @floatFromInt(now - last_time));
        last_time = now;

        if (frame_ms > 250.0) {
            frame_ms = 250.0;
        }
        accumulator += frame_ms / 1000.0;

        procesarEventos();

        while (accumulator >= TICK_S) {
            tick(@floatCast(TICK_S), tick_count);
            tick_count += 1;
            accumulator -= TICK_S;
            input_state.mouse_dx = 0;
            input_state.mouse_dy = 0;
        }

        render(window);
    }

    gl.glDeleteBuffers(1, &render_state.vbo_suelo);
    gl.glDeleteBuffers(1, &render_state.vbo_cubo);
    c.glDeleteTextures(1, &render_state.tex_suelo);
    c.glDeleteTextures(1, &render_state.tex_cubo);
    gl.glDeleteProgram(render_state.programa);

    std.log.info("OpenStrike cerrado limpiamente.", .{});
}

// ─── Funciones ───────────────────────────────────────────────────────────────

fn cargarShadersConFallback(allocator: std.mem.Allocator) !c.GLuint {
    const vert_src = std.fs.cwd().readFileAlloc(allocator, "assets/shaders/world.vert", 64 * 1024) catch |err| {
        std.log.warn("No se pudo leer assets/shaders/world.vert ({}) — usando shader embedded", .{err});
        return gl.crearPrograma(VERT_FALLBACK, FRAG_FALLBACK);
    };
    defer allocator.free(vert_src);

    const frag_src = std.fs.cwd().readFileAlloc(allocator, "assets/shaders/world.frag", 64 * 1024) catch |err| {
        std.log.warn("No se pudo leer assets/shaders/world.frag ({}) — usando shader embedded", .{err});
        return gl.crearPrograma(VERT_FALLBACK, FRAG_FALLBACK);
    };
    defer allocator.free(frag_src);

    std.log.info("Shaders cargados desde assets/shaders/", .{});
    return gl.crearPrograma(vert_src, frag_src);
}

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

    const teclas = c.SDL_GetKeyboardState(null);
    input_state.adelante = teclas[c.SDL_SCANCODE_W] != 0;
    input_state.atras = teclas[c.SDL_SCANCODE_S] != 0;
    input_state.izquierda = teclas[c.SDL_SCANCODE_A] != 0;
    input_state.derecha = teclas[c.SDL_SCANCODE_D] != 0;
    input_state.saltar = teclas[c.SDL_SCANCODE_SPACE] != 0;
    input_state.agacharse = teclas[c.SDL_SCANCODE_LCTRL] != 0;
    input_state.caminar = teclas[c.SDL_SCANCODE_LSHIFT] != 0;
}

fn tick(dt: f32, tick_num: u64) void {
    _ = tick_num;
    camara.procesarMouse(input_state.mouse_dx, input_state.mouse_dy);
    camara.moverLibre(
        input_state.adelante,
        input_state.atras,
        input_state.izquierda,
        input_state.derecha,
        dt,
    );
}

fn render(window: *c.SDL_Window) void {
    c.glClear(c.GL_COLOR_BUFFER_BIT | c.GL_DEPTH_BUFFER_BIT);

    gl.glUseProgram(render_state.programa);

    const mat_view = camara.matrizVista();
    gl.glUniformMatrix4fv(render_state.loc_view, 1, c.GL_FALSE, &mat_view.m);
    gl.glUniformMatrix4fv(render_state.loc_proj, 1, c.GL_FALSE, &mat_proj.m);

    gl.glActiveTexture(gl.GL_TEXTURE0);
    gl.glUniform1i(render_state.loc_diffuse, 0);

    c.glBindTexture(c.GL_TEXTURE_2D, render_state.tex_suelo);
    dibujarVBO(render_state.vbo_suelo, SUELO_VERTICES.len);

    c.glBindTexture(c.GL_TEXTURE_2D, render_state.tex_cubo);
    dibujarVBO(render_state.vbo_cubo, 36);

    c.glBindTexture(c.GL_TEXTURE_2D, 0);
    gl.glUseProgram(0);

    c.SDL_GL_SwapWindow(window);
}

fn dibujarVBO(vbo: c.GLuint, vertex_count: usize) void {
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo);

    const stride: c.GLsizei = @sizeOf(Vertex);

    gl.glEnableVertexAttribArray(render_state.attr_pos);
    gl.glVertexAttribPointer(render_state.attr_pos, 3, c.GL_FLOAT, c.GL_FALSE, stride, @ptrFromInt(0));

    gl.glEnableVertexAttribArray(render_state.attr_uv);
    gl.glVertexAttribPointer(render_state.attr_uv, 2, c.GL_FLOAT, c.GL_FALSE, stride, @ptrFromInt(12));

    gl.glEnableVertexAttribArray(render_state.attr_light);
    gl.glVertexAttribPointer(render_state.attr_light, 3, c.GL_FLOAT, c.GL_FALSE, stride, @ptrFromInt(20));

    c.glDrawArrays(c.GL_TRIANGLES, 0, @intCast(vertex_count));

    gl.glDisableVertexAttribArray(render_state.attr_pos);
    gl.glDisableVertexAttribArray(render_state.attr_uv);
    gl.glDisableVertexAttribArray(render_state.attr_light);
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, 0);
}
