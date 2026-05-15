#include "Game.h"
#include <iostream>
#include <cstdio>
#include "tinyfiledialogs.h"
#include "../Editor/FileUtils.h"

// ── init ─────────────────────────────────────────────────────────────────────

bool Game::init() {
    std::cout << "[1] init started\n" << std::flush;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) { std::cout << "SDL init failed\n"; return false; }
    std::cout << "[2] SDL ok\n" << std::flush;

    window = SDL_CreateWindow("Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenWidth, screenHeight, SDL_WINDOW_SHOWN);
    if (!window) { std::cout << "Window failed\n"; return false; }
    std::cout << "[3] window ok\n" << std::flush;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { std::cout << "Renderer failed\n"; return false; }
    std::cout << "[4] renderer ok\n" << std::flush;

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "IMG_Init failed: " << IMG_GetError() << "\n";
        return false;
    }
    std::cout << "[5] IMG ok\n" << std::flush;

    toolbar.init(renderer);
    if (!textRenderer.load("../Assets/Fonts/DejaVuSans.ttf", 13)) {
        std::cout << "Font load failed\n";
        return false;
    }
    std::cout << "[5b] font ok\n" << std::flush;

    camera = {0, 0, 1.0f};

    if (!map.init(renderer)) { std::cout << "Map init failed\n"; return false; }
    std::cout << "[6] map ok\n" << std::flush;

    tileLibrary.load("../Assets/Tilesets/library.tileset");
    std::cout << "[7] library load attempted\n" << std::flush;

    // Pre-load textures for all known tiles
    for (const auto& def : tileLibrary.all())
        tileRenderer.loadTexture(renderer, def.imagePath);
    std::cout << "[8] textures loaded\n" << std::flush;

    // Load map index
    mapManager.load(MAPS_INDEX);
    if (mapManager.getMaps().empty()) {
        // First run — create a default map
        mapManager.addMap("Level 1", "level1.map");
        mapManager.save(MAPS_INDEX);
    }

    // Load active map
    const MapEntry* active = mapManager.getActiveEntry();
    if (active)
        map.load("../Maps/" + active->file);

    // Load tileset index
    tilesetManager.load(TILESETS_INDEX);
    // Reload all tilesets into library
    for (const auto& ts : tilesetManager.getTilesets()) {
        TileLibrary tmp;
        if (tmp.load("../Assets/Tilesets/" + ts.file)) {
            for (const auto& def : tmp.all()) {
                tileRenderer.loadTexture(renderer, def.imagePath);
                tileLibrary.addTile(const_cast<TileDefinition&>(def));
            }
        }
    }

    return true;
}

// ── run ──────────────────────────────────────────────────────────────────────

void Game::run() {
    Uint32 lastTime = SDL_GetTicks();
    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - lastTime) / 1000.0f;
        lastTime   = now;
        handleEvents();
        update(dt);
        render();
    }
}

// ── handleEvents ─────────────────────────────────────────────────────────────

void Game::handleEvents() {
    SDL_Event e;
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { running = false; continue; }

        // Global keys (work in any mode)
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.scancode == SDL_SCANCODE_G)
                showGrid = !showGrid;
        }

        // Naming overlay blocks everything
        if (namingMap) {
            handleNamingInput(e);
            continue;
        }

        // Menu bar (always on top)
        if (handleMenuEvents(e)) continue;

        // Mode-specific
        if (editorMode == EditorMode::TilesetEditor) {
            handleTilesetEditorEvents(e);
        } else {
            handlePaintModeEvents(e, keys);
        }
    }
}

bool Game::handleMenuEvents(const SDL_Event& e) {
    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
        return menuBar.isOpen(); // consume all clicks if menu is open

    int mx = e.button.x;
    int my = e.button.y;

    // Toolbar tool selection (lives in menu bar row)
    if (my >= 0 && my < menuBarHeight) {
        ToolMode newMode = toolbar.handleClick(mx, my, toolMode, toolbarStartX);
        if (newMode != toolMode) {
            toolMode = newMode;
            return true;
        }
    }

    if (my >= menuBarHeight && !menuBar.isOpen()) return false;

    MenuResult result = menuBar.handleClick(mx, my, mapManager, tilesetManager, screenWidth);

    switch (result.action) {
        case MenuAction::NewMap:
            namingMap = true;
            mapNameInput = "";
            SDL_StartTextInput();
            break;

        case MenuAction::SaveMap: {
            const MapEntry* entry = mapManager.getActiveEntry();
            if (entry) map.save("../Maps/" + entry->file);
            break;
        }

        case MenuAction::SaveMapAs:
            namingMap = true;
            mapNameInput = "";
            SDL_StartTextInput();
            break;

        case MenuAction::SwitchMap: {
            const MapEntry* cur = mapManager.getActiveEntry();
            if (cur) map.save("../Maps/" + cur->file);
            mapManager.setActive(result.payload);
            mapManager.save(MAPS_INDEX);
            const MapEntry* next = mapManager.getActiveEntry();
            if (next) map.load("../Maps/" + next->file);
            break;
        }

        case MenuAction::LoadTileset: {
            const char* filters[] = {"*.tileset"};
            const char* res = tinyfd_openFileDialog(
                "Select Tileset File", "../Assets/Tilesets/",
                1, filters, "Tileset Files", 0);
            if (res) {
                std::string path(res);
                std::string fname = path.substr(path.find_last_of("/\\") + 1);
                TileLibrary tmp;
                if (tmp.load(path)) {
                    for (auto def : tmp.all()) {
                        tileRenderer.loadTexture(renderer, def.imagePath);
                        tileLibrary.addTile(def);
                    }
                    tilesetManager.addTileset(fname, fname);
                    tilesetManager.save(TILESETS_INDEX);
                }
            }
            break;
        }

        case MenuAction::RemoveTileset:
            tilesetManager.removeTileset(result.payload);
            tilesetManager.save(TILESETS_INDEX);
            break;

        default: break;
    }

    return result.action != MenuAction::None || menuBar.isOpen() || my < menuBarHeight;
}

