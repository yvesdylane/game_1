//
// Created by yves-dylane on 5/2/26.
//

#ifndef GAME1_MAP_H
#define GAME1_MAP_H

#pragma once
#include <SDL2/SDL.h>
#include "../Rendering/Camera.h"
#include "../Rendering/TileRenderer.h"
#include "../World/TileLibrary.h"

constexpr int MAP_WIDTH  = 100;
constexpr int MAP_HEIGHT = 100;
constexpr int TILE_SIZE  = 64;

class Map {
public:
	Map();
	bool init(SDL_Renderer* renderer);

	// Updated signature — takes library + renderer for tile lookup
	void render(SDL_Renderer* renderer, const Camera& camera,
				TileLibrary& library, TileRenderer& tileRenderer);

	void setTile(int layer, int x, int y, int tileID);
	void clearTile(int layer, int x, int y);

	bool save(const std::string& path) const;
	bool load(const std::string& path);

	static constexpr int LAYER_COUNT = 5;

private:
	int tiles[LAYER_COUNT][MAP_HEIGHT][MAP_WIDTH];
};
#endif //GAME1_MAP_H