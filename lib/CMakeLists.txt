add_library(response_handler INTERFACE)
add_library(listener STATIC listener.cpp)
add_library(session STATIC session.cpp)
add_library(class_common INTERFACE)
add_library(utils STATIC utils.cpp http_handler.hpp file_handler.cpp)

target_include_directories(listener PUBLIC class_common)
target_include_directories(session PUBLIC class_common)
target_include_directories(response_handler INTERFACE class_common)
target_include_directories(class_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(class_common INTERFACE Boost::filesystem Boost::system Boost::json utils)
target_link_libraries(listener PUBLIC class_common session)
target_link_libraries(session PUBLIC class_common )
target_link_libraries(response_handler INTERFACE Boost::system Boost::json)

target_sources(utils PRIVATE utils.hpp http_handler.hpp file_handler.hpp)

target_include_directories(utils PUBLIC class_common)

target_link_libraries(
    utils PUBLIC 
    simdjson::simdjson
    pugixml::pugixml
    Boost::system
    Boost::filesystem
    Boost::json
    CLI11::CLI11
)