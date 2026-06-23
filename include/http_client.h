#pragma once

#include <string>

// Performs an HTTPS POST with a JSON body and returns the response body.
// Returns true on success (2xx status). On failure, out_error is set.
bool HttpPostJson(const std::string& url, const std::string& json_body, std::string& out_response, std::string& out_error);
