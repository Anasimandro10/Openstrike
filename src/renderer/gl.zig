// Copyright (c) 2026 OpenStrike Project
// gl.zig is part of OpenStrike.
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
const sdl = @cImport({ @cInclude("SDL2/SDL.h"); });
const c = @cImport({
    @cInclude("SDL2/SDL_opengl.h");
    @cInclude("stb_image.h");
});

// ------------------------------------------------------------
// Tipos de funciones OpenGL (ajustados a punteros simples)
// ------------------------------------------------------------
const PFNGLGENBUFFERSPROC = *const fn (n: c.GLsizei, buffers: *c.GLuint) callconv(.c) void;
const PFNGLBINDBUFFERPROC = *const fn (target: c.GLenum, buffer: c.GLuint) callconv(.c) void;
const PFNGLBUFFERDATAPROC = *const fn (target: c.GLenum, size: c.GLsizeiptr, data: ?*const anyopaque, usage: c.GLenum) callconv(.c) void;
const PFNGLCREATEPROGRAMPROC = *const fn () callconv(.c) c.GLuint;
const PFNGLCREATESHADERPROC = *const fn (shaderType: c.GLenum) callconv(.c) c.GLuint;
const PFNGLSHADERSOURCEPROC = *const fn (shader: c.GLuint, count: c.GLsizei, strings: [*]const [*]const u8, lengths: ?[*]const c.GLint) callconv(.c) void;
const PFNGLCOMPILESHADERPROC = *const fn (shader: c.GLuint) callconv(.c) void;
const PFNGLATTACHSHADERPROC = *const fn (program: c.GLuint, shader: c.GLuint) callconv(.c) void;
const PFNGLLINKPROGRAMPROC = *const fn (program: c.GLuint) callconv(.c) void;
const PFNGLUSEPROGRAMPROC = *const fn (program: c.GLuint) callconv(.c) void;
const PFNGLGETSHADERIVPROC = *const fn (shader: c.GLuint, pname: c.GLenum, params: *c.GLint) callconv(.c) void;
const PFNGLGETSHADERINFOLOGPROC = *const fn (shader: c.GLuint, bufSize: c.GLsizei, length: ?[*]c.GLsizei, infoLog: [*]u8) callconv(.c) void;
const PFNGLGETPROGRAMIVPROC = *const fn (program: c.GLuint, pname: c.GLenum, params: *c.GLint) callconv(.c) void;
const PFNGLGETPROGRAMINFOLOGPROC = *const fn (program: c.GLuint, bufSize: c.GLsizei, length: ?[*]c.GLsizei, infoLog: [*]u8) callconv(.c) void;
const PFNGLVERTEXATTRIBPOINTERPROC = *const fn (index: c.GLuint, size: c.GLint, tipo: c.GLenum, normalized: c.GLboolean, stride: c.GLsizei, pointer: ?*const anyopaque) callconv(.c) void;
const PFNGLENABLEVERTEXATTRIBARRAYPROC = *const fn (index: c.GLuint) callconv(.c) void;
const PFNGLGETATTRIBLOCATIONPROC = *const fn (program: c.GLuint, name: [*:0]const u8) callconv(.c) c.GLint;
const PFNGLGETUNIFORMLOCATIONPROC = *const fn (program: c.GLuint, name: [*:0]const u8) callconv(.c) c.GLint;
const PFNGLUNIFORMMATRIX4FVPROC = *const fn (location: c.GLint, count: c.GLsizei, transpose: c.GLboolean, value: [*]const c.GLfloat) callconv(.c) void;
const PFNGLUNIFORM1IPROC = *const fn (location: c.GLint, v0: c.GLint) callconv(.c) void;
const PFNGLUNIFORM3FVPROC = *const fn (location: c.GLint, count: c.GLsizei, value: [*]const c.GLfloat) callconv(.c) void;
const PFNGLDELETEBUFFERSPROC = *const fn (n: c.GLsizei, buffers: *const c.GLuint) callconv(.c) void;
const PFNGLDELETESHADERPROC = *const fn (shader: c.GLuint) callconv(.c) void;
const PFNGLDELETEPROGRAMPROC = *const fn (program: c.GLuint) callconv(.c) void;
const PFNGLACTIVETEXTUREPROC = *const fn (texture: c.GLenum) callconv(.c) void;

