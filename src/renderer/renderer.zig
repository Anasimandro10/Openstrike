// Copyright (c) 2026 OpenStrike Project
// renderer.zig is part of OpenStrike.
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

const gl = @import("gl.zig");

// ─── Constantes ───────────────────────────────────────────────────────────────

/// FOV horizontal en grados (igual que cl_fov de CS:GO).
const FOV_H: f32         = 90.0;
const NEAR_PLANE: f32    = 0.1;
const FAR_PLANE: f32     = 4096.0;
const WIN_W: f32         = 1280.0;
const WIN_H: f32         = 720.0;
/// Grados por pixel de ratón.
const SENSIBILIDAD: f32  = 0.15;
/// Velocidad de la cámara de debug (sustituida por pmove en Sistema 3).
const CAM_SPEED: f32     = 5.0;

// Stride del VBO: pos(3) + uv(2) + normal(3) + lightColor(3) = 11 f32 = 44 bytes.
const STRIDE: c.GLsizei = @sizeOf(f32) * 11;
const OFF_POS:   usize  = 0;
const OFF_UV:    usize  = 3 * @sizeOf(f32);  // 12
const OFF_NORM:  usize  = 5 * @sizeOf(f32);  // 20
const OFF_LIGHT: usize  = 8 * @sizeOf(f32);  // 32

// ─── Shaders GLSL 1.20 ───────────────────────────────────────────────────────
// OBLIGATORIO: attribute/varying/texture2D/gl_FragColor.
// "in"/"out"/texture() son GLSL 1.30+ — no disponibles en OpenGL 2.1.

const VERT_SRC: []const u8 =
    \\#version 120
    \\attribute vec3 a_position;
    \\attribute vec2 a_texcoord;
    \\attribute vec3 a_normal;
    \\attribute vec3 a_lightColor;
    \\varying vec2 v_texcoord;
    \\varying vec3 v_lightColor;
    \\uniform mat4 u_projection;
    \\uniform mat4 u_modelview;
    \\void main() {
    \\    v_texcoord   = a_texcoord;
    \\    v_lightColor = a_lightColor;
    \\    gl_Position  = u_projection * u_modelview * vec4(a_position, 1.0);
    \\}
;

const FRAG_SRC: []const u8 =
    \\#version 120
    \\varying vec2 v_texcoord;
    \\varying vec3 v_lightColor;
    \\uniform sampler2D u_diffuse;
    \\void main() {
    \\    vec4 tex     = texture2D(u_diffuse, v_texcoord);
    \\    gl_FragColor = vec4(tex.rgb * v_lightColor, tex.a);
    \\}
;