void Game::handleTilesetEditorEvents(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;

        SDL_Rect modeSingle = {10, 45, 120, 26};
        SDL_Rect modeGrid   = {140, 45, 120, 26};
        SDL_Rect modeManual = {270, 45, 130, 26};

        if (mx >= modeSingle.x && mx <= modeSingle.x + modeSingle.w &&
            my >= modeSingle.y && my <= modeSingle.y + modeSingle.h) {
            tsEditor.mode = TilesetEditorMode::SingleObject;
            tsEditor.included.assign(1, false);
            SDL_StopTextInput();
            tsEditor.focused = TilesetEditorState::FocusedField::None;
        }
        else if (mx >= modeGrid.x && mx <= modeGrid.x + modeGrid.w &&
                 my >= modeGrid.y && my <= modeGrid.y + modeGrid.h) {
            tsEditor.mode = TilesetEditorMode::GridUniform;
            tsEditor.included.assign(tsEditor.count(), false);
            SDL_StopTextInput();
            tsEditor.focused = TilesetEditorState::FocusedField::None;
        }
        else if (mx >= modeManual.x && mx <= modeManual.x + modeManual.w &&
                 my >= modeManual.y && my <= modeManual.y + modeManual.h) {
            tsEditor.mode = TilesetEditorMode::GridManual;
            tsEditor.included.assign(tsEditor.count(), false);
        }

        SDL_Rect saveBtn   = {10, screenHeight - 50, 120, 36};
        SDL_Rect cancelBtn = {140, screenHeight - 50, 100, 36};
        bool clickedButton = false;

        if (mx >= saveBtn.x && mx <= saveBtn.x + saveBtn.w &&
            my >= saveBtn.y && my <= saveBtn.y + saveBtn.h) {
            clickedButton = true;
            tsEditor.applyManualInput();
            commitTilesetEditor();
            SDL_StopTextInput();
            // If there are more pending imports, open next one
            advancePendingImport();
        }
        else if (mx >= cancelBtn.x && mx <= cancelBtn.x + cancelBtn.w &&
                 my >= cancelBtn.y && my <= cancelBtn.y + cancelBtn.h) {
            clickedButton = true;
            editorMode = EditorMode::Paint;
            pendingImports.clear();
            SDL_StopTextInput();
            tsEditor.focused = TilesetEditorState::FocusedField::None;
        }

        if (!clickedButton && tsEditor.mode == TilesetEditorMode::GridManual) {
            SDL_Rect fieldW = {10, 75, 80, 24};
            SDL_Rect fieldH = {100, 75, 80, 24};
            bool clickedField = false;

            if (mx >= fieldW.x && mx <= fieldW.x + fieldW.w &&
                my >= fieldW.y && my <= fieldW.y + fieldW.h) {
                tsEditor.focused = TilesetEditorState::FocusedField::Width;
                SDL_StartTextInput();
                clickedField = true;
            }
            else if (mx >= fieldH.x && mx <= fieldH.x + fieldH.w &&
                     my >= fieldH.y && my <= fieldH.y + fieldH.h) {
                tsEditor.focused = TilesetEditorState::FocusedField::Height;
                SDL_StartTextInput();
                clickedField = true;
            }

            if (!clickedField) {
                tsEditor.focused = TilesetEditorState::FocusedField::None;
                SDL_StopTextInput();
                tsEditor.applyManualInput();
            }
        }

        if (!clickedButton) {
            const int gridOffX = 10;
            const int gridOffY = 110;
            float previewZoom  = 0.5f;

            if (tsEditor.mode == TilesetEditorMode::SingleObject) {
                int imgDrawW = static_cast<int>(tsEditor.imageW * previewZoom);
                int imgDrawH = static_cast<int>(tsEditor.imageH * previewZoom);
                SDL_Rect imgRect = {gridOffX, gridOffY, imgDrawW, imgDrawH};
                if (mx >= imgRect.x && mx <= imgRect.x + imgRect.w &&
                    my >= imgRect.y && my <= imgRect.y + imgRect.h)
                    tsEditor.included[0] = !tsEditor.included[0];
            } else {
                int cellW = static_cast<int>(tsEditor.tileW * previewZoom);
                int cellH = static_cast<int>(tsEditor.tileH * previewZoom);
                int col   = (mx - gridOffX) / cellW;
                int row   = (my - gridOffY) / cellH;
                if (col >= 0 && col < tsEditor.cols() &&
                    row >= 0 && row < tsEditor.rows()) {
                    int idx = row * tsEditor.cols() + col;
                    if (idx < (int)tsEditor.included.size())
                        tsEditor.included[idx] = !tsEditor.included[idx];
                }
            }

            for (int i = 0; i < 4; i++) {
                SDL_Rect r = {10 + i * 90, 10, 80, 28};
                if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h)
                    tsEditor.category = static_cast<TileCategory>(i);
            }
        }
    }

    if (e.type == SDL_TEXTINPUT &&
        tsEditor.focused != TilesetEditorState::FocusedField::None) {
        std::string ch(e.text.text);
        if (!ch.empty() && std::isdigit(ch[0])) {
            if (tsEditor.focused == TilesetEditorState::FocusedField::Width)
                tsEditor.inputW += ch;
            else
                tsEditor.inputH += ch;
        }
    }

    if (e.type == SDL_KEYDOWN &&
        tsEditor.focused != TilesetEditorState::FocusedField::None) {
        if (e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
            auto& field = (tsEditor.focused == TilesetEditorState::FocusedField::Width)
                          ? tsEditor.inputW : tsEditor.inputH;
            if (!field.empty()) field.pop_back();
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_RETURN) {
            tsEditor.applyManualInput();
            tsEditor.focused = TilesetEditorState::FocusedField::None;
            SDL_StopTextInput();
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
            tsEditor.applyManualInput();
            tsEditor.focused = (tsEditor.focused == TilesetEditorState::FocusedField::Width)
                ? TilesetEditorState::FocusedField::Height
                : TilesetEditorState::FocusedField::Width;
        }
    }

    if (e.type == SDL_MOUSEWHEEL &&
        tsEditor.mode == TilesetEditorMode::GridUniform) {
        tsEditor.tileW = std::max(8, tsEditor.tileW + e.wheel.y * 8);
        tsEditor.tileH = tsEditor.tileW;
        tsEditor.reset(tsEditor.imageW, tsEditor.imageH,
                       tsEditor.tileW, tsEditor.tileH);
    }
}

