//
// Created by yves-dylane on 5/9/26.
//

#ifndef GAME1_TILELIBRARY_H
#define GAME1_TILELIBRARY_H

// World/TileLibrary.h
#pragma once
#include "TileDefinition.h"
#include <vector>
#include <string>

class TileLibrary {
public:
	// Add a tile definition, auto-assigns id
	int addTile(TileDefinition def);

	// Get all tiles for a category
	std::vector<TileDefinition> getByCategory(TileCategory cat) const;

	// Get a single tile by global id
	const TileDefinition* getById(int id) const;

	// Save/load the whole library as JSON
	bool save(const std::string& path) const;
	bool load(const std::string& path);

	// Clear everything
	void clear();

	const std::vector<TileDefinition>& all() const { return tiles; }

private:
	std::vector<TileDefinition> tiles;
	int nextId = 0;
};


#endif //GAME1_TILELIBRARY_H