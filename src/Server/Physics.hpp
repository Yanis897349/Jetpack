/**
 * @file Physics.hpp
 * @brief Declaration of the Physics utility class for player movement,
 *        bounds checking, and collision detection.
 */

#pragma once

#include "../Shared/Protocol.hpp"
#include <cmath>

namespace Jetpack::Server {

/**
 * @class Physics
 * @brief Provides static methods to apply gravity/jetpack forces,
 *        enforce world bounds.
 */
class Physics {
public:
  /**
   * @brief Updates a player's vertical velocity and position.
   * @param player The player whose physics to update.
   *
   * Applies gravity, subtracts jetpack thrust if active,
   * clamps velocity, and advances horizontal and vertical position.
   */
  static void applyPhysics(Shared::Protocol::Player &player);

  /**
   * @brief Constrains a player to the vertical bounds of the map.
   * @param player The player to clamp.
   * @param map    The game map providing height limits.
   *
   * If the player is above the ceiling or below the floor,
   * resets vertical velocity and moves them inside the map.
   */
  static void checkBounds(Shared::Protocol::Player &player,
                          const Shared::Protocol::GameMap &map);

private:
  static constexpr float GRAVITY = 0.008f;
  static constexpr float JETPACK_FORCE = 0.013f;
  static constexpr float MAX_VELOCITY = 0.05f;
  static constexpr float HORIZONTAL_SPEED = 0.05f;
};

} // namespace Jetpack::Server
