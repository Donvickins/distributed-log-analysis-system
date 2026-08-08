#include "utils.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <print>
#include <sstream>

#include "CLI/CLI.hpp"

// Returns a reasonable mime type based on the extension of a file
std::string_view mime_type(std::string_view path) {
  using beast::iequals;
  auto const pos = path.rfind('.');
  if (pos == std::string_view::npos) return "application/text";
  auto extension = path.substr(pos);

  static constexpr std::array mime_map = {
      std::pair{".htm", "text/html"},
      std::pair{".html", "text/html"},
      std::pair{".php", "text/html"},
      std::pair{".css", "text/css"},
      std::pair{".txt", "text/plain"},
      std::pair{".js", "application/javascript"},
      std::pair{".json", "application/json"},
      std::pair{".xml", "application/xml"},
      std::pair{".swf", "application/x-shockwave-flash"},
      std::pair{".flv", "video/x-flv"},
      std::pair{".png", "image/png"},
      std::pair{".jpe", "image/jpeg"},
      std::pair{".jpeg", "image/jpeg"},
      std::pair{".jpg", "image/jpeg"},
      std::pair{".gif", "image/gif"},
      std::pair{".bmp", "image/bmp"},
      std::pair{".ico", "image/vnd.microsoft.icon"},
      std::pair{".tiff", "image/tiff"},
      std::pair{".tif", "image/tiff"},
      std::pair{".svg", "image/svg+xml"},
      std::pair{".svgz", "image/svg+xml"},
  };

  for (auto const& [ext, mime] : mime_map) {
    if (iequals(ext, extension)) return mime;
  }
  return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path) {
  if (base.empty()) return std::string(path);
  std::string result(base);
#ifdef BOOST_MSVC
  char constexpr path_separator = '\\';
  if (result.back() == path_separator) result.resize(result.size() - 1);
  result.append(path.data(), path.size());
  for (auto& c : result)
    if (c == '/') c = path_separator;
#else
  char constexpr path_separator = '/';
  if (result.back() == path_separator) result.resize(result.size() - 1);
  result.append(path.data(), path.size());
#endif
  return result;
}

// Report a failure
void fail(beast::error_code ec, char const* what) { std::println("{}: {}", what, ec.message()); }

std::string sanitize_ip(std::string ip) {
  std::replace(ip.begin(), ip.end(), '.', '_');
  return ip;
}

std::string get_timestamp_str() {
  auto now = std::chrono::system_clock::now();
  auto itt = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&itt),
      "%Y-%m-%d_%H:%M:%S");  // Local time with normal date format
  return ss.str();
}

std::map<std::string, std::string> save_file(const std::string& content,
    const std::string& client_id,
    const std::string& ip,
    const std::string& ext) {
  std::string directory = "./storage/Client#" + client_id + "/";
  fs::path dir = directory;
  std::string construct_file_name = get_timestamp_str() + "_" + sanitize_ip(ip) + ext;
  fs::path file_name = construct_file_name;
  fs::path file_path = dir / file_name;

  if (!fs::exists(dir)) {
    if (!fs::create_directories(dir)) throw std::runtime_error("Failed to create directory");
  }

  std::fstream file;
  file.open(file_path, std::ios::out);

  if (!file.is_open()) {
    throw std::runtime_error("[ERROR] Failed to open file: " + file_path.string());
  }

  file << content;
  file.close();
  return std::map<std::string, std::string>{{"status", "success"},
      {"file_name", file_name.string()}};
}

bool is_valid_content_type(const std::string& content_type) {
  static constexpr std::array valid_types{"application/json", "application/xml", "text/plain"};
  return std::find(valid_types.begin(), valid_types.end(), content_type) != valid_types.end();
}

std::string trim(const std::string& data) {
  size_t first = data.find_first_not_of(" \t\n\r\f\v");
  size_t last = data.find_last_not_of(" \t\n\r\f\v");

  if (first == std::string::npos) return "";

  return data.substr(first, last - first + 1);
}

