# BreadEditor - Game Editor Application
project(BreadEditor_App)

# Collect editor source files
file(GLOB_RECURSE EDITOR_SOURCES
        "*.cpp"
        "*/*.cpp"
        "*/*/*.cpp"
)

file(GLOB_RECURSE EDITOR_HEADERS
        "*.h"
        "*/*.h"
        "*/*/*.h"
)

# Add RayGUI header for CLion IntelliSense
set(RAYGUI_HEADERS
    ${CMAKE_SOURCE_DIR}/lib/editor/raygui.h
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
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(BreadEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/editor
    ${CMAKE_SOURCE_DIR}/modules/engine
)

# Link with BreadEngine and system libraries
target_link_libraries(BreadEditor
    BreadEngine
    ${PLATFORM_LIBS}
)

# Compiler-specific options
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

message(STATUS "BreadEditor executable configured")
message(STATUS "  Working directory will be: ${CMAKE_SOURCE_DIR}")
