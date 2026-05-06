# 🎯 Caro Game Diagnostics - Visual Guide

## 📊 Decision Tree (Chọn Đường Đi)

```
┌─────────────────────────────────────────────────────────────┐
│                   Caro Game Issue?                         │
└────────┬──────────────────────────────────────────┬────────┘
         │                                          │
         ▼                                          ▼
    [PERFORMANCE]                            [QUALITY]
      Lag/Stutter                           AI misses win
         │                                       │
         ├─ Lag in PVE?                         ├─ CheckTerminal() bug?
         │                                      │
         ├─ YES ─→ TROUBLESHOOTING.md            ├─ YES ─→ Fix & test
         │         Section 1                     │
         │                                      ├─ NO ─→ TROUBLESHOOTING.md
         ├─ Animation stutter?                  │          Section 2
         │                                      │
         ├─ YES ─→ TROUBLESHOOTING.md            └─ Debug CountLine()
         │         Section 3                       or GetCandidates()
         │
         └─ Check FPS ──────→ < 30 ──→ Reduce AI depth
                            ≥ 60 ──→ Other issue


┌─────────────────────────────────────────────────────────────┐
│             What File to Use? (Choose One)                 │
└─────────────────────────────────────────────────────────────┘

Issue Type              │ Primary File         │ Time    │ Priority
────────────────────────┼──────────────────────┼─────────┼─────────
Lag khi Player vs AI    │ QUICK_ACTION.md      │ 5 min   │ 🔴 HIGH
                        │ TROUBLESHOOTING.md   │ 30 min  │
────────────────────────┼──────────────────────┼─────────┼─────────
AI miss winning move    │ QUICK_ACTION.md      │ 10 min  │ 🔴 HIGH
                        │ TROUBLESHOOTING.md   │ 1 hour  │
────────────────────────┼──────────────────────┼─────────┼─────────
Want to benchmark?      │ BENCHMARK_GUIDE.md   │ 1 hour  │ 🟡 MED
                        │ Profiler.h           │         │
────────────────────────┼──────────────────────┼─────────┼─────────
Want full evaluation?   │ PROJECT_EVALUATION   │ 30 min  │ 🟢 LOW
                        │ BENCHMARK_GUIDE.md   │ + 1h    │
────────────────────────┼──────────────────────┼─────────┼─────────
Have crashes?           │ TROUBLESHOOTING.md   │ 30 min  │ 🔴 HIGH
                        │ Section 4            │         │
────────────────────────┼──────────────────────┼─────────┼─────────
Want to optimize?       │ TROUBLESHOOTING.md   │ 1-3h    │ 🟡 MED
                        │ Profiler.h           │         │


┌─────────────────────────────────────────────────────────────┐
│               Performance Diagnosis Flow                    │
└─────────────────────────────────────────────────────────────┘

START: "Game lags when playing vs AI"
  │
  ├─→ [1] Add logging (5 min)
  │      └─→ Measure AI time
  │          ├─ EASY: ? ms
  │          ├─ MEDIUM: ? ms
  │          └─ HARD: ? ms
  │
  ├─→ [2] Compare to targets
  │      ├─ All < target? ──→ [6] Other issue (skip to animation check)
  │      └─ Some > target? ──→ [3] Reduce depth
  │
  ├─→ [3] Reduce AI depth by 1-2
  │      ├─ Smooth now? ──→ [5] OK! (use reduced depth)
  │      └─ Still lag? ──→ [4] Go async
  │
  ├─→ [4] Move AI to thread
  │      └─ Smooth now? ──→ [5] OK!
  │
  ├─→ [5] Re-benchmark
  │      └─ Document findings
  │
  ├─→ [6] Check animation
  │      ├─ Smooth? ──→ [7] Check render layers
  │      └─ Stutter? ──→ [8] Fix animation
  │
  ├─→ [7] Profile renderer
  │      └─ Find bottleneck
  │
  ├─→ [8] Check FPS/timestep
  │      └─ See TROUBLESHOOTING.md Section 3
  │
  └─→ END: Log issue & move to optimization phase


┌─────────────────────────────────────────────────────────────┐
│          AI Quality Diagnosis Flow                          │
└─────────────────────────────────────────────────────────────┘

START: "AI misses winning move"
  │
  ├─→ [1] Create test case (5 min)
  │      └─→ Setup board with winning move
  │
  ├─→ [2] Run AI_FindBestMove()
  │      ├─ Found correct move? ──→ [6] Not a bug (edge case)
  │      └─ Wrong move? ──→ [3] Debug
  │
  ├─→ [3] Test CheckTerminal()
  │      ├─ Returns INT_MAX/2? ──→ [4] OK, check CountLine()
  │      └─ Wrong value? ──→ [5] Fix CheckTerminal()
  │
  ├─→ [4] Test CountLine()
  │      ├─ Returns SCORES[5]? ──→ [7] Move not in candidates
  │      └─ Wrong score? ──→ [5] Fix CountLine()
  │
  ├─→ [5] Debug & fix logic
  │      └─ Re-test
  │
  ├─→ [6] Re-benchmark
  │      └─ Document findings
  │
  ├─→ [7] Debug GetCandidates()
  │      ├─ Includes winning? ──→ [8] Alpha-beta pruning issue
  │      └─ Not included? ──→ [9] Expand search radius
  │
  ├─→ [8] Disable pruning temporarily
  │      └─ AI finds move? → Keep pruning disabled or fix
  │
  ├─→ [9] Adjust GetCandidates()
  │      └─ Includes winning now? → Test & validate
  │
  └─→ END: Validate fix & re-benchmark


┌─────────────────────────────────────────────────────────────┐
│              File Selection Flowchart                       │
└─────────────────────────────────────────────────────────────┘

        ┌─────────────────────────────┐
        │   What's your situation?    │
        └──────────────┬──────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
    [Need    [Want to [Want full
     quick   benchmark evaluation]
     fix]    game]    
     │       │        │
     ▼       ▼        ▼
┌────────┐ ┌──────────┐ ┌─────────────┐
│QUICK   │ │BENCHMARK │ │PROJECT_     │
│ACTION  │ │GUIDE.md  │ │EVALUATION   │
│.md     │ │+Profiler │ │.md          │
└────────┘ └──────────┘ └─────────────┘
     │       │              │
     ▼       ▼              ▼
   5 min   1-2 hours    30 min + 1h
   Result  Result       Result
   ─────   ──────       ──────
   Fix!    Report       Score + Grade


┌─────────────────────────────────────────────────────────────┐
│              Timeline & Next Steps                          │
└─────────────────────────────────────────────────────────────┘

TODAY (1-2 hours):
  ☐ Read QUICK_ACTION.md (20 min)
  ☐ Add logging (10 min)
  ☐ Benchmark game (30 min)
  ☐ Identify bottleneck (20 min)

TOMORROW (1-3 hours):
  ☐ Read TROUBLESHOOTING.md (30 min)
  ☐ Implement quick fix (30 min - 1 hour)
  ☐ Test & verify (30 min)
  ☐ Re-benchmark (20 min)

THIS WEEK (2-4 hours):
  ☐ Run PROJECT_EVALUATION.md (2 hours)
  ☐ Document findings (30 min)
  ☐ Commit changes (20 min)
  ☐ Plan optimization (20 min)

NEXT WEEK (4-8 hours):
  ☐ Optimize based on findings (2-4 hours)
  ☐ Full evaluation (1-2 hours)
  ☐ Final testing (2 hours)
  ☐ Push to GitHub (20 min)


┌─────────────────────────────────────────────────────────────┐
│        Success Criteria (Green = Good)                      │
└─────────────────────────────────────────────────────────────┘

Metric              Target      Current   Status
─────────────────────────────────────────────────
FPS (during play)   ≥ 60        ? fps     ☐ ✅ ☐ ❌
AI EASY time        < 100ms     ? ms      ☐ ✅ ☐ ❌
AI MEDIUM time      < 500ms     ? ms      ☐ ✅ ☐ ❌
AI HARD time        < 3000ms    ? ms      ☐ ✅ ☐ ❌
Animation smooth    YES         ?         ☐ ✅ ☐ ❌
No frame drops      YES         ?         ☐ ✅ ☐ ❌
AI finds win        100%        ?%        ☐ ✅ ☐ ❌
No crashes          YES         ?         ☐ ✅ ☐ ❌
Project score       ≥ 85/100    ?/100     ☐ ✅ ☐ ❌


READY TO START? 🚀

→ Open QUICK_ACTION.md
→ Follow Step 1
→ Measure AI time
→ Report findings
→ Then decide next steps
```

---

## 🎓 **File Recommendation Matrix**

```
Your Situation              Perfect Files              Time
──────────────────────────────────────────────────────────
"Game lags!"                QUICK_ACTION.md            5-30 min
                            TROUBLESHOOTING.md (Sec 1)

"AI plays dumb"             QUICK_ACTION.md            10 min
                            TROUBLESHOOTING.md (Sec 2) 30 min

"Want numbers"              BENCHMARK_GUIDE.md         1-2 hrs
                            Profiler.h (integrate)     

"Rate my project"           PROJECT_EVALUATION.md      30 min
                            BENCHMARK_GUIDE.md         1 hr

"Animation stutters"        TROUBLESHOOTING.md (Sec 3) 30 min
                            BENCHMARK_GUIDE.md         1 hr

"Want it all"               All files above            4-6 hrs
                            Do systematically

"Something crashes"         TROUBLESHOOTING.md (Sec 4) 30 min
                            Add null checks
```

---

**Now go pick a file and start solving!** 🚀
