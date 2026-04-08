project(ExampleGame_App)

file(GLOB_RECURSE GAME_SOURCES
        "*.cpp"
        "*.c"
)

file(GLOB_RECURSE GAME_HEADERS
        "*.hpp"
        "*.h"
)

list(REMOVE_ITEM GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_executable(ExampleGame
    main.cpp
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

add_library(ExampleGameLib SHARED
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

set_target_properties(ExampleGameLib PROPERTIES
    OUTPUT_NAME "game"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(ExampleGame PROPERTIES
    OUTPUT_NAME "ExampleGame"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(ExampleGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/engine
)

target_include_directories(ExampleGameLib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/engine
)

target_link_libraries(ExampleGame
    BreadEngine
    ${PLATFORM_LIBS}
)

target_link_libraries(ExampleGameLib
    BreadEngine
    ${PLATFORM_LIBS}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(ExampleGame PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
    )
endif()

source_group("Game" FILES ${GAME_SOURCES} ${GAME_HEADERS})
set_target_properties(ExampleGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(TARGET ExampleGame POST_BUILD
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

message(STATUS "ExampleGame executable configured")
message(STATUS "  Working directory will be: ${CMAKE_SOURCE_DIR}")
