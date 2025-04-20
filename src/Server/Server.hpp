/**
 * @file Server.hpp
 * @brief Declaration of the GameServer class, managing network I/O,
 *        game loop, and player state for Jetpack Server.
 */

#pragma once

#include "../Shared/Protocol.hpp"
#include "Broadcaster.hpp"
#include <filesystem>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <unordered_map>

namespace Jetpack::Server {

/**
 * @class GameServer
 * @brief Main server: accepts clients, drives game loop, and broadcasts
 * updates.
 */
class GameServer {
public:
  /**
   * @brief Creates and configures a GameServer.
   * @param port      TCP port to listen on.
   * @param mapFile   Path to the text map definition.
   * @param debugMode If true, logs raw packet data.
   * @throws Shared::Exceptions::MapLoaderException on map load failure.
   * @throws Shared::Exceptions::SocketException    on socket errors.
   */
  GameServer(int port, const std::string &mapFile, bool debugMode = false);

  /**
   * @brief Cleans up sockets and connected clients.
   */
  ~GameServer();

  /**
   * @brief Enters the main loop: polling, updating, and broadcasting.
   */
  void start();

private:
  static constexpr int MAX_CLIENTS = 2;
  static constexpr int MIN_PLAYERS = 2;
  static constexpr int GAME_TICK_MS = 16;
  static constexpr int BUFFER_SIZE = 1024;

  /**
   * @brief Loads map data from m_mapFile into m_map.
   * @return True on success, false on error/invalid map.
   */
  bool loadMap();

  /**
   * @brief Creates, configures, binds, and listens on server socket.
   * @throws Shared::Exceptions::SocketException on error.
   */
  void initializeSocket();

  /**
   * @brief Polls m_pollfds and dispatches events: new clients, data,
   * disconnects.
   */
  void handleSocketEvents();

  /** @brief Accepts a new incoming TCP connection. */
  void acceptNewClient();

  /**
   * @brief Reads available bytes from a client.
   * @param clientSocket The descriptor to read from.
   */
  void handleClientData(int clientSocket);

  /**
   * @brief Cleans up after a client hangs up or errors.
   * @param clientSocket The descriptor to remove.
   */
  void handleClientDisconnect(int clientSocket);

  /**
   * @brief Replies to CONNECT_REQUEST with assigned player ID and count.
   * @param clientSocket Descriptor to send on.
   * @param playerId     ID assigned to this client.
   */
  void sendConnectResponse(int clientSocket, int playerId) const;

  /**
   * @brief Sends the entire map layout and coin states to a client.
   * @param clientSocket Descriptor to send on.
   */
  void sendMapData(int clientSocket);

  /** @brief If enough players are connected, starts the game. */
  void checkGameStart();

  /** @brief Advances game logic: input, physics, collisions, broadcasts. */
  void updateGameState();

  /** @brief Applies physics and bounds to each playing player. */
  void updatePlayers();

  /** @brief Detects and handles coin pickups and electric‐square hits. */
  void checkCollisions();

  /** @brief Determines if game over conditions are met and broadcasts. */
  void checkGameEnd();

  /**
   * @brief Parses a raw packet buffer and dispatches by type.
   * @param clientSocket Client sending the data.
   * @param data         Byte buffer pointer.
   * @param length       Number of valid bytes.
   */
  void processPacket(int clientSocket, const uint8_t *data, size_t length);

  /**
   * @brief Handles PLAYER_INPUT packets (jetpack toggle).
   * @param clientSocket Sender descriptor.
   * @param data         Packet body.
   * @param length       Number of bytes.
   */
  void handlePlayerInput(int clientSocket, const uint8_t *data, size_t length);

  /** @brief Reset the game and reinitialize the map. */
  void resetGame();

private:
  int m_port;
  std::filesystem::path m_mapFile;
  bool m_debugMode;
  Shared::Protocol::GameMap m_map;
  int m_serverSocket = -1;
  std::vector<pollfd> m_pollfds;
  std::unordered_map<int, Shared::Protocol::Player> m_players;
  Broadcaster m_broadcaster;
  Shared::Protocol::GameState m_gameState =
      Shared::Protocol::GameState::WAITING_FOR_PLAYERS;
  bool m_running = true;
};

} // namespace Jetpack::Server