// Variables globales con las funciones cargadas
var glGenBuffers: PFNGLGENBUFFERSPROC = undefined;
var glBindBuffer: PFNGLBINDBUFFERPROC = undefined;
var glBufferData: PFNGLBUFFERDATAPROC = undefined;
var glCreateProgram: PFNGLCREATEPROGRAMPROC = undefined;
var glCreateShader: PFNGLCREATESHADERPROC = undefined;
var glShaderSource: PFNGLSHADERSOURCEPROC = undefined;
var glCompileShader: PFNGLCOMPILESHADERPROC = undefined;
var glAttachShader: PFNGLATTACHSHADERPROC = undefined;
var glLinkProgram: PFNGLLINKPROGRAMPROC = undefined;
var glUseProgram: PFNGLUSEPROGRAMPROC = undefined;
var glGetShaderiv: PFNGLGETSHADERIVPROC = undefined;
var glGetShaderInfoLog: PFNGLGETSHADERINFOLOGPROC = undefined;
var glGetProgramiv: PFNGLGETPROGRAMIVPROC = undefined;
var glGetProgramInfoLog: PFNGLGETPROGRAMINFOLOGPROC = undefined;
var glVertexAttribPointer: PFNGLVERTEXATTRIBPOINTERPROC = undefined;
var glEnableVertexAttribArray: PFNGLENABLEVERTEXATTRIBARRAYPROC = undefined;
var glGetAttribLocation: PFNGLGETATTRIBLOCATIONPROC = undefined;
var glGetUniformLocation: PFNGLGETUNIFORMLOCATIONPROC = undefined;
var glUniformMatrix4fv: PFNGLUNIFORMMATRIX4FVPROC = undefined;
var glUniform1i: PFNGLUNIFORM1IPROC = undefined;
var glUniform3fv: PFNGLUNIFORM3FVPROC = undefined;
var glDeleteBuffers: PFNGLDELETEBUFFERSPROC = undefined;
var glDeleteShader: PFNGLDELETESHADERPROC = undefined;
var glDeleteProgram: PFNGLDELETEPROGRAMPROC = undefined;
var glActiveTexture: PFNGLACTIVETEXTUREPROC = undefined;

// ------------------------------------------------------------
// Cargar todas las funciones OpenGL necesarias
// ------------------------------------------------------------
fn cargarFuncion(comptime T: type, nombre: [:0]const u8) !T {
    const ptr = sdl.SDL_GL_GetProcAddress(nombre.ptr) orelse {
        std.log.err("Fallo al cargar función OpenGL: {s}", .{nombre});
        return error.GLExtensionFaltante;
    };
    return @ptrCast(ptr);
}

pub fn cargarExtensiones() !void {
    glGenBuffers = try cargarFuncion(PFNGLGENBUFFERSPROC, "glGenBuffers");
    glBindBuffer = try cargarFuncion(PFNGLBINDBUFFERPROC, "glBindBuffer");
    glBufferData = try cargarFuncion(PFNGLBUFFERDATAPROC, "glBufferData");
    glCreateProgram = try cargarFuncion(PFNGLCREATEPROGRAMPROC, "glCreateProgram");
    glCreateShader = try cargarFuncion(PFNGLCREATESHADERPROC, "glCreateShader");
    glShaderSource = try cargarFuncion(PFNGLSHADERSOURCEPROC, "glShaderSource");
    glCompileShader = try cargarFuncion(PFNGLCOMPILESHADERPROC, "glCompileShader");
    glAttachShader = try cargarFuncion(PFNGLATTACHSHADERPROC, "glAttachShader");
    glLinkProgram = try cargarFuncion(PFNGLLINKPROGRAMPROC, "glLinkProgram");
    glUseProgram = try cargarFuncion(PFNGLUSEPROGRAMPROC, "glUseProgram");
    glGetShaderiv = try cargarFuncion(PFNGLGETSHADERIVPROC, "glGetShaderiv");
    glGetShaderInfoLog = try cargarFuncion(PFNGLGETSHADERINFOLOGPROC, "glGetShaderInfoLog");
    glGetProgramiv = try cargarFuncion(PFNGLGETPROGRAMIVPROC, "glGetProgramiv");
    glGetProgramInfoLog = try cargarFuncion(PFNGLGETPROGRAMINFOLOGPROC, "glGetProgramInfoLog");
    glVertexAttribPointer = try cargarFuncion(PFNGLVERTEXATTRIBPOINTERPROC, "glVertexAttribPointer");
    glEnableVertexAttribArray = try cargarFuncion(PFNGLENABLEVERTEXATTRIBARRAYPROC, "glEnableVertexAttribArray");
    glGetAttribLocation = try cargarFuncion(PFNGLGETATTRIBLOCATIONPROC, "glGetAttribLocation");
    glGetUniformLocation = try cargarFuncion(PFNGLGETUNIFORMLOCATIONPROC, "glGetUniformLocation");
    glUniformMatrix4fv = try cargarFuncion(PFNGLUNIFORMMATRIX4FVPROC, "glUniformMatrix4fv");
    glUniform1i = try cargarFuncion(PFNGLUNIFORM1IPROC, "glUniform1i");
    glUniform3fv = try cargarFuncion(PFNGLUNIFORM3FVPROC, "glUniform3fv");
    glDeleteBuffers = try cargarFuncion(PFNGLDELETEBUFFERSPROC, "glDeleteBuffers");
    glDeleteShader = try cargarFuncion(PFNGLDELETESHADERPROC, "glDeleteShader");
    glDeleteProgram = try cargarFuncion(PFNGLDELETEPROGRAMPROC, "glDeleteProgram");
    glActiveTexture = try cargarFuncion(PFNGLACTIVETEXTUREPROC, "glActiveTexture");

    std.log.info("Extensiones OpenGL cargadas correctamente", .{});
}

