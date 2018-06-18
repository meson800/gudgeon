cmake_minimum_required (VERSION 2.6)
project (subsim)

# Set the current version number, save it into the version header
set (CMAKE_VERSION_MAJOR 0)
set (CMAKE_VERSION_MINOR 1)
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")

include_directories("${PROJECT_BINARY_DIR}")

# Make the game master executable
add_executable(subsim_gm game_master/main.cpp)

# Make the client executable
add_executable(subsim_client client/main.cpp)
