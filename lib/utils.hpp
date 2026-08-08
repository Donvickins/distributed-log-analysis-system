#pragma once

#include <boost/beast/core.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <map>
#include <string>

namespace beast = boost::beast;
namespace fs = boost::filesystem;

struct ClientConfig {
  int port = 7654;
  std::string clientId{};
};

std::string programNameResolver(std::string programPath);
void fail(beast::error_code ec, char const* what);
std::string_view mime_type(std::string_view path);
std::string path_cat(beast::string_view base, beast::string_view path);
std::string sanitize_ip(std::string ip_address);
std::string get_timestamp_str();
bool is_valid_content_type(const std::string& content_type);
std::string trim(const std::string& data);
bool is_log_level(const std::string& log_level);
std::string get_file(
    const fs::path& doc_root, const std::string& file_name, const std::string& file_ext);
bool has_ext(const std::string& filename, const std::string& ext);
std::map<std::string, std::string> save_file(const std::string& content,
    const std::string& client_id,
    const std::string& ip,
    const std::string& ext);
void print_response(const boost::json::value& j);
std::optional<ClientConfig> parse_cli_args_client(int, char**);
unsigned int parse_cli_args_server(int, char**);
