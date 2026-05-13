#include "Game.h"
#include <iostream>
#include <cstdio>

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

        if (e.type == SDL_QUIT) { running = false; }

        // ── Menu bar ──────────────────────────────────────────────────────────────────
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;

        // Offset my for menu bar height check
        if (my < menuBarHeight || menuBar.isOpen()) {
            MenuResult result = menuBar.handleClick(mx, my, mapManager, tilesetManager, screenWidth);

            switch (result.action) {

                case MenuAction::NewMap: {
                    // Prompt for name via simple input overlay
                    namingMap    = true;
                    mapNameInput = "";
                    SDL_StartTextInput();
                    break;
                }

                case MenuAction::SaveMap: {
                    const MapEntry* entry = mapManager.getActiveEntry();
                    if (entry) map.save("../Maps/" + entry->file);
                    break;
                }

                case MenuAction::SaveMapAs: {
                    namingMap    = true;
                    mapNameInput = "";
                    SDL_StartTextInput();
                    break;
                }

                case MenuAction::SwitchMap: {
                    // Save current first
                    const MapEntry* cur = mapManager.getActiveEntry();
                    if (cur) map.save("../Maps/" + cur->file);
                    // Switch
                    mapManager.setActive(result.payload);
                    mapManager.save(MAPS_INDEX);
                    const MapEntry* next = mapManager.getActiveEntry();
                    if (next) map.load("../Maps/" + next->file);
                    break;
                }

                case MenuAction::LoadTileset: {
                    // Open file dialog for .tileset file
                    FILE* f = popen("zenity --file-selection --file-filter='Tileset files | *.tileset'", "r");
                    if (f) {
                        char buf[512] = {};
                        fgets(buf, sizeof(buf), f);
                        pclose(f);
                        std::string path(buf);
                        if (!path.empty() && path.back() == '\n') path.pop_back();
                        if (!path.empty()) {
                            // Extract filename
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
                    }
                    break;
                }

                case MenuAction::RemoveTileset: {
                    // For now just remove from index, keep tiles in memory this session
                    tilesetManager.removeTileset(result.payload);
                    tilesetManager.save(TILESETS_INDEX);
                    break;
                }

                default: break;
            }

            // If menu bar consumed the click, don't process world clicks
            if (result.action != MenuAction::None || menuBar.isOpen() || my < menuBarHeight)
                continue;
        }
    }

    // ── New map naming overlay input ──────────────────────────────────────────────
    if (namingMap) {
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
                map = Map(); // clear current map
                map.init(renderer);
                namingMap = false;
                SDL_StopTextInput();
            }

            if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                namingMap = false;
                SDL_StopTextInput();
            }
        }
        continue; // block all other input while naming
    }

        // ── Tileset editor mode gets its own input ────────────────────────────
        if (editorMode == EditorMode::TilesetEditor) {

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;

                SDL_Rect modeSingle = {10,  45, 120, 26};
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

                SDL_Rect saveBtn   = {10,  screenHeight - 50, 120, 36};
                SDL_Rect cancelBtn = {140, screenHeight - 50, 100, 36};
                bool clickedButton = false;

                if (mx >= saveBtn.x && mx <= saveBtn.x + saveBtn.w &&
                    my >= saveBtn.y && my <= saveBtn.y + saveBtn.h) {
                    clickedButton = true;
                    tsEditor.applyManualInput();
                    commitTilesetEditor();
                    SDL_StopTextInput();
                }
                else if (mx >= cancelBtn.x && mx <= cancelBtn.x + cancelBtn.w &&
                         my >= cancelBtn.y && my <= cancelBtn.y + cancelBtn.h) {
                    clickedButton = true;
                    editorMode = EditorMode::Paint;
                    SDL_StopTextInput();
                    tsEditor.focused = TilesetEditorState::FocusedField::None;
                }

                if (!clickedButton && tsEditor.mode == TilesetEditorMode::GridManual) {
                    SDL_Rect fieldW = {10,  75, 80, 24};
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
                            my >= imgRect.y && my <= imgRect.y + imgRect.h) {
                            tsEditor.included[0] = !tsEditor.included[0];
                        }
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
                }

                if (!clickedButton) {
                    for (int i = 0; i < 4; i++) {
                        SDL_Rect r = {10 + i * 90, 10, 80, 28};
                        if (mx >= r.x && mx <= r.x + r.w &&
                            my >= r.y && my <= r.y + r.h) {
                            tsEditor.category = static_cast<TileCategory>(i);
                        }
                    }
                }
            }

            // Keyboard input for manual fields
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

            continue; // ← stops here, nothing below runs in editor mode
        }

        // ── Normal paint mode ─────────────────────────────────────────────────

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.scancode == SDL_SCANCODE_S && keys[SDL_SCANCODE_LCTRL])
                map.save("../Maps/level1.map");
            if (e.key.keysym.scancode == SDL_SCANCODE_L && keys[SDL_SCANCODE_LCTRL])
                map.load("../Maps/level1.map");

            // Tab toggles placement mode
            if (e.key.keysym.scancode == SDL_SCANCODE_TAB) {
                placementMode = (placementMode == PlacementMode::Grid)
                    ? PlacementMode::Free
                    : PlacementMode::Grid;
                selectedObject = -1; // deselect on mode switch
            }

            // Delete removes selected object
            if (e.key.keysym.scancode == SDL_SCANCODE_DELETE && selectedObject >= 0) {
                map.removeObject(selectedObject);
                selectedObject = -1;
            }
        }

        // ── Left click ────────────────────────────────────────────────────────
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x;
            int my = e.button.y;
            bool consumed = false;

            if (mx >= screenWidth - panelWidth) {
                consumed = true;
                if (my >= menuBarHeight && my < menuBarHeight + tabBarHeight) {
                    int tabW = panelWidth / 4;
                    int tab  = (mx - (screenWidth - panelWidth)) / tabW;
                    if (tab >= 0 && tab < 4)
                        activeCategory = static_cast<TileCategory>(tab);
                }
                else if (my >= screenHeight - 40) {
                    std::string path = openFileDialog();
                    if (!path.empty())
                        openTilesetEditor(path);
                }
                else {
                    int localX  = mx - (screenWidth - panelWidth);
                    int localY  = my - tabBarHeight + panelScrollY;
                    int cols    = panelWidth / panelTileSize;
                    int col     = localX / panelTileSize;
                    int row     = localY / panelTileSize;
                    auto catTiles = tileLibrary.getByCategory(activeCategory);
                    int  idx      = row * cols + col;
                    if (idx >= 0 && idx < (int)catTiles.size())
                        selectedTile = catTiles[idx].id;
                }
            }

            if (!consumed && my >= screenHeight - bottomBarHeight) {
                for (int i = 0; i < 5; i++) {
                    SDL_Rect r = {20 + i * 120, screenHeight - 50, 100, 36};
                    if (mx >= r.x && mx <= r.x + r.w &&
                        my >= r.y && my <= r.y + r.h) {
                        selectedLayer = i;
                        consumed = true;
                        break;
                    }
                }
            }

            if (!consumed) {
                if (keys[SDL_SCANCODE_SPACE]) {
                    dragging = true;
                    SDL_GetMouseState(&lastMouseX, &lastMouseY);
                } else {
                    float worldX = camera.x + mx / camera.zoom;
                    float worldY = camera.y + my / camera.zoom;

                    // Check if we clicked an existing object first
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

        // ── Right click erase ─────────────────────────────────────────────────
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
            int mx = e.button.x;
            int my = e.button.y;
            if (mx < screenWidth - panelWidth && my < screenHeight - bottomBarHeight)
                eraseTileAtMouse();
        }

        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            dragging = false;

        if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_SPACE)
            dragging = false;

        // ── Mouse motion ──────────────────────────────────────────────────────
        if (e.type == SDL_MOUSEMOTION) {
            if (dragging && keys[SDL_SCANCODE_SPACE]) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                camera.x -= (mx - lastMouseX) / camera.zoom;
                camera.y -= (my - lastMouseY) / camera.zoom;
                lastMouseX = mx;
                lastMouseY = my;
            }
            // Left drag = paint
            if ((e.motion.state & SDL_BUTTON_LMASK) && !keys[SDL_SCANCODE_SPACE])
                paintTileAtMouse();
            // Right drag = erase
            if (e.motion.state & SDL_BUTTON_RMASK) {
                int mx = e.motion.x;
                int my = e.motion.y;
                if (mx < screenWidth - panelWidth && my < screenHeight - bottomBarHeight)
                    eraseTileAtMouse();
            }
        }

        // ── Scroll / zoom ─────────────────────────────────────────────────────
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
}
// ── paintTileAtMouse ─────────────────────────────────────────────────────────

