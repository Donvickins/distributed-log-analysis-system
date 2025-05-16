#pragma once

// Windows-specific definitions
#ifdef _WIN32
    // Define Windows version for Boost.Asio - use Windows 10 as minimum target
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0A00  // Windows 10
    #endif
    
    // Disable deprecation warnings for standard library functions
    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS
    #endif
    
    // Disable warnings about dll-interface
    #pragma warning(disable: 4251)
    
    // Include Winsock before Boost to avoid conflicts
    #include <WinSock2.h>
    #include <Windows.h>
    #include <ws2tcpip.h>
    
    // Link with Winsock library
    #pragma comment(lib, "Ws2_32.lib")
#endif

// Standard includes
#include <map>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <utility>  // For std::pair

// Boost includes
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>

// Namespace definitions
namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace fs = boost::filesystem;  // Changed from 'using' to 'namespace'

