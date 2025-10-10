# Bread Engine Modular Build System (Cross-Platform)
OS := $(shell uname -s 2>/dev/null || echo WINDOWS)
ifeq ($(OS),Windows_NT)
    OS = WINDOWS
else ifeq ($(findstring MINGW,$(OS)),MINGW)
    OS = WINDOWS
else ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
    OS = WINDOWS
endif
$(info Detected OS: '$(OS)')

# Compiler and Flags (Conditional)
# macOS: clang++ с фреймворками.
# Linux: g++ с X11/OpenGL libs.
# Windows: g++ с WinAPI libs (адаптировано под raylib).
ifeq ($(OS),Darwin)
    COMPILER = clang++
    RAYLIB_LIB = -L$(LIB_ENGINE_DIR) -lraylib
    LINK_FLAGS = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    SED_INPLACE = -i ''
else ifeq ($(OS),Linux)
    COMPILER = g++
    RAYLIB_LIB = -L$(LIB_ENGINE_DIR) -lraylib
    LINK_FLAGS = -lGL -lGLU -lX11 -lpthread -ldl -lXi -lSM -lICE -lm
    SED_INPLACE = -i
else ifeq ($(OS),WINDOWS)
    COMPILER = clang++
    # Use MinGW-compiled raylib for Windows
    RAYLIB_LIB = $(LIB_ENGINE_DIR)/libraylib-win64.a
    LINK_FLAGS = -lopengl32 -lgdi32 -lwinmm -luser32 -lkernel32
    SED_INPLACE = -i
    PATH := C:/mingw64/bin;$(PATH)
    PATH := "C:/Program Files (x86)/GnuWin32/bin";$(PATH)
    export PATH
endif

SHARED_FLAGS = -fPIC -shared
STD_FLAGS = -std=c++20

# Directories
ENGINE_DIR = modules/Engine
EDITOR_DIR = modules/Editor
GAME_DIR = games/example_game
LIB_ENGINE_DIR = lib/engine
LIB_EDITOR_DIR = lib/editor
LIB_PUGIXML_DIR = lib/pugixml
BIN_DIR = bin
OBJ_DIR = build/obj

# Include paths
INCLUDES = -I$(LIB_ENGINE_DIR) -I$(LIB_EDITOR_DIR) -I$(LIB_PUGIXML_DIR) -I$(ENGINE_DIR) -I$(EDITOR_DIR)

# Object files for incremental compilation
ENGINE_CORE_OBJS = $(OBJ_DIR)/Engine.o $(OBJ_DIR)/ModuleLoader.o
EDITOR_MAIN_OBJS = $(OBJ_DIR)/EditorMain.o
EDITOR_CORE_OBJS = $(OBJ_DIR)/Editor.o
GAME_MAIN_OBJS = $(OBJ_DIR)/GameMain.o
GAME_CORE_OBJS = $(OBJ_DIR)/game.o
PUGIXML_OBJS = $(OBJ_DIR)/pugixml.o

# Conditional Commands
ifeq ($(OS),WINDOWS)
    MKDIR_CMD := if not exist build\obj mkdir build\obj && if not exist bin mkdir bin
else
    MKDIR_CMD := mkdir -p $(OBJ_DIR) && mkdir -p $(BIN_DIR)
endif

ifeq ($(OS),WINDOWS)
    COPY_EDITOR_ASSETS_CMD := if not exist $(BIN_DIR)\assets\editor mkdir $(BIN_DIR)\assets\editor && xcopy /s /q /y "$(EDITOR_DIR)\assets\*" "$(BIN_DIR)\assets\editor\" >nul 2>&1 || exit 0
