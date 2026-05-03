#pragma once

#include <SDL2/SDL.h>
#include "../World/Map.h"
#include "../Rendering/Camera.h"

class Game {
public:
	bool init();
	void run();
	void clean();

private:
	void handleEvents();
	void update(float deltaTime);
	void render();

	void paintTileAtMouse(); // helper

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	bool running = true;

	// World
	Map map;
	Camera camera;

	// Input state
	bool dragging = false;
	int lastMouseX = 0;
	int lastMouseY = 0;

	// Editor state
	int selectedTile = 1;
};