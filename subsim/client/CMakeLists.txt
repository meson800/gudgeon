# Make the client executable
add_executable(subsim_client main.cpp ${COMMONSRC})

# Find the audio library
find_package(OpenAL)
# Audio support is only required for the client
target_link_libraries(subsim_client openal RakNetLibStatic)
