#include "config.h"
#include "logger.h"
#include "json.h"
#include <fstream>
#include <sstream>

using pjson::Json;

Config g_config;

Config::Config() : loaded_(false) {
    SetDefaults();
}

Config::~Config() {
}

bool Config::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        g_logger.Warning("Config file not found: " + filepath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    try {
        Json root = Json::parse(buffer.str());
        Json discord = root.get_child("discord");
        Json plugin = root.get_child("plugin");

        if (discord.is_object()) {
            webhook_url_ = discord.get_string("webhook_url", webhook_url_);
            bot_token_ = discord.get_string("bot_token", bot_token_);
            server_id_ = discord.get_string("server_id", server_id_);
            channel_id_ = discord.get_string("channel_id", channel_id_);
            bot_name_ = discord.get_string("bot_name", bot_name_);
            language_ = discord.get_string("language", language_);
            discord_to_game_ = discord.get_bool("discord_to_game", discord_to_game_);
            poll_interval_ = discord.get_int("poll_interval", poll_interval_);
        }

        if (plugin.is_object()) {
            debug_mode_ = plugin.get_bool("debug_mode", debug_mode_);
            log_file_ = plugin.get_string("log_file", log_file_);
            http_port_ = plugin.get_int("http_port", http_port_);
            http_bind_ = plugin.get_string("http_bind", http_bind_);
            max_message_length_ = plugin.get_int("max_message_length", max_message_length_);
            api_key_ = plugin.get_string("api_key", api_key_);
        }

        Json events = root.get_child("events");
        if (events.is_object()) {
            send_chat_ = events.get_bool("chat", send_chat_);
            send_join_ = events.get_bool("join", send_join_);
            send_leave_ = events.get_bool("leave", send_leave_);
            send_death_ = events.get_bool("death", send_death_);
        }

        Json ue4ss = root.get_child("ue4ss");
        if (ue4ss.is_object()) {
            chat_recv_function_ = ue4ss.get_string("chat_recv_function", chat_recv_function_);
            chat_message_param_ = ue4ss.get_string("chat_message_param", chat_message_param_);
            chat_sender_param_ = ue4ss.get_string("chat_sender_param", chat_sender_param_);
            chat_send_function_ = ue4ss.get_string("chat_send_function", chat_send_function_);
        }

        Json bridge = root.get_child("bridge");
        if (bridge.is_object()) {
            bridge_enabled_ = bridge.get_bool("enabled", bridge_enabled_);
            bridge_incoming_file_ = bridge.get_string("incoming_file", bridge_incoming_file_);
            bridge_outgoing_file_ = bridge.get_string("outgoing_file", bridge_outgoing_file_);
        }

        loaded_ = true;
        g_logger.Info("Config loaded from " + filepath);
        return true;
    } catch (const std::exception& e) {
        g_logger.Error("Failed to parse config: " + std::string(e.what()));
        SetDefaults();
        return false;
    }
}

bool Config::SaveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        g_logger.Error("Failed to open config file for writing: " + filepath);
        return false;
    }

    Json root = Json::MakeObject();
    Json discord = Json::MakeObject();
    discord["webhook_url"] = webhook_url_;
    discord["bot_token"] = bot_token_;
    discord["server_id"] = server_id_;
    discord["channel_id"] = channel_id_;
    discord["bot_name"] = bot_name_;
    discord["language"] = language_;
    discord["discord_to_game"] = discord_to_game_;
    discord["poll_interval"] = poll_interval_;
    root["discord"] = discord;

    Json plugin = Json::MakeObject();
    plugin["debug_mode"] = debug_mode_;
    plugin["log_file"] = log_file_;
    plugin["http_port"] = http_port_;
    plugin["http_bind"] = http_bind_;
    plugin["max_message_length"] = max_message_length_;
    plugin["api_key"] = api_key_;
    root["plugin"] = plugin;

    Json ue4ss = Json::MakeObject();
    ue4ss["chat_recv_function"] = chat_recv_function_;
    ue4ss["chat_message_param"] = chat_message_param_;
    ue4ss["chat_sender_param"] = chat_sender_param_;
    ue4ss["chat_send_function"] = chat_send_function_;
    root["ue4ss"] = ue4ss;

    Json events = Json::MakeObject();
    events["chat"] = send_chat_;
    events["join"] = send_join_;
    events["leave"] = send_leave_;
    events["death"] = send_death_;
    root["events"] = events;

    Json bridge = Json::MakeObject();
    bridge["enabled"] = bridge_enabled_;
    bridge["incoming_file"] = bridge_incoming_file_;
    bridge["outgoing_file"] = bridge_outgoing_file_;
    root["bridge"] = bridge;

    file << root.dump(2);
    g_logger.Info("Config saved to " + filepath);
    return true;
}

