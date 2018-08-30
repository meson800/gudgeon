# Make the event test executable
add_executable(event_test EventTest.cpp ../common/EventSystem.cpp ../common/Log.cpp)

# Require C++11
set_property(TARGET event_test PROPERTY CXX_STANDARD 11)

# Set pthreads manually because this is the test code :/
find_package(Threads REQUIRED)
target_link_libraries(event_test Threads::Threads)
