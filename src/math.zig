// src/math.zig
// OpenStrike Project — GPL v3, 2026

const std = @import("std");

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
    pub fn longitud(a: Vec3) f32 {
        return std.math.sqrt(a.dot(a));
    }
    pub fn normalizar(a: Vec3) Vec3 {
        const inv = 1.0 / std.math.sqrt(a.dot(a));
        return a.scale(inv);
    }
    pub fn longitudXZ(a: Vec3) f32 {
        return std.math.sqrt(a.x * a.x + a.z * a.z);
    }
    pub fn distancia(a: Vec3, b: Vec3) f32 {
        return a.sub(b).longitud();
    }
};