void Game::handleNamingInput(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT)
        mapNameInput += e.text.text;

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE && !mapNameInput.empty())
            mapNameInput.pop_back();

        if (e.key.keysym.scancode == SDL_SCANCODE_RETURN && !mapNameInput.empty()) {
            std::string file = MapManager::nameToFile(mapNameInput);
            int idx = mapManager.addMap(mapNameInput, file);
            mapManager.setActive(idx);
            mapManager.save(MAPS_INDEX);
            map = Map();
            map.init(renderer);
            namingMap = false;
            SDL_StopTextInput();
        }

        if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            namingMap = false;
            SDL_StopTextInput();
        }
    }
}

void Game::handlePaintModeEvents(const SDL_Event& e, const Uint8* keys) {

    // ── Keyboard ──────────────────────────────────────────────────────────────
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.scancode == SDL_SCANCODE_S && keys[SDL_SCANCODE_LCTRL]) {
            const MapEntry* entry = mapManager.getActiveEntry();
            if (entry) map.save("../Maps/" + entry->file);
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_L && keys[SDL_SCANCODE_LCTRL]) {
            const MapEntry* entry = mapManager.getActiveEntry();
            if (entry) map.load("../Maps/" + entry->file);
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
            placementMode = (placementMode == PlacementMode::Grid)
                ? PlacementMode::Free : PlacementMode::Grid;
            selectedObject = -1;
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_DELETE && selectedObject >= 0) {
            undoSystem.recordRemoveObject(selectedObject,
                map.getObjects()[selectedObject]);
            map.removeObject(selectedObject);
            selectedObject = -1;
        }

        // Tool shortcuts
        if (e.key.keysym.scancode == SDL_SCANCODE_H) toolMode = ToolMode::Hand;
        if (e.key.keysym.scancode == SDL_SCANCODE_P) toolMode = ToolMode::Paint;
        if (e.key.keysym.scancode == SDL_SCANCODE_E) toolMode = ToolMode::Eraser;

        // ✅ Undo/redo inside SDL_KEYDOWN where scancode is valid
        if (e.key.keysym.scancode == SDL_SCANCODE_Z && keys[SDL_SCANCODE_LCTRL]) {
            if (keys[SDL_SCANCODE_LSHIFT]) {
                if (undoSystem.canRedo()) {
                    Command cmd = undoSystem.redo();
                    std::visit([&](auto&& c) { applyRedo(c); }, cmd);
                }
            } else {
                if (undoSystem.canUndo()) {
                    Command cmd = undoSystem.undo();
                    std::visit([&](auto&& c) { applyUndo(c); }, cmd);
                }
            }
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_Y && keys[SDL_SCANCODE_LCTRL]) {
            if (undoSystem.canRedo()) {
                Command cmd = undoSystem.redo();
                std::visit([&](auto&& c) { applyRedo(c); }, cmd);
            }
        }
    }

    // ── Left click ────────────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        bool consumed = false;

        // Right panel
        if (mx >= screenWidth - panelWidth) {
            consumed = true;
            handlePanelClick(mx, my);
        }

        // ✅ Bottom bar — layer select + eye/lock — all here where mx/my/consumed exist
        if (!consumed && my >= screenHeight - bottomBarHeight) {
            for (int i = 0; i < 5; i++) {
                int bx = 10 + i * 90;
                int by = screenHeight - 54;

                SDL_Rect r    = {bx,      by,      70, 20};
                SDL_Rect eye  = {bx,      by + 22, 33, 16};
                SDL_Rect lock = {bx + 35, by + 22, 33, 16};

                if (mx >= eye.x && mx <= eye.x + eye.w &&
                    my >= eye.y && my <= eye.y + eye.h) {
                    layerStates[i].visible = !layerStates[i].visible;
                    consumed = true; break;
                }
                if (mx >= lock.x && mx <= lock.x + lock.w &&
                    my >= lock.y && my <= lock.y + lock.h) {
                    layerStates[i].locked = !layerStates[i].locked;
                    consumed = true; break;
                }
                if (mx >= r.x && mx <= r.x + r.w &&
                    my >= r.y && my <= r.y + r.h) {
                    selectedLayer = i;
                    consumed = true; break;
                }
            }
        }

        // World
        if (!consumed && my > menuBarHeight) {
            if (toolMode == ToolMode::Paint || toolMode == ToolMode::Eraser)
                undoSystem.beginStroke(toolMode == ToolMode::Eraser);

            if (toolMode == ToolMode::Hand || keys[SDL_SCANCODE_SPACE]) {
                dragging = true;
                SDL_GetMouseState(&lastMouseX, &lastMouseY);
            } else if (toolMode == ToolMode::Eraser) {
                eraseTileAtMouse();
            } else {
                float worldX = camera.x + mx / camera.zoom;
                float worldY = camera.y + my / camera.zoom;
                bool clickedObject = false;

                const auto& objects = map.getObjects();
                for (int i = (int)objects.size() - 1; i >= 0; i--) {
                    const ObjectInstance& obj = objects[i];
                    const TileDefinition* def = tileLibrary.getById(obj.tileID);
                    if (!def) continue;
                    if (worldX >= obj.x && worldX <= obj.x + def->srcW &&
                        worldY >= obj.y && worldY <= obj.y + def->srcH) {
                        selectedObject = i;
                        clickedObject  = true;
                        break;
                    }
                }
                if (!clickedObject) {
                    selectedObject = -1;
                    paintTileAtMouse();
                }
            }
        }
    }

    // ── Right click erase ─────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        int mx = e.button.x;
        int my = e.button.y;
        if (mx < screenWidth - panelWidth &&
            my > menuBarHeight &&
            my < screenHeight - bottomBarHeight) {
            undoSystem.beginStroke(true);
            eraseTileAtMouse();
            undoSystem.endStroke();
        }
    }

    // ── Mouse button up ───────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        undoSystem.endStroke();
        dragging = false;
    }

    if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_SPACE)
        dragging = false;

    // ── Mouse motion ──────────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEMOTION) {
        if (dragging && (toolMode == ToolMode::Hand || keys[SDL_SCANCODE_SPACE])) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            camera.x -= (mx - lastMouseX) / camera.zoom;
            camera.y -= (my - lastMouseY) / camera.zoom;
            lastMouseX = mx;
            lastMouseY = my;
        }
        if ((e.motion.state & SDL_BUTTON_LMASK) &&
            toolMode == ToolMode::Paint &&
            !keys[SDL_SCANCODE_SPACE])
            paintTileAtMouse();
        if ((e.motion.state & SDL_BUTTON_LMASK) &&
            toolMode == ToolMode::Eraser)
            eraseTileAtMouse();
        if (e.motion.state & SDL_BUTTON_RMASK)
            eraseTileAtMouse();
    }

    // ── Scroll / zoom ─────────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEWHEEL) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        if (mx >= screenWidth - panelWidth) {
            panelScrollY = std::max(0, panelScrollY - e.wheel.y * panelTileSize);
        } else {
            float scrollSpeed = 50.0f;
            if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
                camera.x -= e.wheel.y * scrollSpeed;
            } else if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) {
                camera.y -= e.wheel.y * scrollSpeed;
            } else {
                float worldXBefore = camera.x + mx / camera.zoom;
                float worldYBefore = camera.y + my / camera.zoom;
                camera.zoom *= (e.wheel.y > 0) ? 1.1f : 0.9f;
                camera.zoom  = std::max(0.2f, std::min(4.0f, camera.zoom));
                camera.x = worldXBefore - mx / camera.zoom;
                camera.y = worldYBefore - my / camera.zoom;
            }
        }
    }
}

