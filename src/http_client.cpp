#include "http_client.h"

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

static std::wstring ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], len);
    return result;
}

bool HttpPostJson(const std::string& url, const std::string& json_body, std::string& out_response, std::string& out_error) {
    out_response.clear();
    out_error.clear();

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
        WINHTTP_ACCESS_TYPE_NO_PROXY,
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

    if (comp.nScheme == INTERNET_SCHEME_HTTPS) {
        DWORD ssl_flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA
            | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE
            | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
            | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &ssl_flags, sizeof(ssl_flags));
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

        // Read response body regardless of status code so the caller can
        // parse error messages such as {"reason": "API-Key gesperrt"}.
        DWORD available = 0;
        std::vector<char> buffer;
        do {
            available = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &available)) break;
            if (available == 0) break;
            size_t old_size = buffer.size();
            buffer.resize(old_size + available);
            DWORD read = 0;
            if (!WinHttpReadData(hRequest, &buffer[old_size], available, &read)) break;
            buffer.resize(old_size + read);
        } while (available > 0);
        out_response.assign(buffer.begin(), buffer.end());

        if (status_code >= 200 && status_code < 300) {
            success = true;
        } else {
            out_error = "HTTP status " + std::to_string(status_code) +
                        (out_response.empty() ? "" : ": " + out_response);
        }
    } else {
        out_error = "WinHttpSendRequest/ReceiveResponse failed (error " + std::to_string(GetLastError()) + ")";
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}
