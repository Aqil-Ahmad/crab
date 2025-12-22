# =============================================================================
# Grid Image Editor - Makefile
# =============================================================================
# This Makefile builds the Grid Image Editor with AI Navigation System.
# It compiles all source files and links with SFML, ImGui, and llama.cpp.
#
# Usage:
#   make          - Build the project
#   make run      - Build and run the project
#   make clean    - Remove all build artifacts
#   make check    - Verify all dependencies exist
# =============================================================================

# Compiler and base flags
COMPILER = g++
FLAGS = -std=c++17 -Wno-unused-result -O2

# Debug build (uncomment for debugging)
# FLAGS = -std=c++17 -Wno-unused-result -g -DDEBUG

# Output executable
OUTPUT = bin/grid

# =============================================================================
# INCLUDE PATHS
# =============================================================================

# SFML headers
SFML_INCLUDE = -Idependencies/sfml/include

# ImGui headers
IMGUI_INCLUDE = -Idependencies/imgui

# llama.cpp headers
LLAMA_INCLUDE = -Idependencies/llama.cpp/include \
                -Idependencies/llama.cpp/ggml/include \
                -Idependencies/llama.cpp/common

# JSON parser (nlohmann/json)
JSON_INCLUDE = -Idependencies/json

# Combined include paths
INCLUDE_PATHS = $(SFML_INCLUDE) $(IMGUI_INCLUDE) $(LLAMA_INCLUDE) $(JSON_INCLUDE)

# =============================================================================
# LIBRARY PATHS AND LIBRARIES
# =============================================================================

# SFML library path and libraries
SFML_LIB_PATH = -Ldependencies/sfml/lib
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system -lopengl32 -lsfml-main

# llama.cpp library paths (MSVC Release build)
LLAMA_LIB_PATH = -Ldependencies/llama.cpp/build/src/Release \
                 -Ldependencies/llama.cpp/build/ggml/src/Release \
                 -Ldependencies/llama.cpp/build/common/Release

# llama.cpp libraries to link
LLAMA_LIBS = -lllama -lggml -lggml-base -lggml-cpu -lcommon

# Combined library settings
LIB_PATHS = $(SFML_LIB_PATH) $(LLAMA_LIB_PATH)
LIBS = $(LIB_PATHS) $(SFML_LIBS) $(LLAMA_LIBS)

# =============================================================================
# SOURCE AND OBJECT FILES
# =============================================================================

