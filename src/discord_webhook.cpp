#include "discord_webhook.h"
#include "logger.h"
#include "json.h"
#include "config.h"
#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

using pjson::Json;

namespace {

std::wstring ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], len);
    return result;
}

// Performs an HTTPS POST with a JSON body. Returns true on 2xx response.
bool HttpPostJson(const std::string& url, const std::string& json_body, std::string& out_error) {
    std::wstring wurl = ToWide(url);

    URL_COMPONENTS comp = {};
    comp.dwStructSize = sizeof(comp);
    wchar_t host[256] = {};
    wchar_t path[1024] = {};
    comp.lpszHostName = host;
    comp.dwHostNameLength = (DWORD)(sizeof(host) / sizeof(wchar_t));
    comp.lpszUrlPath = path;
    comp.dwUrlPathLength = (DWORD)(sizeof(path) / sizeof(wchar_t));

    if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &comp)) {
        out_error = "WinHttpCrackUrl failed";
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"PalworldDiscordPlugin/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { out_error = "WinHttpOpen failed"; return false; }

    HINTERNET hConnect = WinHttpConnect(hSession, host, comp.nPort, 0);
    if (!hConnect) {
        out_error = "WinHttpConnect failed";
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = (comp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        out_error = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    const wchar_t* headers = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(hRequest, headers, (DWORD)-1L,
        (LPVOID)json_body.c_str(), (DWORD)json_body.size(),
        (DWORD)json_body.size(), 0);

    bool success = false;
    if (sent && WinHttpReceiveResponse(hRequest, nullptr)) {
        DWORD status_code = 0;
        DWORD size = sizeof(status_code);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX);
        if (status_code >= 200 && status_code < 300) {
            success = true;
        } else {
            out_error = "HTTP status " + std::to_string(status_code);
        }
    } else {
        out_error = "WinHttpSendRequest/ReceiveResponse failed (error " + std::to_string(GetLastError()) + ")";
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

} // namespace

DiscordWebhook::DiscordWebhook(const std::string& webhook_url) : webhook_url_(webhook_url) {
}

DiscordWebhook::~DiscordWebhook() {
}

bool DiscordWebhook::SendChatMessage(const std::string& player_name, const std::string& message) {
    printf("[PalworldDiscordPlugin] SendChatMessage ENTER: [%s]: %s\n", player_name.c_str(), message.c_str());
    fflush(stdout);
    if (!ValidateUrl()) {
        printf("[PalworldDiscordPlugin] SendChatMessage: Invalid URL\n");
        fflush(stdout);
        g_logger.Error("Invalid webhook URL");
        return false;
    }

    std::string bot_name = g_config.GetBotName();
    if (bot_name.empty()) bot_name = "Server Chat";

    bool de = (g_config.GetLanguage() == "de");
    std::string prefix = de ? "[Palworld] " : "[Palworld] ";

    Json payload = Json::MakeObject();
    payload["content"] = prefix + player_name + ": " + message;
    payload["username"] = bot_name;
    std::string json_str = payload.dump();
    printf("[PalworldDiscordPlugin] SendChatMessage JSON: %s\n", json_str.c_str());
    fflush(stdout);

    std::string error;
    bool ok = HttpPostJson(webhook_url_, json_str, error);
    if (!ok) {
        printf("[PalworldDiscordPlugin] SendChatMessage FAILED: %s\n", error.c_str());
        fflush(stdout);
        g_logger.Error("Failed to send Discord message: " + error);
        return false;
    }

    printf("[PalworldDiscordPlugin] SendChatMessage SUCCESS\n");
    fflush(stdout);
    g_logger.Debug("Discord message sent: " + player_name + ": " + message);
    return true;
}

bool DiscordWebhook::SendEmbed(const std::string& title, const std::string& description, const std::string& author) {
    if (!ValidateUrl()) {
        g_logger.Error("Invalid webhook URL");
        return false;
    }

    Json embed = Json::MakeObject();
    embed["title"] = title;
    embed["description"] = description;
    if (!author.empty()) {
        Json author_obj = Json::MakeObject();
        author_obj["name"] = author;
        embed["author"] = author_obj;
    }

    Json embeds = Json::MakeArray();
    embeds.push_back(embed);

    Json payload = Json::MakeObject();
    payload["embeds"] = embeds;

    std::string error;
    if (!HttpPostJson(webhook_url_, payload.dump(), error)) {
        g_logger.Error("Failed to send Discord embed: " + error);
        return false;
    }

    g_logger.Debug("Discord embed sent: " + title);
    return true;
}

void DiscordWebhook::SetWebhookUrl(const std::string& url) {
    webhook_url_ = url;
}

std::string DiscordWebhook::GetWebhookUrl() const {
    return webhook_url_;
}

bool DiscordWebhook::ValidateUrl() const {
    return !webhook_url_.empty() && webhook_url_.find("discord.com/api/webhooks/") != std::string::npos;
}

std::string DiscordWebhook::FormatChatMessage(const std::string& player_name, const std::string& message) const {
    return "[Palworld] " + player_name + ": " + message;
}
