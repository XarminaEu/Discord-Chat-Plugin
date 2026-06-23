cmake_minimum_required(VERSION 3.20)
project(PalworldDiscordPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_compile_options(/W3)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
else()
    add_compile_options(-Wall -Wextra)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

# Source files
set(SOURCES
    src/main.cpp
    src/plugin.cpp
    src/discord_webhook.cpp
    src/discord_bot.cpp
    src/config.cpp
    src/logger.cpp
    src/install.cpp
    src/http_server.cpp
    src/copyright_crypto.cpp
    src/copyright_endpoint.cpp
    src/blocked_key_store.cpp
    src/chat_hook.cpp
    src/console_hook.cpp
    src/console_pipe.cpp
    src/console_reader.cpp
    src/file_bridge.cpp
    src/log_watcher.cpp
    src/ue4ss_mod.cpp
)

set(HEADERS
    include/plugin.h
    include/discord_webhook.h
    include/discord_bot.h
    include/config.h
    include/logger.h
    include/json.h
    include/install.h
    include/http_server.h
    include/copyright_crypto.h
    include/copyright_endpoint.h
    include/blocked_key_store.h
    include/chat_hook.h
    include/console_hook.h
    include/console_pipe.h
    include/console_reader.h
    include/file_bridge.h
    include/log_watcher.h
    include/ue4ss_dynamic.h
)

# NOTE: This CMakeLists builds the STANDALONE plugin (Discord webhook + HTTP
# server), which is fully testable without UE4SS.
#
# For the UE4SS C++ mod build (in-game chat hooking), use the dedicated
# superbuild in the `ue4ss/` folder instead. See UE4SS_INTEGRATION.md.

# Create DLL
add_library(PalworldDiscordPlugin SHARED ${SOURCES} ${HEADERS})

# Set DLL output name
set_target_properties(PalworldDiscordPlugin PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    OUTPUT_NAME "PalworldDiscordPlugin"
)

# External libraries
# libcurl
find_package(CURL)
if(CURL_FOUND)
    target_link_libraries(PalworldDiscordPlugin PRIVATE CURL::libcurl)
else()
    message(WARNING "libcurl not found - will use WinHTTP instead")
endif()

# nlohmann/json (header-only)
find_package(nlohmann_json)
if(nlohmann_json_FOUND)
    target_link_libraries(PalworldDiscordPlugin PRIVATE nlohmann_json::nlohmann_json)
else()
    message(WARNING "nlohmann_json not found - using bundled version")
endif()

# sqlite3
find_package(SQLite3)
if(SQLite3_FOUND)
    target_link_libraries(PalworldDiscordPlugin PRIVATE SQLite::SQLite3)
else()
    message(WARNING "SQLite3 not found - database features may be limited")
endif()

# Windows libraries
if(WIN32)
    target_link_libraries(PalworldDiscordPlugin PRIVATE
        ws2_32
        winhttp
        crypt32
    )
endif()

# Compiler flags for DLL
if(MSVC)
    target_compile_options(PalworldDiscordPlugin PRIVATE /EHsc /permissive-)
    set_target_properties(PalworldDiscordPlugin PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Install target
install(TARGETS PalworldDiscordPlugin
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Copy config file to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
    ${CMAKE_BINARY_DIR}/config/config.json
    COPYONLY
)
