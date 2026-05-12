#include "Map.h"
#include <iostream>
#include <fstream>

Map::Map() {
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                tiles[l][y][x] = -1;
}

bool Map::init(SDL_Renderer* renderer) {
    // Nothing to load anymore — tileset is managed by TileLibrary
    return true;
}

void Map::render(SDL_Renderer* renderer, const Camera& camera,
                 TileLibrary& library, TileRenderer& tileRenderer) {

    // Compute visible tile range to avoid drawing off-screen tiles
    int startX = std::max(0, (int)(camera.x / TILE_SIZE));
    int startY = std::max(0, (int)(camera.y / TILE_SIZE));
    int endX   = std::min(MAP_WIDTH,
                    (int)((camera.x + 1280 / camera.zoom) / TILE_SIZE) + 1);
    int endY   = std::min(MAP_HEIGHT,
                    (int)((camera.y + 720  / camera.zoom) / TILE_SIZE) + 1);

    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {

                int screenX = static_cast<int>((x * TILE_SIZE - camera.x) * camera.zoom);
                int screenY = static_cast<int>((y * TILE_SIZE - camera.y) * camera.zoom);

                // Draw grid on layer 0
                if (layer == 0) {
                    SDL_Rect rect = {
                        screenX, screenY,
                        static_cast<int>(TILE_SIZE * camera.zoom),
                        static_cast<int>(TILE_SIZE * camera.zoom)
                    };
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                }

                int tileID = tiles[layer][y][x];
                if (tileID < 0) continue;

                const TileDefinition* def = library.getById(tileID);
                if (!def) continue;

                tileRenderer.renderTile(renderer, *def, screenX, screenY, camera.zoom);
            }
        }
    }
}

void Map::setTile(int layer, int x, int y, int tileID) {
    if (layer < 0 || layer >= LAYER_COUNT) return;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return;
    tiles[layer][y][x] = tileID;
}

void Map::clearTile(int layer, int x, int y) {
    setTile(layer, x, y, -1);
}

bool Map::save(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "Map: failed to save to " << path << "\n";
        return false;
    }
    file.write(reinterpret_cast<const char*>(&MAP_WIDTH),        sizeof(int));
    file.write(reinterpret_cast<const char*>(&MAP_HEIGHT),       sizeof(int));
    file.write(reinterpret_cast<const char*>(&LAYER_COUNT),      sizeof(int));
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                file.write(reinterpret_cast<const char*>(&tiles[l][y][x]), sizeof(int));

    std::cout << "Map saved to: " << path << "\n";
    return true;
}

bool Map::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "Map: failed to load " << path << "\n";
        return false;
    }
    int savedWidth, savedHeight, savedLayers;
    file.read(reinterpret_cast<char*>(&savedWidth),  sizeof(int));
    file.read(reinterpret_cast<char*>(&savedHeight), sizeof(int));
    file.read(reinterpret_cast<char*>(&savedLayers), sizeof(int));

    if (savedWidth != MAP_WIDTH || savedHeight != MAP_HEIGHT || savedLayers != LAYER_COUNT) {
        std::cout << "Map: dimension mismatch on load\n";
        return false;
    }
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                file.read(reinterpret_cast<char*>(&tiles[l][y][x]), sizeof(int));

    std::cout << "Map loaded from: " << path << "\n";
    return true;
}