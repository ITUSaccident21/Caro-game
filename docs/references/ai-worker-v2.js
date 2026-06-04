/**
 * AI Worker v3 — Enhanced Difficulty System
 *
 * Easy:   Shallow search + evaluation noise + softmax temperature + weak defense
 * Medium: Deep search + transposition table + killer moves + history heuristic
 *         + null move pruning + PVS — no pre-search shortcuts, pure search strength
 * Hard:   Moderate search + pre-search tactical checks (dead four, compound threats)
 *
 * All difficulties feature opening variety for replayability.
 */

console.log('AI Worker 腳本開始載入...');
self.postMessage({ type: 'WORKER_READY' });

const BOARD_SIZE = 15;
const PLAYER_STONE = 1;
const COMPUTER_STONE = 2;
let isCancelled = false;
var currentRuleType = 'gomoku';

// ================================================================
//  Zobrist Hashing (used by Hard mode transposition table)
// ================================================================
const zobrist = [];
let zobristSide = 0;

(function initZobrist() {
    const rng = () => (Math.random() * 0x100000000) >>> 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        zobrist[i] = [];
        for (let j = 0; j < BOARD_SIZE; j++)
            zobrist[i][j] = [0, rng(), rng()];
    }
    zobristSide = rng();
})();

function computeHash(board) {
    let h = 0;
    for (let i = 0; i < BOARD_SIZE; i++)
        for (let j = 0; j < BOARD_SIZE; j++)
            if (board[i][j]) h ^= zobrist[i][j][board[i][j]];
    return h;
}

// ================================================================
//  Transposition Table
// ================================================================
const TT_SIZE = 1 << 20;
const TT_MASK = TT_SIZE - 1;
const FL_EXACT = 0, FL_LOWER = 1, FL_UPPER = 2;
let transTable = null;

function ttClear() {
    transTable = new Array(TT_SIZE);
    for (let i = 0; i < TT_SIZE; i++) transTable[i] = null;
}

function ttProbe(hash, depth, alpha, beta) {
    const e = transTable[hash & TT_MASK];
    if (!e || e.hash !== hash) return null;
    const result = { bestMove: e.move };
    if (e.depth >= depth) {
        if (e.flag === FL_EXACT) result.score = e.score;
        else if (e.flag === FL_LOWER && e.score >= beta) result.score = e.score;
        else if (e.flag === FL_UPPER && e.score <= alpha) result.score = e.score;
    }
    return result;
}

function ttStore(hash, depth, score, flag, move) {
    const idx = hash & TT_MASK;
    const e = transTable[idx];
    if (!e || e.depth <= depth || e.hash === hash)
        transTable[idx] = { hash, depth, score, flag, move };
}

// ================================================================
//  Killer Moves & History Heuristic
// ================================================================
const MAX_PLY = 32;
let killerMoves = [];
let historyTable = [];

function clearSearchTables() {
    killerMoves = [];
    for (let i = 0; i < MAX_PLY; i++) killerMoves.push([null, null]);
    historyTable = [];
    for (let i = 0; i < BOARD_SIZE; i++) {
        historyTable[i] = [];
        for (let j = 0; j < BOARD_SIZE; j++) historyTable[i][j] = 0;
    }
}

function storeKiller(ply, row, col) {
    if (ply >= MAX_PLY) return;
    const k = killerMoves[ply];
    if (k[0] && k[0][0] === row && k[0][1] === col) return;
    k[1] = k[0];
    k[0] = [row, col];
}

console.log('AI Worker 初始化完成，等待消息...');

// ================================================================
//  Device Benchmark (for Medium & Hard mode adaptive parameters)
// ================================================================
var deviceBenchmarkScore = 0;
var hardAdaptiveConfig = null;
var hardDiffAdaptiveConfig = null;

function runWorkerBenchmark() {
    var iterations = 2000000;
    var t0 = performance.now();
    var x = 0;
    for (var i = 0; i < iterations; i++) {
        x = (x * 1103515245 + 12345) & 0x7fffffff;
    }
    var elapsed = performance.now() - t0;
    deviceBenchmarkScore = Math.round(iterations / elapsed);

    var FAST_THRESHOLD = 60000;
    var MEDIUM_THRESHOLD = 35000;

    if (deviceBenchmarkScore >= FAST_THRESHOLD) {
        hardAdaptiveConfig = null;
        hardDiffAdaptiveConfig = null;
    } else if (deviceBenchmarkScore >= MEDIUM_THRESHOLD) {
        hardAdaptiveConfig = { maxDepth: 5, timeLimit: 3000, searchWidth: 18 };
        hardDiffAdaptiveConfig = { maxDepth: 7, timeLimit: 3500, searchWidth: 20 };
    } else {
        hardAdaptiveConfig = { maxDepth: 4, timeLimit: 2000, searchWidth: 15 };
        hardDiffAdaptiveConfig = { maxDepth: 6, timeLimit: 3000, searchWidth: 18 };
    }

    console.log('Worker benchmark: score=' + deviceBenchmarkScore + ', mediumAdaptive=' + JSON.stringify(hardAdaptiveConfig) + ', hardAdaptive=' + JSON.stringify(hardDiffAdaptiveConfig));
    self.postMessage({ type: 'WORKER_BENCHMARK', score: deviceBenchmarkScore, hardAdaptive: hardAdaptiveConfig });
}

