#include "http_client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

static std::wstring ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], len);
    return result;
}

static bool IsWine() {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    return ntdll && GetProcAddress(ntdll, "wine_get_version") != nullptr;
}

// Winsock-based plain HTTP POST - works reliably on Wine/Linux
static bool HttpPostJsonWinsock(const std::string& host, const std::string& path,
    const std::string& json_body, std::string& out_response, std::string& out_error) {

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        out_error = "WSAStartup failed";
        return false;
    }

    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0) {
        WSACleanup();
        out_error = "DNS lookup failed for " + host;
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(res);
        WSACleanup();
        out_error = "socket() failed";
        return false;
    }

    DWORD timeout_ms = 8000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));

    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        closesocket(sock);
        WSACleanup();
        out_error = "connect() failed to " + host + ":80";
        return false;
    }
    freeaddrinfo(res);

    std::ostringstream req;
    req << "POST " << path << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n"
        << "Content-Type: application/json\r\n"
        << "Content-Length: " << json_body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << json_body;
    std::string request = req.str();

    if (send(sock, request.c_str(), (int)request.size(), 0) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        out_error = "send() failed";
        return false;
    }

    std::string raw;
    char buf[4096];
    int received;
    while ((received = recv(sock, buf, sizeof(buf), 0)) > 0)
        raw.append(buf, received);

    closesocket(sock);
    WSACleanup();

    if (raw.empty()) {
        out_error = "empty response from server";
        return false;
    }

    // Parse status line
    size_t sp1 = raw.find(' ');
    size_t sp2 = raw.find(' ', sp1 + 1);
    int status = 0;
    if (sp1 != std::string::npos && sp2 != std::string::npos)
        status = std::stoi(raw.substr(sp1 + 1, sp2 - sp1 - 1));

    // Extract body (after \r\n\r\n)
    size_t body_pos = raw.find("\r\n\r\n");
    if (body_pos != std::string::npos)
        out_response = raw.substr(body_pos + 4);
    else
        out_response = raw;

    if (status >= 200 && status < 300)
        return true;

    out_error = "HTTP status " + std::to_string(status);
    return false;
}

bool HttpPostJson(const std::string& url, const std::string& json_body, std::string& out_response, std::string& out_error) {
    out_response.clear();
    out_error.clear();

    // On Wine: skip WinHTTP (HTTPS unreliable) and use Winsock HTTP directly
    if (IsWine()) {
        // Extract host and path from url for Winsock fallback
        std::string u = url;
        std::string scheme;
        if (u.substr(0, 8) == "https://") { scheme = "https"; u = u.substr(8); }
        else if (u.substr(0, 7) == "http://") { scheme = "http"; u = u.substr(7); }
        size_t slash = u.find('/');
        std::string host = (slash != std::string::npos) ? u.substr(0, slash) : u;
        std::string path = (slash != std::string::npos) ? u.substr(slash) : "/";
        return HttpPostJsonWinsock(host, path, json_body, out_response, out_error);
    }

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
