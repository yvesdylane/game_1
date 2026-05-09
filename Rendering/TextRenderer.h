//
// Created by yves-dylane on 5/9/26.
//

#ifndef GAME1_TEXTRENDERER_H
#define GAME1_TEXTRENDERER_H


#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class TextRenderer {
public:
    bool load(const std::string& fontPath, int fontSize);
    void clean();

    // Draw text at (x, y), centered inside a rect if provided
    void draw(
        SDL_Renderer*      renderer,
        const std::string& text,
        int x, int y,
        SDL_Color color = {255, 255, 255, 255}
    );

    // Draw text centered inside a rect
    void drawCentered(
        SDL_Renderer*      renderer,
        const std::string& text,
        SDL_Rect           rect,
        SDL_Color          color = {255, 255, 255, 255}
    );

    ~TextRenderer() { clean(); }

private:
    TTF_Font* font = nullptr;
};


#endif //GAME1_TEXTRENDERER_H