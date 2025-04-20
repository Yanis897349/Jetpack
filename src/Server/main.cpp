/**
 * @file main.cpp
 * @brief Entry point for the Jetpack server application.
 */

#include "Server.hpp"
#include <iostream>

static void usage(const char *program_name) {
  std::cerr << "Usage: " << program_name << "-p <port> -m <map> [-d]"
            << std::endl;
}

int main(const int argc, char *argv[]) {
  int port = 8080;
  std::string map_file;
  bool debug_mode = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-p" && i + 1 < argc) {
      port = std::stoi(argv[++i]);
    } else if (arg == "-m" && i + 1 < argc) {
      map_file = argv[++i];
    } else if (arg == "-d") {
      debug_mode = true;
    } else {
      usage(argv[0]);
      return 1;
    }
  }

  if (map_file.empty()) {
    std::cerr << "Error: Map file is required" << std::endl;
    usage(argv[0]);
    return 1;
  }
  if (port <= 0 || port > 65535) {
    std::cerr << "Error: Invalid port number" << std::endl;
    usage(argv[0]);
    return 1;
  }

  try {
    Jetpack::Server::GameServer server(port, map_file, debug_mode);
    server.start();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
