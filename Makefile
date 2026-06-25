# ==============================================================================
#  Makefile — CaroGame SDL2 (Gomoku)
#  Yêu cầu: g++ >= 7 (C++17), SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
#
#  Linux / macOS:
#    sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
#    make
#
#  Windows (MinGW / MSYS2):
#    pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image \
#              mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_mixer
#    make
#    (hoặc set SDL2_DIR=<đường dẫn SDL2 của bạn> rồi make)
# ==============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

# --------------------------------------------------------------------------
# SDL2 flags — tự động theo hệ điều hành
# --------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    # MinGW/MSYS2: dùng pkg-config nếu có, fallback về SDL2_DIR
    SDL2_PKG := $(shell pkg-config --silence-errors sdl2 SDL2_image SDL2_ttf SDL2_mixer && echo yes)
    ifeq ($(SDL2_PKG),yes)
        SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf SDL2_mixer)
        SDL2_LIBS   := $(shell pkg-config --libs   sdl2 SDL2_image SDL2_ttf SDL2_mixer)
    else
        SDL2_DIR    ?= C:/SDL2
        SDL2_CFLAGS := -I$(SDL2_DIR)/include
        SDL2_LIBS   := -L$(SDL2_DIR)/lib -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
    endif
    CXXFLAGS += $(SDL2_CFLAGS)
    LDFLAGS  := $(SDL2_LIBS) -Wl,-subsystem,windows
    TARGET   := CaroGame.exe
else
    CXXFLAGS += $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf SDL2_mixer)
    LDFLAGS  := $(shell pkg-config --libs   sdl2 SDL2_image SDL2_ttf SDL2_mixer)
    TARGET   := CaroGame
endif

# --------------------------------------------------------------------------
# Nguồn
# --------------------------------------------------------------------------
SRCS :=	src/main.cpp             \
	src/game/Model.cpp           \
	src/game/FileHandling.cpp    \
	src/ai/AIPlayer.cpp          \
	src/ai/AIWorker.cpp          \
	src/sdl/App.cpp              \
	src/sdl/Renderer.cpp         \
	src/sdl/UIManager.cpp        \
	src/sdl/HarvestResult.cpp    \
	src/sdl/Particle.cpp         \
	src/sdl/Fade.cpp             \
	src/sdl/AudioManager.cpp     \
	src/tests/SelfTest.cpp

OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

# --------------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------------
.PHONY: all clean selftest

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "==> Build OK: $(TARGET)"

# Tự sinh dependency files (.d) để header thay đổi → recompile đúng file
-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

# Chạy bộ kiểm thử logic thuần (không cần SDL / cửa sổ đồ họa)
selftest: $(TARGET)
	./$(TARGET) --selftest
