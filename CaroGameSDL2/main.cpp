#include "src/sdl/App.h"
#include "src/game/GameDef.h"

// ================================================================
//  main.cpp — SDL2 entry point
//  Lưu ý: SDL2 trên Windows yêu cầu int main(int, char**)
//  (SDL internally redefines main để xử lý WinMain)
// ================================================================

int main(int argc, char* argv[]) {
    AppContext  ctx = {};   // zero-initialize toàn bộ
    _GAMESTATE  game = {};
    AppState    appState = STATE_MENU;

    if (!App_Init(ctx, game, appState)) {
        return 1;
    }

    App_Run(ctx, game, appState);
    App_Shutdown(ctx);
    return 0;
}