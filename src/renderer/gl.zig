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

const c = @cImport({
    @cInclude("SDL2/SDL.h");
    @cInclude("SDL2/SDL_opengl.h");
    @cInclude("stb_image.h");
});

// ─── Constantes OpenGL 2.1 no incluidas en SDL_opengl.h básico ───────────────

pub const GL_ARRAY_BUFFER: c.GLenum = 0x8892;
pub const GL_STATIC_DRAW: c.GLenum = 0x88B4;
pub const GL_COMPILE_STATUS: c.GLenum = 0x8B81;
pub const GL_LINK_STATUS: c.GLenum = 0x8B82;
pub const GL_VERTEX_SHADER: c.GLenum = 0x8B31;
pub const GL_FRAGMENT_SHADER: c.GLenum = 0x8B30;
pub const GL_INFO_LOG_LENGTH: c.GLenum = 0x8B84;

// ─── Tipos de puntero a función — callconv(.c) obligatorio en 0.14.0 ─────────

const PFNGLGENBUFFERSPROC = *const fn (n: c.GLsizei, buffers: [*]c.GLuint) callconv(.c) void;
const PFNGLBINDBUFFERPROC = *const fn (target: c.GLenum, buffer: c.GLuint) callconv(.c) void;
const PFNGLBUFFERDATAPROC = *const fn (target: c.GLenum, size: c.GLsizeiptr, data: ?*const anyopaque, usage: c.GLenum) callconv(.c) void;
const PFNGLDELETEBUFFERSPROC = *const fn (n: c.GLsizei, buffers: [*]const c.GLuint) callconv(.c) void;
const PFNGLCREATEPROGRAMPROC = *const fn () callconv(.c) c.GLuint;
const PFNGLCREATESHADERPROC = *const fn (shaderType: c.GLenum) callconv(.c) c.GLuint;
const PFNGLSHADERSOURCEPROC = *const fn (shader: c.GLuint, count: c.GLsizei, strings: [*]const [*c]const u8, lengths: ?[*]const c.GLint) callconv(.c) void;
const PFNGLCOMPILESHADERPROC = *const fn (shader: c.GLuint) callconv(.c) void;
const PFNGLATTACHSHADERPROC = *const fn (program: c.GLuint, shader: c.GLuint) callconv(.c) void;
const PFNGLLINKPROGRAMPROC = *const fn (program: c.GLuint) callconv(.c) void;
const PFNGLUSEPROGRAMPROC = *const fn (program: c.GLuint) callconv(.c) void;
const PFNGLGETSHADERIVPROC = *const fn (shader: c.GLuint, pname: c.GLenum, params: [*]c.GLint) callconv(.c) void;
const PFNGLGETSHADERINFOLOGPROC = *const fn (shader: c.GLuint, bufSize: c.GLsizei, length: ?[*]c.GLsizei, infoLog: [*]u8) callconv(.c) void;
const PFNGLGETPROGRAMIVPROC = *const fn (program: c.GLuint, pname: c.GLenum, params: [*]c.GLint) callconv(.c) void;
const PFNGLGETPROGRAMINFOLOGPROC = *const fn (program: c.GLuint, bufSize: c.GLsizei, length: ?[*]c.GLsizei, infoLog: [*]u8) callconv(.c) void;
const PFNGLVERTEXATTRIBPOINTERPROC = *const fn (index: c.GLuint, size: c.GLint, tipo: c.GLenum, normalized: c.GLboolean, stride: c.GLsizei, pointer: ?*const anyopaque) callconv(.c) void;
const PFNGLENABLEVERTEXATTRIBARRAYPROC = *const fn (index: c.GLuint) callconv(.c) void;
const PFNGLDISABLEVERTEXATTRIBARRAYPROC = *const fn (index: c.GLuint) callconv(.c) void;
const PFNGLGETATTRIBLOCATIONPROC = *const fn (program: c.GLuint, name: [*c]const u8) callconv(.c) c.GLint;
const PFNGLGETUNIFORMLOCATIONPROC = *const fn (program: c.GLuint, name: [*c]const u8) callconv(.c) c.GLint;
const PFNGLUNIFORMMATRIX4FVPROC = *const fn (location: c.GLint, count: c.GLsizei, transpose: c.GLboolean, value: [*]const c.GLfloat) callconv(.c) void;
const PFNGLUNIFORM1IPROC = *const fn (location: c.GLint, v0: c.GLint) callconv(.c) void;
const PFNGLUNIFORM3FVPROC = *const fn (location: c.GLint, count: c.GLsizei, value: [*]const c.GLfloat) callconv(.c) void;
const PFNGLDELETESHADERPROC = *const fn (shader: c.GLuint) callconv(.c) void;
const PFNGLDELETEPROGRAMPROC = *const fn (program: c.GLuint) callconv(.c) void;
const PFNGLACTIVETEXTUREPROC = *const fn (texture: c.GLenum) callconv(.c) void;

// ─── Punteros globales ────────────────────────────────────────────────────────

