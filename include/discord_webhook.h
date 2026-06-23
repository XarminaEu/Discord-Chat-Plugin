#pragma once

#include <string>
#include <memory>

class DiscordWebhook {
public:
    DiscordWebhook(const std::string& webhook_url);
    ~DiscordWebhook();

    bool SendChatMessage(const std::string& player_name, const std::string& message);
    bool SendEmbed(const std::string& title, const std::string& description, const std::string& author = "");

    void SetWebhookUrl(const std::string& url);
    std::string GetWebhookUrl() const;

private:
    std::string webhook_url_;
    bool ValidateUrl() const;
    std::string FormatChatMessage(const std::string& player_name, const std::string& message) const;
};
