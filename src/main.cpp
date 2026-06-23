#include "plugin.h"
#include "logger.h"
#include "config.h"
#include "install.h"
#include "copyright_crypto.h"
#include "http_client.h"
#include "json.h"

#include <windows.h>
#include <iostream>
#include <sstream>
#include <filesystem>

using pjson::Json;

std::unique_ptr<PalworldDiscordPlugin> g_plugin;

void PrintToConsole(const std::string& message) {
    std::cout << message << std::endl;
}

static void InitializePlugin(HMODULE hModule);
static bool g_initialized = false;

static bool PerformRemoteCopyrightCheck(std::string& out_reason) {
    std::string url = CopyrightCrypto::GetExternalCheckUrl();
    std::string api_key = CopyrightCrypto::GetApiKey();
    std::string program = CopyrightCrypto::GetProductName();
    std::string copyright = CopyrightCrypto::GetCopyrightText();
    std::string product = CopyrightCrypto::GetProductName();

    Json payload = Json::MakeObject();
    payload["api_key"] = api_key;
    payload["program"] = program;
    payload["copyright"] = copyright;
    std::string json_body = payload.dump();

    g_logger.Info("Performing remote copyright check at " + url);

    std::string response;
    std::string error;
    bool http_ok = HttpPostJson(url, json_body, response, error);

    g_logger.Info("Remote copyright check response: " + response);

    if (!http_ok && response.empty()) {
        out_reason = "[" + product + "] ERROR: Remote copyright check failed: " + error;
        return false;
    }

    try {
        Json root = Json::parse(response);
        if (root.contains("reason")) {
            std::string reason = root.get_string("reason", "");
            if (reason.find("gesperrt") != std::string::npos || reason.find("blocked") != std::string::npos) {
                out_reason = "[" + product + "] ERROR: " + reason;
                return false;
            }
        }
        if (root.get_string("status", "") == "ok" && root.get_bool("allowed", false)) {
            out_reason = "[" + product + "] Remote copyright check passed";
            return true;
        }
        std::string message = root.get_string("message", "unknown");
        out_reason = "[" + product + "] ERROR: Remote copyright check denied: " + message;
        return false;
    } catch (const std::exception& e) {
        if (!http_ok) {
            out_reason = "[" + product + "] ERROR: Remote copyright check failed: " + error;
        } else {
            out_reason = "[" + product + "] ERROR: Failed to parse remote response: " + std::string(e.what());
        }
        return false;
    }
}

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

    // Validate against external rl-dev.de copyright check service.
    // This can be skipped for local testing via the environment variable below.
    char skip_remote_check[16] = {};
    DWORD env_len = GetEnvironmentVariableA("PALDISCORDPLUGIN_SKIP_REMOTE_CHECK", skip_remote_check, sizeof(skip_remote_check));
    if (env_len > 0 && (skip_remote_check[0] == '1' || skip_remote_check[0] == 't' || skip_remote_check[0] == 'T')) {
        PrintToConsole("[" + product + "] WARNING: Remote copyright check skipped (test mode)");
        g_logger.Warning("Remote copyright check skipped (PALDISCORDPLUGIN_SKIP_REMOTE_CHECK is set)");
    } else {
        std::string remote_reason;
        if (!PerformRemoteCopyrightCheck(remote_reason)) {
            PrintToConsole("[" + product + "] ERROR: " + remote_reason);
            PrintToConsole("[" + product + "] Plugin startup aborted by remote copyright check.");
            PrintToConsole("================================================================================");
            g_logger.Critical(remote_reason);
            g_logger.Critical("Plugin startup aborted by remote copyright check");
            return;
        }
        PrintToConsole("[" + product + "] " + remote_reason);
        g_logger.Info(remote_reason);
    }

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
    PrintToConsole("[" + product + "] [OK] Logging: " + g_config.GetLogFile());
    PrintToConsole("================================================================================");
    PrintToConsole("[" + product + "] Plugin loaded and ready!");
    PrintToConsole("================================================================================");

    g_logger.Info("Plugin loaded and ready!");
    g_logger.Info("ConsoleReader: Active (reading console buffer)");
    g_logger.Info("Discord Webhook: " + std::string(g_config.GetWebhookUrl().empty() ? "Not configured" : "Configured"));
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
