# ExampleGame - Sample Game Application
project(ExampleGame_App)

# Collect game source files
file(GLOB_RECURSE GAME_SOURCES
        "*.cpp"
)

file(GLOB_RECURSE GAME_HEADERS
        "*.h"
)

# Create executable
add_executable(ExampleGame
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

# Set target properties
set_target_properties(ExampleGame PROPERTIES
    OUTPUT_NAME "ExampleGame"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(ExampleGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/engine
)

# Link with BreadEngine and system libraries
target_link_libraries(ExampleGame
    BreadEngine
    ${PLATFORM_LIBS}
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(ExampleGame PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
    )
endif()

# Create source groups for CLion
source_group("Game" FILES ${GAME_SOURCES} ${GAME_HEADERS})

# Set working directory for CLion run configurations
set_target_properties(ExampleGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

message(STATUS "ExampleGame executable configured")
message(STATUS "  Working directory will be: ${CMAKE_SOURCE_DIR}")
