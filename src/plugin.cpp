#include "plugin.h"
#include "logger.h"
#include "discord_webhook.h"
#include "discord_bot.h"
#include "config.h"
#include "http_server.h"
#include "copyright_endpoint.h"
#include "blocked_key_store.h"
#include "json.h"
#include <filesystem>

#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <vector>
#include <windows.h>

using pjson::Json;

PalworldDiscordPlugin::PalworldDiscordPlugin() : initialized_(false), bridge_running_(false), poll_running_(false) {
}

PalworldDiscordPlugin::~PalworldDiscordPlugin() {
    bridge_running_ = false;
    poll_running_ = false;
    if (bridge_thread_.joinable()) {
        bridge_thread_.join();
    }
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
    if (initialized_) {
        Shutdown();
    }
}

bool PalworldDiscordPlugin::Initialize() {
    if (initialized_) {
        return true;
    }

    g_logger.Info("Initializing PalworldDiscordPlugin v3.2 (File Bridge mode)...");

    // Discord webhook (game -> Discord)
    webhook_ = std::make_unique<DiscordWebhook>(g_config.GetWebhookUrl());

    // Send startup notification to Discord
    if (webhook_ && !g_config.GetWebhookUrl().empty()) {
        bool de = (g_config.GetLanguage() == "de");
        webhook_->SendEmbed(
            de ? "Palworld Discord Bridge [Alpha]" : "Palworld Discord Bridge [Alpha]",
            de ? "Chat System nach Restart wieder Online" : "Chat System back online after restart",
            de ? "Palworld Discord Bridge" : "Palworld Discord Bridge"
        );
        g_logger.Info("Startup notification sent to Discord");
    }

    // Start file bridge reader thread (reads Lua-written events)
    bridge_running_ = true;
    bridge_thread_ = std::thread(&PalworldDiscordPlugin::BridgeThread, this);
    g_logger.Info("File bridge reader started");

    // Start Discord bot polling thread (reads Discord -> writes to file for Lua)
    if (g_config.IsDiscordToGameEnabled() && !g_config.GetBotToken().empty() && !g_config.GetChannelId().empty()) {
        discord_bot_ = std::make_unique<DiscordBot>(g_config.GetBotToken(), g_config.GetChannelId());
        poll_running_ = true;
        poll_thread_ = std::thread(&PalworldDiscordPlugin::PollThread, this);
        g_logger.Info("Discord bot polling started");
    } else {
        g_logger.Info("Discord bot polling disabled (set discord_to_game + bot_token + channel_id to enable)");
    }

    // Load blocked API key list
    g_blocked_keys.Load("blocked_keys.json");

    // Start HTTP server for Discord->Game messages and copyright checks
    http_server_ = std::make_unique<HttpServer>(g_config.GetHttpBind(), g_config.GetHttpPort());
    http_server_->RegisterHandler("POST", "/discord/message",
        [](const HttpRequest& req) { return PalworldDiscordPlugin::HandleDiscordMessage(req.body); });
    http_server_->RegisterHandler("POST", "/api/copyright-check", HandleCopyrightCheck);
    http_server_->RegisterHandler("POST", "/api/admin/block", HandleAdminBlock);
    http_server_->RegisterHandler("POST", "/api/admin/unblock", HandleAdminUnblock);
    if (!http_server_->Start()) {
        g_logger.Critical("Failed to start HTTP server");
        return false;
    }
    g_logger.Info("HTTP server started on " + g_config.GetHttpBind() + ":" + std::to_string(g_config.GetHttpPort()));

    initialized_ = true;
    g_logger.Info("PalworldDiscordPlugin initialized successfully");
    return true;
}

static std::string FindIncomingBridgeFile();

