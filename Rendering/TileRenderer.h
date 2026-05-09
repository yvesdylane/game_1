//
// Created by yves-dylane on 5/9/26.
//

#ifndef GAME1_TILERENDERER_H
#define GAME1_TILERENDERER_H

// Rendering/TileRenderer.h
#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>
#include "../World/TileDefinition.h"

class TileRenderer {
public:
	~TileRenderer() { clear(); }

	// Pre-load a texture (called when importing an image)
	bool loadTexture(SDL_Renderer* renderer, const std::string& path);

	// Render a TileDefinition at screen position (x, y) with zoom
	void renderTile(
		SDL_Renderer*        renderer,
		const TileDefinition& def,
		int x, int y,
		float zoom,
		Uint8 alpha = 255
	);

	// Render from raw source rect (used by tileset editor grid)
	void renderRaw(
		SDL_Renderer*      renderer,
		const std::string& imagePath,
		SDL_Rect           src,
		int x, int y,
		float zoom,
		Uint8 alpha = 255
	);

	SDL_Texture* getTexture(const std::string& path);

	void clear();

private:
	std::unordered_map<std::string, SDL_Texture*> textures;
};

#endif //GAME1_TILERENDERER_H