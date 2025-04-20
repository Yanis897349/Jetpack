/**
 * @file Broadcaster.cpp
 * @brief Implements the Broadcaster class for client communication.
 */

#include "Broadcaster.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <sys/socket.h>

namespace Jetpack::Server {

Broadcaster::Broadcaster(
    std::unordered_map<int, Shared::Protocol::Player> &serverPlayersReference,
    const bool debugMode)
    : m_serverPlayersReference(serverPlayersReference), m_debugMode(debugMode) {
}

/**
 * @brief Sends a raw byte buffer to a client and optionally logs the result.
 * @param clientSocket The file descriptor of the client socket.
 * @param data         The bytes to send.
 */
void Broadcaster::sendToClient(const int clientSocket,
                               const std::vector<std::byte> &data) const {
  const ssize_t bytesSent = send(clientSocket, data.data(), data.size(), 0);
  const bool success = (bytesSent == static_cast<ssize_t>(data.size()));

  if (m_debugMode) {
    logDebugInfo(clientSocket, data, success);
  }
}

/**
 * @brief Broadcasts a raw byte buffer to all connected clients.
 * @param data The buffer to broadcast.
 */
void Broadcaster::broadcastToAll(const std::vector<std::byte> &data) const {
  for (const auto &[playerSocket, _] : m_serverPlayersReference) {
    sendToClient(playerSocket, data);
  }
}

/**
 * @brief Logs debug information for a send operation.
 * @param clientSocket The client socket descriptor.
 * @param data         The data buffer.
 * @param success      True if send succeeded.
 */
void Broadcaster::logDebugInfo(int clientSocket,
                               const std::vector<std::byte> &data,
                               const bool success) {
  if (success) {
    std::cout << std::format("Debug: Sent {} bytes to client {}: ", data.size(),
                             clientSocket);
  } else {
    std::cerr << std::format("Debug: Failed to send to client {}: ",
                             clientSocket);
  }
  for (size_t i = 0; i < data.size(); i++) {
    std::cout << std::format("{:02X} ", static_cast<unsigned char>(data[i]));
  }
  std::cout << std::endl;
}

/**
 * @brief Constructs and broadcasts a GAME_START packet.
 */
void Broadcaster::broadcastGameStart() const {
  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::GAME_START);
  packet.addByte(static_cast<uint8_t>(m_serverPlayersReference.size()));
  packet.addByte(static_cast<uint8_t>(0));
  broadcastToAll(packet.serialize());
}

/**
 * @brief Constructs and broadcasts a GAME_STATE_UPDATE packet containing
 *        each player's id, state, position, score, and jetpack status.
 */
void Broadcaster::broadcastGameState() const {
  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::GAME_STATE_UPDATE);

  packet.addByte(static_cast<uint8_t>(m_serverPlayersReference.size()));

  for (const auto &[_, player] : m_serverPlayersReference) {
    packet.addByte(static_cast<uint8_t>(player.getId()));
    packet.addByte(static_cast<uint8_t>(player.getState()));

    const int16_t xFixedPrecision =
        static_cast<int16_t>(player.getPosition().x * 100);
    const int16_t yFixedPrecision =
        static_cast<int16_t>(player.getPosition().y * 100);

    packet.addShort(static_cast<uint16_t>(xFixedPrecision));
    packet.addShort(static_cast<uint16_t>(yFixedPrecision));

    packet.addShort(static_cast<uint16_t>(player.getScore()));
    packet.addByte(static_cast<uint8_t>(player.isJetpacking() ? 1 : 0));
    packet.addByte(0);
  }

  broadcastToAll(packet.serialize());
}

/**
 * @brief Constructs and broadcasts a COIN_COLLECTED event.
 * @param playerId Identifier of the player who collected the coin.
 * @param x        X coordinate of the coin.
 * @param y        Y coordinate of the coin.
 * @param coinState New state of the coin after collection.
 */
void Broadcaster::broadcastCoinCollected(int playerId, int x, int y,
                                         int coinState) const {
  const auto playerIt = std::ranges::find_if(
      m_serverPlayersReference,
      [playerId](const auto &pair) { return pair.second.getId() == playerId; });

  int score = 0;
  if (playerIt != m_serverPlayersReference.end()) {
    score = playerIt->second.getScore();
  }

  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::COIN_COLLECTED);
  packet.addByte(static_cast<uint8_t>(playerId));
  packet.addByte(static_cast<uint8_t>(x));
  packet.addByte(static_cast<uint8_t>(y));
  packet.addByte(static_cast<uint8_t>(score));
  packet.addByte(static_cast<uint8_t>(coinState));

  broadcastToAll(packet.serialize());
}

/**
 * @brief Constructs and broadcasts a PLAYER_DEATH event.
 * @param playerId Identifier of the player who died.
 */
void Broadcaster::broadcastPlayerDeath(int playerId) const {
  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::PLAYER_DEATH);
  packet.addByte(static_cast<uint8_t>(playerId));
  broadcastToAll(packet.serialize());
}

/**
 * @brief Constructs and broadcasts a GAME_OVER event.
 * @param winnerId Identifier of the winning player, or negative if tie.
 */
void Broadcaster::broadcastGameOver(int winnerId) const {
  const bool hasWinner = (winnerId > 0);

  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::GAME_OVER);
  packet.addByte(static_cast<uint8_t>(hasWinner ? 1 : 0));
  packet.addByte(static_cast<uint8_t>(hasWinner ? winnerId : 0));

  broadcastToAll(packet.serialize());
}

} // namespace Jetpack::Server
