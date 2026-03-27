// Copyright (c) 2026 OpenStrike Project
// renderer.h is part of OpenStrike.
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

#ifndef OPENSTRIKE_RENDERER_H
#define OPENSTRIKE_RENDERER_H

#include "math.h"

/* Inicializar renderer: cargar GL, compilar shaders, subir geometria y textura.
   Llamar despues de SDL_GL_CreateContext y el setup basico de GL en main.c.
   Devuelve 1 en exito, 0 en fallo. */
int  renderer_init(void);

/* Liberar todos los recursos GL del renderer. */
void renderer_shutdown(void);

/* Fijar la camara para el siguiente frame.
   x,y,z  -> posicion en unidades Hammer.
   yaw    -> giro horizontal en grados (0 = mirando -Z).
   pitch  -> inclinacion vertical en grados (+90 = arriba, -90 = abajo). */
void renderer_set_camera(float x, float y, float z, float yaw, float pitch);

/* Dibujar el frame completo (incluye glClear).
   alpha: fraccion del tick actual para interpolacion (0.0 - 1.0).
          Reservado para Sistema 3+; de momento se ignora. */
void renderer_draw_frame(float alpha);

#endif /* OPENSTRIKE_RENDERER_H */