pub var glGenBuffers: PFNGLGENBUFFERSPROC = undefined;
pub var glBindBuffer: PFNGLBINDBUFFERPROC = undefined;
pub var glBufferData: PFNGLBUFFERDATAPROC = undefined;
pub var glDeleteBuffers: PFNGLDELETEBUFFERSPROC = undefined;
pub var glCreateProgram: PFNGLCREATEPROGRAMPROC = undefined;
pub var glCreateShader: PFNGLCREATESHADERPROC = undefined;
pub var glShaderSource: PFNGLSHADERSOURCEPROC = undefined;
pub var glCompileShader: PFNGLCOMPILESHADERPROC = undefined;
pub var glAttachShader: PFNGLATTACHSHADERPROC = undefined;
pub var glLinkProgram: PFNGLLINKPROGRAMPROC = undefined;
pub var glUseProgram: PFNGLUSEPROGRAMPROC = undefined;
pub var glGetShaderiv: PFNGLGETSHADERIVPROC = undefined;
pub var glGetShaderInfoLog: PFNGLGETSHADERINFOLOGPROC = undefined;
pub var glGetProgramiv: PFNGLGETPROGRAMIVPROC = undefined;
pub var glGetProgramInfoLog: PFNGLGETPROGRAMINFOLOGPROC = undefined;
pub var glVertexAttribPointer: PFNGLVERTEXATTRIBPOINTERPROC = undefined;
pub var glEnableVertexAttribArray: PFNGLENABLEVERTEXATTRIBARRAYPROC = undefined;
pub var glDisableVertexAttribArray: PFNGLDISABLEVERTEXATTRIBARRAYPROC = undefined;
pub var glGetAttribLocation: PFNGLGETATTRIBLOCATIONPROC = undefined;
pub var glGetUniformLocation: PFNGLGETUNIFORMLOCATIONPROC = undefined;
pub var glUniformMatrix4fv: PFNGLUNIFORMMATRIX4FVPROC = undefined;
pub var glUniform1i: PFNGLUNIFORM1IPROC = undefined;
pub var glUniform3fv: PFNGLUNIFORM3FVPROC = undefined;
pub var glDeleteShader: PFNGLDELETESHADERPROC = undefined;
pub var glDeleteProgram: PFNGLDELETEPROGRAMPROC = undefined;
pub var glActiveTexture: PFNGLACTIVETEXTUREPROC = undefined;

// ─── Cargador de funciones ────────────────────────────────────────────────────

fn cargarGL(comptime T: type, nombre: [:0]const u8) !T {
    const ptr = c.SDL_GL_GetProcAddress(nombre.ptr) orelse {
        std.log.err("GL: no se pudo cargar '{s}'", .{nombre});
        return error.GLFuncionFaltante;
    };
    return @ptrCast(ptr);
}

pub fn cargarFuncionesGL() !void {
    glGenBuffers = try cargarGL(PFNGLGENBUFFERSPROC, "glGenBuffers");
    glBindBuffer = try cargarGL(PFNGLBINDBUFFERPROC, "glBindBuffer");
    glBufferData = try cargarGL(PFNGLBUFFERDATAPROC, "glBufferData");
    glDeleteBuffers = try cargarGL(PFNGLDELETEBUFFERSPROC, "glDeleteBuffers");
    glCreateProgram = try cargarGL(PFNGLCREATEPROGRAMPROC, "glCreateProgram");
    glCreateShader = try cargarGL(PFNGLCREATESHADERPROC, "glCreateShader");
    glShaderSource = try cargarGL(PFNGLSHADERSOURCEPROC, "glShaderSource");
    glCompileShader = try cargarGL(PFNGLCOMPILESHADERPROC, "glCompileShader");
    glAttachShader = try cargarGL(PFNGLATTACHSHADERPROC, "glAttachShader");
    glLinkProgram = try cargarGL(PFNGLLINKPROGRAMPROC, "glLinkProgram");
    glUseProgram = try cargarGL(PFNGLUSEPROGRAMPROC, "glUseProgram");
    glGetShaderiv = try cargarGL(PFNGLGETSHADERIVPROC, "glGetShaderiv");
    glGetShaderInfoLog = try cargarGL(PFNGLGETSHADERINFOLOGPROC, "glGetShaderInfoLog");
    glGetProgramiv = try cargarGL(PFNGLGETPROGRAMIVPROC, "glGetProgramiv");
    glGetProgramInfoLog = try cargarGL(PFNGLGETPROGRAMINFOLOGPROC, "glGetProgramInfoLog");
    glVertexAttribPointer = try cargarGL(PFNGLVERTEXATTRIBPOINTERPROC, "glVertexAttribPointer");
    glEnableVertexAttribArray = try cargarGL(PFNGLENABLEVERTEXATTRIBARRAYPROC, "glEnableVertexAttribArray");
    glDisableVertexAttribArray = try cargarGL(PFNGLDISABLEVERTEXATTRIBARRAYPROC, "glDisableVertexAttribArray");
    glGetAttribLocation = try cargarGL(PFNGLGETATTRIBLOCATIONPROC, "glGetAttribLocation");
    glGetUniformLocation = try cargarGL(PFNGLGETUNIFORMLOCATIONPROC, "glGetUniformLocation");
    glUniformMatrix4fv = try cargarGL(PFNGLUNIFORMMATRIX4FVPROC, "glUniformMatrix4fv");
    glUniform1i = try cargarGL(PFNGLUNIFORM1IPROC, "glUniform1i");
    glUniform3fv = try cargarGL(PFNGLUNIFORM3FVPROC, "glUniform3fv");
    glDeleteShader = try cargarGL(PFNGLDELETESHADERPROC, "glDeleteShader");
    glDeleteProgram = try cargarGL(PFNGLDELETEPROGRAMPROC, "glDeleteProgram");
    glActiveTexture = try cargarGL(PFNGLACTIVETEXTUREPROC, "glActiveTexture");
    std.log.info("GL: todas las funciones OpenGL 2.1 cargadas", .{});
}

