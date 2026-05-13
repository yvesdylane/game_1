//
// Created by yves-dylane on 5/13/26.
//

#include "MapManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

bool MapManager::load(const std::string& indexPath) {
    std::ifstream file(indexPath);
    if (!file) {
        std::cout << "MapManager: no index found, starting fresh\n";
        return false;
    }
    json j;
    try { file >> j; }
    catch (...) { std::cout << "MapManager: parse error\n"; return false; }

    maps.clear();
    for (const auto& m : j["maps"])
        maps.push_back({m["name"], m["file"]});
    activeIndex = j.value("active", 0);
    return true;
}

bool MapManager::save(const std::string& indexPath) const {
    json j;
    j["active"] = activeIndex;
    j["maps"]   = json::array();
    for (const auto& m : maps)
        j["maps"].push_back({{"name", m.name}, {"file", m.file}});

    std::ofstream file(indexPath);
    if (!file) { std::cout << "MapManager: failed to save index\n"; return false; }
    file << j.dump(2);
    return true;
}

int MapManager::addMap(const std::string& name, const std::string& file) {
    maps.push_back({name, file});
    return (int)maps.size() - 1;
}

void MapManager::removeMap(int index) {
    if (index < 0 || index >= (int)maps.size()) return;
    maps.erase(maps.begin() + index);
    if (activeIndex >= (int)maps.size())
        activeIndex = std::max(0, (int)maps.size() - 1);
}

void MapManager::renameMap(int index, const std::string& newName) {
    if (index < 0 || index >= (int)maps.size()) return;
    maps[index].name = newName;
}

const MapEntry* MapManager::getActiveEntry() const {
    if (maps.empty() || activeIndex < 0 || activeIndex >= (int)maps.size())
        return nullptr;
    return &maps[activeIndex];
}

std::string MapManager::nameToFile(const std::string& name) {
    std::string file = name;
    // Replace spaces and special chars with underscores
    std::transform(file.begin(), file.end(), file.begin(), [](char c) {
        return (std::isalnum(c) || c == '_') ? std::tolower(c) : '_';
    });
    return file + ".map";
}