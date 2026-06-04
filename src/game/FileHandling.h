#pragma once
#include <string>
#include <vector>
#include "GameDef.h"

// Sửa void SaveGame thành bool
bool SaveGame(const _GAMESTATE& state, const std::string& filename);

// Thêm hàm chỉ để kiểm tra file tồn tại
bool CheckSaveExists(const std::string& filename);

bool LoadGame(_GAMESTATE& state, const std::string& filename);
void InitSaveFolder();
std::vector<std::string> GetSaveFiles();
void ShowSaveList();
std::string InputFileName();