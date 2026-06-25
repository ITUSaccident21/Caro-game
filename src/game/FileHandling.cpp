#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include "game/GameDef.h"
#include "game/FileHandling.h"

using namespace std;

// SAVE FORMAT

// fixed order implemented by WriteState_v2 / ReadState_v2. This decouples the
// on-disk format from _GAMESTATE's in-memory layout:
//     WriteState_v2, and in ReadState_v2 gate the read behind the new version

namespace {
    constexpr char     SAVE_MAGIC[4] = { 'B', 'G', 'S', 'V' };
    constexpr uint32_t SAVE_VERSION  = 2;
    struct SaveHeader {
        char     magic[4];
        uint32_t version;
        uint32_t reserved;
        uint32_t boardSize;
        uint32_t winCount;
    };

    void WriteState_v2(ofstream& out, const _GAMESTATE& s) {
        auto wr32 = [&](int32_t v) { out.write((const char*)&v, sizeof(v)); };

        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                wr32(s._BOARD[i][j].c);

        for (int p = 0; p < 2; p++) {
            out.write(s.players[p].name, sizeof(s.players[p].name));
            wr32(s.players[p].moves);
            wr32(s.players[p].wins);
            wr32(s.players[p].losses);
            wr32(s.players[p].draws);
            wr32((int32_t)s.players[p].mark);
        }

        wr32(s._X);
        wr32(s._Y);
        wr32((int32_t)s.turn);
        wr32(s._LastI);
        wr32(s._LastJ);
        wr32(s.command);
        wr32(s.gameStatus);
        wr32(s.totalMoves);

        for (int k = 0; k < WIN_COUNT; k++) {
            wr32(s._WIN_CELLS[k].x);
            wr32(s._WIN_CELLS[k].y);
            wr32(s._WIN_CELLS[k].c);
        }

        wr32((int32_t)s.mode);
        wr32((int32_t)s.difficulty);
        wr32((int32_t)s.aiThinking);
        wr32(s.hoveredRow);
        wr32(s.hoveredCol);
        wr32(s.selectedRow);
        wr32(s.selectedCol);
    }

    bool ReadState_v2(ifstream& in, _GAMESTATE& s) {
        bool ok = true;
        auto rd32 = [&](int32_t& v) {
            in.read((char*)&v, sizeof(v));
            if (in.gcount() != (std::streamsize)sizeof(v)) ok = false;
        };

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                rd32(s._BOARD[i][j].c);
                s._BOARD[i][j].x = j;
                s._BOARD[i][j].y = i;
            }
        }

        for (int p = 0; p < 2; p++) {
            in.read(s.players[p].name, sizeof(s.players[p].name));
            if (in.gcount() != (std::streamsize)sizeof(s.players[p].name)) ok = false;
            rd32(s.players[p].moves);
            rd32(s.players[p].wins);
            rd32(s.players[p].losses);
            rd32(s.players[p].draws);
            int32_t mark = 0; rd32(mark);
            s.players[p].mark = (wchar_t)mark;
        }

        rd32(s._X);
        rd32(s._Y);
        int32_t turn = 0; rd32(turn);
        s.turn = (turn != 0);
        rd32(s._LastI);
        rd32(s._LastJ);
        rd32(s.command);
        rd32(s.gameStatus);
        rd32(s.totalMoves);

        for (int k = 0; k < WIN_COUNT; k++) {
            rd32(s._WIN_CELLS[k].x);
            rd32(s._WIN_CELLS[k].y);
            rd32(s._WIN_CELLS[k].c);
        }

        int32_t mode = 0, difficulty = 0, aiThinking = 0;
        rd32(mode);       s.mode       = (GameMode)mode;
        rd32(difficulty); s.difficulty = (AIDifficulty)difficulty;
        rd32(aiThinking); s.aiThinking = (aiThinking != 0);

        rd32(s.hoveredRow);
        rd32(s.hoveredCol);
        rd32(s.selectedRow);
        rd32(s.selectedCol);

        return ok;
    }
}

bool SaveGame(const _GAMESTATE& state, const string& filename) {
    string filepath = "Saves/" + filename + ".dat";
    ofstream out(filepath, ios::binary);
    if (!out) return false;
    SaveHeader h{};
    memcpy(h.magic, SAVE_MAGIC, 4);
    h.version   = SAVE_VERSION;
    h.reserved  = 0;
    h.boardSize = (uint32_t)BOARD_SIZE;
    h.winCount  = (uint32_t)WIN_COUNT;
    out.write((char*)&h, sizeof(h));
    WriteState_v2(out, state);
    return out.good();
}

bool CheckSaveExists(const string& filename) {
    string filepath = "Saves/" + filename + ".dat";
    return std::filesystem::exists(filepath);
}

bool DeleteSave(const string& filename) {
    std::error_code ec;
    return std::filesystem::remove("Saves/" + filename + ".dat", ec);
}

bool LoadGame(_GAMESTATE& state, const string& filename) {
    string filepath = "Saves/" + filename + ".dat";
    ifstream in(filepath, ios::binary);
    if (!in) {
        fprintf(stderr, "[LoadGame] %s: cannot open file\n", filepath.c_str());
        return false;
    }

    // Validate header before trusting any bytes. Each rejection logs WHY, so an
    SaveHeader h{};
    in.read((char*)&h, sizeof(h));
    if (in.gcount() != (std::streamsize)sizeof(h)) {
        fprintf(stderr, "[LoadGame] %s: file too short for header\n", filepath.c_str());
        return false;
    }
    if (memcmp(h.magic, SAVE_MAGIC, 4) != 0) {
        fprintf(stderr, "[LoadGame] %s: not a Berry Grove save (bad magic)\n", filepath.c_str());
        return false;
    }
    if (h.boardSize != (uint32_t)BOARD_SIZE) {
        fprintf(stderr, "[LoadGame] %s: board size %u != current %d\n",
                filepath.c_str(), h.boardSize, BOARD_SIZE);
        return false;
    }
    if (h.winCount != (uint32_t)WIN_COUNT) {
        fprintf(stderr, "[LoadGame] %s: win count %u != current %d\n",
                filepath.c_str(), h.winCount, WIN_COUNT);
        return false;
    }
    if (h.version != SAVE_VERSION) {
        // v1 was a raw POD dump whose body layout depends on the exact
        fprintf(stderr, "[LoadGame] %s: save version %u != supported %u (old format, not migratable)\n",
                filepath.c_str(), h.version, SAVE_VERSION);
        return false;
    }

    // Read into a temp and commit only on a complete, valid read — a bad file never

    _GAMESTATE tmp{};
    if (!ReadState_v2(in, tmp)) {
        fprintf(stderr, "[LoadGame] %s: truncated or corrupt save body\n", filepath.c_str());
        return false;
    }
    state = tmp;
    return true;
}

void InitSaveFolder() {
    if (!std::filesystem::exists("Saves")) {
        std::filesystem::create_directory("Saves");
    }
}

std::vector<std::string> GetSaveFiles() {
    std::vector<std::string> files;
    if (!std::filesystem::exists("Saves")) return files;
    for (const auto& entry : std::filesystem::directory_iterator("Saves")) {
        if (entry.path().extension() == ".dat")
            files.push_back(entry.path().stem().string());
    }
    return files;
}

