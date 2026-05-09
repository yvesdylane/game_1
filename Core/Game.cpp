#include "Game.h"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "../World/WorldConfig.h"

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

    if (!textRenderer.load("../Assets/Fonts/DejaVuSans.ttf", 13)) {
        std::cout << "Font load failed\n";
        return false;
    }
    std::cout << "[5b] font ok\n" << std::flush;

    std::cout << "[5] IMG ok\n" << std::flush;
    camera = {0, 0, 1.0f};

    if (!map.init(renderer)) { std::cout << "Map init failed\n"; return false; }
    std::cout << "[6] map ok\n" << std::flush;
    // Load saved library if it exists
    tileLibrary.load("../Assets/Tilesets/library.tileset");
    std::cout << "[7] library load attempted\n" << std::flush;
    // Pre-load textures for all known tiles
    for (const auto& def : tileLibrary.all())
        tileRenderer.loadTexture(renderer, def.imagePath);
    std::cout << "[8] textures loaded\n" << std::flush;
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

        // ── Tileset editor mode gets its own input ────────────────────────────
        if (editorMode == EditorMode::TilesetEditor) {

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;

                // Hit-test: is the click inside the image grid area?
                // We render the image starting at (10, 60) in the overlay
                const int gridOffX = 10;
                const int gridOffY = 60;
                float previewZoom  = 0.5f;
                int cellW = static_cast<int>(tsEditor.tileW * previewZoom);
                int cellH = static_cast<int>(tsEditor.tileH * previewZoom);

                int col = (mx - gridOffX) / cellW;
                int row = (my - gridOffY) / cellH;

                if (col >= 0 && col < tsEditor.cols() &&
                    row >= 0 && row < tsEditor.rows()) {
                    int idx = row * tsEditor.cols() + col;
                    if (idx < (int)tsEditor.included.size())
                        tsEditor.included[idx] = !tsEditor.included[idx];
                }

                // "Save" button — placed at bottom of overlay
                SDL_Rect saveBtn = {10, screenHeight - 50, 120, 36};
                if (mx >= saveBtn.x && mx <= saveBtn.x + saveBtn.w &&
                    my >= saveBtn.y && my <= saveBtn.y + saveBtn.h) {
                    commitTilesetEditor();
                }

                // "Cancel" button
                SDL_Rect cancelBtn = {140, screenHeight - 50, 100, 36};
                if (mx >= cancelBtn.x && mx <= cancelBtn.x + cancelBtn.w &&
                    my >= cancelBtn.y && my <= cancelBtn.y + cancelBtn.h) {
                    editorMode = EditorMode::Paint;
                }

                // Category selector buttons (T / O / D / R) inside overlay
                const char* catLabels[] = {"Terrain","Object","Deco","Resource"};
                for (int i = 0; i < 4; i++) {
                    SDL_Rect r = {10 + i * 90, 10, 80, 28};
                    if (mx >= r.x && mx <= r.x + r.w &&
                        my >= r.y && my <= r.y + r.h) {
                        tsEditor.category = static_cast<TileCategory>(i);
                    }
                }
            }

            // Scroll tile size with mouse wheel inside editor
            if (e.type == SDL_MOUSEWHEEL) {
                tsEditor.tileW = std::max(8, tsEditor.tileW + e.wheel.y * 8);
                tsEditor.tileH = tsEditor.tileW; // keep square for now
                tsEditor.reset(tsEditor.imageW, tsEditor.imageH,
                               tsEditor.tileW, tsEditor.tileH);
            }

            continue; // don't process paint events while in editor
        }

        // ── Normal paint mode ─────────────────────────────────────────────────

        if (e.type == SDL_KEYDOWN) {
            // Ctrl+S save map
            if (e.key.keysym.scancode == SDL_SCANCODE_S && keys[SDL_SCANCODE_LCTRL])
                map.save("../Maps/level1.map");
            // Ctrl+L load map
            if (e.key.keysym.scancode == SDL_SCANCODE_L && keys[SDL_SCANCODE_LCTRL])
                map.load("../Maps/level1.map");
        }

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x;
            int my = e.button.y;
            bool consumed = false;

            // ── Right panel ───────────────────────────────────────────────────
            if (mx >= screenWidth - panelWidth) {
                consumed = true;

                // Category tab bar
                if (my < tabBarHeight) {
                    int tabW = panelWidth / 4;
                    int tab  = (mx - (screenWidth - panelWidth)) / tabW;
                    if (tab >= 0 && tab < 4)
                        activeCategory = static_cast<TileCategory>(tab);
                }
                // "+ Import" button at bottom of panel
                else if (my >= screenHeight - 40) {
                    std::string path = openFileDialog();
                    if (!path.empty())
                        openTilesetEditor(path);
                }
                // Tile selection in panel
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

            // ── Bottom bar (layer selection) ──────────────────────────────────
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

            // ── World paint ───────────────────────────────────────────────────
            if (!consumed) {
                if (keys[SDL_SCANCODE_SPACE]) {
                    dragging = true;
                    SDL_GetMouseState(&lastMouseX, &lastMouseY);
                } else {
                    paintTileAtMouse();
                }
            }
        }

        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            dragging = false;

        if (e.type == SDL_KEYUP && e.key.keysym.scancode == SDL_SCANCODE_SPACE)
            dragging = false;

        if (e.type == SDL_MOUSEMOTION) {
            if (dragging && keys[SDL_SCANCODE_SPACE]) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                camera.x -= (mx - lastMouseX) / camera.zoom;
                camera.y -= (my - lastMouseY) / camera.zoom;
                lastMouseX = mx;
                lastMouseY = my;
            }
            if ((e.motion.state & SDL_BUTTON_LMASK) && !keys[SDL_SCANCODE_SPACE])
                paintTileAtMouse();
        }

        // Panel scroll
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

    // Don't paint over UI
    if (mx >= screenWidth - panelWidth) return;
    if (my >= screenHeight - bottomBarHeight) return;

    float worldX = camera.x + mx / camera.zoom;
    float worldY = camera.y + my / camera.zoom;

    int tileX = static_cast<int>(worldX / TILE_SIZE);
    int tileY = static_cast<int>(worldY / TILE_SIZE);

    const TileDefinition* def = tileLibrary.getById(selectedTile);
    if (!def) return;

    // Stamp all cells the object occupies
    for (int dy = 0; dy < def->tileH; dy++)
        for (int dx = 0; dx < def->tileW; dx++)
            map.setTile(selectedLayer, tileX + dx, tileY + dy, selectedTile);
}