runWorkerBenchmark();

// ================================================================
//  Difficulty Configuration
// ================================================================
function getConfig(difficulty) {
    switch (difficulty) {
        case 'easy':
            return {
                maxDepth: 2,
                timeLimit: 1500,
                searchWidth: 15,
                temperature: 0.5,
                topN: 5,
                noise: 50,
                atkWeight: 1.0,
                defWeight: 0.9,
                advanced: false,
                fullBoardEval: false
            };
        case 'medium':
            var hd = hardAdaptiveConfig || {};
            return {
                maxDepth: hd.maxDepth || 6,
                timeLimit: hd.timeLimit || 3500,
                searchWidth: hd.searchWidth || 20,
                temperature: 0,
                topN: 1,
                noise: 0,
                atkWeight: 1.0,
                defWeight: 1.0,
                advanced: true,
                fullBoardEval: true
            };
        case 'hard':
            var hda = hardDiffAdaptiveConfig || {};
            return {
                maxDepth: hda.maxDepth || 8,
                timeLimit: hda.timeLimit || 4000,
                searchWidth: hda.searchWidth || 22,
                temperature: 0,
                topN: 1,
                noise: 0,
                atkWeight: 1.0,
                defWeight: 1.0,
                advanced: false,
                fullBoardEval: true
            };
        case 'reducedHard':
            return {
                maxDepth: 4,
                timeLimit: 2000,
                searchWidth: 15,
                temperature: 0,
                topN: 1,
                noise: 0,
                atkWeight: 1.0,
                defWeight: 1.0,
                advanced: false,
                fullBoardEval: true
            };
        case 'reducedMedium':
            return {
                maxDepth: 4,
                timeLimit: 2000,
                searchWidth: 15,
                temperature: 0,
                topN: 1,
                noise: 0,
                atkWeight: 1.0,
                defWeight: 1.0,
                advanced: true,
                fullBoardEval: true
            };
        default:
            return getConfig('medium');
    }
}

// ================================================================
//  Utility: Gaussian Noise & Softmax Selection
// ================================================================
function gaussianNoise(sigma) {
    let u = 0, v = 0;
    while (u === 0) u = Math.random();
    while (v === 0) v = Math.random();
    return sigma * Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
}

function softmaxSelect(candidates, temperature, topN) {
    if (temperature <= 0 || topN <= 1 || candidates.length <= 1) return candidates[0];
    const top = candidates.slice(0, Math.min(topN, candidates.length));
    if (top.length <= 1) return top[0];

    const maxS = top[0].score;
    const minS = top[top.length - 1].score;
    const range = Math.max(1, maxS - minS);

    const weights = top.map(c => Math.exp(((c.score - minS) / range) / temperature));
    const total = weights.reduce((a, b) => a + b, 0);

    let r = Math.random() * total;
    for (let i = 0; i < weights.length; i++) {
        r -= weights[i];
        if (r <= 0) return top[i];
    }
    return top[top.length - 1];
}

// ================================================================
//  Opening Variety
// ================================================================
function getOpeningResponse(board) {
    let count = 0;
    let firstStone = null;
    for (let i = 0; i < BOARD_SIZE; i++)
        for (let j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] !== 0) {
                count++;
                if (count === 1) firstStone = { row: i, col: j };
            }

    if (count === 0) return { row: 7, col: 7 };

    if (count === 1) {
        const neighbors = [];
        for (let di = -1; di <= 1; di++)
            for (let dj = -1; dj <= 1; dj++) {
                if (di === 0 && dj === 0) continue;
                const r = firstStone.row + di, c = firstStone.col + dj;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === 0)
                    neighbors.push({ row: r, col: c });
            }
        if (neighbors.length > 0)
            return neighbors[Math.floor(Math.random() * neighbors.length)];
    }

    return null;
}

// ================================================================
//  Worker Message Handler
// ================================================================
var currentSeq = null;

self.onmessage = function(e) {
    const { action, data, seq } = e.data;
    currentSeq = seq != null ? seq : null;
    switch (action) {
        case 'START_AI_SEARCH':
            startAISearch(data);
            break;
        case 'CANCEL_AI_SEARCH':
            isCancelled = true;
            self.postMessage({ type: 'AI_SEARCH_CANCELLED', message: 'AI搜索已被取消' });
            break;
    }
};

