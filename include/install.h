#pragma once
#include <string>

// Self-installation: when the DLL is dropped into Win64, it creates the
// UE4SS Lua mod files and a default config if they are missing.
bool SelfInstall(const std::string& game_root);
