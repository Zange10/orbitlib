cmake_minimum_required(VERSION 4.0)
project(orbitlib C)

set(CMAKE_C_STANDARD 11)

include_directories(./src ./include)

# Add the submodule directory
add_subdirectory(external/geometrylib)

add_library(orbitlib STATIC src/orbitlib.c
        src/celestial.c
        include/orbitlib_celestial.h
        src/orbit.c
        include/orbitlib_orbit.h)

target_link_libraries(orbitlib PRIVATE geometrylib)

target_include_directories(orbitlib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/geometrylib/include
)