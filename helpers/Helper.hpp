#pragma once

#include "boost.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

// Define LOG macro
#define LOG(...) std::cout << __VA_ARGS__ << std::endl

// Function declarations
std::string programNameResolver(std::string programPath);
void fail(beast::error_code ec, char const* what);
beast::string_view mime_type(beast::string_view path);
std::string path_cat(beast::string_view base, beast::string_view path);
std::string sanitize_ip(const std::string& ip_address);
std::string get_timestamp_str();
std::string save_file(const std::string& clientId, const std::string& ip, const std::string& content, const std::string& extension);
bool is_valid_content_type(const std::string& content_type);
bool is_valid_log_level(const std::string& level);