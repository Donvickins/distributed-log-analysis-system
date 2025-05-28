# Create a library called "helper_classes" which includes the source files
add_library(helper_classes
    STATIC
    Session.cpp
    Listener.cpp
    ResponseHandler.hpp
    simdjson.cpp
)

# Make sure the compiler can find include files for the library
target_include_directories(helper_classes
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${VCPKG_INSTALLED_DIR}/include  # Add the vcpkg include directory
    ${Boost_INCLUDE_DIRS}  # Add Boost include directories
)

# Set additional properties for the target
set_target_properties(helper_classes PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add header files (they don't need to be in add_library, but good for IDE integration)
target_sources(helper_classes
    PRIVATE
    Session.hpp
    Listener.hpp
    ResponseHandler.hpp
    simdjson.h
)

# Link with required dependencies
target_link_libraries(helper_classes
    PUBLIC
    Boost::filesystem
    Boost::system
    Boost::json
)
