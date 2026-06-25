#include "sdl/App.h"
#include "game/GameDef.h"
#include "tests/SelfTest.h"
#include <cstring>

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++)
        if (std::strcmp(argv[i], "--selftest") == 0)
            return SelfTest_RunAll() == 0 ? 0 : 1;

    AppContext  ctx = {};
    _GAMESTATE  game = {};
    AppState    appState = STATE_MENU;

    if (!App_Init(ctx, game, appState)) {
        return 1;
    }

    App_Run(ctx, game, appState);
    App_Shutdown(ctx);
    return 0;
}
