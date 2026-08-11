add_library(cli_parse STATIC parse.cpp)

target_link_libraries(cli_parse PRIVATE CLI11::CLI11)