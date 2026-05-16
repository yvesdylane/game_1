#include "Game.h"
#include <iostream>
#include <cstdio>
#include <filesystem>
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

    if (!map.init(DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT, DEFAULT_TILE_SIZE)) {
        std::cout << "Map init failed\n"; return false;
    }
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
    if (active) {
        map.load("../Maps/" + active->file);
        centerCameraOnMap(); //
    }

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
        if (newMapDialog.active) {
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
        ToolMode newMode = showToolbar ? toolbar.handleClick(mx, my, toolMode, toolbarStartX) : toolMode;
        if (newMode != toolMode) {
            toolMode = newMode;
            return true;
        }
    }

    if (my >= menuBarHeight && !menuBar.isOpen()) return false;

    MenuResult result = menuBar.handleClick(mx, my, mapManager, tilesetManager, screenWidth);

    switch (result.action) {
        case MenuAction::NewMap:
            newMapDialog.active = true;
            newMapDialog.inputName = "";
            newMapDialog.inputTileSize = "64";
            newMapDialog.inputWidth    = "100";
            newMapDialog.inputHeight   = "100";
            newMapDialog.focused = NewMapDialog::Field::Name;
            SDL_StartTextInput();
            break;

        case MenuAction::SaveMap: {
            const MapEntry* entry = mapManager.getActiveEntry();
            if (entry) map.save("../Maps/" + entry->file);
            break;
        }

        case MenuAction::SaveMapAs:
            newMapDialog.active = true;
            newMapDialog.inputName = "";
            newMapDialog.inputTileSize = std::to_string(map.getTileSize());
            newMapDialog.inputWidth    = std::to_string(map.getWidth());
            newMapDialog.inputHeight   = std::to_string(map.getHeight());
            newMapDialog.focused = NewMapDialog::Field::Name;
            SDL_StartTextInput();
            break;

        case MenuAction::SwitchMap: {
            const MapEntry* cur = mapManager.getActiveEntry();
            if (cur) map.save("../Maps/" + cur->file);
            mapManager.setActive(result.payload);
            mapManager.save(MAPS_INDEX);
            const MapEntry* next = mapManager.getActiveEntry();
            if (next) {
                map.load("../Maps/" + next->file);
                centerCameraOnMap(); // ✅ center on switch too
            }
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

        case MenuAction::ToggleTilePanel:
            showTilePanel = !showTilePanel;
            break;

        case MenuAction::ToggleSettingsPanel:
            showSettingsPanel = !showSettingsPanel;
            break;

        case MenuAction::ToggleBottomBar:
            showBottomBar = !showBottomBar;
            break;

        case MenuAction::ToggleToolbar:
            showToolbar = !showToolbar;
            break;

        case MenuAction::ToggleGrid:
            showGrid = !showGrid;
            break;

        case MenuAction::CenterCamera:
            centerCameraOnMap();
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
    auto& d = newMapDialog;

    if (e.type == SDL_MOUSEBUTTONDOWN) {
        int mx = e.button.x;
        int my = e.button.y;

        // Field rects (centered dialog box)
        int bx = screenWidth / 2 - 160;
        int by = screenHeight / 2 - 110;

        SDL_Rect nameField = {bx + 100, by + 10,  200, 24};
        SDL_Rect tsField   = {bx + 100, by + 44,  200, 24};
        SDL_Rect wField    = {bx + 100, by + 78,  200, 24};
        SDL_Rect hField    = {bx + 100, by + 112, 200, 24};
        SDL_Rect createBtn = {bx + 10,  by + 150, 120, 30};
        SDL_Rect cancelBtn = {bx + 140, by + 150, 100, 30};

        auto hit = [&](SDL_Rect r) {
            return mx >= r.x && mx <= r.x + r.w &&
                   my >= r.y && my <= r.y + r.h;
        };

        if (hit(nameField)) { d.focused = NewMapDialog::Field::Name;     SDL_StartTextInput(); }
        if (hit(tsField))   { d.focused = NewMapDialog::Field::TileSize; SDL_StartTextInput(); }
        if (hit(wField))    { d.focused = NewMapDialog::Field::Width;    SDL_StartTextInput(); }
        if (hit(hField))    { d.focused = NewMapDialog::Field::Height;   SDL_StartTextInput(); }

        if (hit(createBtn) && !d.inputName.empty()) {
            createNewMap();
        }
        if (hit(cancelBtn)) {
            d.active = false;
            SDL_StopTextInput();
        }
    }

    if (e.type == SDL_TEXTINPUT) {
        std::string ch(e.text.text);
        switch (d.focused) {
            case NewMapDialog::Field::Name:
                d.inputName += ch; break;
            case NewMapDialog::Field::TileSize:
                if (!ch.empty() && std::isdigit(ch[0])) d.inputTileSize += ch; break;
            case NewMapDialog::Field::Width:
                if (!ch.empty() && std::isdigit(ch[0])) d.inputWidth += ch; break;
            case NewMapDialog::Field::Height:
                if (!ch.empty() && std::isdigit(ch[0])) d.inputHeight += ch; break;
        }
    }

    if (e.type == SDL_KEYDOWN) {
        // Backspace
        std::string* field = nullptr;
        switch (d.focused) {
            case NewMapDialog::Field::Name:     field = &d.inputName;     break;
            case NewMapDialog::Field::TileSize: field = &d.inputTileSize; break;
            case NewMapDialog::Field::Width:    field = &d.inputWidth;    break;
            case NewMapDialog::Field::Height:   field = &d.inputHeight;   break;
        }
        if (field && e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE
            && !field->empty())
            field->pop_back();

        // Tab cycles fields
        if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
            d.focused = static_cast<NewMapDialog::Field>(
                (static_cast<int>(d.focused) + 1) % 4);
        }

        // Enter creates map
        if (e.key.keysym.scancode == SDL_SCANCODE_RETURN && !d.inputName.empty())
            createNewMap();

        if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            d.active = false;
            SDL_StopTextInput();
        }
    }
}

