#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>

enum class ObjectType {
    Map,
    Sprite,
    Character,
    Trigger,
    Camera,
    Light,
    Sound,
    Script,
    Group
};

struct ObjectPropertyValue {
    std::string key;
    std::string value;
};

struct ObjectDefinition {
    std::string id;
    std::string name;
    ObjectType  type = ObjectType::Sprite;
    std::string className;
    std::string spritePath;
    std::vector<ObjectPropertyValue> properties;

    // Map-only defaults. Tile size is locked after creation.
    int tileSize = 64;
    int widthTiles = 100;
    int heightTiles = 80;
    int layerCount = 5;
};

struct SceneObjectInstance {
    std::string definitionId;
    std::string instanceId;
    float x = 0.0f;
    float y = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotation = 0.0f;
    bool visible = true;
    std::vector<std::string> childrenIds;
    std::unordered_map<std::string, std::string> overrides;

    // Runtime-only hierarchy links can be resolved from childrenIds.
    std::vector<SceneObjectInstance*> children;
    SceneObjectInstance* parent = nullptr;

    int tileID = -1;
};

struct TileLayerData {
    std::string name;
    bool visible = true;
    bool locked = false;
    std::vector<std::vector<int>> tiles;
};

struct MapData {
    int tileSize = 64;
    int widthTiles = 100;
    int heightTiles = 80;
    std::vector<TileLayerData> layers;
};

struct SceneData {
    std::string name;
    std::string file;
    std::vector<SceneObjectInstance> rootObjects;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float cameraZoom = 1.0f;
    SDL_Color backgroundColor = {20, 20, 20, 255};
};

struct ProjectData {
    std::string name;
    std::string folder;
    std::vector<ObjectDefinition> objects;
    std::vector<std::string> sceneFiles;
    std::string activeScene;
};

class ObjectDefinitionLibrary {
public:
    bool load(const std::string& path);
    bool save(const std::string& path) const;

    int add(ObjectType type);
    const std::vector<ObjectDefinition>& all() const { return definitions; }
    const ObjectDefinition* get(int index) const;
    ObjectDefinition* get(int index);

    static std::string typeName(ObjectType type);
    static std::string classPrefix(ObjectType type);
    static bool generateClassFiles(const ObjectDefinition& def, const std::string& folder);

private:
    std::vector<ObjectDefinition> definitions;
    int nextId = 1;

    std::string makeUniqueClassName(ObjectType type) const;
};
