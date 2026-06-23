#include "console_hook.h"
#include "logger.h"

#include <windows.h>
#include <cstring>

ConsoleHook::ChatCallback ConsoleHook::s_callback = nullptr;
bool ConsoleHook::s_installed = false;

namespace {
    unsigned char g_trampolineWriteFile[32] = {0};
    unsigned char g_trampolineWriteConsoleA[32] = {0};
}

// Thread-safe line buffer
static std::string& GetLineBuffer() {
    thread_local std::string buffer;
    return buffer;
}

void ConsoleHook::HandleLine(const std::string& line) {
    if (!s_callback) return;

    // === CHAT ===
    size_t chat = line.find("[Chat::Global]");
    if (chat == std::string::npos) chat = line.find("[Chat::");
    if (chat != std::string::npos) {
        size_t nameStart = line.find("['", chat);
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("']", nameStart);
            if (nameEnd != std::string::npos) {
                std::string player = line.substr(nameStart, nameEnd - nameStart);
                if (player.empty() || player == "''") {
                    size_t uid = line.find("UserId=", nameEnd);
                    if (uid != std::string::npos) {
                        uid += 7;
                        size_t uidEnd = line.find(",", uid);
                        if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
                        if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
                    }
                }
                size_t msgStart = line.find(":", nameEnd);
                if (msgStart != std::string::npos) {
                    std::string message = line.substr(msgStart + 2);
                    g_logger.Debug("ConsoleHook chat: [" + player + "]: " + message);
                    s_callback("chat", player, message);
                    return;
                }
            }
        }
    }

    // === JOIN ===
    if (line.find("connected to the server") != std::string::npos ||
        line.find("has logged in") != std::string::npos) {
        std::string player;
        size_t nameStart = line.find("['");
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("'", nameStart);
            if (nameEnd != std::string::npos) player = line.substr(nameStart, nameEnd - nameStart);
        }
        if (player.empty() || player == "''") {
            size_t uid = line.find("UserId=");
            if (uid != std::string::npos) {
                uid += 7;
                size_t uidEnd = line.find(",", uid);
                if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
                if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsoleHook join: " + player);
            s_callback("join", player, "");
        }
        return;
    }

    // === LEAVE ===
    if (line.find("disconnected") != std::string::npos ||
        line.find("logged out") != std::string::npos ||
        line.find("has left") != std::string::npos) {
        std::string player;
        size_t uid = line.find("UserId=");
        if (uid != std::string::npos) {
            uid += 7;
            size_t uidEnd = line.find(",", uid);
            if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
            if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
        }
        if (player.empty()) {
            size_t steam = line.find("steam_");
            if (steam != std::string::npos) {
                size_t end = line.find_first_of(" )'\"\t", steam);
                if (end == std::string::npos) end = line.length();
                player = line.substr(steam, end - steam);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsoleHook leave: " + player);
            s_callback("leave", player, "");
        }
        return;
    }

    // === DEATH ===
    if (line.find("died") != std::string::npos ||
        line.find("was killed") != std::string::npos) {
        std::string player;
        size_t nameStart = line.find("['");
        if (nameStart != std::string::npos) {
            nameStart += 2;
            size_t nameEnd = line.find("'", nameStart);
            if (nameEnd != std::string::npos) player = line.substr(nameStart, nameEnd - nameStart);
        }
        if (player.empty()) {
            size_t uid = line.find("UserId=");
            if (uid != std::string::npos) {
                uid += 7;
                size_t uidEnd = line.find(",", uid);
                if (uidEnd == std::string::npos) uidEnd = line.find(")", uid);
                if (uidEnd != std::string::npos) player = line.substr(uid, uidEnd - uid);
            }
        }
        if (!player.empty()) {
            g_logger.Debug("ConsoleHook death: " + player);
            s_callback("death", player, "");
        }
        return;
    }
}

static void ParseAndForward(const char* data, size_t len) {
    if (!ConsoleHook::s_callback) return;

    std::string& buffer = GetLineBuffer();
    buffer.append(data, len);

    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            ConsoleHook::HandleLine(line);
        }
    }
}

BOOL WINAPI HookedWriteFile(
    HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {

    DWORD ft = GetFileType(hFile);
    if (ft == FILE_TYPE_CHAR && lpBuffer && nNumberOfBytesToWrite > 0) {
        ParseAndForward(static_cast<const char*>(lpBuffer), nNumberOfBytesToWrite);
    }

    typedef BOOL (WINAPI *pfn)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
    return reinterpret_cast<pfn>(&g_trampolineWriteFile[0])(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI HookedWriteConsoleA(
    HANDLE hConsoleOutput, const VOID* lpBuffer, DWORD nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved) {

    if (lpBuffer && nNumberOfCharsToWrite > 0) {
        ParseAndForward(static_cast<const char*>(lpBuffer), nNumberOfCharsToWrite);
    }

    typedef BOOL (WINAPI *pfn)(HANDLE, const VOID*, DWORD, LPDWORD, LPVOID);
    return reinterpret_cast<pfn>(&g_trampolineWriteConsoleA[0])(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
}

// 5-byte relative JMP hook (x64 compatible within ±2GB)
static bool PlaceRelJmp(void* target, void* dest) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    intptr_t rel = (intptr_t)dest - ((intptr_t)target + 5);
    unsigned char* p = (unsigned char*)target;
    p[0] = 0xE9; // rel JMP
    *(int32_t*)(p + 1) = (int32_t)rel;

    VirtualProtect(target, 5, oldProtect, &oldProtect);
    return true;
}

static bool SetupHook(void* target, void* hook, unsigned char* trampoline) {
    // Trampoline layout:
    // [0..4]  original 5 bytes from target
    // [5]     rel JMP back to target+5
    // [6..9]  32-bit relative offset

    // Read original 5 bytes
    memcpy(trampoline, target, 5);

    // Place relative JMP back to target+5
    unsigned char* jmpBack = trampoline + 5;
    jmpBack[0] = 0xE9;
    intptr_t relBack = ((intptr_t)target + 5) - ((intptr_t)jmpBack + 5);
    *(int32_t*)(jmpBack + 1) = (int32_t)relBack;

    // Place hook JMP at target
    return PlaceRelJmp(target, hook);
}

bool ConsoleHook::Install(ChatCallback callback) {
    if (s_installed) return true;

    s_callback = callback;

    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    if (!hKernel) return false;

    void* pWriteFile = GetProcAddress(hKernel, "WriteFile");
    void* pWriteConsoleA = GetProcAddress(hKernel, "WriteConsoleA");

    if (!pWriteFile || !pWriteConsoleA) return false;

    DWORD oldProtect;
    VirtualProtect(g_trampolineWriteFile, sizeof(g_trampolineWriteFile), PAGE_EXECUTE_READWRITE, &oldProtect);
    VirtualProtect(g_trampolineWriteConsoleA, sizeof(g_trampolineWriteConsoleA), PAGE_EXECUTE_READWRITE, &oldProtect);

    if (!SetupHook(pWriteFile, HookedWriteFile, g_trampolineWriteFile)) return false;
    if (!SetupHook(pWriteConsoleA, HookedWriteConsoleA, g_trampolineWriteConsoleA)) return false;

    s_installed = true;
    g_logger.Info("ConsoleHook installed (WriteFile + WriteConsoleA) — works without console handle");
    return true;
}

void ConsoleHook::Uninstall() {
    if (!s_installed) return;
    s_installed = false;
}

bool ConsoleHook::IsInstalled() {
    return s_installed;
}
