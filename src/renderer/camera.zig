// Copyright (c) 2026 OpenStrike Project
// camera.zig is part of OpenStrike.
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

// ─── Vec3 ────────────────────────────────────────────────────────────────────

pub const Vec3 = struct {
    x: f32 = 0,
    y: f32 = 0,
    z: f32 = 0,

    pub fn add(a: Vec3, b: Vec3) Vec3 {
        return .{ .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };
    }

    pub fn sub(a: Vec3, b: Vec3) Vec3 {
        return .{ .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z };
    }

    pub fn scale(a: Vec3, s: f32) Vec3 {
        return .{ .x = a.x * s, .y = a.y * s, .z = a.z * s };
    }

    pub fn dot(a: Vec3, b: Vec3) f32 {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    pub fn cross(a: Vec3, b: Vec3) Vec3 {
        return .{
            .x = a.y * b.z - a.z * b.y,
            .y = a.z * b.x - a.x * b.z,
            .z = a.x * b.y - a.y * b.x,
        };
    }

    pub fn normalizar(a: Vec3) Vec3 {
        const len = std.math.sqrt(a.dot(a));
        if (len < 0.0001) {
            return .{ .x = 0, .y = 0, .z = 0 };
        }
        return a.scale(1.0 / len);
    }
};

// ─── Mat4 — columna mayor (OpenGL convention) ────────────────────────────────

pub const Mat4 = struct {
    // m[col][row] — columna mayor para glUniformMatrix4fv
    m: [16]f32,

    pub fn identidad() Mat4 {
        return .{ .m = .{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
        } };
    }

    /// Matriz de perspectiva — equivalente a gluPerspective
    pub fn perspectiva(fov_deg: f32, aspect: f32, near: f32, far: f32) Mat4 {
        const fov_rad = std.math.degreesToRadians(fov_deg);
        const f = 1.0 / std.math.tan(fov_rad * 0.5);
        const rng = 1.0 / (near - far);

        return .{ .m = .{
            f / aspect, 0,                      0,  0,
            0,          f,                      0,  0,
            0,          0,  (near + far) * rng, -1,
            0,          0,  2.0 * near * far * rng, 0,
        } };
    }

    /// Matriz de vista — lookAt desde pos mirando hacia target con up
    pub fn lookAt(pos: Vec3, target: Vec3, up: Vec3) Mat4 {
        const f = target.sub(pos).normalizar();
        const s = f.cross(up).normalizar();
        const u = s.cross(f);

        return .{ .m = .{
            s.x,             u.x,             -f.x,            0,
            s.y,             u.y,             -f.y,            0,
            s.z,             u.z,             -f.z,            0,
            -s.dot(pos),     -u.dot(pos),     f.dot(pos),      1,
        } };
    }
};

// ─── Camara FPS ──────────────────────────────────────────────────────────────

pub const SENSIBILIDAD: f32 = 0.15;

pub const Camera = struct {
    pos: Vec3 = .{ .x = 0, .y = 40, .z = 0 },
    yaw: f32 = 0,
    pitch: f32 = 0,

    pub fn procesarMouse(self: *Camera, dx: f32, dy: f32) void {
        self.yaw += dx * SENSIBILIDAD;
        self.pitch -= dy * SENSIBILIDAD;
        self.pitch = std.math.clamp(self.pitch, -89.0, 89.0);
        self.yaw = @mod(self.yaw, 360.0);
    }

    pub fn forward(self: Camera) Vec3 {
        const y = std.math.degreesToRadians(self.yaw);
        const p = std.math.degreesToRadians(self.pitch);
        return Vec3{
            .x = std.math.cos(p) * std.math.sin(y),
            .y = std.math.sin(p),
            .z = std.math.cos(p) * std.math.cos(y),
        };
    }

    pub fn right(self: Camera) Vec3 {
        const fwd = self.forward();
        const up = Vec3{ .x = 0, .y = 1, .z = 0 };
        return fwd.cross(up).normalizar();
    }

    pub fn matrizVista(self: Camera) Mat4 {
        const fwd = self.forward();
        const target = self.pos.add(fwd);
        const up = Vec3{ .x = 0, .y = 1, .z = 0 };
        return Mat4.lookAt(self.pos, target, up);
    }

    pub fn moverLibre(self: *Camera, adelante: bool, atras: bool, izquierda: bool, derecha: bool, dt: f32) void {
        const VELOCIDAD: f32 = 250.0;
        const fwd = self.forward();
        const fwd_xz = Vec3{ .x = fwd.x, .y = 0, .z = fwd.z };
        const fwd_n = fwd_xz.normalizar();
        const r = self.right();

        if (adelante) {
            self.pos = self.pos.add(fwd_n.scale(VELOCIDAD * dt));
        }
        if (atras) {
            self.pos = self.pos.sub(fwd_n.scale(VELOCIDAD * dt));
        }
        if (derecha) {
            self.pos = self.pos.add(r.scale(VELOCIDAD * dt));
        }
        if (izquierda) {
            self.pos = self.pos.sub(r.scale(VELOCIDAD * dt));
        }
    }
};
