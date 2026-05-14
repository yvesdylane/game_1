//
// Created by yves-dylane on 5/14/26.
//

#ifndef GAME1_TOOLBAR_H
#define GAME1_TOOLBAR_H

#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "../Rendering/TileRenderer.h"
#include "../Rendering/TextRenderer.h"

enum class ToolMode {
	Hand,
	Paint,
	Eraser
};

class Toolbar {
public:
	static constexpr int HEIGHT = 32;
	static constexpr int ICON_W = 32;

	bool init(SDL_Renderer* renderer);
	void render(SDL_Renderer* renderer, TextRenderer& text,
				ToolMode active, int startX, int screenWidth);

	// Returns new ToolMode if a tool was clicked, or current if not
	ToolMode handleClick(int mx, int my, ToolMode current, int startX);

	void clean();

private:
	SDL_Texture* loadIcon(SDL_Renderer* renderer, const std::string& path);

	SDL_Texture* iconHand   = nullptr;
	SDL_Texture* iconPaint  = nullptr;
	SDL_Texture* iconEraser = nullptr;
};

#endif //GAME1_TOOLBAR_H