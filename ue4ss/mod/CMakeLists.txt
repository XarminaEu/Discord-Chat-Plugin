# PalworldDiscordPlugin - UE4SS C++ mod target.
# Built as part of the ue4ss/ superbuild (links the UE4SS CMake target).

set(TARGET PalworldDiscordPlugin)

# Our source tree lives two levels up (the main project root).
set(PDP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_library(${TARGET} SHARED
    ${PDP_ROOT}/src/main.cpp
    ${PDP_ROOT}/src/plugin.cpp
    ${PDP_ROOT}/src/chat_hook.cpp
    ${PDP_ROOT}/src/discord_webhook.cpp
    ${PDP_ROOT}/src/http_server.cpp
    ${PDP_ROOT}/src/config.cpp
    ${PDP_ROOT}/src/logger.cpp
    ${PDP_ROOT}/src/file_bridge.cpp
    ${PDP_ROOT}/src/ue4ss_mod.cpp
)

target_include_directories(${TARGET} PRIVATE ${PDP_ROOT}/include)

target_compile_definitions(${TARGET} PRIVATE
    PDP_ENABLE_UE4SS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

# UE4SS requires a modern C++ standard; match it.
target_compile_features(${TARGET} PRIVATE cxx_std_23)

# Link the UE4SS target (provides include dirs + Unreal API).
target_link_libraries(${TARGET} PUBLIC UE4SS)

# Our networking dependencies (Discord webhook + HTTP server).
target_link_libraries(${TARGET} PRIVATE winhttp ws2_32)

# Optional: auto-copy the built DLL into the game's mod directory.
# Adjust the path to your server, then uncomment.
# add_custom_command(TARGET ${TARGET} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}>
#         "C:/PalServer/Pal/Binaries/Win64/ue4ss/Mods/PalworldDiscordPlugin/dlls/main.dll")
