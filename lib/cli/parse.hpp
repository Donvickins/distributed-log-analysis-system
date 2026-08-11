#pragma once

#include <optional>
#include <string>

struct ClientConfig {
  int port = 7654;
  std::string clientId{};
};

std::optional<ClientConfig> parse_cli_args_client(int, char**);
unsigned short parse_cli_args_server(int, char**);