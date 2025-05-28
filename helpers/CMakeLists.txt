# First add the subdirectory for classes
add_subdirectory(classes)

# Create the helper library
add_library(helper Helper.cpp Helper.hpp boost.hpp http_handler.hpp file_handler.hpp)

# Add include directories
target_include_directories(helper
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/classes
)

# Link helper with helper_classes and other dependencies
target_link_libraries(helper 
    PUBLIC 
    helper_classes
)
