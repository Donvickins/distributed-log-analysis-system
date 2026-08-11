add_library(utils STATIC utils.cpp)

target_link_libraries(utils PUBLIC Boost::json listener)