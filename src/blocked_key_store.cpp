#include "blocked_key_store.h"
#include "json.h"
#include "logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>

using pjson::Json;

BlockedKeyStore g_blocked_keys;

BlockedKeyStore::BlockedKeyStore() {
}

void BlockedKeyStore::Load(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    filepath_ = filepath;
    blocked_keys_.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        // Create an empty blocklist file.
        Json root = Json::MakeObject();
        root["blocked_keys"] = Json::MakeArray();
        std::ofstream out(filepath);
        if (out.is_open()) {
            out << root.dump(2);
        }
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    try {
        Json root = Json::parse(buffer.str());
        Json arr = root.get_child("blocked_keys");
        if (arr.is_array()) {
            for (const auto& item : arr.items()) {
                std::string key = item.as_string();
                if (!key.empty()) {
                    blocked_keys_.push_back(key);
                }
            }
        }
        g_logger.Info("Loaded " + std::to_string(blocked_keys_.size()) + " blocked keys from " + filepath);
    } catch (const std::exception& e) {
        g_logger.Error("Failed to parse blocked_keys.json: " + std::string(e.what()));
    }
}

void BlockedKeyStore::Save(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    Json root = Json::MakeObject();
    Json arr = Json::MakeArray();
    for (const auto& key : blocked_keys_) {
        arr.push_back(key);
    }
    root["blocked_keys"] = arr;

    std::ofstream out(filepath.empty() ? filepath_ : filepath);
    if (out.is_open()) {
        out << root.dump(2);
    } else {
        g_logger.Error("Failed to write blocked_keys.json");
    }
}

bool BlockedKeyStore::IsBlocked(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Reload on every check so manual edits are picked up immediately.
    if (!filepath_.empty()) {
        const_cast<BlockedKeyStore*>(this)->Load(filepath_);
    }
    return std::find(blocked_keys_.begin(), blocked_keys_.end(), key) != blocked_keys_.end();
}

void BlockedKeyStore::Block(const std::string& key) {
    if (key.empty()) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (std::find(blocked_keys_.begin(), blocked_keys_.end(), key) == blocked_keys_.end()) {
            blocked_keys_.push_back(key);
        }
    }
    Save(filepath_);
    g_logger.Info("Blocked API key: " + key);
}

void BlockedKeyStore::Unblock(const std::string& key) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        blocked_keys_.erase(
            std::remove(blocked_keys_.begin(), blocked_keys_.end(), key),
            blocked_keys_.end()
        );
    }
    Save(filepath_);
    g_logger.Info("Unblocked API key: " + key);
}