// ------------------------------------------------------------
// Compilación de shaders
// ------------------------------------------------------------
pub fn compilarShader(tipo: c.GLenum, fuente: []const u8) !c.GLuint {
    const shader = glCreateShader(tipo);
    if (shader == 0) return error.CrearShaderFallo;

    const ptr: [*]const u8 = fuente.ptr;
    const len: c.GLint = @intCast(fuente.len);
    glShaderSource(shader, 1, &ptr, &len);
    glCompileShader(shader);

    var status: c.GLint = 0;
    glGetShaderiv(shader, c.GL_COMPILE_STATUS, &status);
    if (status == 0) {
        var log: [512]u8 = undefined;
        var log_len: c.GLsizei = 0;
        glGetShaderInfoLog(shader, 512, &log_len, &log);
        std.log.err("Error compilando shader:\n{s}", .{log[0..@intCast(log_len)]});
        return error.CompilarShaderFallo;
    }
    return shader;
}

pub fn crearPrograma(vert_src: []const u8, frag_src: []const u8) !c.GLuint {
    const vs = try compilarShader(c.GL_VERTEX_SHADER, vert_src);
    const fs = try compilarShader(c.GL_FRAGMENT_SHADER, frag_src);
    defer glDeleteShader(vs);
    defer glDeleteShader(fs);

    const prog = glCreateProgram();
    if (prog == 0) return error.CrearProgramaFallo;
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    var status: c.GLint = 0;
    glGetProgramiv(prog, c.GL_LINK_STATUS, &status);
    if (status == 0) {
        var log: [512]u8 = undefined;
        var log_len: c.GLsizei = 0;
        glGetProgramInfoLog(prog, 512, &log_len, &log);
        std.log.err("Error linkeando programa:\n{s}", .{log[0..@intCast(log_len)]});
        return error.LinkProgramaFallo;
    }
    return prog;
}

// ------------------------------------------------------------
// Carga de texturas con stb_image
// ------------------------------------------------------------
pub fn cargarTextura(ruta: [*:0]const u8) !c.GLuint {
    var ancho: c_int = 0;
    var alto: c_int = 0;
    var canales: c_int = 0;

    // stb_image espera datos RGB/RGBA, forzamos 4 canales para RGBA
    const pixeles = c.stbi_load(ruta, &ancho, &alto, &canales, 4) orelse {
        std.log.err("stb_image no pudo cargar: {s}", .{ruta});
        return error.TexturaFallo;
    };
    defer _ = c.stbi_image_free(pixeles);

    if (ancho > 512 or alto > 512) {
        std.log.warn("Textura {s} es {d}x{d} — supera el límite 512x512", .{ ruta, ancho, alto });
    }

    var tex_id: c.GLuint = 0;
    c.glGenTextures(1, &tex_id);
    c.glBindTexture(c.GL_TEXTURE_2D, tex_id);

    // Sin filtro bilineal por rendimiento
    c.glTexParameteri(c.GL_TEXTURE_2D, c.GL_TEXTURE_MIN_FILTER, c.GL_NEAREST);
    c.glTexParameteri(c.GL_TEXTURE_2D, c.GL_TEXTURE_MAG_FILTER, c.GL_NEAREST);
    c.glTexParameteri(c.GL_TEXTURE_2D, c.GL_TEXTURE_WRAP_S, c.GL_REPEAT);
    c.glTexParameteri(c.GL_TEXTURE_2D, c.GL_TEXTURE_WRAP_T, c.GL_REPEAT);

    c.glTexImage2D(
        c.GL_TEXTURE_2D, 0, c.GL_RGBA,
        ancho, alto, 0,
        c.GL_RGBA, c.GL_UNSIGNED_BYTE,
        pixeles,
    );

    return tex_id;
}
