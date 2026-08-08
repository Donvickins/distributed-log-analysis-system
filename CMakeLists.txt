cmake_minimum_required(VERSION 3.24)
project(log_analysis_system)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.90 CONFIG REQUIRED COMPONENTS system filesystem json)
find_package(pugixml CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)

add_executable(server server.cpp)
add_executable(client client.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

if(Boost_FOUND)
    target_link_libraries(server PRIVATE utils listener)
    target_link_libraries(client PRIVATE utils Boost::system Boost::filesystem Boost::json)
else()
    message(FATAL_ERROR "Boost with system component not found. Beast and Asio are header-only and will be included automatically.")
endif()