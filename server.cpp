#include "lib/cli/parse.hpp"
#include "lib/utils/utils.hpp"

int main(int argc, char* argv[]) {
  unsigned short const port = parse_cli_args_server(argc, argv);

  if (!port) return 1;

  if (!init_server(port)) return 1;

  return 0;
}