void Game::handlePanelClick(int mx, int my) {
    // Tab bar
    if (my >= menuBarHeight && my < menuBarHeight + tabBarHeight) {
        int tabW = panelWidth / 4;
        int tab  = (mx - (screenWidth - panelWidth)) / tabW;
        if (tab >= 0 && tab < 4)
            activeCategory = static_cast<TileCategory>(tab);
        return;
    }

    // Import buttons
    if (my >= screenHeight - 40) {
        int thirdW = (panelWidth - 12) / 3;
        int localX = mx - (screenWidth - panelWidth);

        if (localX < thirdW + 4) {
            // Single image → always open tileset editor
            std::string path = openFileDialog();
            if (!path.empty())
                openTilesetEditor(path);
        }
        else if (localX < (thirdW + 2) * 2) {
            // Multi select
            importMultipleImages();
        }
        else {
            // Folder
            importFolder();
        }
        return;
    }

    // Tile selection
    int localX = mx - (screenWidth - panelWidth);
    int localY = my - menuBarHeight - tabBarHeight + panelScrollY;
    int cols   = panelWidth / panelTileSize;
    int col    = localX / panelTileSize;
    int row    = localY / panelTileSize;
    auto catTiles = tileLibrary.getByCategory(activeCategory);
    int  idx      = row * cols + col;
    if (idx >= 0 && idx < (int)catTiles.size())
        selectedTile = catTiles[idx].id;
}

// ── paintTileAtMouse ─────────────────────────────────────────────────────────

void Game::paintTileAtMouse() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if (mx >= screenWidth - panelWidth) return;
    if (my >= screenHeight - bottomBarHeight) return;
    if (my < menuBarHeight) return;

    // ✅ Check layer is not locked or hidden
    if (layerStates[selectedLayer].locked)  return;
    if (!layerStates[selectedLayer].visible) return;

    const TileDefinition* def = tileLibrary.getById(selectedTile);
    if (!def) return;

    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;

    if (def->category == TileCategory::Object ||
        placementMode == PlacementMode::Free) {
        float snapX = worldX;
        float snapY = worldY;
        if (placementMode == PlacementMode::Grid) {
            snapX = static_cast<int>(worldX / TILE_SIZE) * TILE_SIZE;
            snapY = static_cast<int>(worldY / TILE_SIZE) * TILE_SIZE;
        }
        ObjectInstance obj;
        obj.tileID = selectedTile;
        obj.x      = snapX;
        obj.y      = snapY;
        obj.layer  = selectedLayer;
        int idx = map.addObject(obj);
        undoSystem.recordPlaceObject(idx, obj); // ✅
    } else {
        int tileX = static_cast<int>(worldX / TILE_SIZE);
        int tileY = static_cast<int>(worldY / TILE_SIZE);
        int oldID = map.getTile(selectedLayer, tileX, tileY);
        undoSystem.recordTile(selectedLayer, tileX, tileY, oldID, selectedTile); // ✅
        map.setTile(selectedLayer, tileX, tileY, selectedTile);
    }
}

