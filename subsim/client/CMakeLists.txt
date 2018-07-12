# Make the client executable
add_executable(subsim_client main.cpp UI.cpp LobbyHandler.cpp ${COMMONSRC})

# Require C++11
set_property(TARGET subsim_client PROPERTY CXX_STANDARD 11)

# Find the audio library and SDL libraries
find_package(OpenAL)
find_package(SDL2)
find_package(SDL2GFX)

target_include_directories(subsim_client PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2GFX_INCLUDE_DIR})

# Audio support is only required for the client
target_link_libraries(subsim_client openal RakNetLibStatic SDL2 SDL2_gfx)
