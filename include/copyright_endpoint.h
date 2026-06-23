#pragma once

#include "http_server.h"
#include <string>

// Handler for POST /api/copyright-check.
// Expected JSON body: { "api_key": "...", "program": "...", "copyright": "..." }
// Logs the caller IP and returns whether the key is allowed.
std::string HandleCopyrightCheck(const HttpRequest& request);

// Handler for POST /api/admin/block (requires master key).
// Expected JSON body: { "api_key": "..." }
std::string HandleAdminBlock(const HttpRequest& request);

// Handler for POST /api/admin/unblock (requires master key).
std::string HandleAdminUnblock(const HttpRequest& request);