# Automatically find all source files in src/
SRC_FILES = $(wildcard src/*.cpp)

# Pre-compiled ImGui object files
IMGUI_FILES = $(wildcard dependencies/imgui/lib/*.o)

# Generate object file names from source files
OBJ_FILES = $(SRC_FILES:src/%.cpp=obj/%.o)

# =============================================================================
# DLL FILES (for Windows runtime)
# =============================================================================

# llama.cpp DLLs that need to be copied to bin/
LLAMA_DLLS = dependencies/llama.cpp/build/bin/Release/llama.dll \
             dependencies/llama.cpp/build/bin/Release/ggml.dll \
             dependencies/llama.cpp/build/bin/Release/ggml-base.dll \
             dependencies/llama.cpp/build/bin/Release/ggml-cpu.dll

# =============================================================================
# BUILD RULES
# =============================================================================

# Default target: build and copy DLLs
all: $(OUTPUT) copy_dlls
	@echo "Build complete: $(OUTPUT)"

# Build and run
run: all
	@echo "Running $(OUTPUT)..."
	./$(OUTPUT)

# Link compiled object files into executable
$(OUTPUT): obj bin $(OBJ_FILES) $(IMGUI_FILES)
	@echo "Linking $(OUTPUT)..."
	$(COMPILER) $(OBJ_FILES) $(IMGUI_FILES) -o $(OUTPUT) $(FLAGS) $(INCLUDE_PATHS) $(LIBS)

# Compile source files into object files
obj/%.o: src/%.cpp
	@echo "Compiling $<..."
	$(COMPILER) -c -g $< -o $@ $(FLAGS) $(INCLUDE_PATHS)

# Create obj directory if it doesn't exist
obj:
	@mkdir -p obj

# Create bin directory if it doesn't exist
bin:
	@mkdir -p bin

# =============================================================================
# DLL COPY RULES
# =============================================================================

# Copy required DLLs to bin directory
copy_dlls: bin
	@echo "Copying llama.cpp DLLs to bin/..."
	@cp -f dependencies/llama.cpp/build/bin/Release/llama.dll bin/ 2>/dev/null || echo "  Warning: llama.dll not found"
	@cp -f dependencies/llama.cpp/build/bin/Release/ggml.dll bin/ 2>/dev/null || echo "  Warning: ggml.dll not found"
	@cp -f dependencies/llama.cpp/build/bin/Release/ggml-base.dll bin/ 2>/dev/null || echo "  Warning: ggml-base.dll not found"
	@cp -f dependencies/llama.cpp/build/bin/Release/ggml-cpu.dll bin/ 2>/dev/null || echo "  Warning: ggml-cpu.dll not found"

# =============================================================================
# VERIFICATION RULES
# =============================================================================

# Check that all required dependencies exist
check:
	@echo "=== Checking Dependencies ==="
	@echo ""
	@echo "Checking llama.cpp libraries..."
	@test -f dependencies/llama.cpp/build/src/Release/llama.lib && echo "  [OK] llama.lib" || echo "  [MISSING] llama.lib"
	@test -f dependencies/llama.cpp/build/ggml/src/Release/ggml.lib && echo "  [OK] ggml.lib" || echo "  [MISSING] ggml.lib"
	@test -f dependencies/llama.cpp/build/ggml/src/Release/ggml-base.lib && echo "  [OK] ggml-base.lib" || echo "  [MISSING] ggml-base.lib"
	@test -f dependencies/llama.cpp/build/ggml/src/Release/ggml-cpu.lib && echo "  [OK] ggml-cpu.lib" || echo "  [MISSING] ggml-cpu.lib"
	@test -f dependencies/llama.cpp/build/common/Release/common.lib && echo "  [OK] common.lib" || echo "  [MISSING] common.lib"
	@echo ""
	@echo "Checking llama.cpp DLLs..."
	@test -f dependencies/llama.cpp/build/bin/Release/llama.dll && echo "  [OK] llama.dll" || echo "  [MISSING] llama.dll"
	@test -f dependencies/llama.cpp/build/bin/Release/ggml.dll && echo "  [OK] ggml.dll" || echo "  [MISSING] ggml.dll"
	@test -f dependencies/llama.cpp/build/bin/Release/ggml-base.dll && echo "  [OK] ggml-base.dll" || echo "  [MISSING] ggml-base.dll"
	@test -f dependencies/llama.cpp/build/bin/Release/ggml-cpu.dll && echo "  [OK] ggml-cpu.dll" || echo "  [MISSING] ggml-cpu.dll"
	@echo ""
	@echo "Checking model file..."
	@test -f dependencies/models/Llama-3.2-1B-Instruct-Q4_K_M.gguf && echo "  [OK] Llama model" || echo "  [MISSING] Llama model"
	@echo ""
	@echo "Checking JSON library..."
	@test -f dependencies/json/json.hpp && echo "  [OK] json.hpp" || echo "  [MISSING] json.hpp"
	@echo ""
	@echo "=== Check Complete ==="

# =============================================================================
# CLEAN RULES
# =============================================================================

# Remove all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf obj $(OUTPUT)
	@echo "Clean complete."

# Deep clean (also removes copied DLLs)
cleanall: clean
	@echo "Removing DLLs from bin/..."
	rm -f bin/*.dll
	@echo "Deep clean complete."

# =============================================================================
# PHONY TARGETS
# =============================================================================

.PHONY: all run clean cleanall copy_dlls check

# =============================================================================
# TROUBLESHOOTING
# =============================================================================
#
# If you get "undefined reference to llama_*":
#   1. Run 'make check' to verify libraries exist
#   2. Ensure llama.cpp was built with: cmake --build build --config Release
#   3. Check that library paths match your llama.cpp build location
#
# If you get "llama.dll not found" at runtime:
#   1. Run 'make copy_dlls' to copy DLLs to bin/
#   2. Or add llama.cpp/build/bin/Release to your PATH
#
# If you get linker errors about missing symbols:
#   1. The llama.cpp version may have changed
#   2. Rebuild llama.cpp and update library paths as needed
#
# =============================================================================