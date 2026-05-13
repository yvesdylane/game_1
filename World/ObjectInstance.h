//
// Created by yves-dylane on 5/13/26.
//

#ifndef GAME1_OBJECTINSTANCE_H
#define GAME1_OBJECTINSTANCE_H

#pragma once

struct ObjectInstance {
    int   tileID;
    float x, y;     // world position in pixels (top-left of object)
    int   layer;
};

#endif //GAME1_OBJECTINSTANCE_H