// ─── Geometría del cubo ───────────────────────────────────────────────────────
// Cubo 1×1×1, 36 vértices (6 caras × 2 tri × 3 vértices).
// Formato por vértice: pos.xyz  uv.st  normal.xyz  lightColor.xyz  (11 f32)
// lightColor simula iluminación bakeada offline — cero coste en runtime.
const CUBE_VERTS = [_]f32{
    // FRONT  (z=+0.5, luz 0.9)
    -0.5, -0.5,  0.5,   0.0, 0.0,   0.0, 0.0, 1.0,   0.9, 0.9, 0.9,
     0.5, -0.5,  0.5,   1.0, 0.0,   0.0, 0.0, 1.0,   0.9, 0.9, 0.9,
     0.5,  0.5,  0.5,   1.0, 1.0,   0.0, 0.0, 1.0,   0.9, 0.9, 0.9,
    -0.5, -0.5,  0.5,   0.0, 0.0,   0.0, 0.0, 1.0,   0.9, 0.9, 0.9,
     0.5,  0.5,  0.5,   1.0, 1.0,   0.0, 0.0, 1.0,   0.9, 0.9, 0.9,
    -0.5,  0.5,  0.5,   0.0, 1.0,   0.0, 0.0, 1.0,   0.9, 0.9, 0.9,
    // BACK   (z=-0.5, luz 0.4)
     0.5, -0.5, -0.5,   0.0, 0.0,   0.0, 0.0,-1.0,   0.4, 0.4, 0.4,
    -0.5, -0.5, -0.5,   1.0, 0.0,   0.0, 0.0,-1.0,   0.4, 0.4, 0.4,
    -0.5,  0.5, -0.5,   1.0, 1.0,   0.0, 0.0,-1.0,   0.4, 0.4, 0.4,
     0.5, -0.5, -0.5,   0.0, 0.0,   0.0, 0.0,-1.0,   0.4, 0.4, 0.4,
    -0.5,  0.5, -0.5,   1.0, 1.0,   0.0, 0.0,-1.0,   0.4, 0.4, 0.4,
     0.5,  0.5, -0.5,   0.0, 1.0,   0.0, 0.0,-1.0,   0.4, 0.4, 0.4,
    // RIGHT  (x=+0.5, luz 0.7)
     0.5, -0.5,  0.5,   0.0, 0.0,   1.0, 0.0, 0.0,   0.7, 0.7, 0.7,
     0.5, -0.5, -0.5,   1.0, 0.0,   1.0, 0.0, 0.0,   0.7, 0.7, 0.7,
     0.5,  0.5, -0.5,   1.0, 1.0,   1.0, 0.0, 0.0,   0.7, 0.7, 0.7,
     0.5, -0.5,  0.5,   0.0, 0.0,   1.0, 0.0, 0.0,   0.7, 0.7, 0.7,
     0.5,  0.5, -0.5,   1.0, 1.0,   1.0, 0.0, 0.0,   0.7, 0.7, 0.7,
     0.5,  0.5,  0.5,   0.0, 1.0,   1.0, 0.0, 0.0,   0.7, 0.7, 0.7,
    // LEFT   (x=-0.5, luz 0.6)
    -0.5, -0.5, -0.5,   0.0, 0.0,  -1.0, 0.0, 0.0,   0.6, 0.6, 0.6,
    -0.5, -0.5,  0.5,   1.0, 0.0,  -1.0, 0.0, 0.0,   0.6, 0.6, 0.6,
    -0.5,  0.5,  0.5,   1.0, 1.0,  -1.0, 0.0, 0.0,   0.6, 0.6, 0.6,
    -0.5, -0.5, -0.5,   0.0, 0.0,  -1.0, 0.0, 0.0,   0.6, 0.6, 0.6,
    -0.5,  0.5,  0.5,   1.0, 1.0,  -1.0, 0.0, 0.0,   0.6, 0.6, 0.6,
    -0.5,  0.5, -0.5,   0.0, 1.0,  -1.0, 0.0, 0.0,   0.6, 0.6, 0.6,
    // TOP    (y=+0.5, luz 1.0)
    -0.5,  0.5,  0.5,   0.0, 0.0,   0.0, 1.0, 0.0,   1.0, 1.0, 1.0,
     0.5,  0.5,  0.5,   1.0, 0.0,   0.0, 1.0, 0.0,   1.0, 1.0, 1.0,
     0.5,  0.5, -0.5,   1.0, 1.0,   0.0, 1.0, 0.0,   1.0, 1.0, 1.0,
    -0.5,  0.5,  0.5,   0.0, 0.0,   0.0, 1.0, 0.0,   1.0, 1.0, 1.0,
     0.5,  0.5, -0.5,   1.0, 1.0,   0.0, 1.0, 0.0,   1.0, 1.0, 1.0,
    -0.5,  0.5, -0.5,   0.0, 1.0,   0.0, 1.0, 0.0,   1.0, 1.0, 1.0,
    // BOTTOM (y=-0.5, luz 0.3)
    -0.5, -0.5, -0.5,   0.0, 0.0,   0.0,-1.0, 0.0,   0.3, 0.3, 0.3,
     0.5, -0.5, -0.5,   1.0, 0.0,   0.0,-1.0, 0.0,   0.3, 0.3, 0.3,
     0.5, -0.5,  0.5,   1.0, 1.0,   0.0,-1.0, 0.0,   0.3, 0.3, 0.3,
    -0.5, -0.5, -0.5,   0.0, 0.0,   0.0,-1.0, 0.0,   0.3, 0.3, 0.3,
     0.5, -0.5,  0.5,   1.0, 1.0,   0.0,-1.0, 0.0,   0.3, 0.3, 0.3,
    -0.5, -0.5,  0.5,   0.0, 1.0,   0.0,-1.0, 0.0,   0.3, 0.3, 0.3,
};

// ─── Structs públicos ─────────────────────────────────────────────────────────

/// Input de cámara — rellenado por main.zig en cada tick.
pub const CameraInput = struct {
    adelante:  bool = false,
    atras:     bool = false,
    izquierda: bool = false,
    derecha:   bool = false,
    mouse_dx:  f32  = 0,
    mouse_dy:  f32  = 0,
};

const Camera = struct {
    /// Posición en espacio mundo.
    pos:   [3]f32 = .{ 3.0, 1.0, 0.0 },
    /// Yaw en grados. 0° = mirando +X, 90° = mirando +Z.
    yaw:   f32    = 180.0,
    /// Pitch en grados. Positivo = arriba, negativo = abajo.
    pitch: f32    = -10.0,
};