// ── update ───────────────────────────────────────────────────────────────────

void Game::update(float) {}

// ── render ───────────────────────────────────────────────────────────────────

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // ── World ─────────────────────────────────────────────────────────────────
    // Map render — still uses old TileSet internally for now (Step 5 will swap)
    map.render(renderer, camera);

    // Ghost preview
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    if (mx < screenWidth - panelWidth && my < screenHeight - bottomBarHeight) {
        float worldX = camera.x + mx / camera.zoom;
        float worldY = camera.y + my / camera.zoom;
        int tileX = static_cast<int>(worldX / TILE_SIZE);
        int tileY = static_cast<int>(worldY / TILE_SIZE);
        int drawX = static_cast<int>((tileX * TILE_SIZE - camera.x) * camera.zoom);
        int drawY = static_cast<int>((tileY * TILE_SIZE - camera.y) * camera.zoom);

        const TileDefinition* def = tileLibrary.getById(selectedTile);
        if (def)
            tileRenderer.renderTile(renderer, *def, drawX, drawY, camera.zoom, 150);
    }

    renderPanel();
    renderBottomBar();

    if (editorMode == EditorMode::TilesetEditor)
        renderTilesetEditor();

    SDL_RenderPresent(renderer);
}

// ── renderPanel ──────────────────────────────────────────────────────────────

