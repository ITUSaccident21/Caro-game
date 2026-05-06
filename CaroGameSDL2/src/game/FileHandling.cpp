#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <conio.h>
#include "GameDef.h"
#include "FileHandling.h"

using namespace std;

bool SaveGame(const _GAMESTATE& state, const string& filename) {
    string filepath = "Saves/" + filename + ".dat";
    ofstream out(filepath, ios::binary);
    if (!out) return false;
    out.write((char*)&state, sizeof(_GAMESTATE));
    out.close();
    return true;
}

bool CheckSaveExists(const string& filename) {
    string filepath = "Saves/" + filename + ".dat";
    return std::filesystem::exists(filepath);
}

bool LoadGame(_GAMESTATE& state, const string& filename) {
    ifstream in("Saves/" + filename + ".dat", ios::binary);
    if (!in) {
        return false;
    }
    in.read((char*)&state, sizeof(_GAMESTATE));
    in.close();
    return true;
}

void InitSaveFolder() {
    if (!std::filesystem::exists("Saves")) {
        std::filesystem::create_directory("Saves");
    }
}

std::vector<std::string> GetSaveFiles() {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator("Saves")) {
        files.push_back(entry.path().stem().string());
    }
    return files;
}

void ShowSaveList() {
    auto files = GetSaveFiles();
    for (int i = 0; i < files.size(); i++) {
        std::cout << "  " << i + 1 << ". " << files[i] << "\n";
    }
}

// Nhap ten file, cho phep xoa ky tu hoac huy bang ESC
std::string InputFileName() {
    const int MAX_INPUT = 15;
    std::string name = "";
    while (true) {
        char c = _getch();
		if (c == 13) { 
            break; 
        }
        else if (c == 27) { 
            return ""; 
        }
        else if (c == 8 && !name.empty()) {  
            name.pop_back();
            std::cout << "\b \b" << std::flush; 
        }
        else if (isalnum(c)) {
            if (name.length() < MAX_INPUT) {
                name += c;
                std::cout << c << std::flush;
            }
        }
    }
    return name;
}