#pragma once

#include <string>
#include <vector>

class DiscordBot {
public:
    DiscordBot(const std::string& bot_token, const std::string& channel_id);
    ~DiscordBot();

    // Fetch latest messages from Discord channel.
    // Returns vector of (username, content) pairs.
    // Excludes messages from webhooks and already-seen messages.
    std::vector<std::pair<std::string, std::string>> FetchMessages();

private:
    std::string bot_token_;
    std::string channel_id_;
    std::string last_message_id_;

    bool HttpGet(const std::string& url, std::string& out_response, std::string& out_error);
};
