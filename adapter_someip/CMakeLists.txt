cmake_minimum_required(VERSION 3.16)

project(lidar_adapter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vsomeip3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(lidar_adapter
    src/main.cpp
)

target_link_libraries(lidar_adapter
    vsomeip3
    Threads::Threads
)