#pragma once

#include <string>
#include <functional>

class ConsoleHook {
public:
    // Callback: type("chat"/"join"/"leave"/"death"), player, msg
    using ChatCallback = std::function<void(const std::string& type, const std::string& player, const std::string& msg)>;

    static bool Install(ChatCallback callback);
    static void Uninstall();
    static bool IsInstalled();

    static void HandleLine(const std::string& line);
    static ChatCallback s_callback;
    static bool s_installed;
};
