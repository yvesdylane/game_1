#pragma once

#include <SDL2/SDL.h>
#include "../World/Map.h"
#include "../Rendering/Camera.h"

class Game {
public:
	bool init();
	void run();
	void clean();
	int screenWidth = 1280;
	int screenHeight = 720;
	int panelWidth = 256;
	int selectedLayer = 0;
	int bottomBarHeight = 80;

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