#include "Map.h"
#include <iostream>
#include <fstream>
#include <algorithm>

Map::Map() {
    init(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT, DEFAULT_TILE_SIZE);
}

bool Map::init(int width, int height, int ts) {
    mapWidth  = std::max(1, width);
    mapHeight = std::max(1, height);
    tileSize  = std::max(8, ts);
    resizeTiles();
    return true;
}

void Map::resizeTiles() {
    tiles.assign(LAYER_COUNT,
        std::vector<std::vector<int>>(mapHeight,
            std::vector<int>(mapWidth, TILE_EMPTY)));
}

// ── Tile access ───────────────────────────────────────────────────────────────

void Map::setTile(int layer, int x, int y, int tileID) {
    if (layer < 0 || layer >= LAYER_COUNT) return;
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return;
    tiles[layer][y][x] = tileID;
}

void Map::clearTile(int layer, int x, int y) {
    setTile(layer, x, y, TILE_EMPTY);
}

int Map::getTile(int layer, int x, int y) const {
    if (layer < 0 || layer >= LAYER_COUNT) return TILE_EMPTY;
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) return TILE_EMPTY;
    return tiles[layer][y][x];
}

// ── Expand ────────────────────────────────────────────────────────────────────

void Map::expandRight(int amount) {
    mapWidth += amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < mapHeight; y++)
            tiles[l][y].resize(mapWidth, TILE_EMPTY);
}

void Map::expandLeft(int amount) {
    mapWidth += amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < mapHeight; y++)
            tiles[l][y].insert(tiles[l][y].begin(), amount, TILE_EMPTY);

    // Shift all object x positions
    for (auto& obj : objects)
        obj.x += amount * tileSize;
}

void Map::expandBottom(int amount) {
    mapHeight += amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        tiles[l].resize(mapHeight,
            std::vector<int>(mapWidth, TILE_EMPTY));
}

void Map::expandTop(int amount) {
    mapHeight += amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        tiles[l].insert(tiles[l].begin(), amount,
            std::vector<int>(mapWidth, TILE_EMPTY));

    // Shift all object y positions
    for (auto& obj : objects)
        obj.y += amount * tileSize;
}

// ── Shrink ────────────────────────────────────────────────────────────────────

void Map::shrinkRight(int amount) {
    amount = std::min(amount, mapWidth - 1);
    mapWidth -= amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < mapHeight; y++)
            tiles[l][y].resize(mapWidth);
    // Remove objects that fell off
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [&](const ObjectInstance& o) {
            return o.x >= mapWidth * tileSize;
        }), objects.end());
}

void Map::shrinkLeft(int amount) {
    amount = std::min(amount, mapWidth - 1);
    mapWidth -= amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < mapHeight; y++)
            tiles[l][y].erase(tiles[l][y].begin(),
                              tiles[l][y].begin() + amount);
    // Shift objects
    for (auto& obj : objects)
        obj.x -= amount * tileSize;
    // Remove objects that fell off
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [&](const ObjectInstance& o) { return o.x < 0; }), objects.end());
}

void Map::shrinkBottom(int amount) {
    amount = std::min(amount, mapHeight - 1);
    mapHeight -= amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        tiles[l].resize(mapHeight);
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [&](const ObjectInstance& o) {
            return o.y >= mapHeight * tileSize;
        }), objects.end());
}

void Map::shrinkTop(int amount) {
    amount = std::min(amount, mapHeight - 1);
    mapHeight -= amount;
    for (int l = 0; l < LAYER_COUNT; l++)
        tiles[l].erase(tiles[l].begin(), tiles[l].begin() + amount);
    for (auto& obj : objects)
        obj.y -= amount * tileSize;
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [&](const ObjectInstance& o) { return o.y < 0; }), objects.end());
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

void Map::insertObject(int index, const ObjectInstance& obj) {
    if (index < 0 || index > (int)objects.size())
        objects.push_back(obj);
    else
        objects.insert(objects.begin() + index, obj);
}

// ── Render ────────────────────────────────────────────────────────────────────

