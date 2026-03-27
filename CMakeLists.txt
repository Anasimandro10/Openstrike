cmake_minimum_required(VERSION 3.16)
project(openstrike C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- Fuentes -------------------------------------------------------------
# Sistema 1: solo main.c y stb_image.c
# Sistemas futuros se anaden aqui (ENet, pmove, etc.)
file(GLOB_RECURSE GAME_SOURCES src/*.c)
list(APPEND GAME_SOURCES csrc/stb_image.c)

add_executable(openstrike ${GAME_SOURCES})

# SDL_MAIN_HANDLED: nuestro propio int main() en todas las plataformas
target_compile_definitions(openstrike PRIVATE SDL_MAIN_HANDLED)

target_include_directories(openstrike PRIVATE
    csrc
    src
)

# ---- Flags de compilacion ------------------------------------------------
target_compile_options(openstrike PRIVATE -Wall -Wextra)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(openstrike PRIVATE -fsanitize=address -g)
    target_link_options(openstrike PRIVATE -fsanitize=address)
endif()

# ---- Plataforma ----------------------------------------------------------
if(WIN32)
    # SDL2 vendorizado en deps/windows/SDL2 (ya esta en el repositorio)
    target_include_directories(openstrike PRIVATE
        deps/windows/SDL2/x86_64-w64-mingw32/include
    )
    target_link_directories(openstrike PRIVATE
        deps/windows/SDL2/x86_64-w64-mingw32/lib
    )
    target_link_libraries(openstrike
        SDL2 opengl32
        gdi32 setupapi ole32 oleaut32 imm32 version
        ws2_32 winmm
    )
else()
    find_package(SDL2 REQUIRED)
    target_include_directories(openstrike PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(openstrike ${SDL2_LIBRARIES} GL)
endif()
