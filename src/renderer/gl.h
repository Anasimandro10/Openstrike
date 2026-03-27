// Copyright (c) 2026 OpenStrike Project
// gl.h is part of OpenStrike.
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

#ifndef OPENSTRIKE_GL_H
#define OPENSTRIKE_GL_H

#include <SDL2/SDL_opengl.h>

/* APIENTRY puede no estar definido en Linux */
#ifndef APIENTRY
#define APIENTRY
#endif

/* GL_TEXTURE0 (OpenGL 1.3) — incluido en SDL_opengl.h normalmente */
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0U
#endif

/* ---- Tipos de puntero de funcion ---- */

typedef void   (APIENTRY *PFN_glGenBuffers_t)(GLsizei n, GLuint *buffers);
typedef void   (APIENTRY *PFN_glBindBuffer_t)(GLenum target, GLuint buffer);
typedef void   (APIENTRY *PFN_glBufferData_t)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void   (APIENTRY *PFN_glDeleteBuffers_t)(GLsizei n, const GLuint *buffers);
typedef GLuint (APIENTRY *PFN_glCreateProgram_t)(void);
typedef GLuint (APIENTRY *PFN_glCreateShader_t)(GLenum type);
typedef void   (APIENTRY *PFN_glShaderSource_t)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void   (APIENTRY *PFN_glCompileShader_t)(GLuint shader);
typedef void   (APIENTRY *PFN_glAttachShader_t)(GLuint program, GLuint shader);
typedef void   (APIENTRY *PFN_glLinkProgram_t)(GLuint program);
typedef void   (APIENTRY *PFN_glUseProgram_t)(GLuint program);
typedef void   (APIENTRY *PFN_glGetShaderiv_t)(GLuint shader, GLenum pname, GLint *params);
typedef void   (APIENTRY *PFN_glGetShaderInfoLog_t)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   (APIENTRY *PFN_glGetProgramiv_t)(GLuint program, GLenum pname, GLint *params);
typedef void   (APIENTRY *PFN_glGetProgramInfoLog_t)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   (APIENTRY *PFN_glVertexAttribPointer_t)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void   (APIENTRY *PFN_glEnableVertexAttribArray_t)(GLuint index);
typedef void   (APIENTRY *PFN_glDisableVertexAttribArray_t)(GLuint index);
typedef GLint  (APIENTRY *PFN_glGetAttribLocation_t)(GLuint program, const GLchar *name);
typedef GLint  (APIENTRY *PFN_glGetUniformLocation_t)(GLuint program, const GLchar *name);
typedef void   (APIENTRY *PFN_glUniformMatrix4fv_t)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void   (APIENTRY *PFN_glUniform1i_t)(GLint location, GLint v0);
typedef void   (APIENTRY *PFN_glUniform3fv_t)(GLint location, GLsizei count, const GLfloat *value);
typedef void   (APIENTRY *PFN_glDeleteShader_t)(GLuint shader);
typedef void   (APIENTRY *PFN_glDeleteProgram_t)(GLuint program);
typedef void   (APIENTRY *PFN_glActiveTexture_t)(GLenum texture);

/* ---- Punteros globales (definidos en gl.c) ---- */

extern PFN_glGenBuffers_t               gl_GenBuffers;
extern PFN_glBindBuffer_t               gl_BindBuffer;
extern PFN_glBufferData_t               gl_BufferData;
extern PFN_glDeleteBuffers_t            gl_DeleteBuffers;
extern PFN_glCreateProgram_t            gl_CreateProgram;
extern PFN_glCreateShader_t             gl_CreateShader;
extern PFN_glShaderSource_t             gl_ShaderSource;
extern PFN_glCompileShader_t            gl_CompileShader;
extern PFN_glAttachShader_t             gl_AttachShader;
extern PFN_glLinkProgram_t              gl_LinkProgram;
extern PFN_glUseProgram_t               gl_UseProgram;
extern PFN_glGetShaderiv_t              gl_GetShaderiv;
extern PFN_glGetShaderInfoLog_t         gl_GetShaderInfoLog;
extern PFN_glGetProgramiv_t             gl_GetProgramiv;
extern PFN_glGetProgramInfoLog_t        gl_GetProgramInfoLog;
extern PFN_glVertexAttribPointer_t      gl_VertexAttribPointer;
extern PFN_glEnableVertexAttribArray_t  gl_EnableVertexAttribArray;
extern PFN_glDisableVertexAttribArray_t gl_DisableVertexAttribArray;
extern PFN_glGetAttribLocation_t        gl_GetAttribLocation;
extern PFN_glGetUniformLocation_t       gl_GetUniformLocation;
extern PFN_glUniformMatrix4fv_t         gl_UniformMatrix4fv;
extern PFN_glUniform1i_t                gl_Uniform1i;
extern PFN_glUniform3fv_t               gl_Uniform3fv;
extern PFN_glDeleteShader_t             gl_DeleteShader;
extern PFN_glDeleteProgram_t            gl_DeleteProgram;
extern PFN_glActiveTexture_t            gl_ActiveTexture;

/* Carga todos los punteros — llamar inmediatamente despues de SDL_GL_CreateContext */
int gl_cargar(void);

#endif /* OPENSTRIKE_GL_H */
