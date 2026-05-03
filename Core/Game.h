 //
// Created by yves-dylane on 5/2/26.
//

#ifndef GAME1_GAME_H
#define GAME1_GAME_H
#pragma once
#include <SDL2/SDL.h>
#include "../World/Map.h"
#include "../Rendering/Camera.h"

class Game {
	public:
		bool init();
		void run();
		void clean();
		bool dragging = false;
		int lastMouseX = 0;
		int lastMouseY = 0;

	private:
		void handleEvents();
		void update(float deltaTime);
		void render();

		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;

		bool running = true;

		Map map;
		Camera camera;
};


#endif //GAME1_GAME_H