bool is_log_level(const std::string& log_level) {
  const std::vector<std::string> valid_log_level = {"INFO", "DEBUG", "WARN", "ERROR", "CRITICAL"};
  return std::find(valid_log_level.begin(), valid_log_level.end(), log_level) !=
         valid_log_level.end();
}

std::string get_file(const fs::path& doc_root,
    const std::string& file_name,
    const std::string& clientId,
    const std::string& file_ext) {
  if (doc_root.empty() || file_name.empty()) {
    std::println("[ERROR] Document root is empty");
    return "";
  }

  if (!fs::is_directory(doc_root)) {
    std::println("[ERROR] Document root is not a directory");
    return "";
  }

  if (!fs::exists(doc_root)) {
    std::println("[ERROR] Document root does not exist");
    return "";
  }

  std::string file_path =
      path_cat(doc_root.string(), "/Client#" + clientId + "/" + file_name + "." + file_ext);

  if (!fs::exists(file_path)) {
    std::println("[ERROR] File does not exist: {}", file_path);
    return "";
  }

  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::println("[ERROR] Failed to open file: {}", file_path);
    return "";
  }

  std::string file_content((std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());

  return file_content;
}

bool has_ext(const std::string& filename, const std::string& ext) {
  std::size_t idx = filename.rfind('.');
  if (idx != std::string::npos) {
    std::string extension = filename.substr(idx + 1);
    return extension == ext;
  }
  return false;
}

void print_response(const boost::json::value& j) {
  int log_level_len = 25, message_len = 35, times_occurred_len = 15;

  std::println("{:<{}} {:<{}} {:<{}}",
      "LOG LEVEL",
      log_level_len,
      "MESSAGE",
      message_len,
      "TIMES OCCURRED",
      times_occurred_len);

  std::println("{:-<{}}", "", log_level_len + message_len + times_occurred_len);

  for (const auto& [log_level, content] : j.as_object()) {
    std::println("{:<{}}", log_level, log_level_len);
    bool is_first = true;

    for (const auto& [key, value] : content.as_object()) {
      if (is_first) {
        std::println("{:<{}}", key, message_len);
        std::println("{:<{}}", value.as_string(), times_occurred_len);
        is_first = false;
      } else {
        std::println("{:<{}} {:<{}}",
            "",
            log_level_len,
            key,
            message_len,
            value.as_string(),
            times_occurred_len);
      }
    }

    std::println("{:-<{}}", "", log_level_len + message_len + times_occurred_len);
  }
};

std::optional<ClientConfig> parse_cli_args_client(int argc, char** argv) {
  ClientConfig config;
  bool port_provided = false;

  CLI::App app{"Distributed Log Analysis System Client"};

  app.add_option("-p,--port", config.port, "client port number. range (1024 to 65535)")
      ->each([&](std::string const&) { port_provided = true; })
      ->check([](std::string const& value) -> std::string {
        try {
          int port = std::stoi(value);
          if (port < 1024 || port > 65535) return "Port must be between 1024 and 65535";
        } catch (...) {
          return "Port must be a number";
        }

        return {};
      });

  app.add_option("-i,--client-id", config.clientId, "client identifier")
      ->check([](std::string const& value) -> std::string {
        if (value.empty()) return "client identifier is required";
        return "";
      });

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return std::nullopt;
  }

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& error) {
    app.exit(error);
  }

  if (!port_provided) {
    std::println("[INFO] No port provided, Using default port: {}", config.port);
  }
  return config;
};

unsigned int parse_cli_args_server(int argc, char** argv) {
  int port = 9000;

  CLI::App app{"Distributed Log Analysis System Server"};
  app.add_option("-p,--port", port, "server port number. range (1024 to 65535)")
      ->check([&port](std::string const& value) {
        try {
          port = std::stoi(value);
          if (port < 1024 || port > 65535) {
            return "port must be between 1024 and 65535";
          }
        } catch (...) {
          return "port must a number";
        }
        return "";
      });

  CLI11_PARSE(app, argc, argv);

  if (port == 9000) {
    std::println("[INFO] No port provided, Using default port: {}", port);
  }

  std::println("[INFO] Server running on port {}", port);

  return static_cast<unsigned int>(port);
};