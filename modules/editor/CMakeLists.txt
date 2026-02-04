# BreadEditor - Game Editor Application
cmake_policy(VERSION 3.5)
project(BreadEditor_App)
include(FetchContent)

file(GLOB_RECURSE EDITOR_SOURCES
        "*.cpp"
        ${CMAKE_SOURCE_DIR}/lib/editor/*.c
)

file(GLOB_RECURSE EDITOR_HEADERS
        "*.h"
        "*.hpp"
)

# Create executable
add_executable(BreadEditor
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
    ${RAYGUI_HEADERS}  # For CLion to index
)

# Set target properties
set_target_properties(BreadEditor PROPERTIES
    OUTPUT_NAME "BreadEditor"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(BreadEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/editor
    ${CMAKE_SOURCE_DIR}/lib/engine
    ${CMAKE_SOURCE_DIR}/lib/engine/assimp
    ${CMAKE_SOURCE_DIR}/modules/engine
)

target_link_libraries(BreadEditor PUBLIC
        BreadEngine
        ${PLATFORM_LIBS}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(BreadEditor PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
    )
endif()

# Create source groups for CLion
source_group("Editor" FILES ${EDITOR_SOURCES} ${EDITOR_HEADERS})
source_group("External\\RayGUI" FILES ${RAYGUI_HEADERS})

# Set working directory for CLion run configurations
set_target_properties(BreadEditor PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(TARGET BreadEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/engine/r3d.dll"
        "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/engine/raylib.dll"
        "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/engine/assimp/assimp-vc143-mt.dll"
        "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/lib/libc++.dll"
        "${CMAKE_BINARY_DIR}/bin"
)

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG master
)
FetchContent_MakeAvailable(yaml-cpp)

target_link_libraries(BreadEditor PUBLIC yaml-cpp::yaml-cpp)

message(STATUS "BreadEditor executable configured")
message(STATUS "${CMAKE_BINARY_DIR}/bin")
message(STATUS "  Working directory will be: ${CMAKE_SOURCE_DIR}")
