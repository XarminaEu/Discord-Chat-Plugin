#pragma once

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

class DiscordWebhook;
class DiscordBot;

class PalworldDiscordPlugin {
public:
    PalworldDiscordPlugin();
    ~PalworldDiscordPlugin();

    bool Initialize();
    void Shutdown();
    bool IsInitialized() const;

    std::string GetVersion() const;
    std::string GetName() const;

    // Called when a player sends an in-game chat message (game -> Discord).
    void OnGameChatMessage(const std::string& player_name, const std::string& message);

    // Send a Discord embed (for join/leave/death events etc).
    void SendEmbed(const std::string& title, const std::string& description, const std::string& footer);

private:
    bool initialized_;
    std::atomic<bool> bridge_running_;
    std::atomic<bool> poll_running_;
    std::unique_ptr<DiscordWebhook> webhook_;
    std::unique_ptr<DiscordBot> discord_bot_;
    std::thread bridge_thread_;
    std::thread poll_thread_;

    void BridgeThread();
    void PollThread();

    static constexpr const char* VERSION = "4.0.2";
    static constexpr const char* NAME = "PalworldDiscordPlugin";
};

extern std::unique_ptr<PalworldDiscordPlugin> g_plugin;
