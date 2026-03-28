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

#include "map/map.h"   /* MapVertex tiene layout identico al Vertex interno */

/* Inicializa GL, compila shaders, crea textura checkerboard.
   NO crea el VBO — llamar renderer_cargar_mapa() despues.
   Retorna 1 OK, 0 fallo. Ejecutar desde raiz del repo. */
int  renderer_init(void);

/* Sube la geometria del mapa al VBO. Crea el VBO si no existe.
   Retorna 1 OK, 0 fallo.
   DEBE llamarse despues de renderer_init() y de map_cargar(). */
int  renderer_cargar_mapa(const Map *map);

/* Libera VBO, shader y textura. */
void renderer_shutdown(void);

/* Guarda posicion y angulos para el siguiente draw. */
void renderer_set_camera(float x, float y, float z, float yaw, float pitch);

/* glClear + calcula view matrix + dibuja el mapa.
   alpha reservado para interpolacion futura. */
void renderer_draw_frame(float alpha);

#endif /* OPENSTRIKE_RENDERER_H */
