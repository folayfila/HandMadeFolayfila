cmake_minimum_required(VERSION 3.20)

project(folayfila C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Put ALL outputs into ../build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Match handmade hero compiler flags
add_compile_options(
    /MTd
    /nologo
    /Gm-
    /GR-
    /EHa-
    /Od
    /Oi
    /WX
    /W4
    /wd4201
    /wd4100
    /wd4505
    /wd4189
    /FC
    /Z7
)

# Handmade Hero defines
add_compile_definitions(
    FOLAYFILA_INTERNAL=1
    FOLAYFILA_SLOW=1
)

#
# GAME DLL
#

add_library(folayfila SHARED
    folayfila.cpp
)

target_link_options(folayfila PRIVATE
    /incremental:no
    /opt:ref
)

# Handmade Hero export
target_link_options(folayfila PRIVATE
    /EXPORT:GameUpdateAndRender
)

# Match handmade hero pdb behavior
set_target_properties(folayfila PROPERTIES
    OUTPUT_NAME "folayfila"
    PDB_NAME "folayfila"
    COMPILE_PDB_NAME "folayfila"
)

#
# PLATFORM EXE
#

add_executable(win32_folayfila
    win32_folayfila.cpp
)

set_target_properties(win32_folayfila PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(win32_folayfila PRIVATE
    user32
    gdi32
    winmm
)

target_link_options(win32_folayfila PRIVATE
    /incremental:no
    /opt:ref
    /subsystem:windows
)

set_target_properties(win32_folayfila PROPERTIES
    OUTPUT_NAME "win32_folayfila"
    PDB_NAME "win32_folayfila"
    COMPILE_PDB_NAME "win32_folayfila"
)