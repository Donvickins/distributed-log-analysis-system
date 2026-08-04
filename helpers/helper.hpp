#pragma once

#include <map>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>

namespace beast = boost::beast;
namespace fs = boost::filesystem;

// Function declarations
std::string programNameResolver(std::string programPath);
void fail(beast::error_code ec, char const *what);
beast::string_view mime_type(beast::string_view path);
std::string path_cat(beast::string_view base, beast::string_view path);
std::string sanitize_ip(const std::string &ip_address);
std::string get_timestamp_str();
bool is_valid_content_type(const std::string &content_type);
std::string trim(const std::string &data);
bool is_log_level(const std::string &log_level);
std::string get_file(const fs::path &doc_root, const std::string &file_name,
                     const std::string &file_ext);
bool has_ext(const std::string &filename, const std::string &ext);
std::map<std::string, std::string> save_file(const std::string &content,
                                             const std::string &client_id,
                                             const std::string &ip,
                                             const std::string &ext);
void print_response(const boost::json::value &j);