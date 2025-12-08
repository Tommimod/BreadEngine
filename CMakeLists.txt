cmake_minimum_required(VERSION 3.20)
project(BreadEngine VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable debug symbols for all configurations
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# CLion-specific optimizations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # For better IntelliSense
set(CMAKE_VERBOSE_MAKEFILE ON)         # For detailed build output

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")


# Platform detection and configuration
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    set(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
    message(STATUS "Configuring for macOS")
elseif(WIN32)
    set(PLATFORM_LIBS opengl32 gdi32 winmm)
    message(STATUS "Configuring for Windows")
    set(ENV{PATH} "C:/mingw64/bin;$ENV{PATH}")
    set(ENV{PATH} "C:/Program Files (x86)/GnuWin32/bin;$ENV{PATH}")
    message(STATUS "Updated PATH for MinGW/GnuWin32: $ENV{PATH}")
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    set(PLATFORM_LIBS ${GTK3_LIBRARIES} GL m pthread dl rt X11)
    message(STATUS "Configuring for Linux")
endif()

# Global include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/lib/engine
    ${CMAKE_SOURCE_DIR}/lib/engine/assimp
    ${CMAKE_SOURCE_DIR}/lib/editor
    ${CMAKE_SOURCE_DIR}/lib/pugixml
    ${CMAKE_SOURCE_DIR}/modules/engine
    ${CMAKE_SOURCE_DIR}/modules/editor
)

# Find Raylib
if(WIN32)
    if(WIN32)
        set(ENV{PATH} "C:/mingw64/bin;$ENV{PATH}")
        set(ENV{PATH} "C:/Program Files (x86)/GnuWin32/bin;$ENV{PATH}")
        message(STATUS "Updated PATH for MinGW/GnuWin32: $ENV{PATH}")
    endif()
    # On Windows, use MinGW-compiled raylib
    find_library(RAYLIB_LIBRARY
        NAMES libraylib-win64.a raylib
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine
        NO_DEFAULT_PATH
        REQUIRED
    )
    find_file(RAYLIB_DLL
        NAMES raylib.dll
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine
        NO_DEFAULT_PATH
        REQUIRED
    )
    find_file(ASSIMP_DLL
            NAMES assimp-vc143-mt.dll
            PATHS ${CMAKE_SOURCE_DIR}/lib/engine/assimp
            NO_DEFAULT_PATH
            REQUIRED
    )
    find_library(ASSIMP_LIB
            NAMES assimp-vc143-mt.lib
            PATHS ${CMAKE_SOURCE_DIR}/lib/engine/assimp
            NO_DEFAULT_PATH
            REQUIRED
    )
    find_file(R3D_DLL
        NAMES r3d.dll
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine
        NO_DEFAULT_PATH
        REQUIRED
    )
    find_library(R3D_LIB
            NAMES r3d.lib
            PATHS ${CMAKE_SOURCE_DIR}/lib/engine
            NO_DEFAULT_PATH
            REQUIRED
    )
else()
    # On Unix-like systems, use libraylib.a
    find_library(RAYLIB_LIBRARY
        NAMES raylib libraylib
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine
        NO_DEFAULT_PATH
        REQUIRED
    )
endif()

if(RAYLIB_LIBRARY)
    message(STATUS "Found Raylib: ${RAYLIB_LIBRARY}")
else()
    message(FATAL_ERROR "Raylib library not found in ${CMAKE_SOURCE_DIR}/lib/engine")
endif()

# Add subdirectories
add_subdirectory(modules/engine)
add_subdirectory(modules/editor)
add_subdirectory(games/example_game)

# Custom target for copying assets
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/assets/editor
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/assets/game
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_SOURCE_DIR}/modules/editor/assets 
        ${CMAKE_BINARY_DIR}/bin/assets/editor
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_SOURCE_DIR}/games/example_game/assets 
        ${CMAKE_BINARY_DIR}/bin/assets/game
    COMMENT "Copying assets to build directory"
    VERBATIM
)

# Ensure assets are copied before executables
add_dependencies(BreadEditor copy_assets)
add_dependencies(ExampleGame copy_assets)

# CLion run configurations (will be created automatically)
message(STATUS "BreadEngine configured for CLion")
message(STATUS "Available targets: BreadEngine (lib), BreadEditor (exe), ExampleGame (exe)")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
