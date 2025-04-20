/**
 * @file Physics.cpp
 * @brief Implements physics updates, bounds enforcement, and collision tests.
 */

#include "Physics.hpp"
#include <algorithm>

namespace Jetpack::Server {

void Physics::applyPhysics(Shared::Protocol::Player &player) {
  float velocityY = player.getVelocityY() + GRAVITY;

  if (player.isJetpacking()) {
    velocityY -= JETPACK_FORCE;
  }

  velocityY = std::clamp(velocityY, -MAX_VELOCITY, MAX_VELOCITY);
  player.setVelocityY(velocityY);

  const auto pos = player.getPosition();
  player.setPosition(pos.x + HORIZONTAL_SPEED, pos.y + velocityY);
}

void Physics::checkBounds(Shared::Protocol::Player &player,
                          const Shared::Protocol::GameMap &map) {
  const auto pos = player.getPosition();

  if (pos.y < 0.0f) {
    player.setPosition(pos.x, 0.0f);
    player.setVelocityY(0.0f);
  } else if (pos.y >= map.height - 1.0f) {
    player.setPosition(pos.x, map.height - 1.0f);
    player.setVelocityY(0.0f);
  }
}

} // namespace Jetpack::Server
