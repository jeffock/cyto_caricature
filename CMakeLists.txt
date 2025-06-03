set(CMAKE_TOOLCHAIN_FILE "C:/Users/jeffo_work/Coding/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

cmake_minimum_required(VERSION 3.10)
project(ImageViewer)

set(CMAKE_CXX_STANDARD 17)

# Add include path
include_directories(include)

# Add source files
add_executable(image_viewer
    src/main.cpp
    src/tinyfiledialogs.c
)

# Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(image_viewer ${OpenCV_LIBS})
