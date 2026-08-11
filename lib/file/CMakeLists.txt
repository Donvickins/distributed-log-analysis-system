add_library(file_handler STATIC handler.cpp)

target_link_libraries(file_handler PUBLIC Boost::json Boost::system simdjson::simdjson pugixml::pugixml)