//method for erasing
void Game::eraseTileAtMouse() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if (mx >= screenWidth - panelWidth) return;
    if (my >= screenHeight - bottomBarHeight) return;
    if (my < menuBarHeight) return;

    if (layerStates[selectedLayer].locked) return;

    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;

    // Check objects first
    const auto& objects = map.getObjects();
    for (int i = (int)objects.size() - 1; i >= 0; i--) {
        const ObjectInstance& obj = objects[i];
        const TileDefinition* def = tileLibrary.getById(obj.tileID);
        if (!def) continue;
        if (worldX >= obj.x && worldX <= obj.x + def->srcW &&
            worldY >= obj.y && worldY <= obj.y + def->srcH) {
            undoSystem.recordRemoveObject(i, obj); // ✅
            map.removeObject(i);
            if (selectedObject == i) selectedObject = -1;
            else if (selectedObject > i) selectedObject--;
            return;
            }
    }

    int tileX = static_cast<int>(worldX / TILE_SIZE);
    int tileY = static_cast<int>(worldY / TILE_SIZE);
    int oldID = map.getTile(selectedLayer, tileX, tileY);
    if (oldID != Map::TILE_EMPTY) {
        undoSystem.recordTile(selectedLayer, tileX, tileY, oldID, Map::TILE_EMPTY); // ✅
        map.clearTile(selectedLayer, tileX, tileY);
    }
}

// ── update ───────────────────────────────────────────────────────────────────
void Game::update(float) {}

// ── render ───────────────────────────────────────────────────────────────────

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    map.render(renderer, camera, tileLibrary, tileRenderer, selectedObject, showGrid, layerStates);

    // Ghost preview
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if (mx < screenWidth - panelWidth &&
        !(my < menuBarHeight || my >= screenHeight - bottomBarHeight)) {
        float worldX = camera.x + mx / camera.zoom;
        float worldY = camera.y + my / camera.zoom;

        const TileDefinition* def = tileLibrary.getById(selectedTile);
        if (def) {
            float ghostX, ghostY;
            if (placementMode == PlacementMode::Free ||
                def->category == TileCategory::Object) {
                ghostX = worldX;
                ghostY = worldY;
            } else {
                ghostX = static_cast<int>(worldX / TILE_SIZE) * TILE_SIZE;
                ghostY = static_cast<int>(worldY / TILE_SIZE) * TILE_SIZE;
            }
            int drawX = static_cast<int>((ghostX - camera.x) * camera.zoom);
            int drawY = static_cast<int>((ghostY - camera.y) * camera.zoom);
            tileRenderer.renderTile(renderer, *def, drawX, drawY, camera.zoom, 150);
        }
    }

    renderPanel();
    renderBottomBar();

    // Tileset editor BEFORE menu bar — so menu bar renders on top
    if (editorMode == EditorMode::TilesetEditor)
        renderTilesetEditor();

    // ✅ Menu bar always on top of everything except naming overlay
    menuBar.render(renderer, textRenderer, mapManager, tilesetManager, screenWidth);
    toolbar.render(renderer, textRenderer, toolMode, toolbarStartX, screenWidth);

    // Naming overlay on top of absolutely everything
    if (namingMap) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect overlay = {0, 0, screenWidth, screenHeight};
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect box = {screenWidth/2 - 150, screenHeight/2 - 30, 300, 60};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &box);

        textRenderer.draw(renderer, "Map name:",
            box.x + 10, box.y + 8, {160, 160, 160, 255});
        textRenderer.draw(renderer, mapNameInput + "|",
            box.x + 10, box.y + 28);
    }

    SDL_RenderPresent(renderer);
}

// ── renderPanel ──────────────────────────────────────────────────────────────