void Game::handlePaintModeEvents(const SDL_Event& e, const Uint8* keys) {

    if (expandInputFocused) {
        if (e.type == SDL_TEXTINPUT) {
            std::string ch(e.text.text);
            if (!ch.empty() && std::isdigit(ch[0])) {
                expandAmountInput += ch;
                try { expandAmount = std::max(1, std::stoi(expandAmountInput)); }
                catch (...) { expandAmount = 1; }
            }
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE && !expandAmountInput.empty()) {
                expandAmountInput.pop_back();
                try { expandAmount = std::max(1, std::stoi(expandAmountInput)); }
                catch (...) { expandAmount = 1; }
            }
            if (e.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                if (expandAmountInput.empty()) expandAmountInput = "1";
                expandAmount = std::max(1, std::stoi(expandAmountInput));
                expandInputFocused = false;
                SDL_StopTextInput();
            }
            return;
        }
    }

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

        // Undo/redo inside SDL_KEYDOWN where scancode is valid
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

        // Map expand/shrink shortcuts
        if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT && keys[SDL_SCANCODE_LCTRL]) {
            if (keys[SDL_SCANCODE_LSHIFT]) map.shrinkRight(expandAmount);
            else                           map.expandRight(expandAmount);
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_LEFT && keys[SDL_SCANCODE_LCTRL]) {
            if (keys[SDL_SCANCODE_LSHIFT]) map.shrinkLeft(expandAmount);
            else                           map.expandLeft(expandAmount);
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_UP && keys[SDL_SCANCODE_LCTRL]) {
            if (keys[SDL_SCANCODE_LSHIFT]) map.shrinkTop(expandAmount);
            else                           map.expandTop(expandAmount);
        }
        if (e.key.keysym.scancode == SDL_SCANCODE_DOWN && keys[SDL_SCANCODE_LCTRL]) {
            if (keys[SDL_SCANCODE_LSHIFT]) map.shrinkBottom(expandAmount);
            else                           map.expandBottom(expandAmount);
        }
    }

    // ── Left click ────────────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        bool consumed = false;

        // Right panel
        if ((showTilePanel || showSettingsPanel) && mx >= screenWidth - panelWidth) {
            consumed = true;
            handlePanelClick(mx, my);
        }

        // ✅ Bottom bar — layer select + eye/lock — all here where mx/my/consumed exist
        if (!consumed && showBottomBar && my >= screenHeight - bottomBarHeight) {
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
        if ((!(showTilePanel || showSettingsPanel) || mx < screenWidth - panelWidth) &&
            my > menuBarHeight &&
            (!showBottomBar || my < screenHeight - bottomBarHeight)) {
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
        if ((showTilePanel || showSettingsPanel) && mx >= screenWidth - panelWidth) {
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
    if (!showTilePanel && !showSettingsPanel) return;

    const int px       = screenWidth - panelWidth;
    const int toggleH  = 20;
    const int toggleY  = menuBarHeight;
    int       toggleW  = panelWidth / 3;

    // ── Panel layout toggle ───────────────────────────────────────────────────
    if (my >= toggleY && my < toggleY + toggleH) {
        int localX = mx - px;
        int tab    = localX / toggleW;
        if (tab == 0) panelLayout = PanelLayout::TilesOnly;
        else if (tab == 1) panelLayout = PanelLayout::SettingsOnly;
        else panelLayout = PanelLayout::Both;
        return;
    }

    // Content area
    int contentY = menuBarHeight + toggleH;
    int contentH = screenHeight - contentY - 42;

    // Import buttons
    if (showTilePanel && my >= screenHeight - 40) {
        int thirdW = (panelWidth - 12) / 3;
        int localX = mx - px;
        if (localX < thirdW + 4) {
            std::string path = openFileDialog();
            if (!path.empty()) openTilesetEditor(path);
        }
        else if (localX < (thirdW + 2) * 2) importMultipleImages();
        else importFolder();
        return;
    }

    // Route to correct panel
    if (!showTilePanel) {
        handleSettingsPanelClick(mx, my, px, contentY, panelWidth);
    }
    else if (!showSettingsPanel) {
        handleTilesPanelClick(mx, my, px, contentY, panelWidth, contentH);
    }
    else if (panelLayout == PanelLayout::TilesOnly) {
        handleTilesPanelClick(mx, my, px, contentY, panelWidth, contentH);
    }
    else if (panelLayout == PanelLayout::SettingsOnly) {
        handleSettingsPanelClick(mx, my, px, contentY, panelWidth);
    }
    else { // Both
        int half = contentH / 2;
        if (!settingsOnTop) {
            if (my < contentY + half)
                handleTilesPanelClick(mx, my, px, contentY, panelWidth, half);
            else
                handleSettingsPanelClick(mx, my, px, contentY + half, panelWidth);
        } else {
            if (my < contentY + half)
                handleSettingsPanelClick(mx, my, px, contentY, panelWidth);
            else
                handleTilesPanelClick(mx, my, px, contentY + half, panelWidth, half);
        }
    }
}

void Game::handleTilesPanelClick(int mx, int my, int px, int contentY, int w, int h) {
    // Tab bar
    if (my >= contentY && my < contentY + tabBarHeight) {
        int tabW = w / 4;
        int tab  = (mx - px) / tabW;
        if (tab >= 0 && tab < 4)
            activeCategory = static_cast<TileCategory>(tab);
        return;
    }

    // Tile selection
    int localX = mx - px;
    int localY = my - (contentY + tabBarHeight) + panelScrollY;
    int cols   = w / panelTileSize;
    int col    = localX / panelTileSize;
    int row    = localY / panelTileSize;
    auto catTiles = tileLibrary.getByCategory(activeCategory);
    int  idx      = row * cols + col;
    if (idx >= 0 && idx < (int)catTiles.size())
        selectedTile = catTiles[idx].id;
}

void Game::handleSettingsPanelClick(int mx, int my, int px, int contentY, int w) {
    auto hit = [&](const SDL_Rect& r) {
        return mx >= r.x && mx < r.x + r.w &&
               my >= r.y && my < r.y + r.h;
    };

    const int pad = 6;
    const int gap = 2;
    const int bh  = 22;
    const int bw  = (w - pad * 2 - gap) / 2;

    int cy = contentY + 8;
    cy += 18; // map heading
    cy += 16; // size line
    cy += 24; // tile line
    cy += 18; // resize heading

    SDL_Rect amtField = {px + 65, cy, 50, 22};
    if (hit(amtField)) {
        expandInputFocused = true;
        SDL_StartTextInput();
        return;
    }

    expandInputFocused = false;
    SDL_StopTextInput();

    cy += 28;
    cy += 16; // expand label

    SDL_Rect eTop   = {px + pad, cy, w - pad * 2, bh};
    cy += bh + gap;
    SDL_Rect eLeft  = {px + pad, cy, bw, bh};
    SDL_Rect eRight = {px + pad + bw + gap, cy, bw, bh};
    cy += bh + gap;
    SDL_Rect eBot   = {px + pad, cy, w - pad * 2, bh};
    cy += bh + 8;

    cy += 16; // shrink label

    SDL_Rect sTop   = {px + pad, cy, w - pad * 2, bh};
    cy += bh + gap;
    SDL_Rect sLeft  = {px + pad, cy, bw, bh};
    SDL_Rect sRight = {px + pad + bw + gap, cy, bw, bh};
    cy += bh + gap;
    SDL_Rect sBot   = {px + pad, cy, w - pad * 2, bh};
    cy += bh + 8;

    SDL_Rect centerBtn = {px + pad, cy, w - pad * 2, bh};

    if (hit(eTop))   { map.expandTop(expandAmount);    centerCameraOnMap(); return; }
    if (hit(eLeft))  { map.expandLeft(expandAmount);   centerCameraOnMap(); return; }
    if (hit(eRight)) { map.expandRight(expandAmount);  centerCameraOnMap(); return; }
    if (hit(eBot))   { map.expandBottom(expandAmount); centerCameraOnMap(); return; }

    if (hit(sTop))   { map.shrinkTop(expandAmount);    centerCameraOnMap(); return; }
    if (hit(sLeft))  { map.shrinkLeft(expandAmount);   centerCameraOnMap(); return; }
    if (hit(sRight)) { map.shrinkRight(expandAmount);  centerCameraOnMap(); return; }
    if (hit(sBot))   { map.shrinkBottom(expandAmount); centerCameraOnMap(); return; }

    if (hit(centerBtn)) { centerCameraOnMap(); return; }
}

// creating map
void Game::createNewMap() {
    auto& d = newMapDialog;
    try {
        int ts = std::max(8,  std::stoi(d.inputTileSize));
        int w  = std::max(1,  std::stoi(d.inputWidth));
        int h  = std::max(1,  std::stoi(d.inputHeight));

        std::string file = MapManager::nameToFile(d.inputName);

        // Save current map first
        const MapEntry* cur = mapManager.getActiveEntry();
        if (cur) map.save("../Maps/" + cur->file);

        // Create and register new map
        int idx = mapManager.addMap(d.inputName, file);
        mapManager.setActive(idx);
        mapManager.save(MAPS_INDEX);

        // Initialize new map
        map = Map();
        map.init(w, h, ts);
        map.save("../Maps/" + file);

        // Center camera on new map
        centerCameraOnMap();

        undoSystem.clear();
        d.active = false;
        SDL_StopTextInput();
    } catch (...) {
        std::cout << "createNewMap: invalid input\n";
    }
}

void Game::centerCameraOnMap() {
    camera.zoom = 1.0f;
    camera.x = map.centerX() - (screenWidth  - panelWidth) / 2.0f;
    camera.y = map.centerY() - (screenHeight - bottomBarHeight - menuBarHeight) / 2.0f;
}

void Game::renderSettingsPanel(int x, int y, int w, int h) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    auto hit = [&](const SDL_Rect& r) {
        return mouseX >= r.x && mouseX < r.x + r.w &&
               mouseY >= r.y && mouseY < r.y + r.h;
    };

    auto drawButton = [&](const SDL_Rect& r, const std::string& label) {
        bool hovered = hit(r);
        SDL_SetRenderDrawColor(renderer,
            hovered ? 92 : 64,
            hovered ? 92 : 64,
            hovered ? 92 : 64,
            255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer,
            hovered ? 235 : 120,
            hovered ? 235 : 120,
            hovered ? 235 : 120,
            255);
        SDL_RenderDrawRect(renderer, &r);
        if (hovered) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 28);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        textRenderer.drawCentered(renderer, label, r, {245, 245, 245, 255});
    };

    int cy = y + 8;

    textRenderer.draw(renderer, "-- Map --", x + 6, cy, {210, 210, 210, 255});
    cy += 18;
    textRenderer.draw(renderer,
        "Size: " + std::to_string(map.getWidth()) +
        " x "   + std::to_string(map.getHeight()) + " tiles",
        x + 6, cy, {185, 185, 185, 255});
    cy += 16;
    textRenderer.draw(renderer,
        "Tile: " + std::to_string(map.getTileSize()) + "px",
        x + 6, cy, {185, 185, 185, 255});
    cy += 24;

    textRenderer.draw(renderer, "-- Resize --", x + 6, cy, {210, 210, 210, 255});
    cy += 18;
    textRenderer.draw(renderer, "Amount:", x + 6, cy + 4, {180, 180, 180, 255});

    SDL_Rect amtField = {x + 65, cy, 50, 22};
    bool fieldHovered = hit(amtField);
    SDL_SetRenderDrawColor(renderer,
        expandInputFocused ? 55 : fieldHovered ? 48 : 34,
        expandInputFocused ? 55 : fieldHovered ? 48 : 34,
        expandInputFocused ? 55 : fieldHovered ? 48 : 34,
        255);
    SDL_RenderFillRect(renderer, &amtField);
    SDL_SetRenderDrawColor(renderer,
        expandInputFocused ? 235 : fieldHovered ? 180 : 105,
        expandInputFocused ? 235 : fieldHovered ? 180 : 105,
        expandInputFocused ? 235 : fieldHovered ? 180 : 105,
        255);
    SDL_RenderDrawRect(renderer, &amtField);
    textRenderer.draw(renderer,
        expandAmountInput + (expandInputFocused ? "|" : ""),
        amtField.x + 4, amtField.y + 4, {245, 245, 245, 255});
    textRenderer.draw(renderer, "tiles", x + 120, cy + 4, {145, 145, 145, 255});
    cy += 28;

    const int pad = 6;
    const int gap = 2;
    const int bw  = (w - pad * 2 - gap) / 2;
    const int bh  = 22;

    textRenderer.draw(renderer, "Expand:", x + 6, cy, {200, 200, 200, 255});
    cy += 16;

    SDL_Rect eTop   = {x + pad, cy, w - pad * 2, bh};
    drawButton(eTop, "+ Top");
    cy += bh + gap;

    SDL_Rect eLeft  = {x + pad, cy, bw, bh};
    SDL_Rect eRight = {x + pad + bw + gap, cy, bw, bh};
    drawButton(eLeft, "+ Left");
    drawButton(eRight, "+ Right");
    cy += bh + gap;

    SDL_Rect eBot   = {x + pad, cy, w - pad * 2, bh};
    drawButton(eBot, "+ Bottom");
    cy += bh + 8;

    textRenderer.draw(renderer, "Shrink:", x + 6, cy, {200, 200, 200, 255});
    cy += 16;

    SDL_Rect sTop   = {x + pad, cy, w - pad * 2, bh};
    drawButton(sTop, "- Top");
    cy += bh + gap;

    SDL_Rect sLeft  = {x + pad, cy, bw, bh};
    SDL_Rect sRight = {x + pad + bw + gap, cy, bw, bh};
    drawButton(sLeft, "- Left");
    drawButton(sRight, "- Right");
    cy += bh + gap;

    SDL_Rect sBot   = {x + pad, cy, w - pad * 2, bh};
    drawButton(sBot, "- Bottom");
    cy += bh + 8;

    SDL_Rect centerBtn = {x + pad, cy, w - pad * 2, bh};
    drawButton(centerBtn, "Center Camera");
}

