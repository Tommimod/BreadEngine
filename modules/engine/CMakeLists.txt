# BreadEngine - Core Engine Library
project(BreadEngine_Core)

# Collect all engine source files
file(GLOB_RECURSE ENGINE_SOURCES
    "*.cpp"
    "*.c"
    "component/*.cpp"
    "ui/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "*.h"
    "*.hpp"
    "component/*.h"
    "ui/*.h"
)

file(GLOB_RECURSE ENGINE_INLINES
        "*.inl"
        "component/*.inl"
        "ui/*.inl"
)

# Add PugiXML sources
set(PUGIXML_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/pugixml/pugixml.cpp
)

set(PUGIXML_HEADERS
    ${CMAKE_SOURCE_DIR}/lib/pugixml/pugixml.hpp
    ${CMAKE_SOURCE_DIR}/lib/pugixml/pugiconfig.hpp
)

# Add Clay renderer
set(CLAY_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/engine/clay_renderer_raylib.c
)

# Add external library headers for CLion IntelliSense
file(GLOB RAYLIB_HEADERS
    ${CMAKE_SOURCE_DIR}/lib/engine/*.h
    ${CMAKE_SOURCE_DIR}/lib/engine/assimp/*.h
    ${CMAKE_SOURCE_DIR}/lib/*.h
)

# Create the static library
add_library(BreadEngine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ${ENGINE_INLINES}
    ${PUGIXML_SOURCES}
    ${PUGIXML_HEADERS}
    ${CLAY_SOURCES}
    ${RAYLIB_HEADERS}
)

# Set target properties
set_target_properties(BreadEngine PROPERTIES
    OUTPUT_NAME "BreadEngine"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Include directories for this target
target_include_directories(BreadEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/
    ${CMAKE_SOURCE_DIR}/lib/engine
    ${CMAKE_SOURCE_DIR}/lib/engine/assimp
    ${CMAKE_SOURCE_DIR}/lib/pugixml
    ${CMAKE_SOURCE_DIR}/modules/engine/component
    ${CMAKE_SOURCE_DIR}/modules/engine/ui
)

# Link libraries
target_link_libraries(BreadEngine PUBLIC
    ${RAYLIB_LIBRARY}
    ${PLATFORM_LIBS}
    ${R3D_DLL}
    ${R3D_LIB}
    ${RAYLIB_DLL}
    ${ASSIMP_DLL}
    ${ASSIMP_LIB}
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(BreadEngine PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
endif()

# Create source groups for CLion project tree
source_group("Engine Core" FILES ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${ENGINE_INLINES})
source_group("Engine\\Component" REGULAR_EXPRESSION "component/.*")
source_group("Engine\\UI" REGULAR_EXPRESSION "ui/.*")
source_group("External\\PugiXML" FILES ${PUGIXML_SOURCES} ${PUGIXML_HEADERS})
source_group("External\\Clay" FILES ${CLAY_SOURCES})
source_group("External\\Raylib" FILES ${RAYLIB_HEADERS})

message(STATUS "BreadEngine library configured")
message(STATUS "  Sources: ${ENGINE_SOURCES}")
message(STATUS "  Headers: ${ENGINE_HEADERS}")
message(STATUS "  Inlines: ${ENGINE_INLINES}")
