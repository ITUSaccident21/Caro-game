# Game Loop & App State Machine

## Fixed Timestep Loop

```
App_Run():
  WHILE running:
    dt = (now - prevTicks) / 1000.0f   // thời gian 1 frame (giây)
    accumulator += dt

    App_HandleEvents()                  // không phụ thuộc timestep

    WHILE accumulator >= FIXED_STEP:    // FIXED_STEP = 1/60 ≈ 0.0167s
        App_Update(FIXED_STEP)
        accumulator -= FIXED_STEP

    App_Render()
```

**Tại sao Fixed Timestep?**
Physics/animation update với bước cố định → kết quả deterministic bất kể FPS.
Render theo frame thực tế → không bị giật dù CPU load cao.

---

## App State Machine

```
STATE_MENU
  │ (PLAY)
  ↓
STATE_NAME_INPUT
  │ (Confirm)
  ↓
STATE_PLAYING ←───────────────── STATE_LOAD_GAME
  │ (ESC)                           ↑ (T)
  └─→ STATE_MENU ──────────────────┘
  │ (EXIT button)
  └─→ STATE_EXIT
```

Mỗi state có 3 handler riêng:
- `App_OnEvent_<State>()` — xử lý input
- `App_Update_<State>()` — update logic
- `App_Render()` switch case — render

---

## Luồng Đặt Quân (STATE_PLAYING)

```
User click ô (row, col)
  └→ Animation_StartMove(state, row, col, charIdx)
       └→ Nhân vật đi bộ đến ô (WALK_TO_CELL)
            └→ Đến nơi → PLACING (0.22s)
                 └→ App_PlacePiece(state, row, col)
                      ├→ state._BOARD[row][col].c = color
                      ├→ TestBoard() → kiểm tra kết quả
                      ├→ Thắng → Animation_StartCelebrate()
                      │          WinEffect_Start()
                      │          UIManager_ShowResult()
                      └→ Tiếp → lượt kế tiếp
```

---

## Lượt AI (MODE_PVE)

```
App_Update_Playing():
  IF lượt AI (turn == false) AND không đang think:
    App_TriggerAITurn(state)
      → s_aiFuture = std::async(AI_FindBestMove, snapshot)
      → state.aiThinking = true

  IF aiThinking AND future ready:
    Move best = s_aiFuture.get()
    state.aiThinking = false
    Animation_StartMove(state, best.row, best.col, 1)
```

**AI chạy trên background thread** — main loop không bị block, game vẫn render 60 FPS trong khi AI tính.

---

## Input Locking

Input bị khóa khi:
- `Animation_IsPlaying()` — nhân vật đang di chuyển
- `state.aiThinking` — AI đang tính
- `WinEffect_IsAnimating()` — hiệu ứng chiến thắng đang chạy
