#pragma once

#include <string>
#include <functional>

class ChatHook {
public:
    ChatHook();
    ~ChatHook();

    bool Initialize();
    void Shutdown();

    using ChatCallback = std::function<void(const std::string& player_name, const std::string& message)>;
    void SetChatCallback(ChatCallback callback);

    // Sink that actually injects a message into the game's chat.
    // Registered by the UE4SS integration layer. Returns true on success.
    using BroadcastSink = std::function<bool(const std::string& author, const std::string& message)>;
    void SetBroadcastSink(BroadcastSink sink);

    // Discord -> game: inject a message into the Palworld chat.
    bool BroadcastMessage(const std::string& author, const std::string& message);

    // Game -> Discord: call this from the UE4SS chat hook when a player
    // sends an in-game message. Invokes the registered ChatCallback.
    void OnIncomingGameChat(const std::string& player_name, const std::string& message);

private:
    ChatCallback chat_callback_;
    BroadcastSink broadcast_sink_;
    bool initialized_;
};