else
    COPY_EDITOR_ASSETS_CMD := mkdir -p $(BIN_DIR)/assets/editor && cp -r $(EDITOR_DIR)/assets/* $(BIN_DIR)/assets/editor/ 2>/dev/null || true
endif

ifeq ($(OS),WINDOWS)
    COPY_GAME_ASSETS_CMD := if not exist $(BIN_DIR)\assets\game mkdir $(BIN_DIR)\assets\game && xcopy /s /q /y "$(GAME_DIR)\assets\*" "$(BIN_DIR)\assets\game\" >nul 2>&1 || exit 0
else
    COPY_GAME_ASSETS_CMD := mkdir -p $(BIN_DIR)/assets/game && cp -r $(GAME_DIR)/assets/* $(BIN_DIR)/assets/game/ 2>/dev/null || true
endif

ifeq ($(OS),WINDOWS)
    CLEAN_CMD := if exist bin (del /s /q "bin\*" 2>nul && rmdir /s /q bin 2>nul) & if exist build\obj (del /s /q "build\obj\*" 2>nul && rmdir /s /q build\obj 2>nul)
else
    CLEAN_CMD := rm -rf $(BIN_DIR)/* $(OBJ_DIR)
endif

# new_game (Unix)
NEW_GAME_UNIX := read -p "Enter game name: " name; \
                 mkdir -p games/$$name; \
                 cp $(GAME_DIR)/game.h games/$$name/; \
                 cp $(GAME_DIR)/game.cpp games/$$name/; \
                 cp $(GAME_DIR)/main.cpp games/$$name/; \
                 cp -r $(GAME_DIR)/assets games/$$name/; \
                 sed $(SED_INPLACE) "s/Example Game/$$name/g" games/$$name/main.cpp; \
                 sed $(SED_INPLACE) "s/Example Game/$$name/g" games/$$name/assets/README.md; \
                 echo "New game '$$name' created in games/$$name/ with assets"

# new_game (Windows)
NEW_GAME_WIN := @echo Enter game name: & set /p name= & \
                if not exist games\%%name%% mkdir games\%%name%% & \
                copy "$(GAME_DIR)\game.h" "games\%%name%%\" & \
                copy "$(GAME_DIR)\game.cpp" "games\%%name%%\" & \
                copy "$(GAME_DIR)\main.cpp" "games\%%name%%\" & \
                xcopy /s /q /y "$(GAME_DIR)\assets" "games\%%name%%\assets\" >nul 2>&1 & \
                powershell -Command "(Get-Content 'games\%%name%%\main.cpp') -replace 'Example Game', '%%name%%' | Set-Content 'games\%%name%%\main.cpp'" & \
                powershell -Command "(Get-Content 'games\%%name%%\assets\README.md') -replace 'Example Game', '%%name%%' | Set-Content 'games\%%name%%\assets\README.md'" & \
                echo New game '%%name%%' created in games\%%name%%\ with assets

# Build all
all: editor game

# Create directories
$(OBJ_DIR):
	@$(MKDIR_CMD)

# Incremental compilation rules
$(OBJ_DIR)/Engine.o: $(ENGINE_DIR)/Engine.cpp $(ENGINE_DIR)/Engine.h | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

$(OBJ_DIR)/ModuleLoader.o: $(ENGINE_DIR)/ModuleLoader.cpp $(ENGINE_DIR)/ModuleLoader.h | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

$(OBJ_DIR)/Editor.o: $(EDITOR_DIR)/Editor.cpp $(EDITOR_DIR)/Editor.h | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

$(OBJ_DIR)/EditorMain.o: $(EDITOR_DIR)/main.cpp | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

$(OBJ_DIR)/GameMain.o: $(GAME_DIR)/main.cpp | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

$(OBJ_DIR)/game.o: $(GAME_DIR)/game.cpp $(GAME_DIR)/game.h | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

$(OBJ_DIR)/pugixml.o: $(LIB_PUGIXML_DIR)/pugixml.cpp | $(OBJ_DIR)
	$(COMPILER) $(STD_FLAGS) -g -c $< $(INCLUDES) -o $@

# Editor executable (Engine + Editor)
editor: $(ENGINE_CORE_OBJS) $(EDITOR_CORE_OBJS) $(EDITOR_MAIN_OBJS) $(PUGIXML_OBJS)
	@$(MKDIR_CMD)
	$(COMPILER) $(STD_FLAGS) -g $^ $(INCLUDES) \
		$(RAYLIB_LIB) \
		$(LINK_FLAGS) \
		-o $(BIN_DIR)/BreadEditor
	@echo "Copying Editor assets..."
	@$(COPY_EDITOR_ASSETS_CMD)
	@echo "Editor built successfully!"

# Game executable (Engine + Game)
game: $(ENGINE_CORE_OBJS) $(GAME_CORE_OBJS) $(GAME_MAIN_OBJS) $(PUGIXML_OBJS)
	@$(MKDIR_CMD)
	$(COMPILER) $(STD_FLAGS) -g $^ $(INCLUDES) \
		$(RAYLIB_LIB) \
		$(LINK_FLAGS) \
		-o $(BIN_DIR)/ExampleGame
	@echo "Copying Game assets..."
	@$(COPY_GAME_ASSETS_CMD)
	@echo "Game built successfully!"

# Engine as static library (for external projects)
engine_lib: $(ENGINE_CORE_OBJS) $(PUGIXML_OBJS)
	@$(MKDIR_CMD)
	ar rcs $(BIN_DIR)/libBreadEngine.a $^
	@echo "Engine library built successfully!"

# Create new game template
new_game:
ifeq ($(OS),WINDOWS)
	$(NEW_GAME_WIN)
else
	$(NEW_GAME_UNIX)
endif

# Generate compile_commands.json for better IntelliSense
compile_commands:
	@echo "Generating compile_commands.json..."
	@echo '[' > compile_commands.json
	@echo '  {' >> compile_commands.json
	@echo '    "directory": "$(shell pwd)",' >> compile_commands.json
	@echo '    "command": "$(COMPILER) $(STD_FLAGS) -g $(INCLUDES) -c $(ENGINE_DIR)/Engine.cpp",' >> compile_commands.json
	@echo '    "file": "$(ENGINE_DIR)/Engine.cpp"' >> compile_commands.json
	@echo '  },' >> compile_commands.json
	@echo '  {' >> compile_commands.json
	@echo '    "directory": "$(shell pwd)",' >> compile_commands.json
	@echo '    "command": "$(COMPILER) $(STD_FLAGS) -g $(INCLUDES) -c $(EDITOR_DIR)/Editor.cpp",' >> compile_commands.json
	@echo '    "file": "$(EDITOR_DIR)/Editor.cpp"' >> compile_commands.json
	@echo '  }' >> compile_commands.json
	@echo ']' >> compile_commands.json
	@echo "compile_commands.json generated!"

# Clean build artifacts
clean:
	@$(CLEAN_CMD)
	@echo "Cleaned build artifacts and assets!"

# Force rebuild everything
rebuild: clean all

# Show build info
info:
	@echo "Bread Engine Build System (Detected OS: $(OS))"
	@echo "Available targets:"
	@echo "  editor       - Build editor executable"
	@echo "  game         - Build example game executable"
	@echo "  engine_lib   - Build engine as static library"
	@echo "  new_game     - Create new game template"
	@echo "  all          - Build editor and game"
	@echo "  clean        - Clean build artifacts"
	@echo "  rebuild      - Clean and build all"
	@echo "  compile_commands - Generate compile_commands.json"
	@echo "  info         - Show this help"

# Phony targets
.PHONY: all editor game engine_lib new_game clean rebuild compile_commands info

# Default target
.DEFAULT_GOAL := all