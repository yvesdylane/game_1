#include "Map.h"
#include "../Rendering/Camera.h"
#include <SDL2/SDL.h>

bool Map::init(SDL_Renderer* renderer) {
	return tileset.load(renderer, "../Assets/Terrain/Tileset/Tilemap_color1.png", TILE_SIZE);
}

void Map::render(SDL_Renderer* renderer, const Camera& camera) {
	for (int y = 0; y < MAP_HEIGHT; y++) {
		for (int x = 0; x < MAP_WIDTH; x++) {
			int tileID = tiles[y][x];

			int screenX = static_cast<int>((x * TILE_SIZE - camera.x) * camera.zoom);
			int screenY = static_cast<int>((y * TILE_SIZE - camera.y) * camera.zoom);

			tileset.renderTile(renderer, tileID, screenX, screenY, camera.zoom);
			// draw grid to help guid the view
			SDL_Rect rect;

			rect.x = static_cast<int>((x * TILE_SIZE - camera.x) * camera.zoom);
			rect.y = static_cast<int>((y * TILE_SIZE - camera.y) * camera.zoom);

			rect.w = static_cast<int>(TILE_SIZE * camera.zoom);
			rect.h = static_cast<int>(TILE_SIZE * camera.zoom);

			SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
			SDL_RenderDrawRect(renderer, &rect);
		}
	}
}