// ================================================================
//  Main Search Entry
// ================================================================
function startAISearch({ board, difficulty, ruleType }) {
    currentRuleType = ruleType || 'gomoku';
    isCancelled = false;

    try {
        const winMove = findImmediateWin(board, COMPUTER_STONE);
        if (winMove) {
            self.postMessage({ type: 'AI_MOVE_FOUND', move: winMove, depth: 0, time: 0, source: 'aiWin', seq: currentSeq });
            return;
        }

        const blockMove = findImmediateWin(board, PLAYER_STONE);
        if (blockMove) {
            self.postMessage({ type: 'AI_MOVE_FOUND', move: blockMove, depth: 0, time: 0, source: 'blockWin', seq: currentSeq });
            return;
        }

        if (difficulty !== 'easy') {
            var aiLiveFour = findLiveFourThreat(board, COMPUTER_STONE);
            if (aiLiveFour) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: aiLiveFour, depth: 0, time: 0, source: 'createLiveFour', seq: currentSeq });
                return;
            }
        }

        {
            var playerLiveFour = findLiveFourThreat(board, PLAYER_STONE);
            if (playerLiveFour) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: playerLiveFour, depth: 0, time: 0, source: 'blockLiveFour', seq: currentSeq });
                return;
            }
        }

        if (difficulty === 'hard' || difficulty === 'reducedHard') {
            var playerCompoundH = findCompoundThreat(board, PLAYER_STONE);
            if (playerCompoundH) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: playerCompoundH, depth: 0, time: 0, source: 'blockCompound:' + playerCompoundH.threatType, seq: currentSeq });
                return;
            }
        }

        if (difficulty === 'medium' || difficulty === 'reducedMedium') {
            var aiDeadFour = findDeadFourThreat(board, COMPUTER_STONE);
            if (aiDeadFour) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: aiDeadFour, depth: 0, time: 0, source: 'createDeadFour', seq: currentSeq });
                return;
            }

            var playerDeadFour = findDeadFourThreat(board, PLAYER_STONE);
            if (playerDeadFour) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: playerDeadFour, depth: 0, time: 0, source: 'blockDeadFour', seq: currentSeq });
                return;
            }

            var aiCompound = findCompoundThreat(board, COMPUTER_STONE);
            if (aiCompound) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: aiCompound, depth: 0, time: 0, source: 'createCompound:' + aiCompound.threatType, seq: currentSeq });
                return;
            }

            var playerCompound = findCompoundThreat(board, PLAYER_STONE);
            if (playerCompound) {
                self.postMessage({ type: 'AI_MOVE_FOUND', move: playerCompound, depth: 0, time: 0, source: 'blockCompound:' + playerCompound.threatType, seq: currentSeq });
                return;
            }
        }

        const opening = getOpeningResponse(board);
        if (opening) {
            self.postMessage({ type: 'AI_MOVE_FOUND', move: opening, depth: 0, time: 0, source: 'workerOpening', seq: currentSeq });
            return;
        }

        const config = getConfig(difficulty);

        if (config.advanced) {
            ttClear();
            clearSearchTables();
        }

        const result = iterativeDeepening(board, config);

        if (result && result.move) {
            self.postMessage({
                type: 'AI_MOVE_FOUND',
                move: result.move,
                depth: result.depth,
                time: result.time,
                source: 'search',
                seq: currentSeq
            });
        } else {
            self.postMessage({
                type: 'AI_MOVE_FOUND',
                move: getFallbackMove(board),
                depth: 0,
                time: 0,
                source: 'searchFallback',
                seq: currentSeq
            });
        }
    } catch (err) {
        console.error('AI search error:', err);
        self.postMessage({
            type: 'AI_MOVE_FOUND',
            move: getFallbackMove(board),
            depth: 0,
            time: 0,
            error: err.message,
            source: 'error',
            seq: currentSeq
        });
    }
}