// ── paintTileAtMouse ─────────────────────────────────────────────────────────
void Game::paintTileAtMouse() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if ((showTilePanel || showSettingsPanel) && mx >= screenWidth - panelWidth) return;
    if (showBottomBar && my >= screenHeight - bottomBarHeight) return;
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
            snapX = static_cast<int>(worldX / map.getTileSize()) * map.getTileSize();
            snapY = static_cast<int>(worldY / map.getTileSize()) * map.getTileSize();
        }
        ObjectInstance obj;
        obj.tileID = selectedTile;
        obj.x      = snapX;
        obj.y      = snapY;
        obj.layer  = selectedLayer;
        int idx = map.addObject(obj);
        undoSystem.recordPlaceObject(idx, obj); // ✅
    } else {
        int tileX = static_cast<int>(worldX / map.getTileSize());
        int tileY = static_cast<int>(worldY / map.getTileSize());
        int oldID = map.getTile(selectedLayer, tileX, tileY);
        undoSystem.recordTile(selectedLayer, tileX, tileY, oldID, selectedTile); // ✅
        map.setTile(selectedLayer, tileX, tileY, selectedTile);
    }
}

//method for erasing
void Game::eraseTileAtMouse() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if ((showTilePanel || showSettingsPanel) && mx >= screenWidth - panelWidth) return;
    if (showBottomBar && my >= screenHeight - bottomBarHeight) return;
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

    int tileX = static_cast<int>(worldX / map.getTileSize());
    int tileY = static_cast<int>(worldY / map.getTileSize());
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
    if ((!(showTilePanel || showSettingsPanel) || mx < screenWidth - panelWidth) &&
        !(my < menuBarHeight || (showBottomBar && my >= screenHeight - bottomBarHeight))) {
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
                ghostX = static_cast<int>(worldX / map.getTileSize()) * map.getTileSize();
                ghostY = static_cast<int>(worldY / map.getTileSize()) * map.getTileSize();
            }
            int drawX = static_cast<int>((ghostX - camera.x) * camera.zoom);
            int drawY = static_cast<int>((ghostY - camera.y) * camera.zoom);
            tileRenderer.renderTile(renderer, *def, drawX, drawY, camera.zoom, 150);
        }
    }

    renderPanel();
    if (showBottomBar)
        renderBottomBar();

    // Tileset editor BEFORE menu bar — so menu bar renders on top
    if (editorMode == EditorMode::TilesetEditor)
        renderTilesetEditor();

    // ✅ Menu bar always on top of everything except naming overlay
    menuBar.render(renderer, textRenderer, mapManager, tilesetManager, screenWidth);
    if (showToolbar)
        toolbar.render(renderer, textRenderer, toolMode, toolbarStartX, screenWidth);

    // Map craetion  overlay on top of absolutely everything
    if (newMapDialog.active) {
        auto& d = newMapDialog;
        int bx = screenWidth / 2 - 160;
        int by = screenHeight / 2 - 110;

        // Dark overlay
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect overlay = {0, 0, screenWidth, screenHeight};
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // Dialog box
        SDL_Rect box = {bx, by, 320, 200};
        SDL_SetRenderDrawColor(renderer, 50, 50, 55, 255);
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_RenderDrawRect(renderer, &box);

        // Title
        textRenderer.draw(renderer, "New Map",
            bx + 10, by + 10, {220, 220, 220, 255});

        // Fields
        struct FieldDef {
            const char* label;
            std::string* value;
            NewMapDialog::Field field;
            int y;
            const char* hint;
        } fields[] = {
            {"Name:",      &d.inputName,     NewMapDialog::Field::Name,     by + 34,  ""},
            {"Tile size:", &d.inputTileSize, NewMapDialog::Field::TileSize, by + 68,  "px (fixed after creation)"},
            {"Width:",     &d.inputWidth,    NewMapDialog::Field::Width,    by + 102, "tiles"},
            {"Height:",    &d.inputHeight,   NewMapDialog::Field::Height,   by + 136, "tiles"},
        };

        for (auto& f : fields) {
            textRenderer.draw(renderer, f.label, bx + 10, f.y + 5,
                {160, 160, 160, 255});

            SDL_Rect fr = {bx + 100, f.y, 200, 24};
            bool focused = d.focused == f.field;
            SDL_SetRenderDrawColor(renderer,
                focused ? 40 : 25, focused ? 40 : 25, focused ? 60 : 25, 255);
            SDL_RenderFillRect(renderer, &fr);
            SDL_SetRenderDrawColor(renderer,
                focused ? 100 : 70, focused ? 150 : 70, focused ? 255 : 70, 255);
            SDL_RenderDrawRect(renderer, &fr);

            std::string display = *f.value + (focused ? "|" : "");
            textRenderer.draw(renderer, display, fr.x + 4, fr.y + 5);

            if (!std::string(f.hint).empty())
                textRenderer.draw(renderer, f.hint,
                    fr.x + fr.w + 6, fr.y + 5, {100, 100, 100, 255});
        }

        // Buttons
        SDL_Rect createBtn = {bx + 10,  by + 162, 120, 28};
        SDL_Rect cancelBtn = {bx + 140, by + 162, 100, 28};

        bool canCreate = !d.inputName.empty();
        SDL_SetRenderDrawColor(renderer, canCreate ? 60 : 30,
                                         canCreate ? 120 : 50,
                                         canCreate ? 60  : 30, 255);
        SDL_RenderFillRect(renderer, &createBtn);
        textRenderer.drawCentered(renderer, "Create", createBtn);

        SDL_SetRenderDrawColor(renderer, 120, 50, 50, 255);
        SDL_RenderFillRect(renderer, &cancelBtn);
        textRenderer.drawCentered(renderer, "Cancel", cancelBtn);
}

    SDL_RenderPresent(renderer);
}

