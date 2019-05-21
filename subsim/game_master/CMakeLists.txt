# Make the game master executable
add_executable(subsim_gm main.cpp LobbyHandler.cpp SimulationMaster.cpp Targeting.cpp ${COMMONSRC})

# Require C++11
set_property(TARGET subsim_gm PROPERTY CXX_STANDARD 11)

# Copy over data directory
add_custom_command(TARGET subsim_gm POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:subsim_gm>/data)

target_link_libraries(subsim_gm RakNetLibStatic)
