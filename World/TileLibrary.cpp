//
// Created by yves-dylane on 5/9/26.
//

// World/TileLibrary.cpp
#include "TileLibrary.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// ── helpers ──────────────────────────────────────────────────────────────────

static std::string categoryToString(TileCategory c) {
    switch (c) {
        case TileCategory::Terrain:    return "terrain";
        case TileCategory::Object:     return "object";
        case TileCategory::Decoration: return "decoration";
        case TileCategory::Resource:   return "resource";
    }
    return "terrain";
}

static TileCategory categoryFromString(const std::string& s) {
    if (s == "object")     return TileCategory::Object;
    if (s == "decoration") return TileCategory::Decoration;
    if (s == "resource")   return TileCategory::Resource;
    return TileCategory::Terrain;
}

// ── public API ────────────────────────────────────────────────────────────────

int TileLibrary::addTile(TileDefinition def) {
    def.id = nextId++;
    tiles.push_back(def);
    return def.id;
}

std::vector<TileDefinition> TileLibrary::getByCategory(TileCategory cat) const {
    std::vector<TileDefinition> result;
    for (const auto& t : tiles)
        if (t.category == cat) result.push_back(t);
    return result;
}

const TileDefinition* TileLibrary::getById(int id) const {
    for (const auto& t : tiles)
        if (t.id == id) return &t;
    return nullptr;
}

void TileLibrary::clear() {
    tiles.clear();
    nextId = 0;
}

bool TileLibrary::save(const std::string& path) const {
    json j;
    j["nextId"] = nextId;
    j["tiles"]  = json::array();

    for (const auto& t : tiles) {
        j["tiles"].push_back({
            {"id",        t.id},
            {"imagePath", t.imagePath},
            {"srcX",      t.srcX},
            {"srcY",      t.srcY},
            {"srcW",      t.srcW},
            {"srcH",      t.srcH},
            {"tileW",     t.tileW},
            {"tileH",     t.tileH},
            {"category",  categoryToString(t.category)},
            {"label",     t.label}
        });
    }

    std::ofstream file(path);
    if (!file) {
        std::cout << "TileLibrary: failed to save to " << path << "\n";
        return false;
    }
    file << j.dump(2); // pretty print with 2-space indent
    return true;
}

bool TileLibrary::load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "TileLibrary: no existing library at " << path << " — starting fresh\n";
        return false;
    }

    json j;
    try { file >> j; }
    catch (const json::exception& e) {
        std::cout << "TileLibrary: JSON parse error: " << e.what() << "\n";
        return false;
    }

    clear();
    nextId = j.value("nextId", 0);

    for (const auto& jt : j["tiles"]) {
        TileDefinition t;
        t.id        = jt["id"];
        t.imagePath = jt["imagePath"];
        t.srcX      = jt["srcX"];
        t.srcY      = jt["srcY"];
        t.srcW      = jt["srcW"];
        t.srcH      = jt["srcH"];
        t.tileW     = jt.value("tileW", 1);
        t.tileH     = jt.value("tileH", 1);
        t.category  = categoryFromString(jt["category"]);
        t.label     = jt.value("label", "");
        tiles.push_back(t);
    }

    return true;
}