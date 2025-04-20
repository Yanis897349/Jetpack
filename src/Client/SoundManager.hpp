/**
 * @file SoundManager.hpp
 * @brief Manages game sound effects and music playback.
 */

#pragma once

#include <SFML/Audio.hpp>
#include <optional>

namespace Jetpack::Client {

/**
 * @class SoundManager
 * @brief Handles loading and playback of game audio assets.
 *
 * SoundManager centralizes sound effect and music management for the game,
 * handling resource loading, playback control, and sound state management.
 */
class SoundManager {
public:
  /**
   * @brief Constructor that loads all audio resources.
   *
   * Loads sound effects and music assets from the resources directory.
   * May throw ResourceException if assets cannot be loaded.
   */
  SoundManager();

  /**
   * @brief Plays the coin pickup sound effect.
   */
  void playCoinPickup() { m_coinPickupSound.play(); }

  /**
   * @brief Starts playing the jetpack continuous sound.
   */
  void playJetpackLoop() { m_jetpackLoopSound.play(); }

  /**
   * @brief Stops the jetpack continuous sound.
   */
  void stopJetpackLoop() { m_jetpackLoopSound.stop(); }

  /**
   * @brief Plays the jetpack startup sound effect.
   */
  void playJetpackStart() { m_jetpackStartSound.play(); }

  /**
   * @brief Plays the jetpack shutdown sound effect.
   */
  void playJetpackStop() { m_jetpackStopSound.play(); }

  /**
   * @brief Plays the zapper/electric hazard sound effect.
   */
  void playZapper() { m_zapperSound.play(); }

  /**
   * @brief Gets a reference to the background music.
   * @return Reference to the optional music object.
   */
  std::optional<sf::Music> &getGameMusic() { return m_gameMusic; }

private:
  sf::SoundBuffer m_coinPickupBuffer;
  sf::SoundBuffer m_jetpackStartBuffer;
  sf::SoundBuffer m_jetpackLoopBuffer;
  sf::SoundBuffer m_jetpackStopBuffer;
  sf::SoundBuffer m_zapperBuffer;

  sf::Sound m_coinPickupSound;
  sf::Sound m_jetpackStartSound;
  sf::Sound m_jetpackLoopSound;
  sf::Sound m_jetpackStopSound;
  sf::Sound m_zapperSound;

  std::optional<sf::Music> m_gameMusic;
};
} // namespace Jetpack::Client
