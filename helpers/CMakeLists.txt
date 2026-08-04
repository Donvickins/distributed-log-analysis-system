# First add the subdirectory for classes
add_subdirectory(classes)

# Create the helper library
add_library(helper STATIC 
    helper.cpp 
    http_handler.hpp 
    file_handler.cpp
)

target_sources(helper PRIVATE
    helper.hpp 
    http_handler.hpp 
    file_handler.hpp 
)

# Add include directories
target_include_directories(helper
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/classes
)

target_link_libraries(
    helper PUBLIC 
    simdjson
    pugixml::pugixml
    Boost::system
    Boost::filesystem
    Boost::json
)