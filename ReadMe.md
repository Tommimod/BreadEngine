# BreadEngine

A lightweight C++ game engine based on pure C-library 'Raylib' with built-in editor.
![preview.png](preview.png)

## ---WARNING---

!!!!THIS PROJECT IS NOT READY FOR USE AND IS IN ACTIVE DEVELOPMENT!!!!

## Requirements

- CMake 3.4 or higher
- C++23 compatible compiler
- Platform-specific dependencies:
  - **Windows**: MinGW64, GnuWin32

## Build Instructions

### Only Windows for now

1. Install MinGW64 and add to PATH
2. Install GnuWin32 and add to PATH
3. Clone the repository
4. Create build directory:
   ```bash
   mkdir build
   cd build
   ```
5. Configure with CMake:
   ```bash
   cmake ..
   ```
6. Build:
   ```bash
   cmake --build . --config Debug
   ```

## Output

After successful build, executables will be located in:
- `build/bin/BreadEditor.exe` - Editor application
- `build/bin/ExampleGame.exe` - Example game

## Project Structure

- `modules/engine/` - Core engine library
- `modules/editor/` - Editor application
- `games/example_game/` - Example game implementation
- `lib/` - Third-party libraries