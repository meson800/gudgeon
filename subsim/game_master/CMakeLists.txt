# Make the game master executable
add_executable(subsim_gm main.cpp ${COMMONSRC})

target_link_libraries(subsim_gm RakNetLibStatic)
