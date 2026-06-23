# UE4SS C++ mod superbuild for PalworldDiscordPlugin.
#
# Layout (you create RE-UE4SS by cloning, see UE4SS_INTEGRATION.md):
#   ue4ss/
#   |- CMakeLists.txt        <- this file
#   |- RE-UE4SS/             <- git clone of https://github.com/UE4SS-RE/RE-UE4SS (recursive)
#   |- mod/CMakeLists.txt    <- our mod target (links UE4SS, compiles ../../src)
#
# Configure & build (from the ue4ss/ folder):
#   cmake -B build -G "Visual Studio 17 2022"
#   cmake --build build --config Game__Shipping__Win64

cmake_minimum_required(VERSION 3.22)
project(PalworldDiscordMods)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/RE-UE4SS/CMakeLists.txt")
    message(FATAL_ERROR
        "RE-UE4SS not found. Clone it first:\n"
        "  cd ${CMAKE_CURRENT_SOURCE_DIR}\n"
        "  git clone https://github.com/UE4SS-RE/RE-UE4SS\n"
        "  cd RE-UE4SS && git submodule update --init --recursive\n"
        "See UE4SS_INTEGRATION.md for prerequisites (Unreal Engine source access).")
endif()

add_subdirectory(RE-UE4SS)
add_subdirectory(mod)
