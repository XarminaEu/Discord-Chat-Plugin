#include "plugin.h"
#include "logger.h"
#include "config.h"
#include "install.h"
#include "copyright_crypto.h"

#include <windows.h>
#include <iostream>
#include <sstream>
#include <filesystem>

std::unique_ptr<PalworldDiscordPlugin> g_plugin;

void PrintToConsole(const std::string& message) {
    std::cout << message << std::endl;
}

static void InitializePlugin(HMODULE hModule);
static bool g_initialized = false;

// Bootstrap export for the UE4SS Lua mod. The Lua mod calls this once via
// `package.loadlib(dll_path, "StartBridge")()`, which loads this DLL into the
// game process. Under Wine/Proton, DllMain(DLL_PROCESS_ATTACH) may NOT be
// called, so we do all initialization here explicitly.
extern "C" __declspec(dllexport) int StartBridge(void* /*lua_State*/) {
    if (!g_initialized) {
        HMODULE hSelf = GetModuleHandleA("PalworldDiscordPlugin.dll");
        InitializePlugin(hSelf);
    }
    return 0;
}

static void InitializePlugin(HMODULE hModule) {
    if (g_initialized) return;
    g_initialized = true;

    std::string product = CopyrightCrypto::GetProductName();
    std::string copyright = CopyrightCrypto::GetCopyrightText();
    std::string notice = CopyrightCrypto::GetNonCommercialNotice();

    PrintToConsole("================================================================================");
    PrintToConsole("[" + product + "] Loading plugin...");
    PrintToConsole("[" + product + "] Version: 4.0.1 [Non-Commercial]");
    PrintToConsole("[" + product + "] " + copyright);
    PrintToConsole("[" + product + "] " + notice);
    PrintToConsole("================================================================================");

    // Spiel-Root ermitteln (Verzeichnis dieser DLL)
    char dll_path[MAX_PATH];
    GetModuleFileNameA(hModule, dll_path, MAX_PATH);
    std::filesystem::path dll_dir = std::filesystem::path(dll_path).parent_path();
    std::string game_root = dll_dir.string();

    // Selbstinstallation: fehlende Dateien erstellen
    SelfInstall(game_root);

    if (!g_config.LoadFromFile("config.json")) {
        PrintToConsole("[" + product + "] WARNING: config.json not found, using defaults");
    } else {
        PrintToConsole("[" + product + "] Config loaded successfully");
    }

    g_logger.Initialize(g_config.GetLogFile());
    g_logger.Info("================================================================================");
    g_logger.Info(product + " v4.0.1 - Loading (Non-Commercial)");
    g_logger.Info(copyright);
    g_logger.Info(notice);
    g_logger.Info("================================================================================");
    if (!g_config.LoadedFromFile()) {
        g_logger.Warning("Failed to load config.json, using defaults");
    } else {
        g_logger.Info("Config loaded from config.json");
    }

    // Validate hardcoded API key against config value.
    std::string validation_reason;
    if (!ValidateCopyrightAndKey(g_config.GetApiKey(), validation_reason)) {
        PrintToConsole("[" + product + "] ERROR: " + validation_reason);
        PrintToConsole("[" + product + "] Plugin startup aborted due to invalid or missing license key.");
        PrintToConsole("[" + product + "] This is a non-commercial version. The API key may not be modified.");
        PrintToConsole("================================================================================");
        g_logger.Critical(validation_reason);
        g_logger.Critical("Plugin startup aborted: invalid or missing API key");
        return;
    }
    PrintToConsole("[" + product + "] " + validation_reason);
    g_logger.Info(validation_reason);

    g_logger.SetDebugMode(g_config.IsDebugMode());
    if (g_config.IsDebugMode()) {
        PrintToConsole("[" + product + "] Debug mode ENABLED");
    }

    PrintToConsole("[" + product + "] Initializing core systems...");
    g_logger.Info("Initializing core systems...");

    g_plugin = std::make_unique<PalworldDiscordPlugin>();
    if (!g_plugin->Initialize()) {
        g_logger.Critical("Failed to initialize plugin");
        PrintToConsole("[" + product + "] ERROR: Failed to initialize plugin!");
        PrintToConsole("================================================================================");
        return;
    }

    PrintToConsole("[" + product + "] [OK] Plugin initialized successfully");
    PrintToConsole("[" + product + "] [OK] ConsoleReader: Active (reading console buffer)");
    PrintToConsole("[" + product + "] [OK] Discord Webhook: " + std::string(g_config.GetWebhookUrl().empty() ? "Not configured" : "Configured"));
    PrintToConsole("[" + product + "] [OK] HTTP Server: " + g_config.GetHttpBind() + ":" + std::to_string(g_config.GetHttpPort()));
    PrintToConsole("[" + product + "] [OK] Logging: " + g_config.GetLogFile());
    PrintToConsole("================================================================================");
    PrintToConsole("[" + product + "] Plugin loaded and ready!");
    PrintToConsole("================================================================================");

    g_logger.Info("Plugin loaded and ready!");
    g_logger.Info("ConsoleReader: Active (reading console buffer)");
    g_logger.Info("Discord Webhook: " + std::string(g_config.GetWebhookUrl().empty() ? "Not configured" : "Configured"));
    g_logger.Info("HTTP Server: " + g_config.GetHttpBind() + ":" + std::to_string(g_config.GetHttpPort()));
    g_logger.Info("================================================================================");
}

