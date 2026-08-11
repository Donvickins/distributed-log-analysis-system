add_library(http_handler INTERFACE)
target_link_libraries(http_handler INTERFACE Boost::system Boost::json utils file_handler response_handler)
target_include_directories(http_handler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(response_handler INTERFACE)
target_link_libraries(response_handler INTERFACE Boost::system Boost::json)
target_include_directories(response_handler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})