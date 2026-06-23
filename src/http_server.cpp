#include "http_server.h"
#include "logger.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <sstream>
#include <cctype>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

static std::string ToLower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

HttpServer::HttpServer(const std::string& bind_addr, int port)
    : bind_addr_(bind_addr), port_(port), running_(false), listen_socket_(INVALID_SOCKET) {
}

HttpServer::~HttpServer() {
    if (running_) {
        Stop();
    }
}

bool HttpServer::Start() {
    if (running_) {
        return true;
    }

    g_logger.Info("Starting HTTP server on port " + std::to_string(port_));

    running_ = true;
    server_thread_ = std::make_unique<std::thread>(&HttpServer::RunServer, this);

    g_logger.Info("HTTP server started");
    return true;
}

void HttpServer::Stop() {
    if (!running_) {
        return;
    }

    g_logger.Info("Stopping HTTP server...");

    running_ = false;
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

    g_logger.Info("HTTP server stopped");
}

bool HttpServer::IsRunning() const {
    return running_;
}

void HttpServer::RegisterHandler(const std::string& method, const std::string& path, RequestHandler handler) {
    std::string key = ToLower(method) + " " + path;
    handlers_[key] = handler;
}

void HttpServer::SetMessageHandler(LegacyHandler handler) {
    legacy_handler_ = handler;
}

void HttpServer::RunServer() {
    g_logger.Debug("HTTP server thread started");

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        g_logger.Error("WSAStartup failed");
        running_ = false;
        return;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        g_logger.Error("Failed to create listen socket");
        WSACleanup();
        running_ = false;
        return;
    }
    listen_socket_ = listener;

    BOOL reuse = TRUE;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port_));
    if (inet_pton(AF_INET, bind_addr_.c_str(), &addr.sin_addr) != 1) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        g_logger.Error("Failed to bind to " + bind_addr_ + ":" + std::to_string(port_) +
                       " (error " + std::to_string(WSAGetLastError()) + ")");
        closesocket(listener);
        WSACleanup();
        running_ = false;
        return;
    }

    if (listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        g_logger.Error("Failed to listen on socket");
        closesocket(listener);
        WSACleanup();
        running_ = false;
        return;
    }

    g_logger.Info("HTTP server listening on " + bind_addr_ + ":" + std::to_string(port_));

    while (running_) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(listener, &read_set);
        timeval timeout = {};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int sel = select(0, &read_set, nullptr, nullptr, &timeout);
        if (sel == SOCKET_ERROR) {
            if (!running_) break;
            continue;
        }
        if (sel == 0) continue;

        SOCKET client = accept(listener, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            continue;
        }
        HandleClient(static_cast<unsigned long long>(client));
    }

    closesocket(listener);
    listen_socket_ = INVALID_SOCKET;
    WSACleanup();
    g_logger.Debug("HTTP server thread stopped");
}

void HttpServer::HandleClient(unsigned long long client_socket) {
    SOCKET client = static_cast<SOCKET>(client_socket);

    std::string request;
    char buffer[4096];
    int received;
    size_t content_length = 0;
    size_t header_end = std::string::npos;

    while ((received = recv(client, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, received);

        if (header_end == std::string::npos) {
            header_end = request.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                std::string headers_lower = request.substr(0, header_end);
                for (auto& ch : headers_lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                size_t cl_pos = headers_lower.find("content-length:");
                if (cl_pos != std::string::npos) {
                    size_t val_start = cl_pos + strlen("content-length:");
                    content_length = static_cast<size_t>(strtoul(headers_lower.c_str() + val_start, nullptr, 10));
                }
            }
        }

        if (header_end != std::string::npos) {
            size_t body_received = request.size() - (header_end + 4);
            if (body_received >= content_length) break;
        }
    }

    HttpRequest req;
    req.body = ExtractBody(request);
    req.client_ip = GetClientIp(client_socket);

    std::string response_body = "{\"status\":\"ok\"}";
    int status_code = 200;

    if (ParseRequestLine(request, req.method, req.path)) {
        g_logger.Debug("HTTP " + req.method + " " + req.path + " from " + req.client_ip);

        std::string key = FindHandlerKey(req.method, req.path);
        if (!key.empty() && handlers_.find(key) != handlers_.end()) {
            try {
                response_body = handlers_[key](req);
            } catch (const std::exception& e) {
                g_logger.Error("Request handler error for " + req.path + ": " + std::string(e.what()));
                response_body = "{\"status\":\"error\",\"message\":\"internal error\"}";
                status_code = 500;
            }
        } else if (legacy_handler_) {
            try {
                response_body = legacy_handler_(req.body);
            } catch (const std::exception& e) {
                g_logger.Error("Legacy request handler error: " + std::string(e.what()));
                response_body = "{\"status\":\"error\",\"message\":\"internal error\"}";
                status_code = 500;
            }
        } else {
            response_body = "{\"status\":\"error\",\"message\":\"not found\"}";
            status_code = 404;
        }
    } else {
        response_body = "{\"status\":\"error\",\"message\":\"bad request\"}";
        status_code = 400;
    }

    std::string status_text = (status_code == 200) ? "OK" : (status_code == 404 ? "Not Found" : "Error");
    std::string response =
        "HTTP/1.1 " + std::to_string(status_code) + " " + status_text + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(response_body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + response_body;

    send(client, response.c_str(), static_cast<int>(response.size()), 0);
    closesocket(client);
}

std::string HttpServer::ExtractBody(const std::string& request) const {
    size_t header_end = request.find("\r\n\r\n");
    if (header_end == std::string::npos) return "";
    return request.substr(header_end + 4);
}

bool HttpServer::ParseRequestLine(const std::string& request, std::string& method, std::string& path) const {
    size_t eol = request.find("\r\n");
    if (eol == std::string::npos) return false;
    std::string line = request.substr(0, eol);

    std::istringstream iss(line);
    iss >> method >> path;
    return !method.empty() && !path.empty();
}

std::string HttpServer::GetClientIp(unsigned long long client_socket) const {
    SOCKET client = static_cast<SOCKET>(client_socket);
    sockaddr_in addr = {};
    int addr_len = sizeof(addr);
    if (getpeername(client, reinterpret_cast<sockaddr*>(&addr), &addr_len) == 0) {
        char ip_str[INET_ADDRSTRLEN] = {};
        if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str))) {
            return std::string(ip_str);
        }
    }
    return "unknown";
}

std::string HttpServer::FindHandlerKey(const std::string& method, const std::string& path) const {
    std::string key = ToLower(method) + " " + path;
    auto it = handlers_.find(key);
    if (it != handlers_.end()) {
        return key;
    }
    return "";
}
