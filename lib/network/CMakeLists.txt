add_library(listener STATIC listener.cpp)
add_library(session STATIC session.cpp)

target_link_libraries(listener PUBLIC session)
target_link_libraries(session PUBLIC Boost::system Boost::json file_handler)