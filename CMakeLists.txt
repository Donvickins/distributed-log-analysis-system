cmake_minimum_required(VERSION 3.24)
project(log_analysis_system)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.90 CONFIG REQUIRED COMPONENTS system json)
find_package(pugixml CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)

add_executable(server server.cpp)
add_executable(client client.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

if(Boost_FOUND)
    target_link_libraries(server PRIVATE utils cli_parse)
    target_link_libraries(client PRIVATE utils cli_parse simdjson::simdjson Boost::system Boost::json)
else()
    message(FATAL_ERROR "Boost with system component not found. Beast and Asio are header-only and will be included automatically.")
endif()