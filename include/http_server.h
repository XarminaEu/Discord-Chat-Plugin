#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <map>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::string client_ip;
    std::map<std::string, std::string> headers;
};

class HttpServer {
public:
    HttpServer(const std::string& bind_addr = "127.0.0.1", int port = 8765);
    ~HttpServer();

    bool Start();
    void Stop();
    bool IsRunning() const;

    // Handler receives the parsed request and returns the response body (JSON).
    using RequestHandler = std::function<std::string(const HttpRequest& request)>;

    // Register a handler for a specific HTTP method + path.
    void RegisterHandler(const std::string& method, const std::string& path, RequestHandler handler);

    // Legacy single-handler registration (applies to all requests).
    using LegacyHandler = std::function<std::string(const std::string& body)>;
    void SetMessageHandler(LegacyHandler handler);

private:
    std::string bind_addr_;
    int port_;
    bool running_;
    unsigned long long listen_socket_;
    std::unique_ptr<std::thread> server_thread_;
    std::map<std::string, RequestHandler> handlers_;
    LegacyHandler legacy_handler_;

    void RunServer();
    void HandleClient(unsigned long long client_socket);
    std::string ExtractBody(const std::string& request) const;
    bool ParseRequestLine(const std::string& request, std::string& method, std::string& path) const;
    std::string GetClientIp(unsigned long long client_socket) const;
    std::string FindHandlerKey(const std::string& method, const std::string& path) const;
};