// ================================================================
//  Iterative Deepening with Softmax Selection
// ================================================================
function iterativeDeepening(board, config) {
    const t0 = Date.now();
    let candidates = config.fullBoardEval ? getOrderedMovesFull(board) : getOrderedMoves(board);
    if (candidates.length === 0) return null;

    const hash0 = config.advanced ? computeHash(board) : 0;
    let bestMove = candidates[0];
    let finalDepth = 0;

    for (let depth = 1; depth <= config.maxDepth; depth++) {
        const depthT0 = Date.now();
        if (isCancelled) break;

        const scored = [];
        let searchOk = true;

        const width = Math.min(config.searchWidth, candidates.length);
        for (let i = 0; i < width; i++) {
            if (isCancelled || Date.now() - t0 > config.timeLimit * 0.75) {
                searchOk = false;
                break;
            }

            const m = candidates[i];
            board[m.row][m.col] = COMPUTER_STONE;

            if (checkWin(board, m.row, m.col, COMPUTER_STONE)) {
                board[m.row][m.col] = 0;
                return { move: { row: m.row, col: m.col }, depth, time: Date.now() - t0 };
            }

            let score;
            if (config.advanced) {
                const h = hash0 ^ zobrist[m.row][m.col][COMPUTER_STONE] ^ zobristSide;
                score = -negamaxAdvanced(board, depth - 1, -Infinity, Infinity,
                    PLAYER_STONE, 1, h, t0, config, true);
            } else {
                score = -negamaxSimple(board, depth - 1, -Infinity, Infinity,
                    PLAYER_STONE, t0, config);
            }

            board[m.row][m.col] = 0;
            scored.push({ row: m.row, col: m.col, score });
        }

        if (scored.length < Math.min(3, candidates.length)) break;

        scored.sort((a, b) => b.score - a.score);
        candidates = scored;
        bestMove = scored[0];
        finalDepth = depth;

        self.postMessage({
            type: 'AI_PROGRESS',
            depth,
            move: bestMove,
            elapsed: Date.now() - t0
        });

        const depthTime = Date.now() - depthT0;
        const elapsed = Date.now() - t0;
        if (elapsed + depthTime * 4 > config.timeLimit * 0.9) break;
    }

    if (config.temperature > 0 && candidates.length > 1) {
        const selected = softmaxSelect(candidates, config.temperature, config.topN);
        bestMove = { row: selected.row, col: selected.col };
    }

    console.log(`Search complete: depth=${finalDepth}, time=${Date.now() - t0}ms, ` +
        `move=(${bestMove.row},${bestMove.col})`);
    return { move: bestMove, depth: finalDepth, time: Date.now() - t0 };
}

// ================================================================
//  Negamax Advanced (Hard) — TT + Killers + History + NullMove + PVS
// ================================================================
function negamaxAdvanced(board, depth, alpha, beta, player, ply, hash, t0, config, allowNull) {
    if (isCancelled || Date.now() - t0 > config.timeLimit * 0.85) {
        return staticEval(board, config, player);
    }

    const origAlpha = alpha;

    let ttBestMove = null;
    const ttHit = ttProbe(hash, depth, alpha, beta);
    if (ttHit) {
        ttBestMove = ttHit.bestMove;
        if (ttHit.score !== undefined) return ttHit.score;
    }

    if (depth <= 0) {
        return staticEval(board, config, player);
    }

    const opponent = player === COMPUTER_STONE ? PLAYER_STONE : COMPUTER_STONE;

    // Null move pruning disabled: Gomoku's sharp tactical nature (one missed
    // block = instant loss) makes the "pass a turn" assumption unreliable.
    // The TT + killer + history + PVS provide sufficient search efficiency.

    const moves = getOrderedMovesAdvanced(board, ply, ttBestMove);
    if (moves.length === 0) return staticEval(board, config, player);

    let bestScore = -Infinity;
    let bestMoveFound = null;
    const width = Math.min(config.searchWidth, moves.length);

    for (let i = 0; i < width; i++) {
        if (isCancelled || Date.now() - t0 > config.timeLimit * 0.85) break;

        const m = moves[i];
        board[m.row][m.col] = player;
        const newHash = hash ^ zobrist[m.row][m.col][player] ^ zobristSide;

        if (checkWinAware(board, m.row, m.col, player)) {
            board[m.row][m.col] = 0;
            const winScore = 100000 + depth;
            ttStore(hash, depth, winScore, FL_EXACT, { row: m.row, col: m.col });
            return winScore;
        }

        let score;
        if (i === 0) {
            score = -negamaxAdvanced(board, depth - 1, -beta, -alpha,
                opponent, ply + 1, newHash, t0, config, true);
        } else {
            score = -negamaxAdvanced(board, depth - 1, -alpha - 1, -alpha,
                opponent, ply + 1, newHash, t0, config, true);
            if (score > alpha && score < beta) {
                score = -negamaxAdvanced(board, depth - 1, -beta, -alpha,
                    opponent, ply + 1, newHash, t0, config, true);
            }
        }

        board[m.row][m.col] = 0;

        if (score > bestScore) {
            bestScore = score;
            bestMoveFound = { row: m.row, col: m.col };
        }
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            storeKiller(ply, m.row, m.col);
            historyTable[m.row][m.col] += depth * depth;
            break;
        }
    }

    if (bestScore === -Infinity) return staticEval(board, config, player);

    let flag;
    if (bestScore <= origAlpha) flag = FL_UPPER;
    else if (bestScore >= beta) flag = FL_LOWER;
    else flag = FL_EXACT;
    ttStore(hash, depth, bestScore, flag, bestMoveFound);

    return bestScore;
}