void Game::renderPanel() {
    const int px = screenWidth - panelWidth;

    SDL_Rect bg = {px, menuBarHeight, panelWidth, screenHeight - menuBarHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &bg);

    // Tab bar
    const char* tabNames[] = {"Terrain", "Object", "Deco", "Resource"};
    int tabW = panelWidth / 4;
    for (int i = 0; i < 4; i++) {
        SDL_Rect tab = {px + i * tabW, menuBarHeight, tabW, tabBarHeight};
        if (static_cast<TileCategory>(i) == activeCategory)
            SDL_SetRenderDrawColor(renderer, 80, 130, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);
        SDL_RenderFillRect(renderer, &tab);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &tab);
        textRenderer.drawCentered(renderer, tabNames[i], tab);
    }

    // ✅ Clip rect — tiles cannot draw above tab bar or below import buttons
    SDL_Rect clipRect = {
        px,
        menuBarHeight + tabBarHeight,
        panelWidth,
        screenHeight - menuBarHeight - tabBarHeight - 42
    };
    SDL_RenderSetClipRect(renderer, &clipRect);

    // Tiles
    auto catTiles = tileLibrary.getByCategory(activeCategory);
    int  cols     = panelWidth / panelTileSize;
    int  startY   = menuBarHeight + tabBarHeight - panelScrollY;

    for (int i = 0; i < (int)catTiles.size(); i++) {
        int col   = i % cols;
        int row   = i / cols;
        int drawX = px + col * panelTileSize;
        int drawY = startY + row * panelTileSize;

        tileRenderer.renderTile(renderer, catTiles[i], drawX, drawY,
                                (float)panelTileSize / catTiles[i].srcW);

        if (catTiles[i].id == selectedTile) {
            SDL_Rect hl = {drawX, drawY, panelTileSize, panelTileSize};
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            SDL_RenderDrawRect(renderer, &hl);
        }
    }

    // Hover
    hoveredTile = -1;
    int hx, hy;
    SDL_GetMouseState(&hx, &hy);
    if (hx >= px && hx < screenWidth &&
        hy > menuBarHeight + tabBarHeight && hy < screenHeight - 40) {
        int localX = hx - px;
        int localY = hy - (menuBarHeight + tabBarHeight) + panelScrollY;
        int col    = localX / panelTileSize;
        int row    = localY / panelTileSize;
        int idx    = row * (panelWidth / panelTileSize) + col;
        if (idx >= 0 && idx < (int)catTiles.size()) {
            hoveredTile = catTiles[idx].id;
            int drawX = px + (idx % (panelWidth / panelTileSize)) * panelTileSize;
            int drawY = menuBarHeight + tabBarHeight - panelScrollY
                      + (idx / (panelWidth / panelTileSize)) * panelTileSize;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
            SDL_Rect hoverRect = {drawX, drawY, panelTileSize, panelTileSize};
            SDL_RenderFillRect(renderer, &hoverRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }

    // ✅ Remove clip rect before drawing fixed UI elements
    SDL_RenderSetClipRect(renderer, nullptr);

    // Tile name hover display
    if (hoveredTile >= 0) {
        const TileDefinition* def = tileLibrary.getById(hoveredTile);
        if (def && !def->label.empty()) {
            SDL_Rect nameBg = {px, screenHeight - 60, panelWidth, 20};
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
            SDL_RenderFillRect(renderer, &nameBg);
            textRenderer.drawCentered(renderer, def->label, nameBg,
                                      {200, 200, 200, 255});
        }
    }

    // Import buttons (always visible, outside clip)
    const int btnY   = screenHeight - 38;
    const int btnH   = 28;
    const int thirdW = (panelWidth - 12) / 3;

    SDL_Rect btn1 = {px + 4,                    btnY, thirdW, btnH};
    SDL_Rect btn2 = {px + 4 + thirdW + 2,       btnY, thirdW, btnH};
    SDL_Rect btn3 = {px + 4 + (thirdW + 2) * 2, btnY, thirdW, btnH};

    SDL_SetRenderDrawColor(renderer, 60, 100, 60, 255);
    SDL_RenderFillRect(renderer, &btn1);
    textRenderer.drawCentered(renderer, "Image", btn1);

    SDL_SetRenderDrawColor(renderer, 60, 80, 120, 255);
    SDL_RenderFillRect(renderer, &btn2);
    textRenderer.drawCentered(renderer, "Multi", btn2);

    SDL_SetRenderDrawColor(renderer, 80, 60, 120, 255);
    SDL_RenderFillRect(renderer, &btn3);
    textRenderer.drawCentered(renderer, "Folder", btn3);
}

// ── renderBottomBar ──────────────────────────────────────────────────────────

void Game::renderBottomBar() {
    SDL_Rect bar = {0, screenHeight - bottomBarHeight,
                    screenWidth - panelWidth, bottomBarHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &bar);

    for (int i = 0; i < 5; i++) {
        int bx = 10 + i * 90;
        int by = screenHeight - 54;

        // Main layer button
        SDL_Rect r = {bx, by, 70, 20};
        if (i == selectedLayer)
            SDL_SetRenderDrawColor(renderer, 200, 180, 50, 255);
        else if (!layerStates[i].visible)
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        else
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderFillRect(renderer, &r);
        textRenderer.drawCentered(renderer, "Layer " + std::to_string(i + 1), r);

        // Eye toggle
        SDL_Rect eye = {bx, by + 22, 33, 16};
        SDL_SetRenderDrawColor(renderer,
            layerStates[i].visible ? 60 : 30,
            layerStates[i].visible ? 120 : 30,
            layerStates[i].visible ? 60  : 30, 255);
        SDL_RenderFillRect(renderer, &eye);
        textRenderer.drawCentered(renderer,
            layerStates[i].visible ? "Show" : "Hide", eye,
            {200, 200, 200, 255});

        // Lock toggle
        SDL_Rect lock = {bx + 35, by + 22, 33, 16};
        SDL_SetRenderDrawColor(renderer,
            layerStates[i].locked ? 120 : 30,
            layerStates[i].locked ? 60  : 30,
            30, 255);
        SDL_RenderFillRect(renderer, &lock);
        textRenderer.drawCentered(renderer,
            layerStates[i].locked ? "Lock" : "Free", lock,
            {200, 200, 200, 255});
    }

    // Undo/redo indicators
    SDL_Color undoColor = undoSystem.canUndo()
        ? SDL_Color{200, 200, 200, 255}
        : SDL_Color{60,  60,  60,  255};
    SDL_Color redoColor = undoSystem.canRedo()
        ? SDL_Color{200, 200, 200, 255}
        : SDL_Color{60,  60,  60,  255};
    textRenderer.draw(renderer, "Ctrl+Z Undo",
        screenWidth - panelWidth - 320,
        screenHeight - bottomBarHeight + 42, undoColor);
    textRenderer.draw(renderer, "Ctrl+Y Redo",
        screenWidth - panelWidth - 200,
        screenHeight - bottomBarHeight + 42, redoColor);

    // Coordinates + zoom — unchanged from before
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;
    int   tileX  = static_cast<int>(worldX / TILE_SIZE);
    int   tileY  = static_cast<int>(worldY / TILE_SIZE);

    textRenderer.draw(renderer,
        "px " + std::to_string((int)worldX) + ", " + std::to_string((int)worldY),
        screenWidth - panelWidth - 320, screenHeight - bottomBarHeight + 8,
        {140, 140, 140, 255});
    textRenderer.draw(renderer,
        "tile " + std::to_string(tileX) + ", " + std::to_string(tileY),
        screenWidth - panelWidth - 320, screenHeight - bottomBarHeight + 24,
        {160, 160, 160, 255});
    textRenderer.draw(renderer,
        "Zoom " + std::to_string((int)(camera.zoom * 100)) + "%",
        screenWidth - panelWidth - 160, screenHeight - bottomBarHeight + 8,
        {140, 140, 140, 255});
}

// ── Tileset editor ────────────────────────────────────────────────────────────

void Game::openTilesetEditor(const std::string& imagePath) {
    if (!tileRenderer.loadTexture(renderer, imagePath)) return;

    SDL_Texture* tex = tileRenderer.getTexture(imagePath);
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);

    tsEditor.imagePath = imagePath;
    tsEditor.reset(w, h, 64, 64);
    editorMode = EditorMode::TilesetEditor;
}

