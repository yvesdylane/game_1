//
// Created by yves-dylane on 5/9/26.
//

#include "TextRenderer.h"
#include <iostream>

bool TextRenderer::load(const std::string& fontPath, int fontSize) {
	if (TTF_Init() < 0) {
		std::cout << "TTF_Init failed: " << TTF_GetError() << "\n";
		return false;
	}

	font = TTF_OpenFont(fontPath.c_str(), fontSize);
	if (!font) {
		std::cout << "Failed to load font: " << TTF_GetError() << "\n";
		return false;
	}
	return true;
}

void TextRenderer::clean() {
	if (font) {
		TTF_CloseFont(font);
		font = nullptr;
	}
	TTF_Quit();
}

void TextRenderer::draw(SDL_Renderer* renderer, const std::string& text,
						int x, int y, SDL_Color color) {
	if (!font || text.empty()) return;

	SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
	if (!surface) return;

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_Rect dst = {x, y, surface->w, surface->h};
	SDL_FreeSurface(surface);

	if (!texture) return;
	SDL_RenderCopy(renderer, texture, nullptr, &dst);
	SDL_DestroyTexture(texture);
}

void TextRenderer::drawCentered(SDL_Renderer* renderer, const std::string& text,
								 SDL_Rect rect, SDL_Color color) {
	if (!font || text.empty()) return;

	SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
	if (!surface) return;

	int x = rect.x + (rect.w - surface->w) / 2;
	int y = rect.y + (rect.h - surface->h) / 2;

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_Rect dst = {x, y, surface->w, surface->h};
	SDL_FreeSurface(surface);

	if (!texture) return;
	SDL_RenderCopy(renderer, texture, nullptr, &dst);
	SDL_DestroyTexture(texture);
}