// ================================================================
//  Negamax Simple (Easy / Medium)
// ================================================================
function negamaxSimple(board, depth, alpha, beta, player, t0, config) {
    if (depth <= 0 || isCancelled || Date.now() - t0 > config.timeLimit * 0.8) {
        let score = staticEval(board, config, player);
        if (config.noise > 0) score += gaussianNoise(config.noise);
        return score;
    }

    const opponent = player === COMPUTER_STONE ? PLAYER_STONE : COMPUTER_STONE;
    const moves = config.fullBoardEval ? getOrderedMovesFull(board) : getOrderedMoves(board);
    const width = Math.min(config.searchWidth, moves.length);

    let bestScore = -Infinity;

    for (let i = 0; i < width; i++) {
        if (isCancelled || Date.now() - t0 > config.timeLimit * 0.8) break;

        const m = moves[i];
        board[m.row][m.col] = player;

        if (checkWinAware(board, m.row, m.col, player)) {
            board[m.row][m.col] = 0;
            return 100000 + depth;
        }

        const score = -negamaxSimple(board, depth - 1, -beta, -alpha, opponent, t0, config);
        board[m.row][m.col] = 0;

        if (score > bestScore) bestScore = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }

    return bestScore === -Infinity ? staticEval(board, config, player) : bestScore;
}

// ================================================================
//  Static Evaluation (always from AI perspective, with difficulty weights)
// ================================================================
function staticEval(board, config, currentPlayer) {
    let score = 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] === COMPUTER_STONE) {
                score += evaluatePosition(board, i, j, COMPUTER_STONE) * config.atkWeight;
            } else if (board[i][j] === PLAYER_STONE) {
                score -= evaluatePosition(board, i, j, PLAYER_STONE) * config.defWeight;
            }
        }
    }
    return currentPlayer === COMPUTER_STONE ? score : -score;
}

// ================================================================
//  Position & Line Evaluation (pattern matching)
// ================================================================
function evaluatePosition(board, row, col, player) {
    let totalScore = 0;
    const directions = [[0, 1], [1, 0], [1, 1], [1, -1]];
    for (const [dx, dy] of directions) {
        totalScore += evaluateLine(board, row, col, dx, dy, player);
    }
    return totalScore;
}

function evaluateLine(board, row, col, dx, dy, player) {
    const line = [];
    for (let i = -4; i <= 4; i++) {
        const r = row + i * dx;
        const c = col + i * dy;
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            line.push(board[r][c]);
        } else {
            line.push(-1);
        }
    }
    return analyzePattern(line, player);
}

const PATTERNS = [
    { regex: /XXXXX/, score: 100000 },
    { regex: /\.XXXX\./, score: 50000 },
    { regex: /XXXX\./, score: 10000 },
    { regex: /\.XXXX/, score: 10000 },
    { regex: /XXX\.X/, score: 10000 },
    { regex: /X\.XXX/, score: 10000 },
    { regex: /XX\.XX/, score: 10000 },
    { regex: /\.XXX\./, score: 5000 },
    { regex: /\.XX\.X\./, score: 5000 },
    { regex: /\.X\.XX\./, score: 5000 },
    { regex: /XXX\./, score: 500 },
    { regex: /\.XXX/, score: 500 },
    { regex: /XX\.X/, score: 500 },
    { regex: /X\.XX/, score: 500 },
    { regex: /\.XX\./, score: 200 },
    { regex: /\.X\.X\./, score: 200 },
    { regex: /XX\./, score: 50 },
    { regex: /\.XX/, score: 50 },
    { regex: /X\.X/, score: 50 },
    { regex: /OOOOO/, score: -100000 },
    { regex: /\.OOOO\./, score: -80000 },
    { regex: /OOOO\./, score: -10000 },
    { regex: /\.OOOO/, score: -10000 },
    { regex: /OOO\.O/, score: -10000 },
    { regex: /O\.OOO/, score: -10000 },
    { regex: /OO\.OO/, score: -10000 },
    { regex: /\.OOO\./, score: -8000 },
    { regex: /\.OO\.O\./, score: -5000 },
    { regex: /\.O\.OO\./, score: -5000 },
    { regex: /OOO\./, score: -500 },
    { regex: /\.OOO/, score: -500 },
    { regex: /OO\.O/, score: -500 },
    { regex: /O\.OO/, score: -500 },
    { regex: /\.OO\./, score: -200 },
    { regex: /\.O\.O\./, score: -200 },
    { regex: /OO\./, score: -50 },
    { regex: /\.OO/, score: -50 },
    { regex: /O\.O/, score: -50 },
];

function analyzePattern(line, player) {
    const opponent = player === COMPUTER_STONE ? PLAYER_STONE : COMPUTER_STONE;
    let score = 0;

    const pattern = line.map(cell => {
        if (cell === player) return 'X';
        if (cell === opponent) return 'O';
        if (cell === 0) return '.';
        return '#';
    }).join('');

    for (const { regex, score: pScore } of PATTERNS) {
        const matches = pattern.match(regex);
        if (matches) score += pScore * matches.length;
    }

    return score;
}

// ================================================================
//  Full Board Evaluation (for Medium/Hard move ordering)
// ================================================================
function evaluateBoard(board, player) {
    const opponent = player === COMPUTER_STONE ? PLAYER_STONE : COMPUTER_STONE;
    let score = 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] === player) {
                score += evaluatePosition(board, i, j, player);
            } else if (board[i][j] === opponent) {
                score -= evaluatePosition(board, i, j, opponent);
            }
        }
    }
    return score;
}

