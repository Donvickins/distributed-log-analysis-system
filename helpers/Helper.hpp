#pragma once

#include "boost.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Define LOG macro
#define LOG(...) std::cout << __VA_ARGS__ << std::endl

// Function declarations
std::string programNameResolver(std::string programPath);
void fail(beast::error_code ec, char const* what);
beast::string_view mime_type(beast::string_view path);
std::string path_cat(beast::string_view base, beast::string_view path);
std::string sanitize_ip(const std::string& ip_address);
std::string get_timestamp_str();bool is_valid_content_type(const std::string& content_type);
std::string trim(const std::string& data);
bool is_log_level(const std::string& log_level);
std::string get_file(const fs::path& doc_root, const std::string& file_name, const std::string& file_ext);
bool has_ext(const std::string& filename, const std::string& ext);
std::map<std::string, std::string> save_file(const std::string& content, const std::string& client_id, const std::string& ip, const std::string& ext);
void print_response(const json& j);