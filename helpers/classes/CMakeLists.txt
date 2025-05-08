# Create a library called "helper_classes" which includes the source files
add_library(helper_classes
    STATIC
    Session.cpp
    Listener.cpp
)

# Make sure the compiler can find include files for the library
target_include_directories(helper_classes
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Set additional properties for the target
set_target_properties(helper_classes PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add header files (they don't need to be in add_library, but good for IDE integration)
target_sources(helper_classes
    PRIVATE
    Session.h
    Listener.h
)
