#pragma once
#include <windows.h>
#include <string>
#include <functional>

// Dynamischer UE4SS API Loader
// Laedt UE4SS-Funktionen zur Laufzeit, ohne SDK.
// Ermoeglicht einen echten C++ Mod mit nur einer DLL.

namespace ue4ss {

// Typedefs fuer UE4SS-API-Funktionen
typedef int (*RegisterHookFn)(const char* func_name, void* callback);
typedef void (*ExecuteInGameThreadFn)(void* callback);
typedef void* (*GetUObjectFn)(const char* class_name);
typedef const char* (*GetUObjectNameFn)(void* obj);

class DynamicLoader {
public:
    bool Initialize();
    bool IsAvailable() const { return loaded_; }

    // Mod-Registration
    bool RegisterAsMod();

    // Chat Hooking
    bool HookChatFunction(const char* hint);

    // Discord -> Game Injection
    bool InjectChatMessage(const std::string& author, const std::string& message);

    // Bridge I/O
    void WriteBridgeOut(const std::string& author, const std::string& message);
    void PollBridgeIn(std::function<void(const std::string&, const std::string&)> callback);

private:
    bool loaded_ = false;
    HMODULE ue4ss_dll_ = nullptr;

    // Geladene Funktionen
    RegisterHookFn register_hook_ = nullptr;
    ExecuteInGameThreadFn execute_in_game_thread_ = nullptr;
    GetUObjectFn find_first_of_ = nullptr;
    GetUObjectNameFn get_object_name_ = nullptr;

    // Chat-Hook Zustand
    void* chat_function_ = nullptr;
    void* send_function_ = nullptr;
    void* send_target_ = nullptr;

    bool FindUE4SS();
    bool LoadExports();
    void* FindChatFunction(const char* hint);
    void* FindSendFunction();
    void* FindSendTarget(void* func);
};

// Globale Instanz
extern DynamicLoader g_ue4ss;

} // namespace ue4ss
