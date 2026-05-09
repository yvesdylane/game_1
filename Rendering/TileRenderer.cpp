//
// Created by yves-dylane on 5/9/26.
//

#include "TileRenderer.h"
#include <SDL2/SDL_image.h>
#include <iostream>

bool TileRenderer::loadTexture(SDL_Renderer* renderer, const std::string& path) {
	if (textures.count(path)) return true; // already loaded

	SDL_Surface* surface = IMG_Load(path.c_str());
	if (!surface) {
		std::cout << "TileRenderer: failed to load " << path
				  << " — " << IMG_GetError() << "\n";
		return false;
	}

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	if (!tex) {
		std::cout << "TileRenderer: texture creation failed\n";
		return false;
	}

	textures[path] = tex;
	return true;
}

SDL_Texture* TileRenderer::getTexture(const std::string& path) {
	auto it = textures.find(path);
	return (it != textures.end()) ? it->second : nullptr;
}

void TileRenderer::renderTile(SDL_Renderer* renderer, const TileDefinition& def,
							   int x, int y, float zoom, Uint8 alpha) {
	SDL_Rect src = { def.srcX, def.srcY, def.srcW, def.srcH };
	renderRaw(renderer, def.imagePath, src, x, y, zoom, alpha);
}

void TileRenderer::renderRaw(SDL_Renderer* renderer, const std::string& imagePath,
							  SDL_Rect src, int x, int y, float zoom, Uint8 alpha) {
	SDL_Texture* tex = getTexture(imagePath);
	if (!tex) return;

	SDL_SetTextureAlphaMod(tex, alpha);

	SDL_Rect dst = {
		x, y,
		static_cast<int>(src.w * zoom),
		static_cast<int>(src.h * zoom)
	};

	SDL_RenderCopy(renderer, tex, &src, &dst);
	SDL_SetTextureAlphaMod(tex, 255);
}

void TileRenderer::clear() {
	for (auto& [path, tex] : textures)
		if (tex) SDL_DestroyTexture(tex);
	textures.clear();
}