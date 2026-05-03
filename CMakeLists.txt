cmake_minimum_required(VERSION 4.1)
project(Game1)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)

add_executable(Game1 main.cpp
        Core/Game.cpp
        Core/Game.h
        Rendering/Camera.h
        World/Map.cpp
        World/Map.h
)

target_include_directories(Game1 PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(Game1 ${SDL2_LIBRARIES})