// ─── Renderer ─────────────────────────────────────────────────────────────────

pub const Renderer = struct {
    vbo:        c.GLuint,
    program:    c.GLuint,
    tex_blanco: c.GLuint,
    loc_proj:   c.GLint,
    loc_mv:     c.GLint,
    loc_diff:   c.GLint,
    loc_pos:    c.GLuint,
    loc_uv:     c.GLuint,
    loc_norm:   c.GLuint,
    loc_light:  c.GLuint,
    proj_mat:   [16]f32,
    camara:     Camera,

    /// Inicializar el renderer. Llamar después de cargarFuncionesGL().
    pub fn init() !Renderer {
        const program    = try gl.crearPrograma(VERT_SRC, FRAG_SRC);
        const vbo        = try gl.subirVBO(f32, &CUBE_VERTS);
        const tex_blanco = try gl.texturaSolida(255, 255, 255, 255);

        const loc_proj  = gl.glGetUniformLocation(program, "u_projection");
        const loc_mv    = gl.glGetUniformLocation(program, "u_modelview");
        const loc_diff  = gl.glGetUniformLocation(program, "u_diffuse");

        const loc_pos_i   = gl.glGetAttribLocation(program, "a_position");
        const loc_uv_i    = gl.glGetAttribLocation(program, "a_texcoord");
        const loc_norm_i  = gl.glGetAttribLocation(program, "a_normal");
        const loc_light_i = gl.glGetAttribLocation(program, "a_lightColor");

        if (loc_pos_i < 0 or loc_uv_i < 0 or loc_norm_i < 0 or loc_light_i < 0) {
            std.log.err("Atributo no encontrado en el shader (pos={} uv={} norm={} light={})",
                .{ loc_pos_i, loc_uv_i, loc_norm_i, loc_light_i });
            return error.AtributoFaltante;
        }

        const proj = calcProj(FOV_H, WIN_W, WIN_H, NEAR_PLANE, FAR_PLANE);

        std.log.info("Renderer: cubo inicializado. Camara en (3,1,0), mirando al origen.", .{});

        return Renderer{
            .vbo        = vbo,
            .program    = program,
            .tex_blanco = tex_blanco,
            .loc_proj   = loc_proj,
            .loc_mv     = loc_mv,
            .loc_diff   = loc_diff,
            .loc_pos    = @intCast(loc_pos_i),
            .loc_uv     = @intCast(loc_uv_i),
            .loc_norm   = @intCast(loc_norm_i),
            .loc_light  = @intCast(loc_light_i),
            .proj_mat   = proj,
            .camara     = .{},
        };
    }

    pub fn deinit(self: *Renderer) void {
        gl.glDeleteBuffers(1, &self.vbo);
        gl.glDeleteProgram(self.program);
        c.glDeleteTextures(1, &self.tex_blanco);
    }

    /// Actualizar la cámara — llamar a 64Hz dentro del tick de lógica.
    /// En Sistema 3 esta función queda reemplazada por pmove para el jugador real.
    pub fn update(self: *Renderer, input: CameraInput, dt: f32) void {
        self.camara.yaw   += input.mouse_dx * SENSIBILIDAD;
        self.camara.pitch -= input.mouse_dy * SENSIBILIDAD;
        self.camara.pitch  = std.math.clamp(self.camara.pitch, -89.0, 89.0);

        const yaw_rad = std.math.degreesToRadians(self.camara.yaw);
        const fw_x    = std.math.cos(yaw_rad);
        const fw_z    = std.math.sin(yaw_rad);
        const ri_x    = -fw_z;
        const ri_z    = fw_x;

        var wx: f32 = 0;
        var wz: f32 = 0;
        if (input.adelante)  { wx += fw_x; wz += fw_z; }
        if (input.atras)     { wx -= fw_x; wz -= fw_z; }
        if (input.derecha)   { wx += ri_x; wz += ri_z; }
        if (input.izquierda) { wx -= ri_x; wz -= ri_z; }

        const len2 = wx * wx + wz * wz;
        if (len2 > 0.0001) {
            const inv = CAM_SPEED * dt / std.math.sqrt(len2);
            self.camara.pos[0] += wx * inv;
            self.camara.pos[2] += wz * inv;
        }
    }

    /// Dibujar un frame. Llamar entre glClear() y SDL_GL_SwapWindow().
    pub fn render(self: *const Renderer) void {
        const mv = calcView(self.camara);

        gl.glUseProgram(self.program);

        gl.glUniformMatrix4fv(
            self.loc_proj, 1, 0,
            @as([*]const c.GLfloat, @ptrCast(&self.proj_mat)),
        );
        gl.glUniformMatrix4fv(
            self.loc_mv, 1, 0,
            @as([*]const c.GLfloat, @ptrCast(&mv)),
        );
        gl.glUniform1i(self.loc_diff, 0);

        c.glBindTexture(c.GL_TEXTURE_2D, self.tex_blanco);
        gl.glBindBuffer(gl.GL_ARRAY_BUFFER, self.vbo);

        gl.glVertexAttribPointer(
            self.loc_pos, 3, c.GL_FLOAT, 0, STRIDE,
            @as(?*const anyopaque, @ptrFromInt(OFF_POS)),
        );
        gl.glEnableVertexAttribArray(self.loc_pos);

        gl.glVertexAttribPointer(
            self.loc_uv, 2, c.GL_FLOAT, 0, STRIDE,
            @as(?*const anyopaque, @ptrFromInt(OFF_UV)),
        );
        gl.glEnableVertexAttribArray(self.loc_uv);

        gl.glVertexAttribPointer(
            self.loc_norm, 3, c.GL_FLOAT, 0, STRIDE,
            @as(?*const anyopaque, @ptrFromInt(OFF_NORM)),
        );
        gl.glEnableVertexAttribArray(self.loc_norm);

        gl.glVertexAttribPointer(
            self.loc_light, 3, c.GL_FLOAT, 0, STRIDE,
            @as(?*const anyopaque, @ptrFromInt(OFF_LIGHT)),
        );
        gl.glEnableVertexAttribArray(self.loc_light);

        c.glDrawArrays(c.GL_TRIANGLES, 0, 36);

        gl.glDisableVertexAttribArray(self.loc_pos);
        gl.glDisableVertexAttribArray(self.loc_uv);
        gl.glDisableVertexAttribArray(self.loc_norm);
        gl.glDisableVertexAttribArray(self.loc_light);
        gl.glBindBuffer(gl.GL_ARRAY_BUFFER, 0);
        gl.glUseProgram(0);
        c.glBindTexture(c.GL_TEXTURE_2D, 0);
    }
};

