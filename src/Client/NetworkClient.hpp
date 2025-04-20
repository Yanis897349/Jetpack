/**
 * @file NetworkClient.hpp
 * @brief Manages network communications between client and server.
 */

#pragma once

#include "../Shared/Protocol.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

#include "GameDisplay.hpp"

namespace Jetpack::Client {

/**
 * @class NetworkClient
 * @brief Handles network communication with the game server.
 *
 * NetworkClient manages the TCP socket connection to the server, sends player input,
 * receives game state updates, and coordinates with the display component.
 * It runs network operations in a separate thread to maintain UI responsiveness.
 */
class NetworkClient {
public:
  /**
   * @brief Constructs a NetworkClient instance.
   * @param serverPort Port number of the server (default: 8080).
   * @param serverAddress IP address of the server (default: "127.0.0.1").
   * @param debugMode When true, prints detailed network packet information.
   */
  explicit NetworkClient(int serverPort = 8080,
                         std::string serverAddress = "127.0.0.1",
                         bool debugMode = false);

  /**
   * @brief Destructor that ensures clean shutdown of network resources.
   *
   * Stops the network thread and closes the server socket.
   */
  ~NetworkClient();

  /**
   * @brief Deleted copy constructor.
   */
  NetworkClient(const NetworkClient &) = delete;

  /**
   * @brief Deleted copy assignment operator.
   */
  NetworkClient &operator=(const NetworkClient &) = delete;

  /**
   * @brief Deleted move constructor.
   */
  NetworkClient(NetworkClient &&) = delete;

  /**
   * @brief Deleted move assignment operator.
   */
  NetworkClient &operator=(NetworkClient &&) = delete;

  /**
   * @brief Establishes a connection to the game server.
   * @return True if connection successful, false otherwise.
   *
   * Creates a TCP socket, connects to the server, and sends a connection
   * request packet.
   */
  [[nodiscard]] bool connectToServer();

  /**
   * @brief Starts the game client.
   *
   * Initializes the display component, starts the network thread,
   * and runs the main game loop.
   */
  void start();

  /**
   * @brief Gets the player ID assigned by the server.
   * @return The local player's ID.
   */
  [[nodiscard]] int getLocalPlayerId() const;

private:
  /**
   * @brief Network thread function that handles incoming data.
   *
   * Continuously reads from the socket, processes incoming packets,
   * and sends player input to the server.
   */
  void networkLoop();

  /**
   * @brief Determines the size of a packet based on its type and header.
   * @param data Pointer to the packet data.
   * @param maxSize Maximum available bytes to read.
   * @return The expected size of the complete packet, or 0 if incomplete.
   */
  [[nodiscard]] size_t getPacketSize(const std::byte *data,
                                     size_t maxSize) const;

  /**
   * @brief Processes a complete packet received from the server.
   * @param data Pointer to the packet data.
   * @param length Length of the packet in bytes.
   */
  void processPacket(const std::byte *data, size_t length);

  /**
   * @brief Handles a connection response packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handleConnectResponse(const std::byte *data, size_t length);

  /**
   * @brief Handles a map data packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handleMapData(const std::byte *data, size_t length);

  /**
   * @brief Handles a game start packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handleGameStart(const std::byte *data, size_t length);

  /**
   * @brief Handles a game state update packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handleGameStateUpdate(const std::byte *data, size_t length);

  /**
   * @brief Handles a coin collected event packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handleCoinCollected(const std::byte *data, size_t length) const;

  /**
   * @brief Handles a player death event packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handlePlayerDeath(const std::byte *data, size_t length) const;

  /**
   * @brief Handles a game over event packet from the server.
   * @param data Packet data.
   * @param length Packet length.
   */
  void handleGameOver(const std::byte *data, size_t length) const;

  /**
   * @brief Sends the player's input state to the server.
   *
   * Queries the display component for the jetpack state and
   * sends it to the server.
   */
  void sendPlayerInput() const;

  int m_serverPort;
  std::string m_serverAddress;
  bool m_debugMode;
  int m_serverSocket{-1};
  int m_localPlayerId{-1};

  Shared::Protocol::GameMap m_map;
  std::vector<Shared::Protocol::Player> m_players;

  std::atomic<bool> m_running{true};
  std::thread m_networkThread;

  std::shared_ptr<GameDisplay> m_display;
};
} // namespace Jetpack::Client
