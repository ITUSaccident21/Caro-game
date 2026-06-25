#include "tests/SelfTest.h"

#include "game/GameDef.h"
#include "game/Model.h"
#include "game/FileHandling.h"
#include "ai/AIPlayer.h"
#include "ai/AIWorker.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>

namespace {

int s_checks = 0;
int s_failed = 0;

void Check(bool cond, const char* expr, int line) {
    s_checks++;
    if (!cond) {
        s_failed++;
        std::printf("  FAIL line %d: %s\n", line, expr);
    }
}
#define CHECK(cond) Check((cond), #cond, __LINE__)

void Section(const char* name) { std::printf("-- %s --\n", name); }

void Test_Model_NewSessionAndCheckBoard() {
    Section("Model: NewSession / CheckBoard");
    _GAMESTATE g{};
    NewSession(g);

    CHECK(g.turn == true);
    bool allEmpty = true;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (g._BOARD[i][j].c != 0) allEmpty = false;
    CHECK(allEmpty);

    CHECK(CheckBoard(g, 0, 0) == -1);
    CHECK(g._BOARD[0][0].c == -1);
    CHECK(g._LastI == 0 && g._LastJ == 0);

    CHECK(CheckBoard(g, 0, 0) == 0);
    CHECK(CheckBoard(g, -1, 0) == 0);
    CHECK(CheckBoard(g, 0, BOARD_SIZE) == 0);
    CHECK(CheckBoard(g, BOARD_SIZE, 0) == 0);

    g.turn = false;
    CHECK(CheckBoard(g, 1, 1) == 1);
    CHECK(g._BOARD[1][1].c == 1);
}

void Test_Model_WinDetection() {
    Section("Model: winCheck (row / column / diagonal / anti-diagonal)");

    {
        _GAMESTATE g{}; NewSession(g);
        for (int c = 0; c < WIN_COUNT; c++) g._BOARD[4][c].c = -1;
        CHECK(winCheck(g, 4, 2));
        bool covered[WIN_COUNT] = {};
        for (int k = 0; k < WIN_COUNT; k++) {
            CHECK(g._WIN_CELLS[k].y == 4);
            CHECK(g._WIN_CELLS[k].x >= 0 && g._WIN_CELLS[k].x < WIN_COUNT);
            covered[g._WIN_CELLS[k].x] = true;
        }
        for (int k = 0; k < WIN_COUNT; k++) CHECK(covered[k]);
    }
    {
        _GAMESTATE g{}; NewSession(g);
        for (int r = 0; r < WIN_COUNT; r++) g._BOARD[r][2].c = 1;
        CHECK(winCheck(g, 2, 2));
    }
    {
        _GAMESTATE g{}; NewSession(g);
        for (int k = 0; k < WIN_COUNT; k++) g._BOARD[k][k].c = -1;
        CHECK(winCheck(g, 2, 2));
    }
    {
        _GAMESTATE g{}; NewSession(g);
        for (int k = 0; k < WIN_COUNT; k++) g._BOARD[k][WIN_COUNT - 1 - k].c = 1;
        CHECK(winCheck(g, 2, 2));
    }
    {
        _GAMESTATE g{}; NewSession(g);
        for (int c = 0; c < WIN_COUNT - 1; c++) g._BOARD[4][c].c = -1;
        CHECK(!winCheck(g, 4, 2));
    }
}

void Test_Model_TestBoardAndProcessFinish() {
    Section("Model: TestBoard / ProcessFinish");

    {
        _GAMESTATE g{}; NewSession(g);
        g._BOARD[0][0].c = -1;
        g._LastI = 0; g._LastJ = 0;
        int result = TestBoard(g);
        CHECK(result == CHUA_KET_THUC);
        bool before = g.turn;
        ProcessFinish(g, result);
        CHECK(g.turn == !before);
        CHECK(g.gameStatus == CHUA_KET_THUC);
    }
    {
        _GAMESTATE g{}; NewSession(g);
        for (int c = 0; c < WIN_COUNT; c++) g._BOARD[6][c].c = -1;
        g._LastI = 6; g._LastJ = 2;
        g.turn = true;
        int result = TestBoard(g);
        CHECK(result == P1_THANG);
        ProcessFinish(g, result);
        CHECK(g.gameStatus == P1_THANG);
        CHECK(g.players[0].wins == 1);
        CHECK(g.players[1].losses == 1);
    }
    {
        _GAMESTATE g{}; NewSession(g);
        for (int i = 0; i < BOARD_SIZE; i++)
            for (int j = 0; j < BOARD_SIZE; j++)
                g._BOARD[i][j].c = -1;
        g._BOARD[0][0].c = 1;
        g._LastI = 0; g._LastJ = 0;
        CHECK(isBoardFull(g));
        CHECK(!winCheck(g, 0, 0));
        int result = TestBoard(g);
        CHECK(result == HOA);
        ProcessFinish(g, result);
        CHECK(g.players[0].draws == 1);
        CHECK(g.players[1].draws == 1);
    }
}

void Test_FileHandling_SaveLoadRoundTrip() {
    Section("FileHandling: SaveGame / LoadGame round trip + validation");

    InitSaveFolder();
    const std::string kName = "_selftest_tmp";
    const std::string kBad  = "_selftest_bad";

    _GAMESTATE g{}; NewSession(g);
    g._BOARD[3][4].c = -1;
    g._BOARD[5][6].c = 1;
    g._LastI = 5; g._LastJ = 6;
    g.turn = false;
    g.mode = MODE_PVE;
    g.difficulty = AI_HARD;
    g.totalMoves = 7;
    std::strcpy(g.players[0].name, "Alice");
    std::strcpy(g.players[1].name, "Bob");
    g.players[0].wins = 3; g.players[1].losses = 3;

    CHECK(SaveGame(g, kName));

    _GAMESTATE loaded{};
    CHECK(LoadGame(loaded, kName));
    CHECK(loaded._BOARD[3][4].c == -1);
    CHECK(loaded._BOARD[5][6].c == 1);
    CHECK(loaded._LastI == 5 && loaded._LastJ == 6);
    CHECK(loaded.turn == false);
    CHECK(loaded.mode == MODE_PVE);
    CHECK(loaded.difficulty == AI_HARD);
    CHECK(loaded.totalMoves == 7);
    CHECK(std::strcmp(loaded.players[0].name, "Alice") == 0);
    CHECK(std::strcmp(loaded.players[1].name, "Bob") == 0);
    CHECK(loaded.players[0].wins == 3 && loaded.players[1].losses == 3);

    _GAMESTATE missing{};
    CHECK(!LoadGame(missing, "_selftest_does_not_exist"));

    {
        std::ofstream bad("Saves/" + kBad + ".dat", std::ios::binary);
        const char junk[] = "not a save file";
        bad.write(junk, sizeof(junk));
    }
    _GAMESTATE corrupt{};
    CHECK(!LoadGame(corrupt, kBad));

    DeleteSave(kName);
    DeleteSave(kBad);
    CHECK(!CheckSaveExists(kName));
    CHECK(!CheckSaveExists(kBad));
}

void Test_AI_ImmediateWinAndBlock() {
    Section("AIPlayer: immediate win / block priorities");

    {
        _GAMESTATE g{}; NewSession(g);
        g.mode = MODE_PVE; g.difficulty = AI_EASY; g.turn = false;
        Move m = AI_FindBestMove(g);
        CHECK(m.row == BOARD_SIZE / 2 && m.col == BOARD_SIZE / 2);
    }

    {
        _GAMESTATE g{}; NewSession(g);
        g.mode = MODE_PVE; g.difficulty = AI_EASY; g.turn = false;
        for (int c = 0; c < WIN_COUNT - 1; c++) g._BOARD[5][c].c = 1;
        Move m = AI_FindBestMove(g);
        CHECK(m.row == 5 && m.col == WIN_COUNT - 1);
        CHECK(g._BOARD[m.row][m.col].c == 0);
    }

    {
        _GAMESTATE g{}; NewSession(g);
        g.mode = MODE_PVE; g.difficulty = AI_EASY; g.turn = false;
        for (int c = 0; c < WIN_COUNT - 1; c++) g._BOARD[7][c].c = -1;
        Move m = AI_FindBestMove(g);
        CHECK(m.row == 7 && m.col == WIN_COUNT - 1);
        CHECK(g._BOARD[m.row][m.col].c == 0);
    }
}

void Test_AI_DeeperSearch() {
    Section("AIPlayer: iterative deepening / TT determinism (MEDIUM)");

    _GAMESTATE g{}; NewSession(g);
    g.mode = MODE_PVE; g.difficulty = AI_MEDIUM; g.turn = false;
    int m = BOARD_SIZE / 2;
    g._BOARD[m][m].c     = 1;
    g._BOARD[m][m+1].c   = -1;
    g._BOARD[m+1][m].c   = -1;
    g._BOARD[m+1][m+1].c = 1;
    g._BOARD[m-1][m-1].c = 1;
    g._BOARD[m-1][m].c   = -1;

    Move m1 = AI_FindBestMove(g);
    CHECK(m1.row >= 0 && m1.row < BOARD_SIZE && m1.col >= 0 && m1.col < BOARD_SIZE);
    CHECK(g._BOARD[m1.row][m1.col].c == 0);

    CHECK(AI_GetLastBenchmark().depth == static_cast<int>(AI_MEDIUM));

    // Determinism: same position + fixed-seed Zobrist/TT -> same move.
    // Guards against ResetSearchTables() failing to clear thread_local

    Move m2 = AI_FindBestMove(g);
    CHECK(m1.row == m2.row && m1.col == m2.col);
}

void Test_AIWorker_RequestAndPoll() {
    Section("AIWorker: async request/poll round trip");

    _GAMESTATE g{}; NewSession(g);
    g.mode = MODE_PVE; g.difficulty = AI_EASY; g.turn = false;
    for (int c = 0; c < WIN_COUNT - 1; c++) g._BOARD[5][c].c = 1;

    AIWorker_Request(g);

    Move m; bool got = false;
    for (int i = 0; i < 100 && !got; i++) {
        got = AIWorker_Poll(m);
        if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    CHECK(got);
    CHECK(m.row == 5 && m.col == WIN_COUNT - 1);
    CHECK(g._BOARD[m.row][m.col].c == 0);

    CHECK(!AIWorker_Poll(m));

    AIWorker_Shutdown();
}

}

int SelfTest_RunAll() {
    s_checks = 0;
    s_failed = 0;

    Test_Model_NewSessionAndCheckBoard();
    Test_Model_WinDetection();
    Test_Model_TestBoardAndProcessFinish();
    Test_FileHandling_SaveLoadRoundTrip();
    Test_AI_ImmediateWinAndBlock();
    Test_AI_DeeperSearch();
    Test_AIWorker_RequestAndPoll();

    std::printf("\n%d/%d checks passed\n", s_checks - s_failed, s_checks);
    if (s_failed == 0) std::printf("SELFTEST: ALL PASS\n");
    else               std::printf("SELFTEST: %d FAILED\n", s_failed);
    return s_failed;
}