// ─── Shader ───────────────────────────────────────────────────────────────────

fn compilarShader(src: []const u8, tipo: c.GLenum) !c.GLuint {
    const shader = glCreateShader(tipo);
    if (shader == 0) {
        return error.CrearShaderFallo;
    }

    const ptr: [*c]const u8 = src.ptr;
    const len: c.GLint = @intCast(src.len);
    glShaderSource(shader, 1, &ptr, &len);
    glCompileShader(shader);

    var status: c.GLint = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        var log_buf: [1024]u8 = undefined;
        var log_len: c.GLsizei = 0;
        glGetShaderInfoLog(shader, 1024, &log_len, &log_buf);
        std.log.err("Shader compile error:\n{s}", .{log_buf[0..@intCast(log_len)]});
        glDeleteShader(shader);
        return error.CompilarShaderFallo;
    }
    return shader;
}

pub fn crearPrograma(vert_src: []const u8, frag_src: []const u8) !c.GLuint {
    const vs = try compilarShader(vert_src, GL_VERTEX_SHADER);
    const fs = try compilarShader(frag_src, GL_FRAGMENT_SHADER);
    defer glDeleteShader(vs);
    defer glDeleteShader(fs);

    const prog = glCreateProgram();
    if (prog == 0) {
        return error.CrearProgramaFallo;
    }
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    var status: c.GLint = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        var log_buf: [1024]u8 = undefined;
        var log_len: c.GLsizei = 0;
        glGetProgramInfoLog(prog, 1024, &log_len, &log_buf);
        std.log.err("Program link error:\n{s}", .{log_buf[0..@intCast(log_len)]});
        glDeleteProgram(prog);
        return error.LinkProgramaFallo;
    }
    return prog;
}

// ─── VBO ─────────────────────────────────────────────────────────────────────

pub fn subirVBO(comptime T: type, datos: []const T) !c.GLuint {
    var vbo: c.GLuint = 0;
    glGenBuffers(1, &vbo);
    if (vbo == 0) {
        return error.VBOFallo;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        @intCast(@sizeOf(T) * datos.len),
        datos.ptr,
        GL_STATIC_DRAW,
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
}

// ─── Textura ─────────────────────────────────────────────────────────────────

pub const GL_TEXTURE0: c.GLenum = 0x84C0;
pub const GL_TEXTURE1: c.GLenum = 0x84C1;

pub fn cargarTextura(ruta: [*:0]const u8) !c.GLuint {
    var ancho: c_int = 0;
    var alto: c_int = 0;
    var canales: c_int = 0;

    c.stbi_set_flip_vertically_on_load(1);

    const pixeles = c.stbi_load(ruta, &ancho, &alto, &canales, 4) orelse {
        std.log.err("stb_image: no se pudo cargar '{s}'", .{ruta});
        return error.TexturaFallo;
    };
    defer c.stbi_image_free(pixeles);

    if (ancho > 512 or alto > 512) {
        std.log.warn("Textura '{s}' es {d}x{d} — supera el limite 512x512", .{ ruta, ancho, alto });
    }

    var tex_id: c.GLuint = 0;
    c.glGenTextures(1, &tex_id);
    c.glBindTexture(c.GL_TEXTURE_2D, tex_id);
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
    c.glBindTexture(c.GL_TEXTURE_2D, 0);
    return tex_id;
}

pub fn texturaSolida(r: u8, g: u8, b: u8, a: u8) !c.GLuint {
    const pixel = [4]u8{ r, g, b, a };
    var tex_id: c.GLuint = 0;
    c.glGenTextures(1, &tex_id);
    c.glBindTexture(c.GL_TEXTURE_2D, tex_id);
    c.glTexParameteri(c.GL_TEXTURE_2D, c.GL_TEXTURE_MIN_FILTER, c.GL_NEAREST);
    c.glTexParameteri(c.GL_TEXTURE_2D, c.GL_TEXTURE_MAG_FILTER, c.GL_NEAREST);
    c.glTexImage2D(
        c.GL_TEXTURE_2D, 0, c.GL_RGBA,
        1, 1, 0,
        c.GL_RGBA, c.GL_UNSIGNED_BYTE,
        &pixel,
    );
    c.glBindTexture(c.GL_TEXTURE_2D, 0);
    return tex_id;
}
