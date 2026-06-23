#include "chat_hook.h"
#include "logger.h"

ChatHook::ChatHook() : initialized_(false) {
}

ChatHook::~ChatHook() {
    if (initialized_) {
        Shutdown();
    }
}

bool ChatHook::Initialize() {
    if (initialized_) {
        return true;
    }

    g_logger.Info("Initializing chat hook...");

    // TODO: Hook into Palworld chat system
    // This requires UE4SS SDK integration

    initialized_ = true;
    g_logger.Info("Chat hook initialized");
    return true;
}

void ChatHook::Shutdown() {
    if (!initialized_) {
        return;
    }

    g_logger.Info("Shutting down chat hook...");

    // TODO: Unhook from Palworld chat system

    initialized_ = false;
}

void ChatHook::SetChatCallback(ChatCallback callback) {
    chat_callback_ = callback;
}

void ChatHook::SetBroadcastSink(BroadcastSink sink) {
    broadcast_sink_ = sink;
}

void ChatHook::OnIncomingGameChat(const std::string& player_name, const std::string& message) {
    if (chat_callback_) {
        chat_callback_(player_name, message);
    }
}

bool ChatHook::BroadcastMessage(const std::string& author, const std::string& message) {
    if (!initialized_) {
        g_logger.Warning("Chat hook not initialized");
        return false;
    }

    g_logger.Debug("Broadcasting message: " + author + ": " + message);

    if (broadcast_sink_) {
        // The UE4SS layer performs the actual in-game chat injection.
        return broadcast_sink_(author, message);
    }

    // No game integration registered (e.g. standalone/test mode): log only.
    g_logger.Debug("No broadcast sink registered; message not injected into game chat");
    return true;
}
