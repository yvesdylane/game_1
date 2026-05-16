//
// Created by yves-dylane on 5/13/26.
//

#ifndef GAME1_OBJECTINSTANCE_H
#define GAME1_OBJECTINSTANCE_H

#pragma once
#include <string>
#include <vector>

struct ObjectProperty {
    std::string key;
    std::string value;
};

enum class CollisionShape {
    None,
    Box,
    Circle
};

enum class CollisionOrigin {
    TopLeft,
    Center,
    Bottom,
    Top,
    Left,
    Right
};

struct ObjectInstance {
    int   tileID = -1;
    float x = 0.0f;
    float y = 0.0f;     // world position in pixels (top-left of object)
    int   layer = 0;

    std::string name = "GameObject";
    std::string type = "Object";
    std::string spritePath;

    CollisionShape collisionShape = CollisionShape::Box;
    CollisionOrigin collisionOrigin = CollisionOrigin::TopLeft;
    float collisionOffsetX = 0.0f;
    float collisionOffsetY = 0.0f;
    float collisionW = 0.0f;
    float collisionH = 0.0f;
    float collisionRadius = 0.0f;

    std::vector<ObjectProperty> properties;
};

#endif //GAME1_OBJECTINSTANCE_H
