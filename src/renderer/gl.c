// Copyright (c) 2026 OpenStrike Project
// gl.c is part of OpenStrike.
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

#include "gl.h"
#include <SDL2/SDL.h>
#include <stdio.h>

/* ---- Definicion de los punteros globales ---- */

PFN_glGenBuffers_t               gl_GenBuffers               = NULL;
PFN_glBindBuffer_t               gl_BindBuffer               = NULL;
PFN_glBufferData_t               gl_BufferData               = NULL;
PFN_glDeleteBuffers_t            gl_DeleteBuffers            = NULL;
PFN_glCreateProgram_t            gl_CreateProgram            = NULL;
PFN_glCreateShader_t             gl_CreateShader             = NULL;
PFN_glShaderSource_t             gl_ShaderSource             = NULL;
PFN_glCompileShader_t            gl_CompileShader            = NULL;
PFN_glAttachShader_t             gl_AttachShader             = NULL;
PFN_glLinkProgram_t              gl_LinkProgram              = NULL;
PFN_glUseProgram_t               gl_UseProgram               = NULL;
PFN_glGetShaderiv_t              gl_GetShaderiv              = NULL;
PFN_glGetShaderInfoLog_t         gl_GetShaderInfoLog         = NULL;
PFN_glGetProgramiv_t             gl_GetProgramiv             = NULL;
PFN_glGetProgramInfoLog_t        gl_GetProgramInfoLog        = NULL;
PFN_glVertexAttribPointer_t      gl_VertexAttribPointer      = NULL;
PFN_glEnableVertexAttribArray_t  gl_EnableVertexAttribArray  = NULL;
PFN_glDisableVertexAttribArray_t gl_DisableVertexAttribArray = NULL;
PFN_glGetAttribLocation_t        gl_GetAttribLocation        = NULL;
PFN_glGetUniformLocation_t       gl_GetUniformLocation       = NULL;
PFN_glUniformMatrix4fv_t         gl_UniformMatrix4fv         = NULL;
PFN_glUniform1i_t                gl_Uniform1i                = NULL;
PFN_glUniform3fv_t               gl_Uniform3fv               = NULL;
PFN_glDeleteShader_t             gl_DeleteShader             = NULL;
PFN_glDeleteProgram_t            gl_DeleteProgram            = NULL;
PFN_glActiveTexture_t            gl_ActiveTexture            = NULL;

/* ---- Macro de carga ---- */

#define CARGAR(nombre) \
    gl_##nombre = (PFN_gl##nombre##_t)SDL_GL_GetProcAddress("gl" #nombre); \
    if (!gl_##nombre) { \
        fprintf(stderr, "gl.c: no se pudo cargar gl" #nombre "\n"); \
        return 0; \
    }

/* ---- gl_cargar ---- */
/* IMPORTANTE: llamar solo despues de SDL_GL_CreateContext.
   Si se llama antes, SDL_GL_GetProcAddress devuelve NULL para todo. */

int gl_cargar(void) {
    CARGAR(GenBuffers)
    CARGAR(BindBuffer)
    CARGAR(BufferData)
    CARGAR(DeleteBuffers)
    CARGAR(CreateProgram)
    CARGAR(CreateShader)
    CARGAR(ShaderSource)
    CARGAR(CompileShader)
    CARGAR(AttachShader)
    CARGAR(LinkProgram)
    CARGAR(UseProgram)
    CARGAR(GetShaderiv)
    CARGAR(GetShaderInfoLog)
    CARGAR(GetProgramiv)
    CARGAR(GetProgramInfoLog)
    CARGAR(VertexAttribPointer)
    CARGAR(EnableVertexAttribArray)
    CARGAR(DisableVertexAttribArray)
    CARGAR(GetAttribLocation)
    CARGAR(GetUniformLocation)
    CARGAR(UniformMatrix4fv)
    CARGAR(Uniform1i)
    CARGAR(Uniform3fv)
    CARGAR(DeleteShader)
    CARGAR(DeleteProgram)
    CARGAR(ActiveTexture)
    return 1;
}
