//
// Created by yves-dylane on 5/2/26.
//

#ifndef GAME1_MAP_H
#define GAME1_MAP_H

#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "../Rendering/Camera.h"
#include "../Rendering/TileRenderer.h"
#include "../World/TileLibrary.h"
#include "../World/ObjectInstance.h"
#include "../Editor/LayerState.h"

constexpr int MAP_WIDTH  = 100;
constexpr int MAP_HEIGHT = 100;
constexpr int TILE_SIZE  = 64;

class Map {
public:
	static constexpr int LAYER_COUNT = 5;
	static constexpr int TILE_EMPTY  = -1;

	Map();
	bool init(SDL_Renderer* renderer);

	// Tile grid
	void setTile(int layer, int x, int y, int tileID);
	void clearTile(int layer, int x, int y);
	int  getTile(int layer, int x, int y) const;

	// Objects
	int  addObject(const ObjectInstance& obj);    // returns index
	void removeObject(int index);
	const std::vector<ObjectInstance>& getObjects() const { return objects; }

	// Render
	void render(SDL_Renderer* renderer, const Camera& camera, TileLibrary& library, TileRenderer& tileRenderer,
				 int selectedObjectIndex = -1, bool showGrid = true, const LayerState* layerStates = nullptr);

	// Save / load
	bool save(const std::string& path) const;
	bool load(const std::string& path);
	void insertObject(int index, const ObjectInstance& obj);

private:
	int tiles[LAYER_COUNT][MAP_HEIGHT][MAP_WIDTH];
	std::vector<ObjectInstance> objects;
};

#endif //GAME1_MAP_H