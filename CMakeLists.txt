cmake_minimum_required(VERSION 3.4...3.14)
project(BreadEngine VERSION 1.0.0 LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# CLion-specific optimizations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")


if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    set(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
    message(STATUS "Configuring for macOS")
elseif (WIN32)
    set(PLATFORM_LIBS opengl32 gdi32 winmm)
    message(STATUS "Configuring for Windows")
    set(ENV{PATH} "C:/mingw64/bin;$ENV{PATH}")
    set(ENV{PATH} "C:/Program Files (x86)/GnuWin32/bin;$ENV{PATH}")
    message(STATUS "Updated PATH for MinGW/GnuWin32: $ENV{PATH}")
elseif (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    set(PLATFORM_LIBS ${GTK3_LIBRARIES} GL m pthread dl rt X11)
    message(STATUS "Configuring for Linux")
endif ()

# NOTE: previously a blanket include_directories() call listing all of the paths below.
# Removed: include_directories() is directory-scoped and leaks into every add_subdirectory()
# added afterward - including third-party vendored trees like DiligentCore, which never asked
# for BreadEngine's include paths. Concretely, lib/engine/assimp/texture.h was shadowing
# DiligentCore's own Graphics/GraphicsEngine/interface/Texture.h (Windows include paths are
# case-insensitive, and assimp's directory came first on the command line), breaking
# DiligentCore's build with an "undeclared identifier 'TextureDesc'" error despite Diligent's
# real Texture.h being just a few -I entries later. Every path here was already redundant with
# an existing target_include_directories() in modules/engine/CMakeLists.txt (PUBLIC, so it
# propagates to BreadEditor/ExampleGame via linking BreadEngine) or modules/editor/CMakeLists.txt
# - removing this only affects add_subdirectory()'d trees that shouldn't have had it anyway.
if (WIN32)
    set(ENV{PATH} "C:/mingw64/bin;$ENV{PATH}")
    set(ENV{PATH} "C:/Program Files (x86)/GnuWin32/bin;$ENV{PATH}")
    message(STATUS "Updated PATH for MinGW/GnuWin32: $ENV{PATH}")
endif ()
find_file(RAYLIB_DLL
        NAMES libraylib.dll
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine
        NO_DEFAULT_PATH
        REQUIRED
)
find_file(ASSIMP_DLL
        NAMES libassimp-6.dll
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine/assimp
        NO_DEFAULT_PATH
        REQUIRED
)
find_file(R3D_DLL
        NAMES libr3d.dll
        PATHS ${CMAKE_SOURCE_DIR}/lib/engine
        NO_DEFAULT_PATH
        REQUIRED
)

if (RAYLIB_DLL)
    message(STATUS "Found Raylib: ${RAYLIB_DLL}")
else ()
    message(FATAL_ERROR "Raylib dll not found in ${CMAKE_SOURCE_DIR}/lib/engine")
endif ()

# Copies the vendored runtime DLLs (r3d/raylib/assimp/libc++) next to a built
# executable. Was duplicated identically in the editor and example_game
# CMakeLists; kept as a single function so new runtime deps (Diligent's) only
# need to be added here once.
function(bread_copy_runtime_dlls target)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/lib/engine/libr3d.dll"
            "${CMAKE_SOURCE_DIR}/lib/engine/libraylib.dll"
            "${CMAKE_SOURCE_DIR}/lib/engine/assimp/libassimp-6.dll"
            "${CMAKE_SOURCE_DIR}/lib/libc++.dll"
            $<TARGET_FILE_DIR:${target}>
    )
endfunction()

# cpptrace (FetchContent'd below) bundles libdwarf, which does a bare find_package(ZLIB)
# (no REQUIRED) to optionally enable compressed-debug-info support - a feature this project
# never uses. DiligentTools (added further down) vendors its own zlib for TextureLoader's PNG
# support, and depending on CMake's version, find_package(ZLIB) can resolve to that local,
# non-exported build target instead of finding nothing - which used to be merely harmless
# before Diligent existed, but now breaks libdwarf two different ways depending on exactly
# when/how it resolves: either its own export(EXPORT libdwarfTargets) fails at generate time
# (target isn't part of any export set), or - seen with CLion's bundled CMake 4.2 specifically -
# it links a bare, pathless "ZLib" token that isn't a real link argument at all, failing at
# the final executable link step with "no such file or directory: 'ZLib'". Both failure modes
# are CMake-version-dependent (add_subdirectory ordering alone was not reliable across
# versions), so force find_package(ZLIB) to deterministically report "not found" for exactly
# this one FetchContent call, restoring libdwarf's pre-Diligent behavior regardless of CMake
# version. Re-enabled immediately after so DiligentTools' own libpng->zlib linkage (which
# needs find_package(ZLIB) to succeed, correctly, against its local vendored target) still works.
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB ON)
add_subdirectory(modules/engine)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB OFF)

