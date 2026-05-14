//
// Created by yves-dylane on 5/14/26.
//

#include "Toolbar.h"
#include <SDL2/SDL_image.h>
#include <iostream>

SDL_Texture* Toolbar::loadIcon(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* s = IMG_Load(path.c_str());
    if (!s) {
        std::cout << "Toolbar: failed to load icon " << path << "\n";
        return nullptr;
    }
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    return t;
}

bool Toolbar::init(SDL_Renderer* renderer) {
    iconHand   = loadIcon(renderer, "../Assets/Icons/honds.png");
    iconPaint  = loadIcon(renderer, "../Assets/Icons/pain.png");
    iconEraser = loadIcon(renderer, "../Assets/Icons/eraser.png");
    return true; // non-fatal if icons missing — fallback to colored rects
}

void Toolbar::render(SDL_Renderer* renderer, TextRenderer& text,
                     ToolMode active, int startX, int screenWidth) {

    struct Tool {
        SDL_Texture** icon;
        const char*   label;
        ToolMode      mode;
    } tools[] = {
        {&iconHand,   "Hand",   ToolMode::Hand},
        {&iconPaint,  "Paint",  ToolMode::Paint},
        {&iconEraser, "Eraser", ToolMode::Eraser},
    };

    for (int i = 0; i < 3; i++) {
        SDL_Rect r = {startX + i * ICON_W, 0, ICON_W, HEIGHT};

        // Background — highlight active tool
        if (tools[i].mode == active)
            SDL_SetRenderDrawColor(renderer, 80, 130, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
        SDL_RenderFillRect(renderer, &r);

        // Border
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderDrawRect(renderer, &r);

        // Icon or fallback text
        if (*tools[i].icon) {
            SDL_Rect iconRect = {r.x + 4, r.y + 4, ICON_W - 8, HEIGHT - 8};
            SDL_RenderCopy(renderer, *tools[i].icon, nullptr, &iconRect);
        } else {
            text.drawCentered(renderer, tools[i].label, r);
        }
    }
}

ToolMode Toolbar::handleClick(int mx, int my, ToolMode current, int startX) {
    if (my < 0 || my >= HEIGHT) return current;
    int localX = mx - startX;
    if (localX < 0 || localX >= ICON_W * 3) return current;
    int idx = localX / ICON_W;
    switch (idx) {
        case 0: return ToolMode::Hand;
        case 1: return ToolMode::Paint;
        case 2: return ToolMode::Eraser;
    }
    return current;
}

void Toolbar::clean() {
    if (iconHand)   SDL_DestroyTexture(iconHand);
    if (iconPaint)  SDL_DestroyTexture(iconPaint);
    if (iconEraser) SDL_DestroyTexture(iconEraser);
    iconHand = iconPaint = iconEraser = nullptr;
}