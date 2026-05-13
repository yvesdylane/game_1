//
// Created by yves-dylane on 5/13/26.
//

#include "TilesetManager.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool TilesetManager::load(const std::string& indexPath) {
	std::ifstream file(indexPath);
	if (!file) {
		std::cout << "TilesetManager: no index found\n";
		return false;
	}
	json j;
	try { file >> j; }
	catch (...) { return false; }

	tilesets.clear();
	for (const auto& t : j["tilesets"])
		tilesets.push_back({t["name"], t["file"], true});
	return true;
}

bool TilesetManager::save(const std::string& indexPath) const {
	json j;
	j["tilesets"] = json::array();
	for (const auto& t : tilesets)
		j["tilesets"].push_back({{"name", t.name}, {"file", t.file}});

	std::ofstream file(indexPath);
	if (!file) return false;
	file << j.dump(2);
	return true;
}

int TilesetManager::addTileset(const std::string& name, const std::string& file) {
	tilesets.push_back({name, file, true});
	return (int)tilesets.size() - 1;
}

void TilesetManager::removeTileset(int index) {
	if (index < 0 || index >= (int)tilesets.size()) return;
	tilesets.erase(tilesets.begin() + index);
}