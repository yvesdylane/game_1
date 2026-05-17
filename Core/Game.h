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
#include "../Editor/MenuBar.h"
#include "../Editor/MapManager.h"
#include "../Editor/TilesetManager.h"
#include "../Editor/Toolbar.h"
#include "../Editor/UndoSystem.h"
#include "../Editor/LayerState.h"
#include "../Editor/ObjectDefinitionLibrary.h"

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

// Placement mode
enum class PlacementMode {
    Grid,   // snaps to tile grid
    Free    // follows mouse exactly (Tab to toggle)
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

// New map creation state
struct NewMapDialog {
    bool     active      = false;
    // which field is focused
    enum class Field { Name, TileSize, Width, Height } focused = Field::Name;
    std::string inputName     = "";
    std::string inputTileSize = "64";
    std::string inputWidth    = "100";
    std::string inputHeight   = "100";
};

enum class PanelLayout {
    TilesOnly,
    SettingsOnly,
    ObjectsOnly,
    Both           // tiles on top, settings on bottom
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

    PlacementMode placementMode  = PlacementMode::Grid;
    int selectedObject = -1;  // index into map.getObjects(), -1 = none

    // panel scroll offset (pixels scrolled down in tile panel)
    int panelScrollY = 0;

    // camera drag
    bool dragging   = false;
    int  lastMouseX = 0;
    int  lastMouseY = 0;

    MenuBar        menuBar;
    MapManager     mapManager;
    TilesetManager tilesetManager;
    ObjectDefinitionLibrary objectLibrary;
    int selectedObjectDefinition = -1;

    static constexpr int menuBarHeight  = MenuBar::HEIGHT;
    static constexpr int toolbarStartX  = 250; // after menu tabs
    // Total top UI height
    static constexpr int topBarHeight   = MenuBar::HEIGHT; // toolbar lives IN menu bar row
    // map creation and state
    NewMapDialog newMapDialog;
    PanelLayout panelLayout = PanelLayout::TilesOnly;
    bool        settingsOnTop = false; // in Both mode, swap order

    int expandAmount = 5; // default expand/shrink step
    std::string expandAmountInput = "5";
    bool expandInputFocused = false;

    // Index paths
    static constexpr const char* MAPS_INDEX     = "../Maps/maps.index";
    static constexpr const char* TILESETS_INDEX = "../Assets/Tilesets/tilesets.index";
    static constexpr const char* OBJECTS_INDEX  = "../Assets/Objects/objects.index";
    static constexpr const char* GAMEOBJECTS_DIR = "../GameObjects";

    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr int screenWidth    = 1280;
    static constexpr int screenHeight   = 720;
    static constexpr int objectPanelWidth = 220;
    static constexpr int panelWidth     = 200;
    static constexpr int bottomBarHeight = 60;
    static constexpr int tabBarHeight   = 36;   // category tabs at top of panel
    static constexpr int panelTileSize  = 48;   // tile preview size in pane
    // helpers
    std::string openFileDialog(); // wraps zenity on Linux
    std::string copyImageToTilesetImages(const std::string& imagePath);
    // text rendering
    TextRenderer textRenderer;
    bool showGrid = true;  // G key toggles
    int  hoveredTile = -1; // for hover name display

    Toolbar  toolbar;
    ToolMode toolMode = ToolMode::Paint;
    // For multi-image import queue
    std::vector<std::string> pendingImports; // images waiting to go through editor
    int                      pendingImportIndex = 0;
    UndoSystem undoSystem;
    LayerState layerStates[5]; // one per layer
    bool       strokeActive = false;

    bool showTilePanel     = true;
    bool showSettingsPanel = true;  // controlled via panelLayout
    bool showBottomBar     = true;
    bool showToolbar       = true;

    // ── Methods ───────────────────────────────────────────────────────────────
    void handleEvents();
    void update(float deltaTime);
    void render();

    bool selectedHierarchyObjectIsMap() const;

    // paint mode
    void paintTileAtMouse();
    void eraseTileAtMouse();

    // creating map
    void centerCameraOnMap();
    void createNewMap();
    void renderSettingsPanel(int x, int y, int w, int h);
    void renderObjectPropertiesPanel(int x, int y, int w, int h);

    // panel
    void renderObjectPanel();
    void renderPanel();
    void renderBottomBar();
    void renderTilesPanelContent(int x, int y, int w, int h);
    void renderTilesetEditor();
    void openTilesetEditor(const std::string& imagePath);
    void commitTilesetEditor(); // saves selected tiles into tileLibrary

    // Bulk import
    void importSingleImage(const std::string& path);
    void importFolder();
    void importMultipleImages();

    // UI polish helpers
    void renderCoordinates();   // draws in bottom bar
    void renderZoomIndicator(); // draws in bottom bar

    bool handleMenuEvents(const SDL_Event& e);
    void handleTilesetEditorEvents(const SDL_Event& e);
    void handlePaintModeEvents(const SDL_Event& e, const Uint8* keys);
    void handleNamingInput(const SDL_Event& e);
    void advancePendingImport();
    void handleObjectPanelClick(int mx, int my);
    void handlePanelClick(int mx, int my);
    void handleTilesPanelClick(int mx, int my, int px, int contentY, int w, int h);
    void handleSettingsPanelClick(int mx, int my, int px, int contentY, int w);
    void handleObjectPropertiesPanelClick(int mx, int my, int px, int contentY, int w);

    void applyUndo(const StrokeCommand& cmd);
    void applyUndo(const PlaceObjectCommand& cmd);
    void applyUndo(const RemoveObjectCommand& cmd);
    void applyRedo(const StrokeCommand& cmd);
    void applyRedo(const PlaceObjectCommand& cmd);
    void applyRedo(const RemoveObjectCommand& cmd);

};