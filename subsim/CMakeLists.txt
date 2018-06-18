cmake_minimum_required (VERSION 2.6)
project (subsim)

# Set the current version number, save it into the version header
set (CMAKE_VERSION_MAJOR 0)
set (CMAKE_VERSION_MINOR 1)
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")

include_directories("${PROJECT_BINARY_DIR}")

# Build RakNet
add_subdirectory(RakNet)

# Build the game master and client
add_subdirectory(game_master)
add_subdirectory(client)