std::string PalworldDiscordPlugin::HandleDiscordMessage(const std::string& body) {
    if (body.empty()) {
        return "{\"status\":\"error\",\"message\":\"empty body\"}";
    }
    try {
        Json root = Json::parse(body);
        std::string author = root.get_string("author", "");
        std::string content = root.get_string("content", "");
        if (author.empty() || content.empty()) {
            return "{\"status\":\"error\",\"message\":\"missing author or content\"}";
        }

        std::string incoming_file = g_config.GetBridgeIncomingFile();
        if (incoming_file.empty()) {
            incoming_file = FindIncomingBridgeFile();
        }
        std::ofstream f(incoming_file, std::ios::app);
        if (f.is_open()) {
            f << "discord|" << author << "|" << content << "\n";
            f.close();
            g_logger.Info("Discord->Game: [" + author + "]: " + content);
            return "{\"status\":\"ok\",\"message\":\"Message sent to chat\"}";
        } else {
            g_logger.Warning("Cannot write incoming bridge file");
            return "{\"status\":\"error\",\"message\":\"cannot write bridge file\"}";
        }
    } catch (const std::exception& e) {
        g_logger.Error("Failed to parse Discord message JSON: " + std::string(e.what()));
        return "{\"status\":\"error\",\"message\":\"invalid json\"}";
    }
}

static std::string FindBridgeFile() {
    std::vector<std::string> candidates = {
        "PalDiscordBridge_out.txt",
        "ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_out.txt",
        "../Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_out.txt",
        "Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_out.txt",
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) {
            return c;
        }
    }
    return candidates[1]; // default expected path
}

void PalworldDiscordPlugin::BridgeThread() {
    g_logger.Info("BridgeThread started");
    std::string bridge_file = g_config.GetBridgeOutgoingFile();
    if (bridge_file.empty()) {
        bridge_file = FindBridgeFile();
    }
    g_logger.Info("Bridge file: " + bridge_file);

    while (bridge_running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (!std::filesystem::exists(bridge_file)) {
            continue;
        }

        // Read entire file
        std::ifstream f(bridge_file);
        if (!f.is_open()) continue;
        std::stringstream buffer;
        buffer << f.rdbuf();
        f.close();

        // Delete file after reading
        std::filesystem::remove(bridge_file);

        std::string content = buffer.str();
        if (content.empty()) continue;

        // Parse each line: event|player|msg
        std::istringstream lines(content);
        std::string line;
        while (std::getline(lines, line)) {
            if (line.empty()) continue;

            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos) continue;

            std::string event = line.substr(0, p1);
            std::string player = line.substr(p1 + 1, p2 - p1 - 1);
            std::string msg = line.substr(p2 + 1);

            g_logger.Info("Bridge event: " + event + " [" + player + "]");

            bool de = (g_config.GetLanguage() == "de");
            if (event == "chat" && g_config.SendChatEvents()) {
                OnGameChatMessage(player, msg);
            } else if (event == "join" && g_config.SendJoinEvents()) {
                SendEmbed(
                    de ? "Spieler beigetreten" : "Player Join",
                    player + (de ? " ist online gegangen" : " has joined the server"),
                    de ? "Palworld Discord Bridge" : "Palworld Discord Bridge"
                );
            } else if (event == "leave" && g_config.SendLeaveEvents()) {
                SendEmbed(
                    de ? "Spieler verlassen" : "Player Leave",
                    player + (de ? " ist offline gegangen" : " has left the server"),
                    de ? "Palworld Discord Bridge" : "Palworld Discord Bridge"
                );
            } else if (event == "death" && g_config.SendDeathEvents()) {
                SendEmbed(
                    de ? "Spieler gestorben" : "Player Death",
                    player + (de ? " ist gestorben" : " has died"),
                    de ? "Palworld Discord Bridge" : "Palworld Discord Bridge"
                );
            }
        }
    }
    g_logger.Info("BridgeThread exiting");
}

static std::string FindIncomingBridgeFile() {
    std::vector<std::string> candidates = {
        "PalDiscordBridge_in.txt",
        "ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_in.txt",
        "../Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_in.txt",
        "Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_in.txt",
    };
    for (const auto& c : candidates) {
        std::filesystem::path p(c);
        if (p.parent_path().empty() || std::filesystem::exists(p.parent_path())) {
            return c;
        }
    }
    return candidates[1];
}

