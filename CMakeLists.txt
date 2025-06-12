set(CMAKE_TOOLCHAIN_FILE "C:/Users/jeffo_work/Coding/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

cmake_minimum_required(VERSION 3.10)
project(CytoCaricature)

set(CMAKE_CXX_STANDARD 17)

# Add include path
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/imgui
)

# Add source files
add_executable(CytoCaricature
    src/main.cpp
    src/tinyfiledialogs.c
    src/imgui/imgui_impl_glfw.cpp 
    src/imgui/imgui.cpp 
    src/imgui/imgui_demo.cpp 
    src/imgui/imgui_draw.cpp 
    src/imgui/imgui_tables.cpp 
    src/imgui/imgui_widgets.cpp 
    src/imgui/imgui_impl_opengl3.cpp
)

# Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(CytoCaricature ${OpenCV_LIBS})

# GLFW3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(CytoCaricature glfw)

# glad (assuming you installed glad via vcpkg or added manually)
find_package(glad CONFIG REQUIRED)
target_link_libraries(CytoCaricature glad::glad)

# OpenGL is still needed for glad
find_package(OpenGL REQUIRED)
target_link_libraries(CytoCaricature OpenGL::GL)

# ImGui (via vcpkg)
#find_package(imgui CONFIG REQUIRED)
#target_link_libraries(CytoCaricature imgui::imgui)
