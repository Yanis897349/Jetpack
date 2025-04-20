/**
 * @file SoundManager.cpp
 * @brief Implements the SoundManager class for loading and managing game audio.
 */

#include "SoundManager.hpp"
#include "../Shared/Exceptions.hpp"
#include <format>
#include <iostream>

Jetpack::Client::SoundManager::SoundManager() {
  try {
    if (!m_coinPickupBuffer.loadFromFile("./resources/coin_pickup_1.wav")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/coin_pickup_1.wav"),
          "No such file or directory");
    }
    if (!m_jetpackStartBuffer.loadFromFile("./resources/jetpack_start.wav")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/jetpack_start.wav"),
          "No such file or directory");
    }
    if (!m_jetpackLoopBuffer.loadFromFile("./resources/jetpack_lp.wav")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/jetpack_lp.wav"), "NSo");
    }
    if (!m_jetpackStopBuffer.loadFromFile("./resources/jetpack_stop.wav")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/jetpack_stop.wav"),
          "No such file or directory");
    }
    if (!m_zapperBuffer.loadFromFile("./resources/dud_zapper_pop.wav")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/dud_zapper_pop.wav"),
          "No such file or directory");
    }

    m_coinPickupSound.setBuffer(m_coinPickupBuffer);
    m_jetpackStartSound.setBuffer(m_jetpackStartBuffer);
    m_jetpackLoopSound.setBuffer(m_jetpackLoopBuffer);
    m_jetpackLoopSound.setLoop(true);
    m_jetpackStopSound.setBuffer(m_jetpackStopBuffer);
    m_zapperSound.setBuffer(m_zapperBuffer);

    m_gameMusic.emplace();
    if (!m_gameMusic->openFromFile("./resources/theme.ogg")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/theme.ogg"),
          "No such file or directory");
    }
    m_gameMusic->setLoop(true);
    m_gameMusic->setVolume(50.0f);
    m_gameMusic->play();
  } catch (const Shared::Exceptions::ResourceException &e) {
    std::cerr << std::format("Error loading sounds: {}", e.what()) << std::endl;
    m_gameMusic = std::nullopt;
  }
}
