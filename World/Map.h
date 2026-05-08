//
// Created by yves-dylane on 5/2/26.
//

#ifndef GAME1_MAP_H
#define GAME1_MAP_H

#pragma once

#include <SDL2/SDL.h>
#include "../Rendering/Camera.h"
#include "TileSet.h"

const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;
const int TILE_SIZE = 64;

class Map {
public:
	Map();
	bool init(SDL_Renderer* renderer);
	void render(SDL_Renderer* renderer, const Camera& camera);
	void setTile(int layer, int x, int y, int tileID);
	TileSet&  getTileSet();

private:
	static constexpr int LAYER_COUNT = 5;
	int tiles[LAYER_COUNT][MAP_HEIGHT][MAP_WIDTH];
	TileSet tileset;
};


#endif //GAME1_MAP_H