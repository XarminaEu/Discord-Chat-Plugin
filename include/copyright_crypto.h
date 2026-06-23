#pragma once

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Copyright / API-Key protection layer
// ---------------------------------------------------------------------------
// Strings are stored XOR-encrypted at rest and decrypted at runtime.  This is
// obfuscation, not absolute security: it raises the bar for casual tampering,
// but anyone with enough time and the binary can still reverse it.  The real
// protection comes from not distributing the private signing/key material.
// ---------------------------------------------------------------------------

class CopyrightCrypto {
public:
    // Expected values (runtime-decrypted).
    static std::string GetApiKey();
    static std::string GetApiKeyPrefix();
    static std::string GetCopyrightText();
    static std::string GetNonCommercialNotice();
    static std::string GetProductName();
    static std::string GetExternalCheckUrl();

    // Verification helpers.
    static bool VerifyApiKey(const std::string& key);
    static bool VerifyCopyright(const std::string& text);
    static bool VerifyProgramName(const std::string& program);

    // Decrypt an in-memory obfuscated byte array.
    static std::string Decrypt(const std::vector<unsigned char>& data);

private:
    // Compile-time generated encrypted blobs.  These are intentionally not
    // plain text in the source/binary.
    static const std::vector<unsigned char> kApiKeyPrefix;
    static const std::vector<unsigned char> kApiKey;
    static const std::vector<unsigned char> kCopyright;
    static const std::vector<unsigned char> kNonCommercial;
    static const std::vector<unsigned char> kProductName;
    static const std::vector<unsigned char> kExternalCheckUrl;
};

// Decrypt and print the copyright notice to the game console.
void PrintCopyrightNotice();

// Returns true if the supplied key and copyright text are valid.
bool ValidateCopyrightAndKey(const std::string& config_key, std::string& out_reason);
