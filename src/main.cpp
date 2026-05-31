#include "Game.h"

int main() {
    Game game(800, 600);
    if (!game.Init()) {
        return -1;
    }
    game.Run();
    return 0;
}