void Game::paintTileAtMouse() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if (mx >= screenWidth - panelWidth) return;
    if (my >= screenHeight - bottomBarHeight) return;
    if (my < menuBarHeight) return;

    const TileDefinition* def = tileLibrary.getById(selectedTile);
    if (!def) return;

    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;

    if (def->category == TileCategory::Object ||
        placementMode == PlacementMode::Free) {
        // Free placement — pixel position
        float snapX = worldX;
        float snapY = worldY;

        if (placementMode == PlacementMode::Grid) {
            // Still snap to grid even for objects if in grid mode
            snapX = static_cast<int>(worldX / TILE_SIZE) * TILE_SIZE;
            snapY = static_cast<int>(worldY / TILE_SIZE) * TILE_SIZE;
        }

        ObjectInstance obj;
        obj.tileID = selectedTile;
        obj.x      = snapX;
        obj.y      = snapY;
        obj.layer  = selectedLayer;
        map.addObject(obj);

        } else {
            // Grid tile placement for terrain/decoration/resource
            int tileX = static_cast<int>(worldX / TILE_SIZE);
            int tileY = static_cast<int>(worldY / TILE_SIZE);
            map.setTile(selectedLayer, tileX, tileY, selectedTile);
        }
}
// Add this new method for erasing
void Game::eraseTileAtMouse() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    if (mx >= screenWidth - panelWidth) return;
    if (my >= screenHeight - bottomBarHeight) return;
    if (my < menuBarHeight) return;

    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;

    int tileX = static_cast<int>(worldX / TILE_SIZE);
    int tileY = static_cast<int>(worldY / TILE_SIZE);

    map.clearTile(selectedLayer, tileX, tileY);
}
// ── update ───────────────────────────────────────────────────────────────────

