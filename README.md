# Co Caro — Gomoku

A polished Gomoku (5-in-a-row) board game built with **C++ and SDL2**.

## Features

- **PvP & PvE** — Play against a friend or an AI opponent
- **Minimax AI** with Alpha-Beta Pruning, Move Ordering, and Time Limiting (Easy / Medium / Hard)
- **Cozy pixel art** visual style inspired by Stardew Valley
- **Character animation** — Characters walk to cells and place pieces
- **WinEffect pipeline** — GLOW → CONVERGE → ZOOM → ANNOUNCE
- **Save / Load** — Binary file-based game persistence
- **AI Benchmark system** — Performance logging per move

## Tech Stack

| Component | Technology |
|---|---|
| Language | C++17 |
| Graphics | SDL2 + SDL2_image + SDL2_ttf |
| Audio | SDL2_mixer |
| Build | Visual Studio 2022 |
| AI | Minimax + Alpha-Beta Pruning |

## Project Structure

```
CaroGame_SDL2/
├── src/
│   ├── ai/          AI player (Minimax, benchmark)
│   ├── game/        Core logic (Model, FileHandling, GameDef)
│   └── sdl/         Rendering, UI, Animation, Audio, WinEffect
├── assets/
│   ├── fonts/       Pixel art font (m5x7)
│   ├── sprites/     Backgrounds, board skins, icons, characters
│   └── sounds/      BGM and SFX
├── docs/
│   ├── architecture/  System design
│   ├── decisions/     Decision Records (DR-001 to DR-007)
│   ├── journal/       Development journal
│   └── concepts/      Technical concepts
├── CaroGameSDL2/    Visual Studio project files
├── main.cpp
└── CaroGameSDL2.slnx
```

## Building

Requires SDL2 libraries (SDL2, SDL2_image, SDL2_ttf, SDL2_mixer).
Open `CaroGameSDL2.slnx` in Visual Studio 2022 → Build Debug/x64.

## Controls

| Key | Action |
|---|---|
| WASD / Mouse | Move cursor |
| Enter / Click | Place piece |
| ESC | Pause menu |
| L | Save game |
| T | Load game |
| R | New session (after game ends) |

## AI Performance (after optimization)

| Difficulty | Depth | Avg Time |
|---|---|---|
| Easy | 2 | < 5ms |
| Medium | 4 | < 100ms |
| Hard | 6 | < 1500ms |

---

*Coursework — Fundamental Software Development, ITUS 2026*
