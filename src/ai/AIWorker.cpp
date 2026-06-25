#include "ai/AIWorker.h"
#include <thread>
#include <mutex>

// The AI runs on a DETACHED thread over a value snapshot of the state — never the
namespace {
    std::mutex  s_mx;
    Move        s_result;
    bool        s_resultReady = false;
    int         s_epoch       = 0;
    std::thread s_thread;
}

void AIWorker_Cancel() {
    std::lock_guard<std::mutex> lk(s_mx);
    ++s_epoch;
    s_resultReady = false;
}

void AIWorker_Request(const _GAMESTATE& state) {
    int myEpoch;
    {
        std::lock_guard<std::mutex> lk(s_mx);
        myEpoch = ++s_epoch;
        s_resultReady = false;
    }
    if (s_thread.joinable()) s_thread.detach();

    _GAMESTATE snapshot = state;  // AI works on a private copy (no shared-board race)
    s_thread = std::thread([snapshot, myEpoch]() {
        Move m = AI_FindBestMove(snapshot);
        std::lock_guard<std::mutex> lk(s_mx);
        if (myEpoch == s_epoch) {
            s_result      = m;
            s_resultReady = true;
        }
    });
}

bool AIWorker_Poll(Move& outMove) {
    std::lock_guard<std::mutex> lk(s_mx);
    if (!s_resultReady) return false;
    outMove = s_result;
    s_resultReady = false;
    return true;
}

void AIWorker_Shutdown() {
    AIWorker_Cancel();
    if (s_thread.joinable()) s_thread.join();
}