void Game::update(float) {}

// ── render ───────────────────────────────────────────────────────────────────

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    map.render(renderer, camera, tileLibrary, tileRenderer, selectedObject);

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

    // Category tab bar
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

    // Tiles for active category
    auto catTiles = tileLibrary.getByCategory(activeCategory);
    int  cols     = panelWidth / panelTileSize;
    int  startY   = menuBarHeight + tabBarHeight - panelScrollY; // ✅

    for (int i = 0; i < (int)catTiles.size(); i++) {
        int col   = i % cols;
        int row   = i / cols;
        int drawX = px + col * panelTileSize;
        int drawY = startY + row * panelTileSize;

        if (drawY + panelTileSize < menuBarHeight + tabBarHeight) continue; // ✅
        if (drawY > screenHeight - 40) continue;

        tileRenderer.renderTile(renderer, catTiles[i], drawX, drawY,
                                (float)panelTileSize / catTiles[i].srcW);

        if (catTiles[i].id == selectedTile) {
            SDL_Rect hl = {drawX, drawY, panelTileSize, panelTileSize};
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            SDL_RenderDrawRect(renderer, &hl);
        }
    }

    // "+ Import" button
    SDL_Rect importBtn = {px + 10, screenHeight - 36, panelWidth - 20, 28};
    SDL_SetRenderDrawColor(renderer, 60, 100, 60, 255);
    SDL_RenderFillRect(renderer, &importBtn);
    textRenderer.drawCentered(renderer, "+ Import", importBtn);
}

// ── renderBottomBar ──────────────────────────────────────────────────────────

void Game::renderBottomBar() {
    SDL_Rect bar = {0, screenHeight - bottomBarHeight,
                    screenWidth - panelWidth, bottomBarHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &bar);

    for (int i = 0; i < 5; i++) {
        SDL_Rect r = {20 + i * 120, screenHeight - 50, 100, 36};
        if (i == selectedLayer)
            SDL_SetRenderDrawColor(renderer, 200, 180, 50, 255);
        else
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderFillRect(renderer, &r);
        textRenderer.drawCentered(renderer, "Layer " + std::to_string(i + 1), r);
    }

    std::string modeStr = (placementMode == PlacementMode::Grid)
    ? "Mode: Grid  [Tab]"
    : "Mode: Free  [Tab]";
textRenderer.draw(renderer, modeStr,
    screenWidth - panelWidth - 160,
    screenHeight - bottomBarHeight + 20,
    placementMode == PlacementMode::Free
        ? SDL_Color{100, 220, 100, 255}
        : SDL_Color{160, 160, 160, 255});
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

// ── openFileDialog ────────────────────────────────────────────────────────────

std::string Game::openFileDialog() {
    FILE* f = popen("zenity --file-selection --file-filter='PNG files | *.png'", "r");
    if (!f) return "";
    char buf[512] = {};
    fgets(buf, sizeof(buf), f);
    pclose(f);
    std::string result(buf);
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}

// ── clean ─────────────────────────────────────────────────────────────────────

void Game::clean() {
    tileRenderer.clear();
    textRenderer.clean();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}