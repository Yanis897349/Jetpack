/**
 * @file Broadcaster.hpp
 * @brief Declaration of the Broadcaster class responsible for sending
 *        network packets to clients in the Jetpack server.
 */

#pragma once

#include "../Shared/Protocol.hpp"
#include <sys/socket.h>
#include <unordered_map>
#include <vector>

namespace Jetpack::Server {

/**
 * @class Broadcaster
 * @brief Handles sending and broadcasting game‐related network packets
 *        to all connected clients.
 */
class Broadcaster {
public:
  /**
   * @brief Constructs a Broadcaster.
   * @param serverPlayersReference Reference to the map of client sockets
   *        to Player objects.
   * @param debugMode When true, logs raw packet bytes and send results
   *        to stdout/stderr.
   */
  Broadcaster(
      std::unordered_map<int, Shared::Protocol::Player> &serverPlayersReference,
      bool debugMode = false);

  /**
   * @brief Sends a raw byte buffer to a single client.
   * @param clientSocket The file descriptor of the client socket.
   * @param data The bytes to send.
   */
  void sendToClient(int clientSocket, const std::vector<std::byte> &data) const;

  /**
   * @brief Broadcasts a GAME_START packet to all clients.
   */
  void broadcastGameStart() const;

  /**
   * @brief Broadcasts the current game state (positions, scores, etc.)
   *        to all clients.
   */
  void broadcastGameState() const;

  /**
   * @brief Broadcasts a COIN_COLLECTED event to all clients.
   * @param playerId   Identifier of the player who collected the coin.
   * @param x          X coordinate of the coin in the game world.
   * @param y          Y coordinate of the coin in the game world.
   * @param coinState  New state/value of the coin after collection.
   */
  void broadcastCoinCollected(int playerId, int x, int y, int coinState) const;

  /**
   * @brief Broadcasts a PLAYER_DEATH event to all clients.
   * @param playerId Identifier of the player who died.
   */
  void broadcastPlayerDeath(int playerId) const;

  /**
   * @brief Broadcasts a GAME_OVER event to all clients.
   * @param winnerId Identifier of the winning player, or negative if no
   *        winner (tie).
   */
  void broadcastGameOver(int winnerId = -1) const;

private:
  /**
   * @brief Sends raw data to every connected client.
   * @param data The buffer to broadcast.
   */
  void broadcastToAll(const std::vector<std::byte> &data) const;

  /**
   * @brief Logs debug information about a send operation.
   * @param clientSocket The socket descriptor used.
   * @param data         The data that was (attempted) sent.
   * @param success      True if the send succeeded.
   */
  static void logDebugInfo(int clientSocket, const std::vector<std::byte> &data,
                           bool success);

  std::unordered_map<int, Shared::Protocol::Player> &m_serverPlayersReference;
  bool m_debugMode = false;
};

} // namespace Jetpack::Server
