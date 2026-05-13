//
// Created by yves-dylane on 5/13/26.
//

#include "MenuBar.h"
#include <vector>
#include <string>

static constexpr int ITEM_H    = 22;
static constexpr int MENU_W    = 180;
static constexpr int BAR_H     = MenuBar::HEIGHT;

// Menu tab positions
static constexpr int TAB_FILE_X     =   0;
static constexpr int TAB_MAPS_X     =  60;
static constexpr int TAB_TILESETS_X = 120;
static constexpr int TAB_W          =  60;

void MenuBar::render(SDL_Renderer* renderer, TextRenderer& text,
                     MapManager& maps, TilesetManager& tilesets,
                     int screenWidth) {

    // Bar background
    SDL_Rect bar = {0, 0, screenWidth, BAR_H};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &bar);

    // Bottom border
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderDrawLine(renderer, 0, BAR_H - 1, screenWidth, BAR_H - 1);

    // Tabs
    struct Tab { int x; const char* label; int id; };
    Tab tabs[] = {
        {TAB_FILE_X,     "File",     0},
        {TAB_MAPS_X,     "Maps",     1},
        {TAB_TILESETS_X, "Tilesets", 2}
    };

    for (auto& tab : tabs) {
        SDL_Rect r = {tab.x, 0, TAB_W, BAR_H};
        if (openMenu == tab.id)
            SDL_SetRenderDrawColor(renderer, 60, 100, 160, 255);
        else
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &r);
        text.drawCentered(renderer, tab.label, r);
    }

    // Active map name in center
    const MapEntry* active = maps.getActiveEntry();
    if (active) {
        std::string title = "[ " + active->name + " ]";
        SDL_Rect center = {screenWidth / 2 - 100, 0, 200, BAR_H};
        text.drawCentered(renderer, title, center, {180, 180, 180, 255});
    }

    // Dropdowns
    if (openMenu == 0) { // File
        std::vector<std::string> items = {
            "New Map", "Save  Ctrl+S", "Save As...", "──────────"
        };
        renderDropdown(renderer, text, TAB_FILE_X, items, screenWidth);
    }
    else if (openMenu == 1) { // Maps
        std::vector<std::string> items;
        for (const auto& m : maps.getMaps())
            items.push_back(m.name);
        items.push_back("──────────");
        items.push_back("+ New Map");
        renderDropdown(renderer, text, TAB_MAPS_X, items, screenWidth);
    }
    else if (openMenu == 2) { // Tilesets
        std::vector<std::string> items;
        for (const auto& t : tilesets.getTilesets())
            items.push_back(t.name);
        items.push_back("──────────");
        items.push_back("+ Load Tileset");
        renderDropdown(renderer, text, TAB_TILESETS_X, items, screenWidth);
    }
}

void MenuBar::renderDropdown(SDL_Renderer* renderer, TextRenderer& text,
                              int x, const std::vector<std::string>& items,
                              int screenWidth) {
    int dropX = std::min(x, screenWidth - MENU_W);
    int dropY = BAR_H;
    int h     = (int)items.size() * ITEM_H + 4;

    // Shadow
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
    SDL_Rect shadow = {dropX + 3, dropY + 3, MENU_W, h};
    SDL_RenderFillRect(renderer, &shadow);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Background
    SDL_Rect bg = {dropX, dropY, MENU_W, h};
    SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_RenderDrawRect(renderer, &bg);

    for (int i = 0; i < (int)items.size(); i++) {
        SDL_Rect row = {dropX, dropY + 2 + i * ITEM_H, MENU_W, ITEM_H};
        if (items[i] == "──────────") {
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderDrawLine(renderer,
                row.x + 4, row.y + ITEM_H / 2,
                row.x + MENU_W - 4, row.y + ITEM_H / 2);
        } else {
            text.draw(renderer, items[i], row.x + 8, row.y + 4);
        }
    }
}

MenuResult MenuBar::handleClick(int mx, int my,
                                 MapManager& maps, TilesetManager& tilesets,
                                 int screenWidth) {
    // Click on bar — toggle menus
    if (my >= 0 && my < BAR_H) {
        if (mx >= TAB_FILE_X     && mx < TAB_FILE_X     + TAB_W) {
            openMenu = (openMenu == 0) ? -1 : 0; return {};
        }
        if (mx >= TAB_MAPS_X     && mx < TAB_MAPS_X     + TAB_W) {
            openMenu = (openMenu == 1) ? -1 : 1; return {};
        }
        if (mx >= TAB_TILESETS_X && mx < TAB_TILESETS_X + TAB_W) {
            openMenu = (openMenu == 2) ? -1 : 2; return {};
        }
        openMenu = -1;
        return {};
    }

    // Click inside dropdown
    if (openMenu >= 0) {
        int tabX = (openMenu == 0) ? TAB_FILE_X
                 : (openMenu == 1) ? TAB_MAPS_X
                 :                   TAB_TILESETS_X;
        int dropX = std::min(tabX, screenWidth - MENU_W);
        int dropY = BAR_H;

        if (mx >= dropX && mx < dropX + MENU_W && my >= dropY) {
            int row = (my - dropY - 2) / ITEM_H;

            if (openMenu == 0) { // File
                openMenu = -1;
                if (row == 0) return {MenuAction::NewMap};
                if (row == 1) return {MenuAction::SaveMap};
                if (row == 2) return {MenuAction::SaveMapAs};
            }
            else if (openMenu == 1) { // Maps
                int mapCount = (int)maps.getMaps().size();
                openMenu = -1;
                if (row < mapCount)
                    return {MenuAction::SwitchMap, row};
                if (row == mapCount + 1) // "+ New Map"
                    return {MenuAction::NewMap};
            }
            else if (openMenu == 2) { // Tilesets
                int tsCount = (int)tilesets.getTilesets().size();
                openMenu = -1;
                if (row < tsCount)
                    return {MenuAction::RemoveTileset, row};
                if (row == tsCount + 1) // "+ Load Tileset"
                    return {MenuAction::LoadTileset};
            }
        } else {
            // Click outside dropdown = close
            openMenu = -1;
        }
    }

    return {MenuAction::None};
}