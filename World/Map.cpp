#include "Map.h"
#include <iostream>
#include <fstream>

Map::Map() {
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                tiles[l][y][x] = TILE_EMPTY;
}

bool Map::init(SDL_Renderer*) { return true; }

// ── Tile grid ─────────────────────────────────────────────────────────────────

void Map::setTile(int layer, int x, int y, int tileID) {
    if (layer < 0 || layer >= LAYER_COUNT) return;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return;
    tiles[layer][y][x] = tileID;
}

void Map::clearTile(int layer, int x, int y) {
    setTile(layer, x, y, TILE_EMPTY);
}

int Map::getTile(int layer, int x, int y) const {
    if (layer < 0 || layer >= LAYER_COUNT) return TILE_EMPTY;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return TILE_EMPTY;
    return tiles[layer][y][x];
}

// ── Objects ───────────────────────────────────────────────────────────────────

int Map::addObject(const ObjectInstance& obj) {
    objects.push_back(obj);
    return (int)objects.size() - 1;
}

void Map::removeObject(int index) {
    if (index < 0 || index >= (int)objects.size()) return;
    objects.erase(objects.begin() + index);
}

// ── Render ────────────────────────────────────────────────────────────────────

void Map::render(SDL_Renderer* renderer, const Camera& camera,
                 TileLibrary& library, TileRenderer& tileRenderer,
                 int selectedObjectIndex, bool showGrid) { // ← add showGrid

    int startX = std::max(0, (int)(camera.x / TILE_SIZE));
    int startY = std::max(0, (int)(camera.y / TILE_SIZE));
    int endX   = std::min(MAP_WIDTH,  (int)((camera.x + 1280 / camera.zoom) / TILE_SIZE) + 2);
    int endY   = std::min(MAP_HEIGHT, (int)((camera.y + 720  / camera.zoom) / TILE_SIZE) + 2);

    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {

                int screenX = static_cast<int>((x * TILE_SIZE - camera.x) * camera.zoom);
                int screenY = static_cast<int>((y * TILE_SIZE - camera.y) * camera.zoom);

                // ✅ Only draw grid on layer 0 when showGrid is true
                if (layer == 0 && showGrid) {
                    SDL_Rect rect = {
                        screenX, screenY,
                        static_cast<int>(TILE_SIZE * camera.zoom),
                        static_cast<int>(TILE_SIZE * camera.zoom)
                    };
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                }

                int tileID = tiles[layer][y][x];
                if (tileID == TILE_EMPTY) continue;

                const TileDefinition* def = library.getById(tileID);
                if (!def) continue;

                tileRenderer.renderTile(renderer, *def, screenX, screenY, camera.zoom);
            }
        }
    }

    // Objects — unchanged
    for (int i = 0; i < (int)objects.size(); i++) {
        const ObjectInstance& obj = objects[i];

        const TileDefinition* def = library.getById(obj.tileID);
        if (!def) continue;

        int screenX = static_cast<int>((obj.x - camera.x) * camera.zoom);
        int screenY = static_cast<int>((obj.y - camera.y) * camera.zoom);

        int drawW = static_cast<int>(def->srcW * camera.zoom);
        int drawH = static_cast<int>(def->srcH * camera.zoom);
        if (screenX + drawW < 0 || screenX > 1280) continue;
        if (screenY + drawH < 0 || screenY > 720)  continue;

        tileRenderer.renderTile(renderer, *def, screenX, screenY, camera.zoom);

        if (i == selectedObjectIndex) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 60);
            SDL_Rect highlight = {screenX, screenY, drawW, drawH};
            SDL_RenderFillRect(renderer, &highlight);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            SDL_RenderDrawRect(renderer, &highlight);
        }
    }
}

// ── Save ──────────────────────────────────────────────────────────────────────

bool Map::save(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) { std::cout << "Map: failed to save\n"; return false; }

    // Tile grid
    file.write(reinterpret_cast<const char*>(&MAP_WIDTH),   sizeof(int));
    file.write(reinterpret_cast<const char*>(&MAP_HEIGHT),  sizeof(int));
    file.write(reinterpret_cast<const char*>(&LAYER_COUNT), sizeof(int));
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                file.write(reinterpret_cast<const char*>(&tiles[l][y][x]), sizeof(int));

    // Objects
    int objCount = (int)objects.size();
    file.write(reinterpret_cast<const char*>(&objCount), sizeof(int));
    for (const auto& obj : objects) {
        file.write(reinterpret_cast<const char*>(&obj.tileID), sizeof(int));
        file.write(reinterpret_cast<const char*>(&obj.x),      sizeof(float));
        file.write(reinterpret_cast<const char*>(&obj.y),      sizeof(float));
        file.write(reinterpret_cast<const char*>(&obj.layer),  sizeof(int));
    }

    std::cout << "Map saved: " << objCount << " objects\n";
    return true;
}

// ── Load ──────────────────────────────────────────────────────────────────────

bool Map::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { std::cout << "Map: failed to load\n"; return false; }

    int savedW, savedH, savedL;
    file.read(reinterpret_cast<char*>(&savedW), sizeof(int));
    file.read(reinterpret_cast<char*>(&savedH), sizeof(int));
    file.read(reinterpret_cast<char*>(&savedL), sizeof(int));

    if (savedW != MAP_WIDTH || savedH != MAP_HEIGHT || savedL != LAYER_COUNT) {
        std::cout << "Map: dimension mismatch\n";
        return false;
    }

    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                file.read(reinterpret_cast<char*>(&tiles[l][y][x]), sizeof(int));

    objects.clear();
    int objCount = 0;
    file.read(reinterpret_cast<char*>(&objCount), sizeof(int));
    for (int i = 0; i < objCount; i++) {
        ObjectInstance obj;
        file.read(reinterpret_cast<char*>(&obj.tileID), sizeof(int));
        file.read(reinterpret_cast<char*>(&obj.x),      sizeof(float));
        file.read(reinterpret_cast<char*>(&obj.y),      sizeof(float));
        file.read(reinterpret_cast<char*>(&obj.layer),  sizeof(int));
        objects.push_back(obj);
    }

    std::cout << "Map loaded: " << objCount << " objects\n";
    return true;
}