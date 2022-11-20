# CMakeList.txt : CMake project for async-call-helper, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (async-call-helper  "async-call-helper.hpp" "main.cpp" "external_c_lib.h" "unsafe-service.h" "unsafe-service.cpp" "safe-service.h" "safe-service.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET async-call-helper PROPERTY CXX_STANDARD 17)
endif()

# TODO: Add tests and install targets if needed.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(async-call-helper PRIVATE Threads::Threads)