cmake_minimum_required(VERSION 3.10)
project(MyApp)

find_package(OpenCV REQUIRED)

add_executable(test06012025 main.cpp)
target_link_libraries(test06012025 PRIVATE ${OpenCV_LIBS})
include_directories(${OpenCV_INCLUDE_DIRS})
