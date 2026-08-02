cmake_minimum_required(VERSION 3.19)
project(DiligentPOC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep configure fast and avoid dragging in unrelated build machinery
set(DILIGENT_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DILIGENT_NO_FORMAT_VALIDATION ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_ARCHIVER OFF CACHE BOOL "" FORCE)

add_subdirectory(DiligentCore)

message(STATUS "=== Spike backend support summary ===")
message(STATUS "D3D11_SUPPORTED:  ${D3D11_SUPPORTED}")
message(STATUS "D3D12_SUPPORTED:  ${D3D12_SUPPORTED}")
message(STATUS "GL_SUPPORTED:     ${GL_SUPPORTED}")
message(STATUS "VULKAN_SUPPORTED: ${VULKAN_SUPPORTED}")

# --- Coexistence test executable: raylib window + Diligent Vulkan device on the same HWND ---
set(BREADENGINE_LIB_ENGINE "${CMAKE_SOURCE_DIR}/../../lib/engine")

find_file(SPIKE_RAYLIB_DLL
        NAMES libraylib.dll
        PATHS ${BREADENGINE_LIB_ENGINE}
        NO_DEFAULT_PATH
        REQUIRED
)

add_executable(diligent_poc main.cpp)
target_include_directories(diligent_poc PRIVATE
        ${BREADENGINE_LIB_ENGINE}
        DiligentCore/Common/interface
)
target_link_libraries(diligent_poc PRIVATE
        Diligent-GraphicsEngineVk-static
        ${SPIKE_RAYLIB_DLL}
        opengl32 gdi32 winmm
)

add_custom_command(TARGET diligent_poc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPIKE_RAYLIB_DLL} $<TARGET_FILE_DIR:diligent_poc>
)

# --- Spike 0.3: GL-backend context sharing for editor viewport interop ---
add_executable(diligent_poc_gl_interop main_gl_interop.cpp)
target_include_directories(diligent_poc_gl_interop PRIVATE
        ${BREADENGINE_LIB_ENGINE}
        DiligentCore/Common/interface
)
target_link_libraries(diligent_poc_gl_interop PRIVATE
        Diligent-GraphicsEngineOpenGL-static
        ${SPIKE_RAYLIB_DLL}
        opengl32 gdi32 winmm
)
add_custom_command(TARGET diligent_poc_gl_interop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPIKE_RAYLIB_DLL} $<TARGET_FILE_DIR:diligent_poc_gl_interop>
)
