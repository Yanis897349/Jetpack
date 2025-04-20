/**
 * @file GameData.hpp
 * @brief Thread-safe container for client-side game state data.
 */

#pragma once

#include "../Shared/Protocol.hpp"
#include <mutex>
#include <vector>

namespace Jetpack::Client {

/**
 * @class GameData
 * @brief Manages and synchronizes access to game state data on the client.
 * 
 * GameData provides thread-safe access to the game map, coin states, player
 * information, and game state. It serves as a central data repository that
 * can be safely accessed from both the rendering thread and the network thread.
 */
class GameData {
public:
  /** @brief Default constructor. */
  GameData() = default;

  /**
   * @brief Get a copy of the current game map.
   * @return The game map containing level layout information.
   */
  Shared::Protocol::GameMap getMap() const {
    std::lock_guard lock(m_dataMutex);
    return m_map;
  }

  /**
   * @brief Updates the game map with new data from the server.
   * @param map New map to store locally.
   * 
   * Also initializes/resizes the coin state tracking matrix to match
   * the map dimensions and copies any provided coin states.
   */
  void updateMap(const Shared::Protocol::GameMap &map) {
    std::lock_guard lock(m_dataMutex);
    m_map = map;

    m_coinStates.resize(
        map.height,
        std::vector(map.width,
                    static_cast<int>(Shared::Protocol::CoinState::AVAILABLE)));

    if (!map.coinStates.empty()) {
      for (int y = 0; y < map.height; y++) {
        for (int x = 0; x < map.width; x++) {
          m_coinStates[y][x] = static_cast<int>(map.coinStates[y][x]);
        }
      }
    }
  }

  /**
   * @brief Get a copy of the current coin states.
   * @return 2D vector containing the state of each coin in the map.
   */
  std::vector<std::vector<int>> getCoinStates() const {
    std::lock_guard lock(m_dataMutex);
    return m_coinStates;
  }

  /**
   * @brief Updates the state of a specific coin in the map.
   * @param x X-coordinate of the coin.
   * @param y Y-coordinate of the coin.
   * @param coinState New state value for the coin.
   * 
   * If the coin is now fully collected (by both players), also
   * updates the map tile to be empty.
   */
  void updateCoinStates(int x, int y, int coinState) {
    std::lock_guard lock(m_dataMutex);
    if (x >= 0 && x < m_map.width && y >= 0 && y < m_map.height) {
      m_coinStates[y][x] = coinState;

      if (coinState ==
          static_cast<int>(Shared::Protocol::CoinState::COLLECTED_BOTH)) {
        m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
      }
    }
  }

  /**
   * @brief Get a copy of the current player list.
   * @return Vector of all players in the game.
   */
  std::vector<Shared::Protocol::Player> getPlayers() const {
    std::lock_guard lock(m_dataMutex);
    return m_players;
  }

  /**
   * @brief Updates the entire player list.
   * @param players New player list from the server.
   */
  void updatePlayers(const std::vector<Shared::Protocol::Player> &players) {
    std::lock_guard lock(m_dataMutex);
    m_players = players;
  }

  /**
   * @brief Updates the state of a specific player.
   * @param playerId ID of the player to update.
   * @param state New state for the player.
   */
  void updatePlayerState(int playerId, Shared::Protocol::PlayerState state) {
    std::lock_guard lock(m_dataMutex);
    for (auto &player : m_players) {
      if (player.getId() == playerId) {
        player.setState(state);
        break;
      }
    }
  }

  /**
   * @brief Get the ID of the local player.
   * @return The player ID assigned to this client.
   */
  int getLocalPlayerId() const {
    std::lock_guard lock(m_dataMutex);
    return m_localPlayerId;
  }

  /**
   * @brief Sets the ID of the local player.
   * @param id Player ID assigned by the server.
   */
  void setLocalPlayerId(int id) {
    std::lock_guard lock(m_dataMutex);
    m_localPlayerId = id;
  }

  /**
   * @brief Check if the game is over.
   * @return True if the game has ended, false otherwise.
   */
  bool isGameOver() const {
    std::lock_guard lock(m_dataMutex);
    return m_gameOver;
  }

  /**
   * @brief Get the ID of the winning player.
   * @return ID of the winner, or -1 if there is no winner.
   */
  int getWinnerId() const {
    std::lock_guard lock(m_dataMutex);
    return m_winnerId;
  }

  /**
   * @brief Sets the game to the "game over" state.
   * @param winnerId ID of the winning player, or -1 if no winner.
   */
  void updateGameOver(int winnerId) {
    std::lock_guard lock(m_dataMutex);
    m_gameOver = true;
    m_winnerId = winnerId;
  }

private:
  mutable std::mutex m_dataMutex;
  Shared::Protocol::GameMap m_map;
  std::vector<std::vector<int>> m_coinStates;
  std::vector<Shared::Protocol::Player> m_players;
  int m_localPlayerId = 1;
  bool m_gameOver = false;
  int m_winnerId = -1;
};

} // namespace Jetpack::Client