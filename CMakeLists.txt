cmake_minimum_required(VERSION 3.16)
project(Server)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT environment variable must be set before running CMake.")
endif()

# Setup vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message(STATUS "Using vcpkg toolchain: $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    
    # Determine platform and architecture for vcpkg installed directory
    if(WIN32)
        set(VCPKG_PLATFORM "windows")
    elseif(APPLE)
        set(VCPKG_PLATFORM "osx")
    elseif(UNIX AND NOT APPLE)
        set(VCPKG_PLATFORM "linux")
    else()
        message(WARNING "Unknown platform, using 'linux' as default")
        set(VCPKG_PLATFORM "linux")
    endif()
    
    # Determine architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VCPKG_ARCH "x64")
    else()
        set(VCPKG_ARCH "x86")
    endif()
    
    # Handle ARM architecture special case
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|aarch64|AARCH64)")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(VCPKG_ARCH "arm64")
        else()
            set(VCPKG_ARCH "arm")
        endif()
    endif()
    
    # Construct the vcpkg installed directory path
    set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/${VCPKG_ARCH}-${VCPKG_PLATFORM}")
    
    # Set the prefix path for installed packages
    set(CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}" CACHE STRING "")
    message(STATUS "Setting CMAKE_PREFIX_PATH to: ${CMAKE_PREFIX_PATH}")
else()
    message(WARNING "VCPKG_ROOT environment variable not defined. Package finding may fail.")
endif()

# Find required packages
find_package(Boost REQUIRED COMPONENTS system filesystem json)
message(STATUS "Boost Found: ${Boost_FOUND}")
message(STATUS "Boost Include Directory: ${Boost_INCLUDE_DIR}")

find_package(pugixml CONFIG REQUIRED)

# Set helper directory
set(HELPER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/helpers)

# Add the executable
add_executable(Server Server.cpp)
add_executable(Client Client.cpp)

add_subdirectory("${HELPER_INCLUDE_DIR}")

target_include_directories(Server PRIVATE "${HELPER_INCLUDE_DIR}")
target_include_directories(Client PRIVATE "${HELPER_INCLUDE_DIR}")

# Setup Server target dependencies
if(Boost_FOUND)
    # Include Boost headers
    target_include_directories(Server PRIVATE "${Boost_INCLUDE_DIRS}")
    target_link_libraries(Server 
    PRIVATE 
    helper 
    Boost::system 
    Boost::filesystem
    Boost::json
    pugixml::static pugixml::pugixml
)

 target_include_directories(Client PRIVATE "${Boost_INCLUDE_DIRS}")
    target_link_libraries(Client 
    PRIVATE 
    helper 
    Boost::system 
    Boost::filesystem
    Boost::json
    pugixml::static pugixml::pugixml
)
else()
    message(FATAL_ERROR "Boost with system component not found. Beast and Asio are header-only and will be included automatically.")
endif()
