#pragma once
#include <string>
#include <vector>
#include "game/GameDef.h"

bool SaveGame(const _GAMESTATE& state, const std::string& filename);
bool CheckSaveExists(const std::string& filename);
bool DeleteSave(const std::string& filename);
bool LoadGame(_GAMESTATE& state, const std::string& filename);
void InitSaveFolder();
std::vector<std::string> GetSaveFiles();