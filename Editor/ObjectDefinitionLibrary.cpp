#include "ObjectDefinitionLibrary.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

static std::string typeToString(ObjectType type) {
    switch (type) {
        case ObjectType::Map:       return "Map";
        case ObjectType::Sprite:    return "Sprite";
        case ObjectType::Character: return "Character";
        case ObjectType::Trigger:   return "Trigger";
        case ObjectType::Camera:    return "Camera";
        case ObjectType::Light:     return "Light";
        case ObjectType::Sound:     return "Sound";
        case ObjectType::Script:    return "Script";
        case ObjectType::Group:     return "Group";
    }
    return "Sprite";
}

static ObjectType typeFromString(const std::string& type) {
    if (type == "Map") return ObjectType::Map;
    if (type == "Character") return ObjectType::Character;
    if (type == "Trigger") return ObjectType::Trigger;
    if (type == "Camera") return ObjectType::Camera;
    if (type == "Light") return ObjectType::Light;
    if (type == "Sound") return ObjectType::Sound;
    if (type == "Script") return ObjectType::Script;
    if (type == "Group") return ObjectType::Group;
    return ObjectType::Sprite;
}

std::string ObjectDefinitionLibrary::typeName(ObjectType type) {
    return typeToString(type);
}

std::string ObjectDefinitionLibrary::classPrefix(ObjectType type) {
    switch (type) {
        case ObjectType::Map:       return "Map";
        case ObjectType::Sprite:    return "Sprite";
        case ObjectType::Character: return "Character";
        case ObjectType::Trigger:   return "Trigger";
        case ObjectType::Camera:    return "Camera";
        case ObjectType::Light:     return "Light";
        case ObjectType::Sound:     return "Sound";
        case ObjectType::Script:    return "Script";
        case ObjectType::Group:     return "Group";
    }
    return "Sprite";
}

std::string ObjectDefinitionLibrary::makeUniqueClassName(ObjectType type) const {
    std::string prefix = classPrefix(type);
    int n = 1;
    while (true) {
        std::string candidate = prefix + std::to_string(n);
        bool used = false;
        for (const auto& def : definitions) {
            if (def.className == candidate) {
                used = true;
                break;
            }
        }
        if (!used) return candidate;
        n++;
    }
}

int ObjectDefinitionLibrary::add(ObjectType type) {
    ObjectDefinition def;
    def.id = "object_" + std::to_string(nextId++);
    def.type = type;
    def.className = makeUniqueClassName(type);
    def.name = def.className;
    definitions.push_back(def);
    return static_cast<int>(definitions.size()) - 1;
}

const ObjectDefinition* ObjectDefinitionLibrary::get(int index) const {
    if (index < 0 || index >= static_cast<int>(definitions.size())) return nullptr;
    return &definitions[index];
}

ObjectDefinition* ObjectDefinitionLibrary::get(int index) {
    if (index < 0 || index >= static_cast<int>(definitions.size())) return nullptr;
    return &definitions[index];
}

bool ObjectDefinitionLibrary::load(const std::string& path) {
    definitions.clear();
    nextId = 1;

    std::ifstream file(path);
    if (!file) return false;

    json j;
    try { file >> j; }
    catch (const json::exception& e) {
        std::cout << "ObjectDefinitionLibrary: JSON parse error: " << e.what() << "\n";
        return false;
    }

    nextId = j.value("nextId", 1);
    for (const auto& item : j.value("objects", json::array())) {
        ObjectDefinition def;
        def.id = item.value("id", "");
        def.name = item.value("name", "GameObject");
        def.type = typeFromString(item.value("type", "GameObject"));
        def.className = item.value("className", def.name);
        def.spritePath = item.value("spritePath", "");
        def.tileSize = item.value("tileSize", 64);
        def.widthTiles = item.value("widthTiles", 100);
        def.heightTiles = item.value("heightTiles", 80);
        def.layerCount = item.value("layerCount", 5);
        definitions.push_back(def);
    }

    return true;
}

bool ObjectDefinitionLibrary::save(const std::string& path) const {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    json j;
    j["nextId"] = nextId;
    j["objects"] = json::array();
    for (const auto& def : definitions) {
        j["objects"].push_back({
            {"id", def.id},
            {"name", def.name},
            {"type", typeToString(def.type)},
            {"className", def.className},
            {"spritePath", def.spritePath},
            {"tileSize", def.tileSize},
            {"widthTiles", def.widthTiles},
            {"heightTiles", def.heightTiles},
            {"layerCount", def.layerCount}
        });
    }

    std::ofstream file(path);
    if (!file) return false;
    file << j.dump(2);
    return true;
}

bool ObjectDefinitionLibrary::generateClassFiles(const ObjectDefinition& def, const std::string& folder) {
    namespace fs = std::filesystem;
    fs::create_directories(folder);

    fs::path header = fs::path(folder) / (def.className + ".h");
    fs::path source = fs::path(folder) / (def.className + ".cpp");

    std::string base = classPrefix(def.type) + "Object";
    if (def.type == ObjectType::Sprite) base = "SpriteObject";

    if (!fs::exists(header)) {
        std::ofstream h(header);
        if (!h) return false;
        h << "#pragma once\n";
        h << "#include \"engine/" << base << ".h\"\n\n";
        h << "class " << def.className << " : public " << base << " {\n";
        h << "public:\n";
        h << "    void onCreate() override;\n";
        h << "    void onUpdate(float dt) override;\n";
        h << "    void onDestroy() override;\n";
        h << "};\n";
    }

    if (!fs::exists(source)) {
        std::ofstream cpp(source);
        if (!cpp) return false;
        cpp << "#include \"" << def.className << ".h\"\n\n";
        cpp << "void " << def.className << "::onCreate() {\n";
        cpp << "}\n\n";
        cpp << "void " << def.className << "::onUpdate(float dt) {\n";
        cpp << "    (void)dt;\n";
        cpp << "}\n\n";
        cpp << "void " << def.className << "::onDestroy() {\n";
        cpp << "}\n";
    }

    return true;
}
