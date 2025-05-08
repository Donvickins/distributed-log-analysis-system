cmake_minimum_required(VERSION 3.16)
project(Server)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add vcpkg toolchain if using vcpkg
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Find Boost (only system component is required, asio and beast are header-only)
find_package(Boost REQUIRED COMPONENTS system)

set(HELPER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/helpers)

# Add the executable
add_executable(Server Server.cpp)
add_subdirectory("${HELPER_INCLUDE_DIR}")
target_include_directories(Server PRIVATE "${HELPER_INCLUDE_DIR}")

add_executable(Client Client.cpp)
target_include_directories(Client PRIVATE "${HELPER_INCLUDE_DIR}")
target_link_libraries(Client PRIVATE helper Boost::system)

set(CMAKE_PREFIX_PATH "/home/sentient/vcpkg/installed/x64-linux")

if(Boost_FOUND)
# Include Boost headers
target_include_directories(Server PRIVATE "${Boost_INCLUDE_DIRS}")
target_link_libraries(Server PRIVATE helper Boost::system)

# Link Boost libraries
#target_link_libraries(Client PRIVATE helpers Boost::system)
message(STATUS "Boost Found: ${Boost_FOUND}")
message(STATUS "Boost Include Directory: ${Boost_INCLUDE_DIRS}")

else()
message(FATAL_ERROR "Boost with system component not found. Beast and Asio are header-only and will be included automatically.")
endif()
