cmake_minimum_required(VERSION 3.5)

project(app_yuvtoh264 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app_yuvtoh264 
    main.cpp)
    
target_link_libraries(app_yuvtoh264 "${PROJECT_SOURCE_DIR}/libs/libx264.a")

