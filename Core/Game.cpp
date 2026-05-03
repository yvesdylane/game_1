//
// Created by yves-dylane on 5/2/26.
//

#include "Game.h"
#include <iostream>
#include <SDL2/SDL_image.h>

bool Game::init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;

	window = SDL_CreateWindow("Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
		SDL_WINDOW_SHOWN);

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		std::cout << "SDL_image init failed: " << IMG_GetError() << std::endl;
		return false;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	camera.x = 0;
	camera.y = 0;
	camera.zoom = 1.0f;
	if (!map.init(renderer)) {
		std::cout << "Map init failed" << std::endl;
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
		if (e.type == SDL_QUIT) running = false;

		// Start dragging
		if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && state[SDL_SCANCODE_SPACE]) {
			dragging = true;
			SDL_GetMouseState(&lastMouseX, &lastMouseY);
		}

		// Stop dragging (mouse release)
		if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
			dragging = false;
		}

		// Stop dragging (space released)
		if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			dragging = false;
		}

		// Dragging movement
		if (e.type == SDL_MOUSEMOTION && dragging && state[SDL_SCANCODE_SPACE]) {
			int mouseX, mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);

			int dx = mouseX - lastMouseX;
			int dy = mouseY - lastMouseY;

			camera.x -= dx / camera.zoom;
			camera.y -= dy / camera.zoom;

			lastMouseX = mouseX;
			lastMouseY = mouseY;
		}

		// Scroll
		if (e.type == SDL_MOUSEWHEEL) {
			float scrollSpeed = 50.0f;

			if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]) {
				camera.x -= e.wheel.y * scrollSpeed;
			} else {
				camera.y -= e.wheel.y * scrollSpeed;
			}
		}
	}
}

void Game::update(float deltaTime) {
}

void Game::render() {
	SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
	SDL_RenderClear(renderer);
	map.render(renderer, camera);
	SDL_RenderPresent(renderer);
}

void Game::clean() {
	SDL_DestroyRenderer(renderer);
	IMG_Quit();
	SDL_DestroyWindow(window);
	SDL_Quit();
}