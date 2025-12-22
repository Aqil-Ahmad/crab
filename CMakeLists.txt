cmake_minimum_required(VERSION 3.16)
project(GridImageEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# OPTION: BUILD CONFIGURATION
# =============================================================================

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# =============================================================================
# INCLUDE DIRECTORIES
# =============================================================================

set(SFML_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/sfml/include")
set(IMGUI_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/imgui")
set(LLAMA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/llama.cpp/include")
set(GGML_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/llama.cpp/ggml/include")
set(LLAMA_COMMON_DIR "${PROJECT_SOURCE_DIR}/dependencies/llama.cpp/common")
set(JSON_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/json")

include_directories(
    src
    ${SFML_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${LLAMA_INCLUDE_DIR}
    ${GGML_INCLUDE_DIR}
    ${LLAMA_COMMON_DIR}
    ${JSON_INCLUDE_DIR}
)

# =============================================================================
# SOURCE FILES
# =============================================================================

# Main application sources
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# ImGui sources (compiling from source instead of using pre-compiled objects)
set(IMGUI_SOURCES
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_demo.cpp
    dependencies/imgui/imgui-SFML.cpp
    dependencies/imgui/ImGuiFileDialog.cpp
)

set(ALL_SOURCES ${SRC_FILES} ${IMGUI_SOURCES})

# =============================================================================
# LIBRARIES
# =============================================================================

# SFML
set(SFML_LIB_DIR "${PROJECT_SOURCE_DIR}/dependencies/sfml/lib")
# No link_directories, we use full paths

# llama.cpp (using paths verified in previous step)
set(LLAMA_BUILD_DIR "${PROJECT_SOURCE_DIR}/dependencies/llama.cpp/build")

set(LLAMA_LIBS
    "${LLAMA_BUILD_DIR}/src/Release/llama.lib"
    "${LLAMA_BUILD_DIR}/ggml/src/Release/ggml.lib"
    "${LLAMA_BUILD_DIR}/ggml/src/Release/ggml-base.lib"
    "${LLAMA_BUILD_DIR}/ggml/src/Release/ggml-cpu.lib"
    "${LLAMA_BUILD_DIR}/common/Release/common.lib"
)

# System libs for Windows
set(SYSTEM_LIBS opengl32 winmm gdi32)

# = :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#  TARGETS
# = :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Link SFML and llama.cpp
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${SFML_LIB_DIR}/libsfml-graphics.a"
    "${SFML_LIB_DIR}/libsfml-window.a"
    "${SFML_LIB_DIR}/libsfml-system.a"
    "${SFML_LIB_DIR}/libsfml-main.a"
    ${SYSTEM_LIBS}
    ${LLAMA_LIBS}
)

# =============================================================================
# POST-BUILD: DLL COPYING
# =============================================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LLAMA_BUILD_DIR}/bin/Release/llama.dll"
        "${LLAMA_BUILD_DIR}/bin/Release/ggml.dll"
        "${LLAMA_BUILD_DIR}/bin/Release/ggml-base.dll"
        "${LLAMA_BUILD_DIR}/bin/Release/ggml-cpu.dll"
        "${EXECUTABLE_OUTPUT_PATH}"
    COMMENT "Copying runtime DLLs to output directory..."
)

# Output executable name as grid.exe on Windows
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "grid")

# =============================================================================
# INSTALL (Optional)
# =============================================================================

# =============================================================================
# MESSAGES
# =============================================================================

message(STATUS "Project Source Dir: ${PROJECT_SOURCE_DIR}")
message(STATUS "SFML Include: ${SFML_INCLUDE_DIR}")
message(STATUS "LLAMA Include: ${LLAMA_INCLUDE_DIR}")
