#include "parse.hpp"

#include <print>

#include "CLI/CLI.hpp"

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

  app.allow_non_standard_option_names();  // Without this CLI11 rejects single-dash multi-char names.
  app.add_option("-id,--client-id", config.clientId, "client identifier")
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

unsigned short parse_cli_args_server(int argc, char** argv) {
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

  return static_cast<unsigned short>(port);
};
