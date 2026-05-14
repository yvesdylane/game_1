cmake_minimum_required(VERSION 4.1)
project(Game1)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(Game1 main.cpp
        Core/Game.cpp
        Core/Game.h
        Rendering/Camera.h
        World/Map.cpp
        World/Map.h
        World/TileSet.cpp
        World/TileSet.h
        World/WorldConfig.h
        World/TileDefinition.h
        World/TileLibrary.cpp
        World/TileLibrary.h
        Rendering/TileRenderer.h
        Rendering/TileRenderer.cpp
        Rendering/TextRenderer.cpp
        Rendering/TextRenderer.h
        World/ObjectInstance.h
        Editor/MapManager.cpp
        Editor/MapManager.h
        Editor/TilesetManager.cpp
        Editor/TilesetManager.h
        Editor/MenuBar.cpp
        Editor/MenuBar.h
        vendor/tinyfiledialogs/tinyfiledialogs.h
        vendor/tinyfiledialogs/tinyfiledialogs.c
        Editor/FileUtils.h
        Editor/Toolbar.cpp
        Editor/Toolbar.h
)

target_include_directories(Game1 PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        vendor/tinyfiledialogs
)
target_link_libraries(Game1
        SDL2
        SDL2_image
        SDL2_ttf
)
