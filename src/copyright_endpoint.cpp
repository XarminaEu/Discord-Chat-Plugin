#include "copyright_endpoint.h"
#include "copyright_crypto.h"
#include "blocked_key_store.h"
#include "logger.h"
#include "json.h"

#include <sstream>

using pjson::Json;

static std::string MakeJson(bool allowed, const std::string& message, const std::string& status) {
    Json root = Json::MakeObject();
    root["status"] = status;
    root["allowed"] = allowed;
    root["message"] = message;
    return root.dump();
}

static bool ExtractMasterKey(const HttpRequest& request, std::string& out_key) {
    if (request.body.empty()) {
        return false;
    }
    try {
        Json root = Json::parse(request.body);
        out_key = root.get_string("api_key", "");
        return !out_key.empty();
    } catch (const std::exception&) {
        return false;
    }
}

std::string HandleCopyrightCheck(const HttpRequest& request) {
    std::string product = CopyrightCrypto::GetProductName();
    std::string expected_program = CopyrightCrypto::GetProductName();
    std::string expected_copyright = CopyrightCrypto::GetCopyrightText();
    std::string expected_key = CopyrightCrypto::GetApiKey();

    if (request.body.empty()) {
        g_logger.Warning("[/api/copyright-check] Empty body from " + request.client_ip);
        return MakeJson(false, "startet nicht", "error");
    }

    std::string api_key;
    std::string program;
    std::string copyright;
    try {
        Json root = Json::parse(request.body);
        api_key = root.get_string("api_key", "");
        program = root.get_string("program", "");
        copyright = root.get_string("copyright", "");
    } catch (const std::exception& e) {
        g_logger.Warning("[/api/copyright-check] Invalid JSON from " + request.client_ip + ": " + e.what());
        return MakeJson(false, "startet nicht", "error");
    }

    g_logger.Info("[/api/copyright-check] IP=" + request.client_ip +
                  " key=" + api_key + " program=" + program);

    if (g_blocked_keys.IsBlocked(api_key)) {
        g_logger.Warning("[/api/copyright-check] Blocked key used from " + request.client_ip);
        return "{\"reason\":\"API-Key gesperrt\"}";
    }

    if (api_key != expected_key) {
        g_logger.Warning("[/api/copyright-check] Invalid API key from " + request.client_ip);
        return MakeJson(false, "startet nicht", "error");
    }

    if (program != expected_program) {
        g_logger.Warning("[/api/copyright-check] Invalid program name from " + request.client_ip);
        return MakeJson(false, "startet nicht", "error");
    }

    if (copyright != expected_copyright) {
        g_logger.Warning("[/api/copyright-check] Invalid copyright text from " + request.client_ip);
        return MakeJson(false, "startet nicht", "error");
    }

    return MakeJson(true, "startet", "ok");
}

std::string HandleAdminBlock(const HttpRequest& request) {
    std::string master_key;
    if (!ExtractMasterKey(request, master_key)) {
        return "{\"status\":\"error\",\"message\":\"missing api_key\"}";
    }
    if (master_key != CopyrightCrypto::GetApiKey()) {
        g_logger.Warning("[/api/admin/block] Invalid master key from " + request.client_ip);
        return "{\"status\":\"error\",\"message\":\"unauthorized\"}";
    }

    std::string target_key;
    try {
        Json root = Json::parse(request.body);
        target_key = root.get_string("target_key", "");
    } catch (const std::exception&) {
        return "{\"status\":\"error\",\"message\":\"invalid json\"}";
    }

    if (target_key.empty()) {
        return "{\"status\":\"error\",\"message\":\"missing target_key\"}";
    }

    g_blocked_keys.Block(target_key);
    return "{\"status\":\"ok\",\"message\":\"key blocked\"}";
}

std::string HandleAdminUnblock(const HttpRequest& request) {
    std::string master_key;
    if (!ExtractMasterKey(request, master_key)) {
        return "{\"status\":\"error\",\"message\":\"missing api_key\"}";
    }
    if (master_key != CopyrightCrypto::GetApiKey()) {
        g_logger.Warning("[/api/admin/unblock] Invalid master key from " + request.client_ip);
        return "{\"status\":\"error\",\"message\":\"unauthorized\"}";
    }

    std::string target_key;
    try {
        Json root = Json::parse(request.body);
        target_key = root.get_string("target_key", "");
    } catch (const std::exception&) {
        return "{\"status\":\"error\",\"message\":\"invalid json\"}";
    }

    if (target_key.empty()) {
        return "{\"status\":\"error\",\"message\":\"missing target_key\"}";
    }

    g_blocked_keys.Unblock(target_key);
    return "{\"status\":\"ok\",\"message\":\"key unblocked\"}";
}
