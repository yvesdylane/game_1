//
// Created by yves-dylane on 5/3/26.
//

#ifndef GAME1_TILESET_H
#define GAME1_TILESET_H


#pragma once

#include <SDL2/SDL.h>
#include <string>

class TileSet {
	public:
		bool load(SDL_Renderer* renderer, const std::string& path, int tileSize);
		void renderTile(SDL_Renderer* renderer, int tileID, int x, int y, float zoom);

		int getTilesPerRow() const;

	private:
		SDL_Texture* texture = nullptr;
		int tileSize = 64;
		int textureWidth = 0;
};


#endif //GAME1_TILESET_H