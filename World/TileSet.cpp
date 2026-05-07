//
// Created by yves-dylane on 5/3/26.
//

#include "TileSet.h"
#include <iostream>
#include "TileSet.h"
#include <SDL2/SDL_image.h>

bool TileSet::load(SDL_Renderer* renderer, const std::string& path, int size) {
	tileSize = size;

	SDL_Surface* surface = IMG_Load(path.c_str());
	if (!surface) {
		std::cout << "Failed to load image: " << path << std::endl;
		std::cout << "SDL_image Error: " << IMG_GetError() << std::endl;
		return false;
	}

	texture = SDL_CreateTextureFromSurface(renderer, surface);
	textureWidth = surface->w;

	SDL_FreeSurface(surface);
	return texture != nullptr;
}

int TileSet::getTilesPerRow() const {
	return textureWidth / tileSize;
}

void TileSet::renderTile(SDL_Renderer* renderer, int tileID, int x, int y, float zoom, Uint8 alpha) {
	if (!texture) return;

	SDL_SetTextureAlphaMod(texture, alpha);

	int tilesPerRow = getTilesPerRow();

	SDL_Rect src;
	src.x = (tileID % tilesPerRow) * tileSize;
	src.y = (tileID / tilesPerRow) * tileSize;
	src.w = tileSize;
	src.h = tileSize;

	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	dst.w = static_cast<int>(tileSize * zoom);
	dst.h = static_cast<int>(tileSize * zoom);

	SDL_RenderCopy(renderer, texture, &src, &dst);

	// IMPORTANT: reset alpha
	SDL_SetTextureAlphaMod(texture, 255);
}