static void ShutdownPlugin() {
    if (!g_initialized) return;

    std::string product = CopyrightCrypto::GetProductName();

    PrintToConsole("================================================================================");
    PrintToConsole("[" + product + "] Unloading plugin...");
    g_logger.Info("================================================================================");
    g_logger.Info(product + " - Shutting down");

    if (g_plugin) {
        g_plugin->Shutdown();
        g_plugin.reset();
    }

    PrintToConsole("[" + product + "] [OK] Plugin unloaded");
    PrintToConsole("================================================================================");
    g_logger.Info("Plugin unloaded successfully");
    g_logger.Info("================================================================================");
    g_logger.Shutdown();
    g_initialized = false;
}

// ============================================================================
// Lua-callable exports (called from main.lua hooks)
// ============================================================================

extern "C" __declspec(dllexport) int SendChatToDiscord(const char* player, const char* message) {
    printf("[PalworldDiscordPlugin] SendChatToDiscord called: player='%s' msg='%s'\n", player ? player : "null", message ? message : "null");
    fflush(stdout);
    if (!g_plugin) {
        printf("[PalworldDiscordPlugin] ERROR: g_plugin is NULL\n");
        fflush(stdout);
        return 0;
    }
    if (!g_plugin->IsInitialized()) {
        printf("[PalworldDiscordPlugin] ERROR: g_plugin not initialized\n");
        fflush(stdout);
        return 0;
    }
    printf("[PalworldDiscordPlugin] g_plugin OK, calling OnGameChatMessage...\n");
    fflush(stdout);
    g_plugin->OnGameChatMessage(player ? player : "", message ? message : "");
    printf("[PalworldDiscordPlugin] OnGameChatMessage returned\n");
    fflush(stdout);
    return 1;
}

extern "C" __declspec(dllexport) int SendEmbedToDiscord(const char* title, const char* description, const char* footer) {
    printf("[PalworldDiscordPlugin] SendEmbedToDiscord called: title='%s'\n", title ? title : "null");
    fflush(stdout);
    if (!g_plugin) {
        printf("[PalworldDiscordPlugin] ERROR: g_plugin is NULL\n");
        fflush(stdout);
        return 0;
    }
    if (!g_plugin->IsInitialized()) {
        printf("[PalworldDiscordPlugin] ERROR: g_plugin not initialized\n");
        fflush(stdout);
        return 0;
    }
    g_plugin->SendEmbed(
        title ? title : "",
        description ? description : "",
        footer ? footer : "Palworld Discord Bridge"
    );
    return 1;
}

extern "C" __declspec(dllexport) int BroadcastToGameChat(const char* message) {
    // Placeholder - actual broadcast requires Lua-side engine call
    (void)message;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Under Wine/Proton, DllMain may not be called. Initialization is done
        // explicitly via StartBridge() from the Lua loader.
        break;

    case DLL_PROCESS_DETACH:
        ShutdownPlugin();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
