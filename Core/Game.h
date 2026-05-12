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

enum class TilesetEditorMode {
    SingleObject,  // whole image = one tile
    GridUniform,   // square tiles, scroll to resize
    GridManual     // manual W/H input via keyboard
};

struct TilesetEditorState {
    std::string      imagePath;
    int              imageW       = 0;
    int              imageH       = 0;
    int              tileW        = 64;
    int              tileH        = 64;
    TileCategory     category     = TileCategory::Terrain;
    TilesetEditorMode mode        = TilesetEditorMode::GridUniform;
    std::vector<bool> included;

    // Manual input state
    enum class FocusedField { None, Width, Height };
    FocusedField focused  = FocusedField::None;
    std::string  inputW   = "64";
    std::string  inputH   = "64";

    int cols() const {
        if (mode == TilesetEditorMode::SingleObject) return 1;
        return std::max(1, imageW / tileW);
    }
    int rows() const {
        if (mode == TilesetEditorMode::SingleObject) return 1;
        return std::max(1, imageH / tileH);
    }
    int count() const { return cols() * rows(); }

    void reset(int w, int h, int tw, int th) {
        imageW = w; imageH = h;
        tileW  = tw; tileH = th;
        inputW = std::to_string(tw);
        inputH = std::to_string(th);
        included.assign(count(), false);
    }

    void applyManualInput() {
        try {
            int w = std::max(8, std::stoi(inputW));
            int h = std::max(8, std::stoi(inputH));

            // ✅ Only reset grid if size actually changed
            if (w != tileW || h != tileH) {
                tileW = w;
                tileH = h;
                included.assign(count(), false);
            }
        } catch (...) {}
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