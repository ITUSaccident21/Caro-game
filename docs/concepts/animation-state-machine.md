# Khái Niệm: Animation State Machine

> Giải thích pattern State Machine dùng cho animation nhân vật trong game,
> và cách nó được implement trong Animation.cpp của dự án.

---

## Vấn Đề Không Có State Machine

Nếu dùng if/else thuần túy:

```cpp
// Cách tệ — khó debug, dễ xung đột
void UpdateCharacter(float dt) {
    if (isWalking && !isPlacing && !isCelebrating) { ... }
    else if (!isWalking && isPlacing && !isCelebrating) { ... }
    // Combinations bùng nổ theo cấp số nhân
}
```

**Vấn đề:** 4 bool = 16 tổ hợp có thể, nhiều tổ hợp vô nghĩa và gây bug.

---

## Giải Pháp: State Machine

```cpp
// Từ GameDef.h trong dự án
enum AnimState {
    ANIM_IDLE,           // đứng yên tại vị trí home
    ANIM_WALK_TO_CELL,   // đang đi đến ô cờ
    ANIM_PLACING,        // animation đặt quân
    ANIM_CELEBRATE,      // nhảy múa sau khi thắng
    ANIM_WALK_HOME       // đi về home
};
```

Mỗi lúc nhân vật chỉ có **1 state duy nhất** → không bao giờ conflict.

---

## Sơ Đồ State Transitions Trong Dự Án

```
        [IDLE] ─────────────────────────────────────────┐
           │                                             │
           │ Animation_StartMove()                       │ hết WALK_HOME
           ▼                                             │
    [WALK_TO_CELL] ──── đến nơi ────► [PLACING] ────────┤
                                          │              │
                                    placeTimer >= 0.22s  │
                                          │              │
                                     App_PlacePiece()    │
                                          │              │
                              ┌───────────┴──────────┐   │
                              │                      │   │
                           game over?              còn lượt
                              │                      │   │
                    Animation_StartCelebrate()   [WALK_HOME]──┘
                              │
                         [CELEBRATE]
```

---

## Implement Trong Animation.cpp

```cpp
struct CharState {
    AnimState anim;       // state hiện tại
    Direction dir;        // hướng nhìn (cho sprite frame)
    float x, y;           // vị trí pixel hiện tại
    float targetX, targetY; // điểm đến
    float placeTimer;     // đếm thời gian placing
    float celebTimer;     // đếm thời gian celebrate
};
```

**Update loop:**
```cpp
void Animation_Update(float dt, _GAMESTATE& state) {
    for (auto& ch : s_chars) {
        switch (ch.anim) {
        case ANIM_WALK_TO_CELL:
            // di chuyển ch.x, ch.y về phía target
            // nếu đến nơi → chuyển sang PLACING
            break;
        case ANIM_PLACING:
            ch.placeTimer += dt;
            if (ch.placeTimer >= PLACE_DELAY) {
                App_PlacePiece(...);  // thực sự đặt quân
                ch.anim = ANIM_WALK_HOME;  // chuyển state
            }
            break;
        // ...
        }
    }
}
```

---

## Tại Sao State Machine Tốt Cho Animation?

| Tiêu chí | if/else | State Machine |
|---|---|---|
| Số state có thể | 2^n combinations | n states rõ ràng |
| Debug | Khó tìm bug | Biết chính xác đang ở state nào |
| Mở rộng | Sửa nhiều chỗ | Thêm 1 case mới |
| Đọc code | Khó hiểu | Sơ đồ transition rõ ràng |

---

## Mở Rộng Tiềm Năng

```cpp
// Thêm state mới rất dễ:
enum AnimState {
    ANIM_IDLE,
    ANIM_WALK_TO_CELL,
    ANIM_PLACING,
    ANIM_CELEBRATE,
    ANIM_WALK_HOME,
    ANIM_THINKING,    // ← mới: AI đang "suy nghĩ"
    ANIM_SURPRISED    // ← mới: reaction khi đối thủ đặt quân hay
};
```

---

## Liên Kết

- Source: [Animation.cpp](../../CaroGameSDL2/src/sdl/Animation.cpp)
- Concept liên quan: [SDL2 Rendering Pipeline](./sdl2-rendering-pipeline.md)
