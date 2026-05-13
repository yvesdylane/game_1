//
// Created by yves-dylane on 5/13/26.
//

#ifndef GAME1_TILESETMANAGER_H
#define GAME1_TILESETMANAGER_H

#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct TilesetEntry {
	std::string name;  // friendly name e.g. "Forest"
	std::string file;  // e.g. "forest.tileset"
	bool        loaded = true;
};

class TilesetManager {
public:
	bool load(const std::string& indexPath);
	bool save(const std::string& indexPath) const;

	int  addTileset(const std::string& name, const std::string& file);
	void removeTileset(int index);

	const std::vector<TilesetEntry>& getTilesets() const { return tilesets; }

private:
	std::vector<TilesetEntry> tilesets;
};

#endif //GAME1_TILESETMANAGER_H