void Game::commitTilesetEditor() {
    std::cout << "commit called, mode=" << (int)tsEditor.mode
              << " included count=" << tsEditor.included.size() << "\n";
    int includedCount = 0;
    for (bool b : tsEditor.included) if (b) includedCount++;
    std::cout << "tiles selected: " << includedCount << "\n" << std::flush;

    if (tsEditor.mode == TilesetEditorMode::SingleObject) {
        if (tsEditor.included[0]) {
            TileDefinition def;
            def.imagePath = tsEditor.imagePath;
            def.srcX      = 0;
            def.srcY      = 0;
            def.srcW      = tsEditor.imageW;
            def.srcH      = tsEditor.imageH;
            def.tileW     = std::max(1, tsEditor.imageW / TILE_SIZE);
            def.tileH     = std::max(1, tsEditor.imageH / TILE_SIZE);
            def.category  = tsEditor.category;
            def.label     = "";
            tileLibrary.addTile(def);
        }
    } else {
        int cols = tsEditor.cols();
        for (int i = 0; i < (int)tsEditor.included.size(); i++) {
            if (!tsEditor.included[i]) continue;
            int col = i % cols;
            int row = i / cols;
            TileDefinition def;
            def.imagePath = tsEditor.imagePath;
            def.srcX      = col * tsEditor.tileW;
            def.srcY      = row * tsEditor.tileH;
            def.srcW      = tsEditor.tileW;
            def.srcH      = tsEditor.tileH;
            def.tileW     = std::max(1, tsEditor.tileW / TILE_SIZE);
            def.tileH     = std::max(1, tsEditor.tileH / TILE_SIZE);
            def.category  = tsEditor.category;
            def.label     = "";
            tileLibrary.addTile(def);
        }
    }

    tileLibrary.save("../Assets/Tilesets/library.tileset");
    editorMode = EditorMode::Paint;
}

void Game::renderTilesetEditor() {
    // Dark overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, screenWidth - panelWidth, screenHeight};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // ── Category buttons ──────────────────────────────────────────────────
    const char* catNames[] = {"Terrain", "Object", "Deco", "Resource"};
    for (int i = 0; i < 4; i++) {
        SDL_Rect r = {10 + i * 90, 10, 80, 28};
        SDL_SetRenderDrawColor(renderer,
            static_cast<TileCategory>(i) == tsEditor.category ? 80 : 60,
            static_cast<TileCategory>(i) == tsEditor.category ? 130 : 60,
            static_cast<TileCategory>(i) == tsEditor.category ? 200 : 60,
            255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &r);
        textRenderer.drawCentered(renderer, catNames[i], r);
    }

    // ── Mode selector ─────────────────────────────────────────────────────
    struct { SDL_Rect r; const char* label; TilesetEditorMode m; } modes[] = {
        {{10,  45, 120, 26}, "Single Object", TilesetEditorMode::SingleObject},
        {{140, 45, 120, 26}, "Grid Uniform",  TilesetEditorMode::GridUniform},
        {{270, 45, 130, 26}, "Grid Manual",   TilesetEditorMode::GridManual},
    };
    for (auto& [r, label, m] : modes) {
        SDL_SetRenderDrawColor(renderer,
            tsEditor.mode == m ? 100 : 60,
            tsEditor.mode == m ? 160 : 60,
            tsEditor.mode == m ? 100 : 60,
            255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &r);
        textRenderer.drawCentered(renderer, label, r);
    }

    // ── Manual input fields ───────────────────────────────────────────────
    if (tsEditor.mode == TilesetEditorMode::GridManual) {
        SDL_Rect fieldW = {10,  75, 80, 24};
        SDL_Rect fieldH = {100, 75, 80, 24};

        auto drawField = [&](SDL_Rect r, const std::string& val,
                             bool focused, const std::string& hint) {
            SDL_SetRenderDrawColor(renderer,
                focused ? 50 : 30, focused ? 50 : 30, focused ? 70 : 30, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer,
                focused ? 100 : 80, focused ? 150 : 80, focused ? 255 : 80, 255);
            SDL_RenderDrawRect(renderer, &r);
            textRenderer.draw(renderer, hint, r.x, r.y - 14, {160, 160, 160, 255});
            std::string display = val + (focused ? "|" : "");
            textRenderer.draw(renderer, display, r.x + 4, r.y + 5);
        };

        bool wFocused = tsEditor.focused == TilesetEditorState::FocusedField::Width;
        bool hFocused = tsEditor.focused == TilesetEditorState::FocusedField::Height;
        drawField(fieldW, tsEditor.inputW, wFocused, "Width");
        drawField(fieldH, tsEditor.inputH, hFocused, "Height");

        textRenderer.draw(renderer, "(Enter to apply, Tab to switch field)",
            190, 80, {140, 140, 140, 255});

    } else if (tsEditor.mode == TilesetEditorMode::GridUniform) {
        textRenderer.draw(renderer,
            "Tile size: " + std::to_string(tsEditor.tileW) +
            " x " + std::to_string(tsEditor.tileH) +
            "   (scroll to resize)",
            10, 78, {200, 200, 200, 255});
    }

    // ── Tile grid ─────────────────────────────────────────────────────────
    const int gridOffX    = 10;
    const int gridOffY    = 110;
    float     previewZoom = 0.5f;

    if (tsEditor.mode == TilesetEditorMode::SingleObject) {
        int drawW = static_cast<int>(tsEditor.imageW * previewZoom);
        int drawH = static_cast<int>(tsEditor.imageH * previewZoom);
        SDL_Rect src = {0, 0, tsEditor.imageW, tsEditor.imageH};
        tileRenderer.renderRaw(renderer, tsEditor.imagePath, src,
                               gridOffX, gridOffY, previewZoom,
                               tsEditor.included[0] ? 255 : 60);
        if (tsEditor.included[0]) {
            SDL_SetRenderDrawColor(renderer, 80, 220, 80, 255);
            SDL_Rect border = {gridOffX, gridOffY, drawW, drawH};
            SDL_RenderDrawRect(renderer, &border);
        }
    } else {
        int cellW = static_cast<int>(tsEditor.tileW * previewZoom);
        int cellH = static_cast<int>(tsEditor.tileH * previewZoom);

        for (int i = 0; i < tsEditor.count(); i++) {
            int col   = i % tsEditor.cols();
            int row   = i / tsEditor.cols();
            int drawX = gridOffX + col * cellW;
            int drawY = gridOffY + row * cellH;

            SDL_Rect src = {
                col * tsEditor.tileW, row * tsEditor.tileH,
                tsEditor.tileW,       tsEditor.tileH
            };
            tileRenderer.renderRaw(renderer, tsEditor.imagePath, src,
                                   drawX, drawY, previewZoom,
                                   tsEditor.included[i] ? 255 : 60);
            if (tsEditor.included[i]) {
                SDL_SetRenderDrawColor(renderer, 80, 220, 80, 255);
                SDL_Rect border = {drawX, drawY, cellW, cellH};
                SDL_RenderDrawRect(renderer, &border);
            }
        }
    }

    // ── Save / Cancel ─────────────────────────────────────────────────────
    SDL_Rect saveBtn   = {10,  screenHeight - 50, 120, 36};
    SDL_Rect cancelBtn = {140, screenHeight - 50, 100, 36};

    SDL_SetRenderDrawColor(renderer, 60, 140, 60, 255);
    SDL_RenderFillRect(renderer, &saveBtn);
    textRenderer.drawCentered(renderer, "Save Tileset", saveBtn);

    SDL_SetRenderDrawColor(renderer, 140, 60, 60, 255);
    SDL_RenderFillRect(renderer, &cancelBtn);
    textRenderer.drawCentered(renderer, "Cancel", cancelBtn);
}

