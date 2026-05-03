//
// Created by yves-dylane on 5/2/26.
//

#ifndef GAME1_MAP_H
#define GAME1_MAP_H

#pragma once

#include <SDL2/SDL.h>
#include "../Rendering/Camera.h"

const int MAP_WIDTH = 100;
const int MAP_HEIGHT = 100;
const int TILE_SIZE = 64;

class Map {
public:
	void render(SDL_Renderer* renderer, const Camera& camera);

private:
	int tiles[MAP_HEIGHT][MAP_WIDTH] = {0};
};


#endif //GAME1_MAP_H