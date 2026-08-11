#pragma once

#include <boost/json.hpp>
#include <filesystem>
#include <map>
#include <string>

std::string path_cat(std::string base, std::string_view path);
std::string get_file(const std::filesystem::path& doc_root,
    const std::string& file_name,
    const std::string& file_ext);
bool has_ext(const std::string& filename, const std::string& ext);
std::map<std::string, std::string> save_file(const std::string& content,
    const std::string& client_id,
    const std::string& ip,
    const std::string& ext);
void print_response(const boost::json::value& j);
bool init_server(unsigned short port);
std::optional<std::filesystem::path> setup_public_dir();
