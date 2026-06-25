# Berry Grove - Caro/Gomoku SDL2

Final project for the "Co so Lap trinh" course.

Berry Grove is a 12x12 Caro/Gomoku game written in C++17 and SDL2. The game supports local PvP, PvE with AI, save/load, sound effects, animated UI, and a cozy berry garden theme.

## Submission Contents

```text
Submission/
  src/                         C++ source files
  include/                     Header files
  assets/                      Runtime game assets
  external/                    SDL2 headers, x64 import libs, and DLLs
  docs/
    24120472_TruongHueTri_Report.pdf
    24120472_TruongHueTri_Presentation.pptx
  Release/
    CaroGame.exe
    SDL2*.dll
    assets/
  CaroGameSubmission.sln       Visual Studio 2022 solution
  CaroGameSubmission.vcxproj   Visual Studio C++ project
  Makefile
```

## Run Without Building

Open:

```text
Release\CaroGame.exe
```

The SDL2 DLL files and assets are included next to the executable.

## Build on Visual Studio 2022

Requirements:

- Windows 10/11 x64
- Visual Studio 2022 with "Desktop development with C++"
- Windows 10/11 SDK

Steps:

1. Open `CaroGameSubmission.sln`.
2. Select `Release | x64`.
3. Choose `Build > Build Solution`.
4. Run `Release\CaroGame.exe`.

The project uses only relative library paths:

```text
$(ProjectDir)external\SDL2
$(ProjectDir)external\SDL2_image
$(ProjectDir)external\SDL2_ttf
$(ProjectDir)external\SDL2_mixer
```

No SDL2 installation is required on the grading machine. The post-build step copies the required SDL2 DLL files from `external` into `Release`.

## Self-Test

The self-test source is included in `src/tests/` and `include/tests/`. It is not run during normal gameplay; it only runs when the executable receives the `--selftest` argument.

After building, run:

```text
Release\CaroGame.exe --selftest
```

Expected result:

```text
72/72 checks passed
SELFTEST: ALL PASS
```

## Main Features

- PvP on the same machine
- PvE against AI with three difficulty levels
- Win/draw detection
- Save/load binary game state
- Fullscreen 1920x1080 layout with letterboxing
- Berry Grove themed UI and gameplay assets
- Sound effects and optional background music hooks
- Automated model, save/load, and AI self-tests

## Libraries

Bundled in `external/`:

| Library | Purpose | License |
| --- | --- | --- |
| SDL2 | Window, renderer, input | zlib |
| SDL2_image | PNG loading | zlib |
| SDL2_ttf | Font rendering | zlib |
| SDL2_mixer | Audio playback | zlib |
| Baloo 2 | UI font | SIL Open Font License |

The original license/readme files for SDL2 libraries are included in each `external/<library>/` directory.