void Game::importSingleImage(const std::string& path) {
    if (path.empty()) return;
    if (!tileRenderer.loadTexture(renderer, path)) return;

    SDL_Texture* tex = tileRenderer.getTexture(path);
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);

    if (tsEditor.mode == TilesetEditorMode::SingleObject) {
        // Whole image as one tile
        TileDefinition def;
        def.imagePath = path;
        def.srcX      = 0;
        def.srcY      = 0;
        def.srcW      = w;
        def.srcH      = h;
        def.tileW     = std::max(1, w / TILE_SIZE);
        def.tileH     = std::max(1, h / TILE_SIZE);
        def.category  = tsEditor.category;
        def.label     = FileUtils::stemName(path);
        tileLibrary.addTile(def);

    } else {
        // Grid mode — cut into tiles using current tsEditor size
        int tw = tsEditor.tileW;
        int th = tsEditor.tileH;
        int cols = std::max(1, w / tw);
        int rows = std::max(1, h / th);
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                TileDefinition def;
                def.imagePath = path;
                def.srcX      = col * tw;
                def.srcY      = row * th;
                def.srcW      = tw;
                def.srcH      = th;
                def.tileW     = std::max(1, tw / TILE_SIZE);
                def.tileH     = std::max(1, th / TILE_SIZE);
                def.category  = tsEditor.category;
                def.label     = FileUtils::stemName(path);
                tileLibrary.addTile(def);
            }
        }
    }
}

// import logic
void Game::importMultipleImages() {
    const char* filters[] = {"*.png", "*.PNG"};
    const char* result = tinyfd_openFileDialog(
        "Select Images", "", 2, filters, "PNG Images", 1);
    if (!result) return;

    // Parse '|' separated paths
    std::string all(result);
    pendingImports.clear();
    size_t pos = 0;
    while ((pos = all.find('|')) != std::string::npos) {
        pendingImports.push_back(all.substr(0, pos));
        all.erase(0, pos + 1);
    }
    if (!all.empty()) pendingImports.push_back(all);

    pendingImportIndex = 0;
    advancePendingImport();
}

void Game::importFolder() {
    const char* result = tinyfd_selectFolderDialog("Select Image Folder", "");
    if (!result) return;

    pendingImports = FileUtils::getPNGsInFolder(result);
    pendingImportIndex = 0;
    advancePendingImport();
}

void Game::advancePendingImport() {
    // Skip already-processed index
    if (pendingImportIndex >= (int)pendingImports.size()) {
        pendingImports.clear();
        tileLibrary.save("../Assets/Tilesets/library.tileset");
        editorMode = EditorMode::Paint;
        return;
    }

    const std::string& path = pendingImports[pendingImportIndex];
    pendingImportIndex++;

    if (tsEditor.mode == TilesetEditorMode::SingleObject) {
        // Direct import — no editor needed
        importSingleImage(path);
        advancePendingImport(); // recurse to next
    } else {
        // Open editor for this image
        openTilesetEditor(path);
    }
}

// undo redo
void Game::applyUndo(const StrokeCommand& cmd) {
    for (const auto& t : cmd.tiles)
        map.setTile(t.layer, t.x, t.y, t.oldTileID);
}

void Game::applyRedo(const StrokeCommand& cmd) {
    for (const auto& t : cmd.tiles)
        map.setTile(t.layer, t.x, t.y, t.newTileID);
}

void Game::applyUndo(const PlaceObjectCommand& cmd) {
    map.removeObject(cmd.objectIndex);
    if (selectedObject == cmd.objectIndex) selectedObject = -1;
}

void Game::applyRedo(const PlaceObjectCommand& cmd) {
    map.addObject(cmd.object);
}

void Game::applyUndo(const RemoveObjectCommand& cmd) {
    // Re-insert at original index
    map.insertObject(cmd.objectIndex, cmd.object);
}

void Game::applyRedo(const RemoveObjectCommand& cmd) {
    map.removeObject(cmd.objectIndex);
}

// ── openFileDialog ────────────────────────────────────────────────────────────

std::string Game::openFileDialog() {
    const char* filters[] = {"*.png"};
    const char* result = tinyfd_openFileDialog(
        "Select Tileset Image",  // title
        "",                      // default path
        1,                       // filter count
        filters,                 // filters
        "PNG Images",            // filter description
        0                        // single select
    );
    return result ? std::string(result) : "";
}

// ── clean ─────────────────────────────────────────────────────────────────────

void Game::clean() {
    tileRenderer.clear();
    textRenderer.clean();
    toolbar.clean();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}