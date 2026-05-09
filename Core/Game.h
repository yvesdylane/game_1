#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include "../World/Map.h"
#include "../World/TileLibrary.h"
#include "../Rendering/TileRenderer.h"
#include "../Rendering/Camera.h"
#include "../Rendering/TextRenderer.h"

// ── Editor mode ───────────────────────────────────────────────────────────────
enum class EditorMode {
    Paint,         // normal tile painting
    TilesetEditor  // importing a new image + picking tiles
};

// ── Tileset editor state ──────────────────────────────────────────────────────
struct TilesetEditorState {
    std::string     imagePath;
    int             imageW      = 0;
    int             imageH      = 0;
    int             tileW       = 64;   // user-chosen tile width
    int             tileH       = 64;   // user-chosen tile height
    TileCategory    category    = TileCategory::Terrain;
    // true = user wants to include this tile
    std::vector<bool> included;

    int cols() const { return imageW / tileW; }
    int rows() const { return imageH / tileH; }
    int count() const { return cols() * rows(); }

    void reset(int w, int h, int tw, int th) {
        imageW = w; imageH = h; tileW = tw; tileH = th;
        included.assign(count(), false);
    }
};

class Game {
public:
    bool init();
    void run();
    void clean();

private:
    // ── Core ──────────────────────────────────────────────────────────────────
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool          running  = true;

    // ── World ─────────────────────────────────────────────────────────────────
    Map         map;
    Camera      camera;
    TileLibrary tileLibrary;
    TileRenderer tileRenderer;

    // ── Editor state ──────────────────────────────────────────────────────────
    EditorMode  editorMode   = EditorMode::Paint;
    int         selectedTile = 0;      // global TileDefinition id
    int         selectedLayer = 0;
    TileCategory activeCategory = TileCategory::Terrain;

    TilesetEditorState tsEditor;

    // panel scroll offset (pixels scrolled down in tile panel)
    int panelScrollY = 0;

    // camera drag
    bool dragging   = false;
    int  lastMouseX = 0;
    int  lastMouseY = 0;

    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr int screenWidth    = 1280;
    static constexpr int screenHeight   = 720;
    static constexpr int panelWidth     = 200;
    static constexpr int bottomBarHeight = 60;
    static constexpr int tabBarHeight   = 36;   // category tabs at top of panel
    static constexpr int panelTileSize  = 48;   // tile preview size in panel

    // ── Methods ───────────────────────────────────────────────────────────────
    void handleEvents();
    void update(float deltaTime);
    void render();

    // paint mode
    void paintTileAtMouse();

    // panel
    void renderPanel();
    void renderBottomBar();

    // tileset editor
    void renderTilesetEditor();
    void openTilesetEditor(const std::string& imagePath);
    void commitTilesetEditor(); // saves selected tiles into tileLibrary

    // helpers
    std::string openFileDialog(); // wraps zenity on Linux

    // text rendering
    TextRenderer textRenderer;
};