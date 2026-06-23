#include "discord_bot.h"
#include "logger.h"
#include "json.h"
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

} // namespace

DiscordBot::DiscordBot(const std::string& bot_token, const std::string& channel_id)
    : bot_token_(bot_token), channel_id_(channel_id) {
}

DiscordBot::~DiscordBot() {
}

bool DiscordBot::HttpGet(const std::string& url, std::string& out_response, std::string& out_error) {
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

    HINTERNET hSession = WinHttpOpen(L"PalworldDiscordPlugin/4.0",
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
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        out_error = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::string auth = "Authorization: Bot " + bot_token_ + "\r\n";
    std::wstring wauth = ToWide(auth);

    BOOL sent = WinHttpSendRequest(hRequest, wauth.c_str(), (DWORD)-1L,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    bool success = false;
    if (sent && WinHttpReceiveResponse(hRequest, nullptr)) {
        DWORD status_code = 0;
        DWORD size = sizeof(status_code);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX);

        if (status_code >= 200 && status_code < 300) {
            // Read response body
            std::string response;
            DWORD bytes_available = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0) {
                std::vector<char> buffer(bytes_available + 1);
                DWORD bytes_read = 0;
                if (WinHttpReadData(hRequest, buffer.data(), bytes_available, &bytes_read)) {
                    response.append(buffer.data(), bytes_read);
                }
            }
            out_response = response;
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

std::vector<std::pair<std::string, std::string>> DiscordBot::FetchMessages() {
    std::vector<std::pair<std::string, std::string>> result;

    if (bot_token_.empty() || channel_id_.empty()) {
        return result;
    }

    std::string url = "https://discord.com/api/v10/channels/" + channel_id_ + "/messages?limit=10";
    if (!last_message_id_.empty()) {
        url += "&after=" + last_message_id_;
    }

    std::string response;
    std::string error;
    if (!HttpGet(url, response, error)) {
        g_logger.Error("DiscordBot fetch failed: " + error);
        return result;
    }

    try {
        Json msgs = Json::parse(response);
        if (!msgs.is_array()) {
            return result;
        }

        std::string newest_id;
        for (const auto& item : msgs.items()) {
            Json msg = item;
            if (!msg.is_object()) continue;

            std::string id = msg.get_string("id", "");
            std::string content = msg.get_string("content", "");
            Json author = msg.get_child("author");
            std::string username = author.is_object() ? author.get_string("username", "") : "";
            bool is_bot = author.is_object() ? author.get_bool("bot", false) : false;

            if (!id.empty()) {
                newest_id = id;
            }

            // Skip webhook/bot messages and empty content
            if (is_bot || content.empty()) {
                continue;
            }

            result.push_back({username, content});
        }

        if (!newest_id.empty()) {
            last_message_id_ = newest_id;
        }
    } catch (const std::exception& e) {
        g_logger.Error("DiscordBot parse error: " + std::string(e.what()));
    }

    return result;
}