void PalworldDiscordPlugin::PollThread() {
    g_logger.Info("PollThread started");
    std::string incoming_file = g_config.GetBridgeIncomingFile();
    if (incoming_file.empty()) {
        incoming_file = FindIncomingBridgeFile();
    }
    g_logger.Info("Incoming bridge file: " + incoming_file);

    // Wait a bit before first poll to let everything initialize
    std::this_thread::sleep_for(std::chrono::seconds(3));

    int interval = g_config.GetPollInterval();
    if (interval < 1) interval = 5;

    while (poll_running_) {
        if (discord_bot_) {
            auto msgs = discord_bot_->FetchMessages();
            if (!msgs.empty()) {
                g_logger.Info("PollThread: fetched " + std::to_string(msgs.size()) + " Discord messages");

                // Append to incoming bridge file for Lua to read
                std::ofstream f(incoming_file, std::ios::app);
                if (f.is_open()) {
                    for (const auto& m : msgs) {
                        f << "discord|" << m.first << "|" << m.second << "\n";
                        g_logger.Info("Discord->Game: [" + m.first + "]: " + m.second);
                    }
                    f.close();
                } else {
                    g_logger.Warning("PollThread: cannot write incoming bridge file");
                }
            }
        }

        // Sleep for poll interval
        for (int i = 0; i < interval * 2 && poll_running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    g_logger.Info("PollThread exiting");
}

void PalworldDiscordPlugin::OnGameChatMessage(const std::string& player_name, const std::string& message) {
    printf("[PalworldDiscordPlugin] OnGameChatMessage ENTER: [%s]: %s\n", player_name.c_str(), message.c_str());
    fflush(stdout);
    g_logger.Info("Game chat: " + player_name + ": " + message);
    if (webhook_ && !g_config.GetWebhookUrl().empty()) {
        printf("[PalworldDiscordPlugin] Webhook OK, calling SendChatMessage...\n");
        fflush(stdout);
        webhook_->SendChatMessage(player_name, message);
        printf("[PalworldDiscordPlugin] SendChatMessage returned OK\n");
        fflush(stdout);
    } else {
        printf("[PalworldDiscordPlugin] Webhook NOT available (webhook=%p, url='%s')\n",
            (void*)webhook_.get(), g_config.GetWebhookUrl().c_str());
        fflush(stdout);
    }
    printf("[PalworldDiscordPlugin] OnGameChatMessage EXIT\n");
    fflush(stdout);
}

void PalworldDiscordPlugin::SendEmbed(const std::string& title, const std::string& description, const std::string& footer) {
    g_logger.Debug("[Plugin] SendEmbed: " + title + " - " + description);
    g_logger.Info("Discord embed: " + title + " - " + description);
    if (webhook_ && !g_config.GetWebhookUrl().empty()) {
        g_logger.Debug("[Plugin] Calling webhook->SendEmbed...");
        webhook_->SendEmbed(title, description, footer);
        g_logger.Debug("[Plugin] SendEmbed returned");
    } else {
        g_logger.Warning("[Plugin] Webhook not available for embed");
    }
}

void PalworldDiscordPlugin::Shutdown() {
    if (!initialized_) {
        return;
    }

    g_logger.Info("Shutting down PalworldDiscordPlugin...");

    bridge_running_ = false;
    poll_running_ = false;
    if (bridge_thread_.joinable()) {
        bridge_thread_.join();
    }
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }

    if (http_server_) {
        http_server_->Stop();
        http_server_.reset();
    }

    webhook_.reset();
    discord_bot_.reset();

    initialized_ = false;
    g_logger.Info("PalworldDiscordPlugin shutdown complete");
}

bool PalworldDiscordPlugin::IsInitialized() const {
    return initialized_;
}

std::string PalworldDiscordPlugin::GetVersion() const {
    return VERSION;
}

std::string PalworldDiscordPlugin::GetName() const {
    return NAME;
}
