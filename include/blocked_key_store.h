#pragma once

#include <string>
#include <vector>
#include <mutex>

// Maintains a list of permanently blocked API keys.  The list is loaded from
// blocked_keys.json at startup and reloaded on every check so external edits
// take effect immediately.
class BlockedKeyStore {
public:
    BlockedKeyStore();

    // Load the blocklist from disk (or create an empty file if missing).
    void Load(const std::string& filepath);

    // Save the current blocklist back to disk.
    void Save(const std::string& filepath);

    // Check whether a key is blocked.
    bool IsBlocked(const std::string& key) const;

    // Block a key permanently.
    void Block(const std::string& key);

    // Unblock a key.
    void Unblock(const std::string& key);

private:
    mutable std::mutex mutex_;
    std::vector<std::string> blocked_keys_;
    std::string filepath_;
};

extern BlockedKeyStore g_blocked_keys;
