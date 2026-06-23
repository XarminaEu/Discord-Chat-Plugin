#pragma once

#include <string>

class Config {
public:
    Config();
    ~Config();

    bool LoadFromFile(const std::string& filepath);
    bool SaveToFile(const std::string& filepath) const;

    // Discord settings
    std::string GetWebhookUrl() const;
    std::string GetBotToken() const;
    std::string GetServerId() const;
    std::string GetChannelId() const;
    std::string GetBotName() const;
    std::string GetLanguage() const;
    bool IsDiscordToGameEnabled() const;
    int GetPollInterval() const;

    // Plugin settings
    int GetHttpPort() const;
    std::string GetHttpBind() const;
    bool IsDebugMode() const;
    std::string GetLogFile() const;
    int GetMaxMessageLength() const;
    std::string GetApiKey() const;

    // UE4SS chat integration (function/property names discovered via Live View)
    std::string GetChatRecvFunction() const;   // UFunction name (substring) fired on incoming chat
    std::string GetChatMessageParam() const;    // property name holding the message text
    std::string GetChatSenderParam() const;     // property name holding the sender name
    std::string GetChatSendFunction() const;    // UFunction used to broadcast a chat message

    // Event toggles (what gets sent to Discord)
    bool SendChatEvents() const;
    bool SendJoinEvents() const;
    bool SendLeaveEvents() const;
    bool SendDeathEvents() const;

    // File-bridge IPC with the UE4SS Lua mod
    bool IsBridgeEnabled() const;
    bool LoadedFromFile() const;
    std::string GetBridgeIncomingFile() const;  // C++ writes, Lua reads (Discord -> game)
    std::string GetBridgeOutgoingFile() const;  // Lua writes, C++ reads (game -> Discord)

    // Setters
    void SetWebhookUrl(const std::string& url);
    void SetHttpPort(int port);
    void SetDebugMode(bool debug);

private:
    std::string webhook_url_;
    std::string bot_token_;
    std::string server_id_;
    std::string channel_id_;
    std::string bot_name_;
    std::string language_;
    int http_port_;
    std::string http_bind_;
    bool debug_mode_;
    std::string log_file_;
    int max_message_length_;
    std::string api_key_;
    std::string chat_recv_function_;
    std::string chat_message_param_;
    std::string chat_sender_param_;
    std::string chat_send_function_;
    bool bridge_enabled_;
    std::string bridge_incoming_file_;
    std::string bridge_outgoing_file_;
    bool loaded_;
    bool send_chat_;
    bool send_join_;
    bool send_leave_;
    bool send_death_;
    bool discord_to_game_;
    int poll_interval_;

    void SetDefaults();
};

extern Config g_config;