function fullBoardMoveEval(board, row, col) {
    let score = 0;
    board[row][col] = COMPUTER_STONE;
    score += evaluateBoard(board, COMPUTER_STONE);
    board[row][col] = 0;
    board[row][col] = PLAYER_STONE;
    score += evaluateBoard(board, PLAYER_STONE) * 1.1;
    board[row][col] = 0;
    return score;
}

function getOrderedMovesFull(board) {
    const occupied = [];
    for (let i = 0; i < BOARD_SIZE; i++)
        for (let j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] !== 0) occupied.push([i, j]);

    if (occupied.length === 0) return [{ row: 7, col: 7, score: 0 }];

    if (occupied.length === 1) {
        const cr = occupied[0][0], cc = occupied[0][1];
        const adj = [];
        for (let di = -1; di <= 1; di++)
            for (let dj = -1; dj <= 1; dj++) {
                if (di === 0 && dj === 0) continue;
                const r = cr + di, c = cc + dj;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === 0)
                    adj.push({ row: r, col: c, score: 0 });
            }
        return adj;
    }

    const candidateSet = new Set();
    for (const [pr, pc] of occupied) {
        for (let di = -2; di <= 2; di++)
            for (let dj = -2; dj <= 2; dj++) {
                const ni = pr + di, nj = pc + dj;
                if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE && board[ni][nj] === 0)
                    candidateSet.add(ni * BOARD_SIZE + nj);
            }
    }

    const moves = [];
    for (const key of candidateSet) {
        const row = Math.floor(key / BOARD_SIZE);
        const col = key % BOARD_SIZE;
        const score = fullBoardMoveEval(board, row, col);
        moves.push({ row, col, score });
    }

    moves.sort((a, b) => b.score - a.score);
    return moves;
}

// ================================================================
//  Move Ordering — Basic (for Easy only)
// ================================================================
function getOrderedMoves(board) {
    const occupied = [];
    for (let i = 0; i < BOARD_SIZE; i++)
        for (let j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] !== 0) occupied.push([i, j]);

    if (occupied.length === 0) {
        return [{ row: 7, col: 7, score: 0 }];
    }

    if (occupied.length === 1) {
        const cr = occupied[0][0], cc = occupied[0][1];
        const adj = [];
        for (let di = -1; di <= 1; di++)
            for (let dj = -1; dj <= 1; dj++) {
                if (di === 0 && dj === 0) continue;
                const r = cr + di, c = cc + dj;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === 0)
                    adj.push({ row: r, col: c, score: 0 });
            }
        return adj;
    }

    const candidateSet = new Set();
    for (const [pr, pc] of occupied) {
        for (let di = -2; di <= 2; di++)
            for (let dj = -2; dj <= 2; dj++) {
                const ni = pr + di, nj = pc + dj;
                if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE && board[ni][nj] === 0)
                    candidateSet.add(ni * BOARD_SIZE + nj);
            }
    }

    const moves = [];
    for (const key of candidateSet) {
        const row = Math.floor(key / BOARD_SIZE);
        const col = key % BOARD_SIZE;
        const score = quickMoveEval(board, row, col);
        moves.push({ row, col, score });
    }

    moves.sort((a, b) => b.score - a.score);
    return moves;
}

function quickMoveEval(board, row, col) {
    let score = 0;
    board[row][col] = COMPUTER_STONE;
    score += evaluatePosition(board, row, col, COMPUTER_STONE) * 1.1;
    board[row][col] = 0;
    board[row][col] = PLAYER_STONE;
    score += evaluatePosition(board, row, col, PLAYER_STONE) * 0.9;
    board[row][col] = 0;
    return score;
}

// ================================================================
//  Move Ordering — Advanced (for Hard non-root: history + killers)
// ================================================================
function getOrderedMovesAdvanced(board, ply, ttBestMove) {
    const occupied = [];
    for (let i = 0; i < BOARD_SIZE; i++)
        for (let j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] !== 0) occupied.push([i, j]);

    if (occupied.length === 0) return [{ row: 7, col: 7, score: 0 }];

    const candidateSet = new Set();
    for (const [pr, pc] of occupied) {
        for (let di = -2; di <= 2; di++)
            for (let dj = -2; dj <= 2; dj++) {
                const ni = pr + di, nj = pc + dj;
                if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE && board[ni][nj] === 0)
                    candidateSet.add(ni * BOARD_SIZE + nj);
            }
    }

    const moves = [];
    for (const key of candidateSet) {
        const row = Math.floor(key / BOARD_SIZE);
        const col = key % BOARD_SIZE;
        let score = historyTable[row][col];

        if (ply <= 2) {
            board[row][col] = COMPUTER_STONE;
            if (checkWin(board, row, col, COMPUTER_STONE)) score += 900000;
            board[row][col] = 0;

            board[row][col] = PLAYER_STONE;
            if (checkWinAware(board, row, col, PLAYER_STONE)) score += 800000;
            board[row][col] = 0;
        }

        if (ply < MAX_PLY) {
            const k = killerMoves[ply];
            if (k[0] && k[0][0] === row && k[0][1] === col) score += 500000;
            if (k[1] && k[1][0] === row && k[1][1] === col) score += 400000;
        }

        if (ttBestMove && ttBestMove.row === row && ttBestMove.col === col)
            score += 1000000;

        moves.push({ row, col, score });
    }

    moves.sort((a, b) => b.score - a.score);
    return moves;
}

