#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "../Rendering/Camera.h"
#include "../Rendering/TileRenderer.h"
#include "../World/TileLibrary.h"
#include "../World/ObjectInstance.h"
#include "../Editor/LayerState.h"

// Default values for new maps
constexpr int DEFAULT_MAP_WIDTH  = 100;
constexpr int DEFAULT_MAP_HEIGHT = 100;
constexpr int DEFAULT_TILE_SIZE  = 64;

class Map {
public:
    static constexpr int LAYER_COUNT = 5;
    static constexpr int TILE_EMPTY  = -1;

    Map();

    // Initialize with specific dimensions
    bool init(int width, int height, int tileSize);

    // Tile grid access
    void setTile(int layer, int x, int y, int tileID);
    void clearTile(int layer, int x, int y);
    int  getTile(int layer, int x, int y) const;

    // Expand — adds tiles filled with TILE_EMPTY
    void expandRight (int amount = 5);
    void expandLeft  (int amount = 5);
    void expandTop   (int amount = 5);
    void expandBottom(int amount = 5);

    // Shrink — removes tiles from edge (lost tiles gone)
    void shrinkRight (int amount = 5);
    void shrinkLeft  (int amount = 5);
    void shrinkTop   (int amount = 5);
    void shrinkBottom(int amount = 5);

    // Dimensions
    int getWidth()    const { return mapWidth; }
    int getHeight()   const { return mapHeight; }
    int getTileSize() const { return tileSize; }

    // Camera helper — world center of map
    float centerX() const { return (mapWidth  * tileSize) / 2.0f; }
    float centerY() const { return (mapHeight * tileSize) / 2.0f; }

    // Objects
    int  addObject(const ObjectInstance& obj);
    void removeObject(int index);
    void insertObject(int index, const ObjectInstance& obj);
    const std::vector<ObjectInstance>& getObjects() const { return objects; }
    ObjectInstance* getObject(int index);
    const ObjectInstance* getObject(int index) const;

    // Render
    void render(SDL_Renderer* renderer, const Camera& camera,
                TileLibrary& library, TileRenderer& tileRenderer,
                int selectedObjectIndex = -1,
                bool showGrid = true,
                const LayerState* layerStates = nullptr);

    // Save / load
    bool save(const std::string& path) const;
    bool load(const std::string& path);

private:
    // Dynamic tile storage [layer][y][x]
    std::vector<std::vector<std::vector<int>>> tiles;
    std::vector<ObjectInstance> objects;

    int mapWidth  = DEFAULT_MAP_WIDTH;
    int mapHeight = DEFAULT_MAP_HEIGHT;
    int tileSize  = DEFAULT_TILE_SIZE;

    void resizeTiles(); // rebuilds tiles vector to match mapWidth/mapHeight
};