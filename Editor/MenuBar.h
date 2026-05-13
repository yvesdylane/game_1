//
// Created by yves-dylane on 5/13/26.
//

#ifndef GAME1_MENUBAR_H
#define GAME1_MENUBAR_H

#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "../Rendering/TextRenderer.h"
#include "MapManager.h"
#include "TilesetManager.h"

enum class MenuAction {
	None,
	NewMap,
	SaveMap,
	SaveMapAs,
	SwitchMap,       // payload: map index
	DeleteMap,       // payload: map index
	LoadTileset,
	RemoveTileset,   // payload: tileset index
	SwitchTileset,   // payload: tileset index (for highlight only)
	RenameMap,       // payload: map index
	CloseMenu
};

struct MenuResult {
	MenuAction action  = MenuAction::None;
	int        payload = -1;
};

class MenuBar {
public:
	static constexpr int HEIGHT = 24;

	void render(SDL_Renderer* renderer, TextRenderer& text,
				MapManager& maps, TilesetManager& tilesets,
				int screenWidth);

	MenuResult handleClick(int mx, int my,
						   MapManager& maps, TilesetManager& tilesets,
						   int screenWidth);

	void close() { openMenu = -1; }
	bool isOpen() const { return openMenu >= 0; }

private:
	int openMenu = -1; // -1=none, 0=File, 1=Maps, 2=Tilesets

	void renderDropdown(SDL_Renderer* renderer, TextRenderer& text,
						int x, const std::vector<std::string>& items,
						int screenWidth);
};


#endif //GAME1_MENUBAR_H