// ================================================================
//  Immediate Win Detection
// ================================================================
function findImmediateWin(board, player) {
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] === 0) {
                board[i][j] = player;
                if (checkWinAware(board, i, j, player)) {
                    board[i][j] = 0;
                    return { row: i, col: j };
                }
                board[i][j] = 0;
            }
        }
    }
    return null;
}

// ================================================================
//  Renju Forbidden Move Check (simplified)
//  Checks overline (6+), double four (2+ fours), double three (2+ live threes)
// ================================================================
function isRenjuForbiddenSimple(board, row, col) {
    var dirs = [[1,0],[0,1],[1,1],[1,-1]];
    board[row][col] = PLAYER_STONE;
    var fourCount = 0, liveThreeCount = 0;

    for (var d = 0; d < dirs.length; d++) {
        var dx = dirs[d][0], dy = dirs[d][1];
        var fwd = 0, bwd = 0, fwdOpen = false, bwdOpen = false;

        for (var k = 1; k <= 5; k++) {
            var r = row + k * dx, c = col + k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === PLAYER_STONE) fwd++;
                else { fwdOpen = (board[r][c] === 0); break; }
            } else break;
        }
        for (var k = 1; k <= 5; k++) {
            var r = row - k * dx, c = col - k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === PLAYER_STONE) bwd++;
                else { bwdOpen = (board[r][c] === 0); break; }
            } else break;
        }

        var total = fwd + bwd;
        var openEnds = (fwdOpen ? 1 : 0) + (bwdOpen ? 1 : 0);

        if (total >= 5) { board[row][col] = 0; return true; }
        if (total >= 3 && openEnds >= 1) fourCount++;
        if (total === 2 && openEnds === 2) liveThreeCount++;
    }

    board[row][col] = 0;
    if (fourCount >= 2) return true;
    if (liveThreeCount >= 2) return true;
    return false;
}

// ================================================================
//  Live Four Threat Detection
//  Finds cells where placing a stone creates an unblockable live four (.XXXX.)
// ================================================================
function findLiveFourThreat(board, player) {
    for (var i = 0; i < BOARD_SIZE; i++) {
        for (var j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] !== 0) continue;
            board[i][j] = player;
            if (hasLiveFour(board, i, j, player)) {
                board[i][j] = 0;
                if (currentRuleType === 'renju' && player === PLAYER_STONE && isRenjuForbiddenSimple(board, i, j)) continue;
                return { row: i, col: j };
            }
            board[i][j] = 0;
        }
    }
    return null;
}

function hasLiveFour(board, row, col, player) {
    var directions = [[0, 1], [1, 0], [1, 1], [1, -1]];
    for (var d = 0; d < directions.length; d++) {
        var dx = directions[d][0], dy = directions[d][1];
        var count = 1;
        var openBefore = false, openAfter = false;

        for (var k = 1; k < 5; k++) {
            var r = row + k * dx, c = col + k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === player) count++;
                else { openAfter = (board[r][c] === 0); break; }
            } else break;
        }

        for (var k = 1; k < 5; k++) {
            var r = row - k * dx, c = col - k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === player) count++;
                else { openBefore = (board[r][c] === 0); break; }
            } else break;
        }

        if (count >= 4 && openBefore && openAfter) return true;
    }
    return false;
}

function hasDeadFour(board, row, col, player) {
    var directions = [[0, 1], [1, 0], [1, 1], [1, -1]];
    for (var d = 0; d < directions.length; d++) {
        var dx = directions[d][0], dy = directions[d][1];
        var count = 1;
        var openBefore = false, openAfter = false;

        for (var k = 1; k < 5; k++) {
            var r = row + k * dx, c = col + k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === player) count++;
                else { openAfter = (board[r][c] === 0); break; }
            } else break;
        }

        for (var k = 1; k < 5; k++) {
            var r = row - k * dx, c = col - k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === player) count++;
                else { openBefore = (board[r][c] === 0); break; }
            } else break;
        }

        if (count >= 4 && (openBefore || openAfter) && !(openBefore && openAfter)) return true;
    }
    return false;
}

