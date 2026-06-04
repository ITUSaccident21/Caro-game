#pragma once
#include "GameDef.h"

// Reset du lieu dau tran, tra ban co ve trang thai clear
void NewSession(_GAMESTATE& state);

// Reset hoan toan diem so va thong tin
void ResetData(_GAMESTATE& state);

// Dat quan tai (row, col) theo luot hien tai. Tra ve -1/1 neu thanh cong, 0 neu o da co quan hoac out-of-bounds.
int CheckBoard(_GAMESTATE& state, int row, int col);

// Xac minh ban co da day (het cho) chua
bool isBoardFull(_GAMESTATE& state);

// Kiem tra thu thuoc tinh Win cua 1 nuoc co
bool winCheck(_GAMESTATE& state, int i, int j);

// Kiem tra xem ban co da ket thuc tran chua
int TestBoard(_GAMESTATE& state);

// Xu li luot di theo ket qua win/lose/draw hay choi tiep return ve
int ProcessFinish(_GAMESTATE& state, int pWhoWin);