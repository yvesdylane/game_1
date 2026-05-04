#include "Game.h"
#include <iostream>
#include <SDL2/SDL_image.h>

bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL init failed\n";
        return false;
    }

    window = SDL_CreateWindow("Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN);

    if (!window) {
        std::cout << "Window creation failed\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cout << "Renderer creation failed\n";
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "SDL_image init failed: " << IMG_GetError() << std::endl;
        return false;
    }

    camera.x = 0;
    camera.y = 0;
    camera.zoom = 1.0f;

    if (!map.init(renderer)) {
        std::cout << "Map init failed\n";
        return false;
    }

    return true;
}

void Game::run() {
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 current = SDL_GetTicks();
        float deltaTime = (current - lastTime) / 1000.0f;
        lastTime = current;

        handleEvents();
        update(deltaTime);
        render();
    }
}

void Game::handleEvents() {
    SDL_Event e;
    const Uint8* state = SDL_GetKeyboardState(NULL);

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        }

        // Mouse click (drag OR paint)
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            if (state[SDL_SCANCODE_SPACE]) {
                // Start camera drag
                dragging = true;
                SDL_GetMouseState(&lastMouseX, &lastMouseY);
            } else {
                // Paint tile
                paintTileAtMouse();
            }
        }

        // Stop dragging (mouse release)
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            dragging = false;
        }

        // Stop dragging (space released)
        if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
            dragging = false;
        }

        // Mouse movement
        if (e.type == SDL_MOUSEMOTION) {

            // Dragging camera
            if (dragging && state[SDL_SCANCODE_SPACE]) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                int dx = mouseX - lastMouseX;
                int dy = mouseY - lastMouseY;

                camera.x -= dx / camera.zoom;
                camera.y -= dy / camera.zoom;

                lastMouseX = mouseX;
                lastMouseY = mouseY;
            }

            // Painting while dragging
            if ((e.motion.state & SDL_BUTTON_LMASK) && !state[SDL_SCANCODE_SPACE]) {
                paintTileAtMouse();
            }
        }

        // Scroll movement
        if (e.type == SDL_MOUSEWHEEL) {
            float scrollSpeed = 50.0f;

            if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]) {
                camera.x -= e.wheel.y * scrollSpeed;
            } else {
                camera.y -= e.wheel.y * scrollSpeed;
            }
        }

        // Tile selection
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_1) selectedTile = 0;
            if (e.key.keysym.sym == SDLK_2) selectedTile = 1;
            if (e.key.keysym.sym == SDLK_3) selectedTile = 2;
        }
    }
}

void Game::paintTileAtMouse() {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    float worldX = camera.x + mouseX / camera.zoom;
    float worldY = camera.y + mouseY / camera.zoom;

    int tileX = worldX / TILE_SIZE;
    int tileY = worldY / TILE_SIZE;

    map.setTile(tileX, tileY, selectedTile);
}

void Game::update(float deltaTime) {
    // future logic (animations, etc.)
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    map.render(renderer, camera);

    // Drawing the panel on the right side
    SDL_Rect panelRect = {
        screenWidth - panelWidth,
        0,
        panelWidth,
        screenHeight
    };
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &panelRect);
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();
}