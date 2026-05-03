#include "Core/Game.h"

int main() {
    Game game;
    if (!game.init()) return -1;

    game.run();
    game.clean();

    return 0;
}