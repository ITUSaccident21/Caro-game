#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "../game/GameDef.h"

// ================================================================
//  Animation.h — Nhân vật pixel art + state machine
//
//  State machine: IDLE → WALK_TO_CELL → PLACING → IDLE
//
//  Đồng bộ với game logic:
//    - PlacePiece() chỉ được gọi khi PLACING animation xong
//    - Input bị khóa khi !IsIdle()
// ================================================================

void Animation_Init();
void Animation_Shutdown();

// Kích hoạt animation đi đến ô (charIdx: 0=P1, 1=P2/AI)
void Animation_StartMove(_GAMESTATE& state, int targetRow, int targetCol, int charIdx);

// Kích hoạt animation ăn mừng thắng
void Animation_StartCelebrate(_GAMESTATE& state, int charIdx);

// Gọi mỗi frame — cập nhật vị trí nhân vật, chuyển state animation
// Khi PLACING kết thúc: tự động gọi callback để ghi quân vào board
void Animation_Update(float dt, _GAMESTATE& state);

// Render sprite nhân vật lên màn hình
void Animation_Render(SDL_Renderer* renderer, const _GAMESTATE& state);

// Reset về IDLE (dùng khi bắt đầu ván mới)
void Animation_Reset();

// true khi đang walk hoặc placing (dùng để khóa input)
bool Animation_IsPlaying();