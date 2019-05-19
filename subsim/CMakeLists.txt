cmake_minimum_required (VERSION 2.6)
project (subsim)
include(CTest)

# Set the current version number, save it into the version header
set (CMAKE_VERSION_MAJOR 0)
set (CMAKE_VERSION_MINOR 1)
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")

# Set the module search path so we can use custom find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules")

# Make sure the re-written version header can be included
include_directories("${PROJECT_BINARY_DIR}")
# Also make sure that we can find the RakNet header files
include_directories("${PROJECT_SOURCE_DIR}/RakNet/Source")
# And include the common headers
include_directories("${PROJECT_SOURCE_DIR}/common")

# Add documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file("${PROJECT_SOURCE_DIR}/Doxyfile.in" "${PROJECT_BINARY_DIR}/Doxyfile")
add_custom_target(doc COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/Doxyfile" WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
COMMENT "Generating documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

# Build RakNet
add_subdirectory(RakNet)

# Let CMake find the files inside the common folder
FILE(GLOB COMMONSRC common/*.cpp)

FILE(GLOB COMMONH common/*.h)

set(CMAKE_BUILD_TYPE Debug)
# Build the game master and client
add_subdirectory(game_master)

add_subdirectory(client)

if(BUILD_TESTING)
    # Build test package
    add_subdirectory(tests)
endif()