std::string Config::GetWebhookUrl() const {
    return webhook_url_;
}

std::string Config::GetBotToken() const {
    return bot_token_;
}

std::string Config::GetServerId() const {
    return server_id_;
}

std::string Config::GetChannelId() const {
    return channel_id_;
}

std::string Config::GetBotName() const {
    return bot_name_;
}

std::string Config::GetLanguage() const {
    return language_;
}

bool Config::IsDiscordToGameEnabled() const {
    return discord_to_game_;
}

int Config::GetPollInterval() const {
    return poll_interval_;
}

int Config::GetHttpPort() const {
    return http_port_;
}

std::string Config::GetHttpBind() const {
    return http_bind_;
}

bool Config::IsDebugMode() const {
    return debug_mode_;
}

std::string Config::GetLogFile() const {
    return log_file_;
}

int Config::GetMaxMessageLength() const {
    return max_message_length_;
}

std::string Config::GetApiKey() const {
    return api_key_;
}

std::string Config::GetChatRecvFunction() const {
    return chat_recv_function_;
}

std::string Config::GetChatMessageParam() const {
    return chat_message_param_;
}

std::string Config::GetChatSenderParam() const {
    return chat_sender_param_;
}

std::string Config::GetChatSendFunction() const {
    return chat_send_function_;
}

bool Config::IsBridgeEnabled() const {
    return bridge_enabled_;
}

bool Config::SendChatEvents() const {
    return send_chat_;
}

bool Config::SendJoinEvents() const {
    return send_join_;
}

bool Config::SendLeaveEvents() const {
    return send_leave_;
}

bool Config::SendDeathEvents() const {
    return send_death_;
}

bool Config::LoadedFromFile() const {
    return loaded_;
}

std::string Config::GetBridgeIncomingFile() const {
    return bridge_incoming_file_;
}

std::string Config::GetBridgeOutgoingFile() const {
    return bridge_outgoing_file_;
}

void Config::SetWebhookUrl(const std::string& url) {
    webhook_url_ = url;
}

void Config::SetHttpPort(int port) {
    http_port_ = port;
}

void Config::SetDebugMode(bool debug) {
    debug_mode_ = debug;
}

void Config::SetDefaults() {
    webhook_url_ = "";
    bot_token_ = "";
    server_id_ = "";
    channel_id_ = "";
    bot_name_ = "Server Chat";
    language_ = "en";
    http_port_ = 8765;
    http_bind_ = "127.0.0.1";
    debug_mode_ = false;
    log_file_ = "PalworldDiscordPlugin.log";
    max_message_length_ = 2000;
    api_key_ = "";
    chat_recv_function_ = "";
    chat_message_param_ = "";
    chat_sender_param_ = "";
    chat_send_function_ = "";
    bridge_enabled_ = true;
    bridge_incoming_file_ = "PalDiscordBridge_in.txt";
    bridge_outgoing_file_ = "PalDiscordBridge_out.txt";
    send_chat_ = true;
    send_join_ = true;
    send_leave_ = true;
    send_death_ = true;
    discord_to_game_ = false;
    poll_interval_ = 5;
}
