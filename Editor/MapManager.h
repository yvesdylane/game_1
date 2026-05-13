//
// Created by yves-dylane on 5/13/26.
//

#ifndef GAME1_MAPMANAGER_H
#define GAME1_MAPMANAGER_H

#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct MapEntry {
	std::string name;
	std::string file; // e.g. "level1.map"
};

class MapManager {
public:
	bool load(const std::string& indexPath);
	bool save(const std::string& indexPath) const;

	int         addMap(const std::string& name, const std::string& file);
	void        removeMap(int index);
	void        renameMap(int index, const std::string& newName);
	void        setActive(int index) { activeIndex = index; }

	int                       getActive()  const { return activeIndex; }
	const MapEntry*           getActiveEntry() const;
	const std::vector<MapEntry>& getMaps() const { return maps; }

	// Generate a safe filename from a map name
	static std::string nameToFile(const std::string& name);

private:
	std::vector<MapEntry> maps;
	int activeIndex = 0;
};

#endif //GAME1_MAPMANAGER_H