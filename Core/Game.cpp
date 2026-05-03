//
// Created by yves-dylane on 5/2/26.
//

#include "Game.h"
#include <iostream>

bool Game::init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;

	window = SDL_CreateWindow("Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
		SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	camera.x = 0;
	camera.y = 0;
	camera.zoom = 1.0f;

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
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) running = false;

		// Start dragging (LEFT mouse)
		if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
			dragging = true;
			SDL_GetMouseState(&lastMouseX, &lastMouseY);
		}

		// Stop dragging
		if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
			dragging = false;
		}

		// Mouse move while dragging
		if (e.type == SDL_MOUSEMOTION && dragging) {
			int mouseX, mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);

			int dx = mouseX - lastMouseX;
			int dy = mouseY - lastMouseY;

			// Move camera (IMPORTANT: divide by zoom)
			camera.x -= dx / camera.zoom;
			camera.y -= dy / camera.zoom;

			lastMouseX = mouseX;
			lastMouseY = mouseY;
		}

		if (e.type == SDL_MOUSEWHEEL) {
			const Uint8* keystate = SDL_GetKeyboardState(NULL);

			float scrollSpeed = 50.0f; // tweak later

			if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
				// Horizontal scroll
				camera.x -= e.wheel.y * scrollSpeed;
			} else {
				// Vertical scroll
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
	SDL_DestroyWindow(window);
	SDL_Quit();
}