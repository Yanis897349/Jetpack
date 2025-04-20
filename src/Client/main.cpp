/**
 * @file main.cpp
 * @brief Entry point for the Jetpack client application.
 */

#include "NetworkClient.hpp"
#include <format>
#include <iostream>
#include <optional>
#include <string>

void printUsage(const std::string &programName) {
  std::cerr << std::format("Usage: {} -h <ip> -p <port> [-d]\n", programName);
}

struct ClientOptions {
  std::string serverIp = "127.0.0.1";
  int serverPort = 8080;
  bool debugMode = false;
};

std::optional<ClientOptions> parseCommandLine(const int argc, char *argv[]) {
  ClientOptions options;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" && i + 1 < argc) {
      options.serverIp = argv[++i];
    } else if (arg == "-p" && i + 1 < argc) {
      try {
        options.serverPort = std::stoi(argv[++i]);
        if (options.serverPort <= 0 || options.serverPort > 65535) {
          std::cerr << "Error: Port number must be between 1 and 65535\n";
          return std::nullopt;
        }
      } catch (const std::exception &e) {
        std::cerr << std::format("Error parsing port number: {}\n", e.what());
        return std::nullopt;
      }
    } else if (arg == "-d") {
      options.debugMode = true;
    } else {
      printUsage(argv[0]);
      return std::nullopt;
    }
  }

  return options;
}

int main(const int argc, char *argv[]) {
  try {
    auto options = parseCommandLine(argc, argv);
    if (!options) {
      return 1;
    }

    Jetpack::Client::NetworkClient client(
        options->serverPort, options->serverIp, options->debugMode);

    if (client.connectToServer()) {
      std::cout << std::format("Connected to server at {}:{}\n",
                               options->serverIp, options->serverPort);
      client.start();
    } else {
      std::cerr << "Failed to connect to server\n";
      return 1;
    }
  } catch (const std::exception &e) {
    std::cerr << std::format("Error: {}\n", e.what());
    return 1;
  }

  return 0;
}