# --- DiligentEngine (target RHI for the r3d->Diligent migration, see DILIGENT_MIGRATION.md) ---
# Vendored as git submodules under third_party/, per Diligent's documented integration
# (its CMake uses custom macros/cache vars across the three modules that add_subdirectory
# handles correctly and FetchContent would not). Only GL+Vulkan backends are in scope for
# this project's toolchain (clang++/MSYS2 lacks ATL, so D3D11/D3D12 can't build here; see
# Phase 0 spike notes) - not disabled explicitly since DiligentCore degrades those backends
# to unsupported automatically when ATL is unavailable.
# Nothing links against these targets yet - Phase 1 only proves they build as part of the
# real project (not just the throwaway spikes/ CMakeLists); the seam/renderer that actually
# uses them lands in Phases 2-3.
set(DILIGENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DILIGENT_NO_FORMAT_VALIDATION ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_RENDER_STATE_PACKAGER ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_RADIENT ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/DiligentEngine/DiligentCore)
add_subdirectory(third_party/DiligentEngine/DiligentTools)
add_subdirectory(third_party/DiligentEngine/DiligentFX)

# DiligentCore always defines a -shared DLL variant alongside each -static target we
# actually use. On MinGW-style builds (MINGW_BUILD, correctly detected for this project's
# clang64 toolchain) it links those with LINKER:--version-script=export.map, a GNU-ld-only
# flag this toolchain's linker (lld) rejects. Nothing in this project links the -shared
# variants, so exclude them from the default build instead of fighting the linker for
# libraries we don't need.
foreach (_diligent_shared_target
        Diligent-GraphicsEngineVk-shared
        Diligent-GraphicsEngineOpenGL-shared
        Diligent-Archiver-shared
        Diligent-SuperResolution-shared)
    if (TARGET ${_diligent_shared_target})
        set_target_properties(${_diligent_shared_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif ()
endforeach ()

# DiligentTools' standalone HLSL2GLSLConverter CLI tool isn't something this project uses
# (Diligent's DXC-based cross-compilation, used directly by the engine at runtime, is the
# actual shader path for later phases) and it doesn't build under MinGW/clang64: it calls
# MSVC CRT debug-heap APIs (_CrtSetDbgFlag) that MinGW-w64's CRT doesn't provide.
if (TARGET HLSL2GLSLConverter)
    set_target_properties(HLSL2GLSLConverter PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif ()

add_subdirectory(modules/editor)
add_subdirectory(games/example_game)

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/assets/editor
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/assets/game

        COMMAND ${CMAKE_COMMAND}
        -D SRC_EDITOR=${CMAKE_SOURCE_DIR}/modules/editor/assets
        -D DEST_EDITOR=${CMAKE_BINARY_DIR}/bin/assets/editor
        -D SRC_GAME=${CMAKE_SOURCE_DIR}/games/example_game/assets
        -D DEST_GAME=${CMAKE_BINARY_DIR}/bin/assets/game
        -P ${CMAKE_SOURCE_DIR}/CopyMissingAssets.cmake

        VERBATIM
)

add_dependencies(BreadEditor copy_assets)
add_dependencies(ExampleGame copy_assets)

message(STATUS "Available targets: BreadEngine (lib), BreadEditor (exe), ExampleGame (exe)")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