void Game::renderPanel() {
    const int px = screenWidth - panelWidth;

    // Background
    SDL_Rect bg = {px, 0, panelWidth, screenHeight};
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, &bg);

    // Tab bar
    const char* tabNames[] = {"Terrain", "Object", "Deco", "Resource"};
    int tabW = panelWidth / 4;
    for (int i = 0; i < 4; i++) {
        SDL_Rect tab = {px + i * tabW, 0, tabW, tabBarHeight};

        if (static_cast<TileCategory>(i) == activeCategory)
            SDL_SetRenderDrawColor(renderer, 80, 130, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);
        SDL_RenderFillRect(renderer, &tab);

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &tab);

        // ← text added
        textRenderer.drawCentered(renderer, tabNames[i], tab);
    }

    // Tiles for active category
    auto catTiles = tileLibrary.getByCategory(activeCategory);
    int  cols     = panelWidth / panelTileSize;
    int  startY   = tabBarHeight - panelScrollY;

    for (int i = 0; i < (int)catTiles.size(); i++) {
        int col  = i % cols;
        int row  = i / cols;
        int drawX = px + col * panelTileSize;
        int drawY = startY + row * panelTileSize;

        // Skip if off-screen
        if (drawY + panelTileSize < tabBarHeight) continue;
        if (drawY > screenHeight - 40)            continue;

        tileRenderer.renderTile(renderer, catTiles[i], drawX, drawY,
                                (float)panelTileSize / catTiles[i].srcW);

        // Highlight selected
        if (catTiles[i].id == selectedTile) {
            SDL_Rect hl = {drawX, drawY, panelTileSize, panelTileSize};
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            SDL_RenderDrawRect(renderer, &hl);
        }
    }

    // "+ Import" button at bottom
    SDL_Rect importBtn = {px + 10, screenHeight - 36, panelWidth - 20, 28};
    SDL_SetRenderDrawColor(renderer, 60, 100, 60, 255);
    SDL_RenderFillRect(renderer, &importBtn);
    textRenderer.drawCentered(renderer, "+ Import", importBtn); // ← added
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

        textRenderer.drawCentered(renderer, "Layer " + std::to_string(i + 1), r); // ← added
    }
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

    tileLibrary.save("../Assets/Tilesets/library.tileset");
    editorMode = EditorMode::Paint;
}

void Game::renderTilesetEditor() {
    // Dark overlay over world
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect overlay = {0, 0, screenWidth - panelWidth, screenHeight};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    float previewZoom = 0.5f;
    int cellW = static_cast<int>(tsEditor.tileW * previewZoom);
    int cellH = static_cast<int>(tsEditor.tileH * previewZoom);
    const int gridOffX = 10;
    const int gridOffY = 60;

    // Draw each cell
    for (int i = 0; i < tsEditor.count(); i++) {
        int col   = i % tsEditor.cols();
        int row   = i / tsEditor.cols();
        int drawX = gridOffX + col * cellW;
        int drawY = gridOffY + row * cellH;

        SDL_Rect src = {
            col * tsEditor.tileW,
            row * tsEditor.tileH,
            tsEditor.tileW,
            tsEditor.tileH
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

    // Category buttons
    const char* catNames[] = {"Terrain", "Object", "Deco", "Resource"};
    for (int i = 0; i < 4; i++) {
        SDL_Rect r = {10 + i * 90, 10, 80, 28};
        if (static_cast<TileCategory>(i) == tsEditor.category)
            SDL_SetRenderDrawColor(renderer, 80, 130, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderDrawRect(renderer, &r);
        textRenderer.drawCentered(renderer, catNames[i], r);
    }

    // Tile size hint
    textRenderer.draw(renderer,
        "Tile size: " + std::to_string(tsEditor.tileW) +
        "x"          + std::to_string(tsEditor.tileH) +
        "  (scroll to resize)",
        10, screenHeight - 90, {200, 200, 200, 255}
    );

    // ✅ Declare FIRST, fill rect, THEN draw text on top
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
    // Uses zenity (Linux). Replace with tinyfd or win32 on Windows.
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