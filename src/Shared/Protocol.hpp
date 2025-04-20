/**
 * @file Protocol.hpp
 * @brief Defines the network protocol, player state, and serialization
 * utilities used by both server and client.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace Jetpack::Shared::Protocol {

/**
 * @enum TileType
 * @brief Types of map tiles.
 */
enum class TileType : uint8_t { EMPTY = 0, COIN = 1, ELECTRICSQUARE = 2 };

/**
 * @enum CoinState
 * @brief States a coin can be in.
 */
enum class CoinState : uint8_t {
  AVAILABLE = 0,
  COLLECTED_P1 = 1,
  COLLECTED_P2 = 2,
  COLLECTED_BOTH = 3
};

/**
 * @struct GameMap
 * @brief Represents the grid of tiles and coin states.
 */
struct GameMap {
  int width = 0;
  int height = 0;

  std::vector<std::vector<TileType>> tiles;
  std::vector<std::vector<CoinState>> coinStates;

  /**
   * @brief Check if a coordinate lies inside the map bounds.
   * @param x Column index
   * @param y Row index
   * @return True if (x,y) is within [0,width)×[0,height)
   */
  [[nodiscard]] bool isValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
  }

  /**
   * @brief Get the tile at a position, or EMPTY if out of bounds.
   * @param x Column index
   * @param y Row index
   * @return TileType at the given cell
   */
  [[nodiscard]] TileType getTileAt(int x, int y) const {
    return isValidPosition(x, y) ? tiles[y][x] : TileType::EMPTY;
  }

  /**
   * @brief Get the coin state at a position, or AVAILABLE if oob.
   * @param x Column index
   * @param y Row index
   * @return CoinState at the given cell
   */
  [[nodiscard]] CoinState getCoinStateAt(int x, int y) const {
    return isValidPosition(x, y) ? coinStates[y][x] : CoinState::AVAILABLE;
  }
};

/**
 * @struct Position
 * @brief 2D floating‑point position.
 */
struct Position {
  float x = 0.0f;
  float y = 0.0f;

  Position() = default;
  Position(float xPos, float yPos) : x(xPos), y(yPos) {}

  /**
   * @brief Compare two positions for equality.
   * @param other Another Position
   * @return True if x and y both match exactly
   */
  bool operator==(const Position &other) const {
    return x == other.x && y == other.y;
  }
};

/**
 * @enum PlayerState
 * @brief Lifecycle states of a player.
 */
enum class PlayerState : uint8_t {
  CONNECTED = 0,
  READY = 1,
  PLAYING = 2,
  DEAD = 3,
  FINISHED = 4,
  DISCONNECTED = 5
};

/**
 * @class Player
 * @brief Encapsulates per‑player network socket, state, position, and score.
 */
class Player {
public:
  /**
   * @brief Construct a player record.
   * @param clientSocket Socket descriptor for communication.
   * @param playerId     Unique in‑game player identifier.
   */
  Player(int clientSocket, int playerId)
      : m_clientSocket(clientSocket), m_id(playerId) {}

  /** @return Associated client socket descriptor */
  [[nodiscard]] int getClientSocket() const { return m_clientSocket; }
  /** @return Unique player ID */
  [[nodiscard]] int getId() const { return m_id; }
  /** @return Current 2D position */
  [[nodiscard]] Position getPosition() const { return m_position; }
  /** @return Vertical velocity component */
  [[nodiscard]] float getVelocityY() const { return m_velocityY; }
  /** @return True if player is currently using jetpack */
  [[nodiscard]] bool isJetpacking() const { return m_isJetpacking; }
  /** @return Current score (coins collected) */
  [[nodiscard]] int getScore() const { return m_score; }
  /** @return Current lifecycle state */
  [[nodiscard]] PlayerState getState() const { return m_state; }

  /**
   * @brief Set the player's position.
   * @param x New X coordinate
   * @param y New Y coordinate
   */
  void setPosition(float x, float y) { m_position = {x, y}; }
  /** @brief Set the vertical velocity. */
  void setVelocityY(float velocity) { m_velocityY = velocity; }
  /** @brief Enable or disable jetpacking. */
  void setJetpacking(bool jetpacking) { m_isJetpacking = jetpacking; }
  /** @brief Update the player's score. */
  void setScore(int score) { m_score = score; }
  /** @brief Change the player's lifecycle state. */
  void setState(PlayerState state) { m_state = state; }

  /**
   * @brief Check if the player is in a playable state.
   * @return True if READY or PLAYING
   */
  [[nodiscard]] bool isActive() const {
    return m_state == PlayerState::PLAYING || m_state == PlayerState::READY;
  }

  /** @return True if the player has finished the map */
  [[nodiscard]] bool hasFinished() const {
    return m_state == PlayerState::FINISHED;
  }

  /** @return True if the player is dead */
  [[nodiscard]] bool isDead() const { return m_state == PlayerState::DEAD; }

private:
  int m_clientSocket;
  int m_id;
  Position m_position;
  float m_velocityY = 0;
  bool m_isJetpacking = false;
  int m_score = 0;
  PlayerState m_state = PlayerState::CONNECTED;
};