// ── renderPanel ──────────────────────────────────────────────────────────────

void Game::renderPanel() {
    if (!showTilePanel && !showSettingsPanel) return;

    const int px = screenWidth - panelWidth;

    // Background
    SDL_Rect bg = {px, menuBarHeight, panelWidth, screenHeight - menuBarHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &bg);

    // ── Panel layout toggle bar ───────────────────────────────────────────────
    const int toggleH = 20;
    const int toggleY = menuBarHeight;
    int toggleW = panelWidth / 3;

    struct { const char* label; PanelLayout mode; } toggles[] = {
        {"Tiles",    PanelLayout::TilesOnly},
        {"Settings", PanelLayout::SettingsOnly},
        {"Both",     PanelLayout::Both},
    };
    for (int i = 0; i < 3; i++) {
        SDL_Rect t = {px + i * toggleW, toggleY, toggleW, toggleH};
        SDL_SetRenderDrawColor(renderer,
            panelLayout == toggles[i].mode ? 70 : 45,
            panelLayout == toggles[i].mode ? 70 : 45,
            panelLayout == toggles[i].mode ? 90 : 45, 255);
        SDL_RenderFillRect(renderer, &t);
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderDrawRect(renderer, &t);
        textRenderer.drawCentered(renderer, toggles[i].label, t,
            {180, 180, 180, 255});
    }

    // Content area starts below toggle bar
    int contentY    = menuBarHeight + toggleH;
    int contentH    = screenHeight - contentY - (showTilePanel ? 42 : 2);

    if (!showTilePanel) {
        renderSettingsPanel(px, contentY, panelWidth, contentH);
    }
    else if (!showSettingsPanel) {
        renderTilesPanelContent(px, contentY, panelWidth, contentH);
    }
    else if (panelLayout == PanelLayout::TilesOnly) {
        renderTilesPanelContent(px, contentY, panelWidth, contentH);
    }
    else if (panelLayout == PanelLayout::SettingsOnly) {
        renderSettingsPanel(px, contentY, panelWidth, contentH);
    }
    else { // Both
        int half = contentH / 2;
        if (!settingsOnTop) {
            renderTilesPanelContent(px, contentY,        panelWidth, half);
            renderSettingsPanel    (px, contentY + half, panelWidth, half);
        } else {
            renderSettingsPanel    (px, contentY,        panelWidth, half);
            renderTilesPanelContent(px, contentY + half, panelWidth, half);
        }
    }

    if (!showTilePanel) return;

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

void Game::renderTilesPanelContent(int x, int y, int w, int h) {
    const int px = x;

    // Tab bar at top of this content area
    const char* tabNames[] = {"Terrain", "Object", "Deco", "Resource"};
    int tabW = w / 4;
    for (int i = 0; i < 4; i++) {
        SDL_Rect tab = {px + i * tabW, y, tabW, tabBarHeight};
        if (static_cast<TileCategory>(i) == activeCategory)
            SDL_SetRenderDrawColor(renderer, 80, 130, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);
        SDL_RenderFillRect(renderer, &tab);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &tab);
        textRenderer.drawCentered(renderer, tabNames[i], tab);
    }

    // Clip to tile area — below tab bar, above bottom of this content block
    SDL_Rect clipRect = {px, y + tabBarHeight, w, h - tabBarHeight};
    SDL_RenderSetClipRect(renderer, &clipRect);

    auto catTiles = tileLibrary.getByCategory(activeCategory);
    int  cols     = w / panelTileSize;
    int  startY   = y + tabBarHeight - panelScrollY;

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

    // Hover detection
    hoveredTile = -1;
    int hx, hy;
    SDL_GetMouseState(&hx, &hy);
    if (hx >= px && hx < px + w &&
        hy > y + tabBarHeight && hy < y + h) {
        int localX = hx - px;
        int localY = hy - (y + tabBarHeight) + panelScrollY;
        int col    = localX / panelTileSize;
        int row    = localY / panelTileSize;
        int idx    = row * (w / panelTileSize) + col;
        if (idx >= 0 && idx < (int)catTiles.size()) {
            hoveredTile = catTiles[idx].id;
            int drawX = px + (idx % (w / panelTileSize)) * panelTileSize;
            int drawY = y + tabBarHeight - panelScrollY
                      + (idx / (w / panelTileSize)) * panelTileSize;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
            SDL_Rect hoverRect = {drawX, drawY, panelTileSize, panelTileSize};
            SDL_RenderFillRect(renderer, &hoverRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    // Hover name
    if (hoveredTile >= 0) {
        const TileDefinition* def = tileLibrary.getById(hoveredTile);
        if (def && !def->label.empty()) {
            SDL_Rect nameBg = {px, y + h - 20, w, 20};
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
            SDL_RenderFillRect(renderer, &nameBg);
            textRenderer.drawCentered(renderer, def->label, nameBg,
                                      {200, 200, 200, 255});
        }
    }
}

// ── renderBottomBar ──────────────────────────────────────────────────────────

void Game::renderBottomBar() {
    const int barWidth = (showTilePanel || showSettingsPanel)
        ? screenWidth - panelWidth
        : screenWidth;
    SDL_Rect bar = {0, screenHeight - bottomBarHeight,
                    barWidth, bottomBarHeight};
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
        barWidth - 320,
        screenHeight - bottomBarHeight + 42, undoColor);
    textRenderer.draw(renderer, "Ctrl+Y Redo",
        barWidth - 200,
        screenHeight - bottomBarHeight + 42, redoColor);

    // Coordinates + zoom — unchanged from before
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;
    int tileX = static_cast<int>(worldX / map.getTileSize());
    int tileY = static_cast<int>(worldY / map.getTileSize());

    textRenderer.draw(renderer,
        "px " + std::to_string((int)worldX) + ", " + std::to_string((int)worldY),
        barWidth - 320, screenHeight - bottomBarHeight + 8,
        {140, 140, 140, 255});
    textRenderer.draw(renderer,
        "tile " + std::to_string(tileX) + ", " + std::to_string(tileY),
        barWidth - 320, screenHeight - bottomBarHeight + 24,
        {160, 160, 160, 255});
    textRenderer.draw(renderer,
        "Zoom " + std::to_string((int)(camera.zoom * 100)) + "%",
        barWidth - 160, screenHeight - bottomBarHeight + 8,
        {140, 140, 140, 255});
}

// ── Tileset editor ────────────────────────────────────────────────────────────

std::string Game::copyImageToTilesetImages(const std::string& imagePath) {
    namespace fs = std::filesystem;

    const fs::path source(imagePath);
    const fs::path targetDir = fs::path("../Assets/Tilesets/Images");

    try {
        if (!fs::exists(source) || !fs::is_regular_file(source))
            return imagePath;

        fs::create_directories(targetDir);

        fs::path target = targetDir / source.filename();
        const fs::path absSource = fs::weakly_canonical(source);
        const fs::path absTargetDir = fs::weakly_canonical(targetDir);

        if (absSource.parent_path() == absTargetDir)
            return (targetDir / source.filename()).generic_string();

        int suffix = 1;
        while (fs::exists(target)) {
            target = targetDir / (source.stem().string() + "_" +
                                  std::to_string(suffix) + source.extension().string());
            suffix++;
        }

        fs::copy_file(source, target);
        return target.generic_string();
    } catch (const std::exception& e) {
        std::cout << "Failed to copy tileset image: " << e.what() << "\n";
        return imagePath;
    }
}

void Game::openTilesetEditor(const std::string& imagePath) {
    std::string copiedPath = copyImageToTilesetImages(imagePath);
    if (!tileRenderer.loadTexture(renderer, copiedPath)) return;

    SDL_Texture* tex = tileRenderer.getTexture(copiedPath);
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);

    tsEditor.imagePath = copiedPath;
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
            def.tileW = std::max(1, def.srcW / map.getTileSize());
            def.tileH = std::max(1, def.srcH / map.getTileSize());
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
            def.tileW     = std::max(1, def.srcW / map.getTileSize());
            def.tileH     = std::max(1, def.srcH / map.getTileSize());
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
    std::string copiedPath = copyImageToTilesetImages(path);
    if (!tileRenderer.loadTexture(renderer, copiedPath)) return;

    SDL_Texture* tex = tileRenderer.getTexture(copiedPath);
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);

    if (tsEditor.mode == TilesetEditorMode::SingleObject) {
        // Whole image as one tile
        TileDefinition def;
        def.imagePath = copiedPath;
        def.srcX      = 0;
        def.srcY      = 0;
        def.srcW      = w;
        def.srcH      = h;
        def.tileW = std::max(1, def.srcW / map.getTileSize());
        def.tileH = std::max(1, def.srcH / map.getTileSize());
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
                def.imagePath = copiedPath;
                def.srcX      = col * tw;
                def.srcY      = row * th;
                def.srcW      = tw;
                def.srcH      = th;
                def.tileW = std::max(1, def.srcW / map.getTileSize());
                def.tileH = std::max(1, def.srcH / map.getTileSize());
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