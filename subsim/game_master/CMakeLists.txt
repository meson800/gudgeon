# Make the game master executable
add_executable(subsim_gm main.cpp LobbyHandler.cpp ${COMMONSRC})

# Require C++11
set_property(TARGET subsim_gm PROPERTY CXX_STANDARD 11)

target_link_libraries(subsim_gm RakNetLibStatic)
