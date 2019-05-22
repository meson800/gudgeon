# Make the client executable
add_executable(subsim_client main.cpp UI.cpp LobbyHandler.cpp SimulationMaster.cpp TacticalStation.cpp HelmStation.cpp VoiceHandler.cpp ${COMMONSRC})

# Require C++11
set_property(TARGET subsim_client PROPERTY CXX_STANDARD 11)

# Find the audio library and SDL libraries
find_package(OpenAL)
find_package(SDL2)
find_package(SDL2GFX)
find_package(SDL2TTF)

target_include_directories(subsim_client PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2GFX_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR})

# Copy over data directory
add_custom_command(TARGET subsim_client POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:subsim_client>/data)

# Audio support is only required for the client
target_link_libraries(subsim_client RakNetLibStatic SDL2 SDL2_gfx SDL2_ttf)
