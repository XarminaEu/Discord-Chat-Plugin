// UE4SS C++ mod integration for PalworldDiscordPlugin.
//
// This file is ONLY compiled when the CMake option ENABLE_UE4SS is set (which
// defines PDP_ENABLE_UE4SS). It depends on the UE4SS C++ Modding SDK headers.
//
// It bridges the in-game Palworld chat with the existing plugin core:
//   * Game  -> Discord : a global ProcessEvent pre-hook detects the chat
//                        UFunction (name from config) and forwards the message
//                        to PalworldDiscordPlugin::OnGameChatMessage().
//   * Discord -> Game  : registers a broadcast sink that calls the configured
//                        chat UFunction to inject a message into the game.
//
// The exact UFunction / property names are NOT hard-coded. They are read from
// config.json (the "ue4ss" section) because they must be discovered for the
// specific Palworld build using the UE4SS Live View / dumpers. See
// UE4SS_INTEGRATION.md for the discovery procedure.

#ifdef PDP_ENABLE_UE4SS

#include "plugin.h"
#include "logger.h"
#include "config.h"

#include <windows.h>

#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/Property/FStrProperty.hpp>

#include <string>
#include <memory>

using namespace RC;
using namespace RC::Unreal;

namespace {

// Convert a UE4SS wide string (TCHAR*/std::wstring) to UTF-8 std::string.
std::string WideToUtf8(const wchar_t* wstr) {
    if (!wstr) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return std::string();
    std::string result(static_cast<size_t>(len - 1), 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len <= 0) return std::wstring();
    std::wstring result(static_cast<size_t>(len - 1), 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
    return result;
}

// Reads an FStrProperty value (by name) from a ProcessEvent parameter block.
// Returns empty string if the property is not found or not a string property.
std::string ReadStringParam(UFunction* function, void* parms, const std::wstring& prop_name) {
    if (prop_name.empty()) return std::string();

    for (FProperty* prop = function->GetFirstProperty(); prop; prop = prop->GetNextFieldAsProperty()) {
        if (prop->GetName() == prop_name) {
            if (auto* str_prop = CastField<FStrProperty>(prop)) {
                FString* value = str_prop->ContainerPtrToValuePtr<FString>(parms);
                if (value) {
                    return WideToUtf8(value->GetCharArray());
                }
            }
            break;
        }
    }
    return std::string();
}

bool NameContains(const std::wstring& haystack, const std::string& needle_utf8) {
    if (needle_utf8.empty()) return false;
    std::wstring needle = Utf8ToWide(needle_utf8);
    return haystack.find(needle) != std::wstring::npos;
}

class PalworldDiscordMod : public CppUserModBase {
public:
    PalworldDiscordMod() {
        ModName = STR("PalworldDiscordPlugin");
        ModVersion = STR("1.0.0");
        ModDescription = STR("Bidirectional Discord <-> Palworld chat bridge");
        ModAuthors = STR("PalworldDiscordPlugin");
    }

    ~PalworldDiscordMod() override = default;

    auto on_unreal_init() -> void override {
        Output::send<LogLevel::Verbose>(STR("[PalworldDiscordPlugin] UE4SS integration initializing...\n"));

        recv_function_ = g_config.GetChatRecvFunction();
        message_param_ = Utf8ToWide(g_config.GetChatMessageParam());
        sender_param_ = Utf8ToWide(g_config.GetChatSenderParam());
        send_function_ = g_config.GetChatSendFunction();

        if (recv_function_.empty()) {
            g_logger.Warning("ue4ss.chat_recv_function not configured; game->Discord chat capture disabled");
        }

        // Game -> Discord: global ProcessEvent pre-hook.
        Unreal::Hook::RegisterProcessEventPreCallback(
            [this](UObject* context, UFunction* function, void* parms) {
                this->OnProcessEvent(context, function, parms);
            });

        // Discord -> Game: register the broadcast sink into the plugin core.
        if (g_plugin) {
            g_plugin->SetBroadcastSink([this](const std::string& author, const std::string& message) {
                return this->BroadcastToGame(author, message);
            });
        }

        g_logger.Info("UE4SS integration initialized (recv='" + recv_function_ + "')");
    }

private:
    std::string recv_function_;
    std::wstring message_param_;
    std::wstring sender_param_;
    std::string send_function_;

    void OnProcessEvent(UObject* context, UFunction* function, void* parms) {
        if (recv_function_.empty() || !function) return;

        std::wstring fn_name = function->GetName();
        if (!NameContains(fn_name, recv_function_)) return;

        std::string message = ReadStringParam(function, parms, message_param_);
        if (message.empty()) return;

        std::string sender = ReadStringParam(function, parms, sender_param_);
        if (sender.empty() && context) {
            sender = WideToUtf8(context->GetName().c_str());
        }
        if (sender.empty()) sender = "Player";

        if (g_plugin) {
            g_plugin->OnGameChatMessage(sender, message);
        }
    }

    // Best-effort Discord -> game injection. The correct object + function
    // signature is game-specific; see UE4SS_INTEGRATION.md. Returns false if
    // not configured so the caller can log appropriately.
    bool BroadcastToGame(const std::string& author, const std::string& message) {
        if (send_function_.empty()) {
            g_logger.Debug("ue4ss.chat_send_function not configured; cannot inject into game chat");
            return false;
        }

        std::wstring fn_name = Utf8ToWide(send_function_);
        UFunction* function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, fn_name.c_str());
        if (!function) {
            g_logger.Warning("Chat send UFunction not found: " + send_function_);
            return false;
        }

        // Find an instance of the function's owner class to call it on.
        UClass* owner = function->GetOuterPrivate() ? static_cast<UClass*>(function->GetOuterPrivate()) : nullptr;
        if (!owner) {
            g_logger.Warning("Could not resolve owner class for send function");
            return false;
        }

        UObject* target = UObjectGlobals::FindFirstOf(owner->GetName());
        if (!target) {
            g_logger.Warning("No live instance found to broadcast chat through");
            return false;
        }

        // Build the parameter block: assumes a single FString message parameter.
        struct {
            FString Message;
        } params{ FString(Utf8ToWide("[Discord] " + author + ": " + message).c_str()) };

        target->ProcessEvent(function, &params);
        return true;
    }
};

} // namespace

#define PDP_API extern "C" __declspec(dllexport)

PDP_API RC::CppUserModBase* start_mod() {
    return new PalworldDiscordMod();
}

PDP_API void uninstall_mod(RC::CppUserModBase* mod) {
    delete mod;
}

#endif // PDP_ENABLE_UE4SS
