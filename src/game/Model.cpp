#include "game/Model.h"
#include <cstring>

void NewSession(_GAMESTATE& state)
{
	for (int i = 0; i < 2; i++)
		state.players[i].moves = 0;

	for (int i = 0; i < BOARD_SIZE; i++)
		for (int j = 0; j < BOARD_SIZE; j++) {
			state._BOARD[i][j].x = j;
			state._BOARD[i][j].y = i;
			state._BOARD[i][j].c = 0;
		}

	state.turn    = true;
	state.command = -1;
	state.players[0].mark = L'X';
	state.players[1].mark = L'O';
}

// Không đụng đến tên người chơi — tên được set bởi App_Init (mặc định)

void ResetData(_GAMESTATE& state)
{
	for (int i = 0; i < 2; i++) {
		state.players[i].moves  = 0;
		state.players[i].wins   = 0;
		state.players[i].losses = 0;
		state.players[i].draws  = 0;
	}
	for (int i = 0; i < BOARD_SIZE; i++)
		for (int j = 0; j < BOARD_SIZE; j++) {
			state._BOARD[i][j].x = j;
			state._BOARD[i][j].y = i;
			state._BOARD[i][j].c = 0;
		}

	state.turn    = true;
	state.command = -1;
	state.players[0].mark = L'X';
	state.players[1].mark = L'O';
}

int CheckBoard(_GAMESTATE& state, int row, int col)
{
	if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return 0;
	if (state._BOARD[row][col].c != 0) return 0;

	state._LastI = row;
	state._LastJ = col;
	state._BOARD[row][col].c = state.turn ? -1 : 1;
	return state._BOARD[row][col].c;
}

bool isBoardFull(_GAMESTATE& state) {
	for (int i = 0; i < BOARD_SIZE; i++)
		for (int j = 0; j < BOARD_SIZE; j++)
			if (state._BOARD[i][j].c == 0)
				return false;
	return true;
}

static int countDir(_GAMESTATE& state, int i, int j, int dx, int dy, bool* blocked)
{
	int count = 0;
	int color = state._BOARD[i][j].c;
	int ni = i + dx;
	int nj = j + dy;

	while (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE && state._BOARD[ni][nj].c == color)
	{
		count++;
		ni += dx;
		nj += dy;
	}

	*blocked = (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE)
	        && (state._BOARD[ni][nj].c != 0 && state._BOARD[ni][nj].c != color);
	return count;
}

bool winCheck(_GAMESTATE& state, int i, int j)
{
	int dx[] = { 1, 0, 1, 1 };
	int dy[] = { 0, 1, 1, -1 };
	for (int k = 0; k < 4; k++)
	{
		bool b1, b2;
		int count1 = countDir(state, i, j,  dx[k],  dy[k], &b1);
		int count2 = countDir(state, i, j, -dx[k], -dy[k], &b2);
		int total  = count1 + count2 + 1;

		if (total >= WIN_COUNT)
		{
			int idx = 0;
			if (idx < WIN_COUNT) state._WIN_CELLS[idx++] = state._BOARD[i][j];

			int ni = i + dx[k], nj = j + dy[k];
			for (int step = 0; step < count1 && idx < WIN_COUNT; step++) {
				state._WIN_CELLS[idx++] = state._BOARD[ni][nj];
				ni += dx[k]; nj += dy[k];
			}

			ni = i - dx[k]; nj = j - dy[k];
			for (int step = 0; step < count2 && idx < WIN_COUNT; step++) {
				state._WIN_CELLS[idx++] = state._BOARD[ni][nj];
				ni -= dx[k]; nj -= dy[k];
			}

			return true;
		}
	}
	return false;
}

int ProcessFinish(_GAMESTATE& state, int pWhoWin)
{
	int winnerIdx = -1;
	int loserIdx  = -1;

	switch (pWhoWin)
	{
	case P1_THANG:
		winnerIdx = 0; loserIdx = 1; break;
	case P2_THANG:
		winnerIdx = 1; loserIdx = 0; break;
	case HOA:
		state.players[0].draws++;
		state.players[1].draws++;
		break;
	case CHUA_KET_THUC:
		state.turn = !state.turn;
		break;
	}

	if (winnerIdx != -1) {
		state.players[winnerIdx].wins++;
		state.players[loserIdx].losses++;
	}

	state.gameStatus = pWhoWin;
	return pWhoWin;
}

int TestBoard(_GAMESTATE& state) {
	if (winCheck(state, state._LastI, state._LastJ))
		return (state.turn ? P1_THANG : P2_THANG);

	if (isBoardFull(state)) return HOA;

	return CHUA_KET_THUC;
}