void Map::render(SDL_Renderer* renderer, const Camera& camera,
                 TileLibrary& library, TileRenderer& tileRenderer,
                 int selectedObjectIndex, bool showGrid,
                 const LayerState* layerStates) {

    // Visible tile range
    int startX = std::max(0, (int)(camera.x / tileSize));
    int startY = std::max(0, (int)(camera.y / tileSize));
    int endX   = std::min(mapWidth,
                    (int)((camera.x + 1280 / camera.zoom) / tileSize) + 2);
    int endY   = std::min(mapHeight,
                    (int)((camera.y + 720  / camera.zoom) / tileSize) + 2);

    // Map boundary outline
    {
        int bx = static_cast<int>(-camera.x * camera.zoom);
        int by = static_cast<int>(-camera.y * camera.zoom);
        int bw = static_cast<int>(mapWidth  * tileSize * camera.zoom);
        int bh = static_cast<int>(mapHeight * tileSize * camera.zoom);
        SDL_Rect boundary = {bx, by, bw, bh};
        SDL_SetRenderDrawColor(renderer, 100, 100, 200, 255);
        SDL_RenderDrawRect(renderer, &boundary);
    }

    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        if (layerStates && !layerStates[layer].visible) continue;

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {

                int screenX = static_cast<int>((x * tileSize - camera.x) * camera.zoom);
                int screenY = static_cast<int>((y * tileSize - camera.y) * camera.zoom);

                if (layer == 0 && showGrid) {
                    SDL_Rect rect = {
                        screenX, screenY,
                        static_cast<int>(tileSize * camera.zoom),
                        static_cast<int>(tileSize * camera.zoom)
                    };
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                }

                int tileID = tiles[layer][y][x];
                if (tileID == TILE_EMPTY) continue;

                const TileDefinition* def = library.getById(tileID);
                if (!def) continue;

                tileRenderer.renderTile(renderer, *def,
                    screenX, screenY, camera.zoom);
            }
        }
    }

    // Objects
    for (int i = 0; i < (int)objects.size(); i++) {
        const ObjectInstance& obj = objects[i];
        const TileDefinition* def = library.getById(obj.tileID);
        if (!def) continue;

        int screenX = static_cast<int>((obj.x - camera.x) * camera.zoom);
        int screenY = static_cast<int>((obj.y - camera.y) * camera.zoom);
        int drawW   = static_cast<int>(def->srcW * camera.zoom);
        int drawH   = static_cast<int>(def->srcH * camera.zoom);

        if (screenX + drawW < 0 || screenX > 1280) continue;
        if (screenY + drawH < 0 || screenY > 720)  continue;

        tileRenderer.renderTile(renderer, *def, screenX, screenY, camera.zoom);

        if (i == selectedObjectIndex) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 60);
            SDL_Rect hl = {screenX, screenY, drawW, drawH};
            SDL_RenderFillRect(renderer, &hl);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            SDL_RenderDrawRect(renderer, &hl);
        }
    }
}

// ── Save ──────────────────────────────────────────────────────────────────────

bool Map::save(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file) { std::cout << "Map: failed to save\n"; return false; }

    // Header
    file.write(reinterpret_cast<const char*>(&mapWidth),   sizeof(int));
    file.write(reinterpret_cast<const char*>(&mapHeight),  sizeof(int));
    file.write(reinterpret_cast<const char*>(&tileSize),   sizeof(int));
    int lc = LAYER_COUNT;
    file.write(reinterpret_cast<const char*>(&lc),         sizeof(int));

    // Tiles
    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < mapHeight; y++)
            for (int x = 0; x < mapWidth; x++)
                file.write(reinterpret_cast<const char*>(
                    &tiles[l][y][x]), sizeof(int));

    // Objects
    int objCount = (int)objects.size();
    file.write(reinterpret_cast<const char*>(&objCount), sizeof(int));
    for (const auto& obj : objects) {
        file.write(reinterpret_cast<const char*>(&obj.tileID), sizeof(int));
        file.write(reinterpret_cast<const char*>(&obj.x),      sizeof(float));
        file.write(reinterpret_cast<const char*>(&obj.y),      sizeof(float));
        file.write(reinterpret_cast<const char*>(&obj.layer),  sizeof(int));
    }

    std::cout << "Map saved: " << mapWidth << "x" << mapHeight
              << " tiles, " << objCount << " objects\n";
    return true;
}

// ── Load ──────────────────────────────────────────────────────────────────────

bool Map::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { std::cout << "Map: failed to load\n"; return false; }

    int savedW, savedH, savedTS, savedL;
    file.read(reinterpret_cast<char*>(&savedW),  sizeof(int));
    file.read(reinterpret_cast<char*>(&savedH),  sizeof(int));
    file.read(reinterpret_cast<char*>(&savedTS), sizeof(int));
    file.read(reinterpret_cast<char*>(&savedL),  sizeof(int));

    if (savedL != LAYER_COUNT) {
        std::cout << "Map: layer count mismatch\n";
        return false;
    }

    mapWidth  = savedW;
    mapHeight = savedH;
    tileSize  = savedTS;
    resizeTiles();

    for (int l = 0; l < LAYER_COUNT; l++)
        for (int y = 0; y < mapHeight; y++)
            for (int x = 0; x < mapWidth; x++)
                file.read(reinterpret_cast<char*>(
                    &tiles[l][y][x]), sizeof(int));

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

    std::cout << "Map loaded: " << mapWidth << "x" << mapHeight
              << " tiles, tileSize=" << tileSize
              << ", " << objCount << " objects\n";
    return true;
}