function findDeadFourThreat(board, player) {
    for (var i = 0; i < BOARD_SIZE; i++) {
        for (var j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] !== 0) continue;
            board[i][j] = player;
            if (hasDeadFour(board, i, j, player)) {
                board[i][j] = 0;
                if (currentRuleType === 'renju' && player === PLAYER_STONE && isRenjuForbiddenSimple(board, i, j)) continue;
                return { row: i, col: j };
            }
            board[i][j] = 0;
        }
    }
    return null;
}

// ================================================================
//  Compound Threat Detection (Hard only)
//  Detects moves that create multiple threats simultaneously:
//  - doubleLiveThree: 2+ live threes → forced win
//  - fourThree: dead four + live three → forced win
//  - doubleDeadFour: 2+ dead fours → forced win
// ================================================================
function analyzeMoveThreat(board, row, col, player) {
    var dirs = [[1,0],[0,1],[1,1],[1,-1]];
    board[row][col] = player;
    var liveThrees = 0, deadFours = 0, liveFours = 0;

    for (var d = 0; d < dirs.length; d++) {
        var dx = dirs[d][0], dy = dirs[d][1];
        var fwd = 0, bwd = 0;
        var fwdOpen = false, bwdOpen = false;

        for (var k = 1; k < 5; k++) {
            var r = row + k * dx, c = col + k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === player) fwd++;
                else { fwdOpen = (board[r][c] === 0); break; }
            } else break;
        }
        for (var k = 1; k < 5; k++) {
            var r = row - k * dx, c = col - k * dy;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                if (board[r][c] === player) bwd++;
                else { bwdOpen = (board[r][c] === 0); break; }
            } else break;
        }

        var total = fwd + bwd;
        var openEnds = (fwdOpen ? 1 : 0) + (bwdOpen ? 1 : 0);

        if (total >= 3 && openEnds === 2) liveFours++;
        else if (total >= 3 && openEnds === 1) deadFours++;
        else if (total === 2 && openEnds === 2) liveThrees++;
    }

    board[row][col] = 0;

    var threatType = null;
    if (liveFours > 0) threatType = null;
    else if (deadFours >= 2) threatType = 'doubleDeadFour';
    else if (deadFours >= 1 && liveThrees >= 1) threatType = 'fourThree';
    else if (liveThrees >= 2) threatType = 'doubleLiveThree';

    return { liveThrees: liveThrees, deadFours: deadFours, liveFours: liveFours, threatType: threatType };
}

function findCompoundThreat(board, player) {
    for (var i = 0; i < BOARD_SIZE; i++) {
        for (var j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] !== 0) continue;
            var result = analyzeMoveThreat(board, i, j, player);
            if (result.threatType) {
                if (currentRuleType === 'renju' && player === PLAYER_STONE) {
                    if (result.threatType === 'doubleLiveThree' || result.threatType === 'doubleDeadFour') continue;
                }
                return { row: i, col: j, threatType: result.threatType };
            }
        }
    }
    return null;
}

// ================================================================
//  Win Check
// ================================================================
function checkWin(board, row, col, player) {
    return (
        checkDirection(board, row, col, 1, 0, player) ||
        checkDirection(board, row, col, 0, 1, player) ||
        checkDirection(board, row, col, 1, 1, player) ||
        checkDirection(board, row, col, 1, -1, player)
    );
}

function checkDirection(board, row, col, dx, dy, player) {
    let count = 1;
    for (let i = 1; i < 5; i++) {
        const r = row + i * dx, c = col + i * dy;
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === player)
            count++;
        else break;
    }
    for (let i = 1; i < 5; i++) {
        const r = row - i * dx, c = col - i * dy;
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === player)
            count++;
        else break;
    }
    return count >= 5;
}

function checkDirectionCount(board, row, col, dx, dy, player) {
    var count = 1;
    for (var i = 1; i <= 5; i++) {
        var r = row + i * dx, c = col + i * dy;
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === player)
            count++;
        else break;
    }
    for (var i = 1; i <= 5; i++) {
        var r = row - i * dx, c = col - i * dy;
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board[r][c] === player)
            count++;
        else break;
    }
    return count;
}

function checkExactFive(board, row, col, player) {
    var dirs = [[1,0],[0,1],[1,1],[1,-1]];
    for (var d = 0; d < dirs.length; d++) {
        if (checkDirectionCount(board, row, col, dirs[d][0], dirs[d][1], player) === 5) return true;
    }
    return false;
}

function checkWinAware(board, row, col, player) {
    if (currentRuleType === 'renju' && player === PLAYER_STONE) {
        return checkExactFive(board, row, col, player);
    }
    return checkWin(board, row, col, player);
}

// ================================================================
//  Fallback Move
// ================================================================
function getFallbackMove(board) {
    const center = Math.floor(BOARD_SIZE / 2);
    for (let radius = 0; radius < BOARD_SIZE; radius++) {
        for (let i = -radius; i <= radius; i++) {
            for (let j = -radius; j <= radius; j++) {
                const row = center + i, col = center + j;
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] === 0)
                    return { row, col };
            }
        }
    }
    return null;
}