/**
 * @enum PacketType
 * @brief Identifiers for network packet types.
 */
enum class PacketType : uint8_t {
  CONNECT_REQUEST = 0x01,
  CONNECT_RESPONSE = 0x02,
  MAP_DATA = 0x03,
  GAME_START = 0x04,
  PLAYER_INPUT = 0x05,
  GAME_STATE_UPDATE = 0x06,
  PLAYER_POSITION = 0x07,
  COIN_COLLECTED = 0x08,
  PLAYER_DEATH = 0x09,
  GAME_OVER = 0x0A,
  PLAYER_DISCONNECT = 0x0B
};

/**
 * @enum GameState
 * @brief Overall game lifecycle on the server.
 */
enum class GameState : uint8_t {
  WAITING_FOR_PLAYERS = 0,
  IN_PROGRESS = 1,
  GAME_OVER = 2
};

/**
 * @struct NetworkPacket
 * @brief Buffer builder for packet serialization.
 */
struct NetworkPacket {
  PacketType type;
  std::vector<std::byte> data;

  /**
   * @brief Create a packet with only its type.
   * @param packetType Type identifier
   */
  explicit NetworkPacket(PacketType packetType) : type(packetType) {}

  /**
   * @brief Create a packet with type and initial payload.
   * @param packetType  Type identifier
   * @param packetData  Initial data payload
   */
  NetworkPacket(PacketType packetType, std::vector<std::byte> packetData)
      : type(packetType), data(std::move(packetData)) {}

  /**
   * @brief Append a raw byte.
   * @param value Byte to add
   */
  void addByte(std::byte value) { data.push_back(value); }

  /**
   * @brief Append an unsigned byte.
   * @param value Value in [0,255]
   */
  void addByte(uint8_t value) { data.push_back(static_cast<std::byte>(value)); }

  /**
   * @brief Append a 16‑bit unsigned value (little endian).
   * @param value Value in [0,65535]
   */
  void addShort(uint16_t value) {
    data.push_back(static_cast<std::byte>(value & 0xFF));
    data.push_back(static_cast<std::byte>((value >> 8) & 0xFF));
  }

  /**
   * @brief Append a 32‑bit unsigned value (little endian).
   * @param value Value in [0,2³²)
   */
  void addInt(uint32_t value) {
    data.push_back(static_cast<std::byte>(value & 0xFF));
    data.push_back(static_cast<std::byte>((value >> 8) & 0xFF));
    data.push_back(static_cast<std::byte>((value >> 16) & 0xFF));
    data.push_back(static_cast<std::byte>((value >> 24) & 0xFF));
  }

  /**
   * @brief Append a 32‑bit float (via uint32 cast, little endian).
   * @param value IEEE‑754 float
   */
  void addFloat(float value) {
    union {
      float f;
      uint32_t i;
    } converter{};
    converter.f = value;
    addInt(converter.i);
  }

  /**
   * @brief Serialize the packet into a contiguous byte buffer.
   * @return Vector of bytes: [type][payload...]
   */
  [[nodiscard]] std::vector<std::byte> serialize() const {
    std::vector<std::byte> result;
    result.reserve(data.size() + 1);
    result.push_back(static_cast<std::byte>(type));
    result.insert(result.end(), data.begin(), data.end());
    return result;
  }
};

/**
 * @name Inline Packet Creators
 * Convenience functions to build common packets.
 * @{
 */

/**
 * @brief Create an empty CONNECT_REQUEST packet.
 * @return NetworkPacket ready to send.
 */
inline NetworkPacket createConnectRequest() {
  return NetworkPacket(PacketType::CONNECT_REQUEST);
}

/**
 * @brief Create a CONNECT_RESPONSE packet.
 * @param playerId      Assigned player ID.
 * @param totalPlayers  Current number of players.
 * @return NetworkPacket ready to send.
 */
inline NetworkPacket createConnectResponse(int playerId, int totalPlayers) {
  NetworkPacket pkt(PacketType::CONNECT_RESPONSE);
  pkt.addByte(static_cast<uint8_t>(playerId));
  pkt.addByte(static_cast<uint8_t>(totalPlayers));
  return pkt;
}

/**
 * @brief Create a PLAYER_INPUT packet.
 * @param jetpackActive True if jetpack button pressed.
 * @return NetworkPacket ready to send.
 */
inline NetworkPacket createPlayerInput(bool jetpackActive) {
  NetworkPacket pkt(PacketType::PLAYER_INPUT);
  pkt.addByte(jetpackActive ? 1 : 0);
  return pkt;
}

/**
 * @brief Create a GAME_OVER packet.
 * @param winnerId ID of the winner, or ≤0 for no winner.
 * @return NetworkPacket ready to send.
 */
inline NetworkPacket createGameOverPacket(int winnerId = -1) {
  NetworkPacket pkt(PacketType::GAME_OVER);
  pkt.addByte(winnerId > 0 ? 1 : 0);
  pkt.addByte(winnerId > 0 ? winnerId : 0);
  return pkt;
}

/** @} */
} // namespace Jetpack::Shared::Protocol
