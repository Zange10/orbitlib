cmake_minimum_required(VERSION 4.0)
project(orbitlib C)

set(CMAKE_C_STANDARD 11)

include_directories(./src ./include)

add_library(orbitlib STATIC src/orbitlib.c
        src/celestial.c
        include/orbitlib_celestial.h
        src/orbit.c
        include/orbitlib_orbit.h)