// ─── Matemáticas de cámara ────────────────────────────────────────────────────

/// Matriz de proyección perspectiva, columna-mayor (para glUniformMatrix4fv).
/// fov_h_deg es el FOV horizontal — igual que cl_fov en CS:GO.
fn calcProj(fov_h_deg: f32, w: f32, h: f32, near: f32, far: f32) [16]f32 {
    const aspect    = w / h;
    const fov_h_rad = std.math.degreesToRadians(fov_h_deg);
    // Convertir FOV horizontal a vertical
    const fov_v_rad = 2.0 * std.math.atan(std.math.tan(fov_h_rad * 0.5) / aspect);
    const f         = 1.0 / std.math.tan(fov_v_rad * 0.5);

    var m: [16]f32 = .{0} ** 16;
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (far + near) / (near - far);
    m[11] = -1.0;
    m[14] = (2.0 * far * near) / (near - far);
    return m;
}

/// Matriz de vista FPS, columna-mayor.
fn calcView(cam: Camera) [16]f32 {
    const yaw   = std.math.degreesToRadians(cam.yaw);
    const pitch = std.math.degreesToRadians(cam.pitch);

    const fx = std.math.cos(pitch) * std.math.cos(yaw);
    const fy = std.math.sin(pitch);
    const fz = std.math.cos(pitch) * std.math.sin(yaw);

    var rx = -fz;
    const ry: f32 = 0.0;
    var rz = fx;
    const r_len = std.math.sqrt(rx * rx + rz * rz);
    if (r_len > 0.0001) {
        rx /= r_len;
        rz /= r_len;
    }

    const ux = ry * fz - rz * fy;
    const uy = rz * fx - rx * fz;
    const uz = rx * fy - ry * fx;

    const ex = cam.pos[0];
    const ey = cam.pos[1];
    const ez = cam.pos[2];

    const dot_r = rx * ex + ry * ey + rz * ez;
    const dot_u = ux * ex + uy * ey + uz * ez;
    const dot_f = fx * ex + fy * ey + fz * ez;

    return .{
        rx,     ux,     -fx,    0.0,
        ry,     uy,     -fy,    0.0,
        rz,     uz,     -fz,    0.0,
        -dot_r, -dot_u, dot_f,  1.0,
    };
}
