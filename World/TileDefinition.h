//
// Created by yves-dylane on 5/9/26.
//

#ifndef GAME1_TILEDEFINITION_H
#define GAME1_TILEDEFINITION_H

// World/TileDefinition.h
#pragma once
#include <string>

enum class TileCategory {
	Terrain,
	Object,
	Decoration,
	Resource
};

struct TileDefinition {
	int         id;           // unique across the whole library
	std::string imagePath;    // relative path to source PNG
	int         srcX, srcY;   // pixel origin in source image
	int         srcW, srcH;   // pixel size in source image (e.g. 64x128 for tall object)
	int         tileW, tileH; // how many map tiles wide/tall this occupies (srcW/TILE_SIZE)
	TileCategory category;
	std::string label;        // optional friendly name e.g. "oak_tree"
};

#endif //GAME1_TILEDEFINITION_H