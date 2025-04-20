/**
 * @file GameDisplay.cpp
 * @brief Implements the GameDisplay class for rendering and user interaction.
 */

#include "GameDisplay.hpp"
#include "../Shared/Exceptions.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <vector>

namespace Jetpack::Client {

GameDisplay::GameDisplay(int windowWidth, int windowHeight)
    : m_window(sf::VideoMode(windowWidth, windowHeight), "Jetpack") {
  loadResources();
  m_window.setFramerateLimit(60);

  m_topBoundary = 48.0f;
  m_bottomBoundary = 50.0f;
  m_backgroundHeight = 341.0f;
  m_playableHeight = m_backgroundHeight - m_topBoundary - m_bottomBoundary;
}

GameDisplay::~GameDisplay() {
  if (m_window.isOpen()) {
    m_window.close();
  }

  if (m_soundManager.getGameMusic()) {
    m_soundManager.getGameMusic()->stop();
  }
}

void GameDisplay::loadResources() {
  try {
    if (!m_gameFont.loadFromFile("./resources/jetpack_font.ttf")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/jetpack_font.ttf"),
          "No such file or directory");
    }
    if (!m_backgroundTexture.loadFromFile("./resources/background.png")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/background.png"),
          "No such file or directory");
    }
    if (!m_playerSpritesheet.loadFromFile(
            "./resources/player_sprite_sheet.png")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/player_sprite_sheet.png"),
          "No such file or directory");
    }
    if (!m_coinSpritesheet.loadFromFile("./resources/coins_sprite_sheet.png")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/coins_sprite_sheet.png"),
          "No such file or directory");
    }
    if (!m_zapperSpritesheet.loadFromFile(
            "./resources/zapper_sprite_sheet.png")) {
      throw Shared::Exceptions::ResourceException(
          std::filesystem::path("./resources/zapper_sprite_sheet.png"),
          "No such file or directory");
    }

    initializeParallaxBackgrounds();
    initializeAnimations();
  } catch (const Shared::Exceptions::ResourceException &e) {
    std::cerr << std::format("Error loading resources: {}", e.what())
              << std::endl;
  }
}

void GameDisplay::initializeParallaxBackgrounds() {
  if (m_backgroundTexture.getSize().x == 0) {
    std::cerr << "Background texture is invalid!" << std::endl;
    return;
  }

  m_parallaxLayers.clear();
  m_parallaxSpeeds.clear();

  const std::vector speeds = {0.2f, 0.4f, 0.6f, 0.8f};

  Shared::Protocol::GameMap map = m_gameData.getMap();

  if (map.width > 0) {
    constexpr float cameraZoom = 2.0f;
    m_visibleMapWidth = map.width / cameraZoom;
  } else {
    m_visibleMapWidth = 10.0f;
  }

  for (size_t i = 0; i < speeds.size(); i++) {
    sf::Sprite layer(m_backgroundTexture);

    float baseScale = static_cast<float>(m_window.getSize().y) /
                      m_backgroundTexture.getSize().y;
    baseScale *= 1.2f;

    switch (i) {
    case 0:
      layer.setScale(baseScale * 0.95f, baseScale * 0.95f);
      layer.setColor(sf::Color(100, 100, 180, 150));
      break;
    case 1:
      layer.setScale(baseScale * 0.97f, baseScale * 0.97f);
      layer.setColor(sf::Color(150, 150, 200, 180));
      break;
    case 2:
      layer.setScale(baseScale * 0.99f, baseScale * 0.99f);
      layer.setColor(sf::Color(200, 200, 230, 210));
      break;
    default:
      layer.setScale(baseScale, baseScale);
      layer.setColor(sf::Color(255, 255, 255, 255));
      break;
    }

    m_parallaxLayers.push_back(layer);
    m_parallaxSpeeds.push_back(speeds[i]);
  }
}

void GameDisplay::updateParallaxBackgrounds(float deltaTime) {
  int localPlayerId = m_gameData.getLocalPlayerId();
  std::vector<Shared::Protocol::Player> players = m_gameData.getPlayers();

  float playerX = 0.0f;

  const auto localPlayerIt =
      std::ranges::find_if(players, [this, localPlayerId](const auto &player) {
        return player.getId() == localPlayerId;
      });

  if (localPlayerIt != players.end()) {
    playerX = localPlayerIt->getPosition().x;
  }

  Shared::Protocol::GameMap map = m_gameData.getMap();

  if (map.width > 0) {
    constexpr float cameraOffsetX = 0.3f;
    constexpr float cameraZoom = 2.0f;
    m_visibleMapWidth = map.width / cameraZoom;

    float targetCameraX = playerX - (m_visibleMapWidth * cameraOffsetX);

    targetCameraX =
        std::clamp(targetCameraX, 0.0f, map.width - m_visibleMapWidth);

    const float cameraLerpFactor = 5.0f * deltaTime;
    m_cameraPositionX = m_cameraPositionX +
                        (targetCameraX - m_cameraPositionX) * cameraLerpFactor;
  } else {
    m_backgroundScrollPosition += 50.0f * deltaTime;
    m_cameraPositionX = m_backgroundScrollPosition;
  }
}

void GameDisplay::initializeAnimations() {
  constexpr int playerFrameWidth = 134;
  constexpr int playerFrameHeight = 134;
  constexpr int numPlayerRunFrames = 4;
  constexpr int numPlayerJetpackFrames = 4;

  for (int i = 0; i < numPlayerRunFrames; i++) {
    sf::IntRect frame(i * playerFrameWidth, 0, playerFrameWidth,
                      playerFrameHeight);
    m_playerRunFrames.push_back(frame);
  }

  for (int i = 0; i < numPlayerJetpackFrames; i++) {
    sf::IntRect frame(i * playerFrameWidth, playerFrameHeight, playerFrameWidth,
                      playerFrameHeight);
    m_playerJetpackFrames.push_back(frame);
  }

  constexpr int numCoinFrames = 6;
  for (int i = 0; i < numCoinFrames; i++) {
    constexpr int coinFrameHeight = 171;
    constexpr int coinFrameWidth = 192;
    sf::IntRect frame(i * coinFrameWidth, 0, coinFrameWidth, coinFrameHeight);
    m_coinFrames.push_back(frame);
  }

  constexpr int numZapperFrames = 4;
  for (int i = 0; i < numZapperFrames; i++) {
    constexpr int zapperFrameHeight = 122;
    constexpr int zapperFrameWidth = 47;
    sf::IntRect frame(i * zapperFrameWidth, 0, zapperFrameWidth,
                      zapperFrameHeight);
    m_zapperFrames.push_back(frame);
  }
}

void GameDisplay::run() {
  m_animationClock.restart();
  sf::Clock deltaClock;

  while (m_window.isOpen()) {
    const float deltaTime = deltaClock.restart().asSeconds();

    processEvents();
    updateAnimations();
    updateParallaxBackgrounds(deltaTime);
    render();
  }
}

void GameDisplay::updateAnimations() {
  const float elapsedTime = m_animationClock.getElapsedTime().asSeconds();

  m_playerAnimFrame =
      static_cast<int>((elapsedTime * 10.0f)) % m_playerRunFrames.size();
  m_jetpackAnimFrame =
      static_cast<int>((elapsedTime * 15.0f)) % m_playerJetpackFrames.size();
  m_coinAnimFrame =
      static_cast<int>((elapsedTime * 8.0f)) % m_coinFrames.size();
  m_zapperAnimFrame =
      static_cast<int>((elapsedTime * 12.0f)) % m_zapperFrames.size();

  handleJetpackSounds();
}

void GameDisplay::handleJetpackSounds() {
  int localPlayerId = m_gameData.getLocalPlayerId();
  std::vector<Shared::Protocol::Player> players = m_gameData.getPlayers();

  bool anyPlayerJetpacking = false;

  const auto localPlayerIt =
      std::ranges::find_if(players, [this, localPlayerId](const auto &player) {
        return player.getId() == localPlayerId && player.isJetpacking();
      });

  anyPlayerJetpacking = localPlayerIt != players.end();

  if (anyPlayerJetpacking && !m_wasJetpacking) {
    m_soundManager.playJetpackStart();
    m_soundManager.playJetpackLoop();
    m_wasJetpacking = true;
  } else if (!anyPlayerJetpacking && m_wasJetpacking) {
    m_soundManager.stopJetpackLoop();
    m_soundManager.playJetpackStop();
    m_wasJetpacking = false;
  }
}

void GameDisplay::processEvents() {
  sf::Event event{};
  while (m_window.pollEvent(event)) {
    switch (event.type) {
    case sf::Event::Closed:
      m_window.close();
      break;

    case sf::Event::KeyPressed:
      if (event.key.code == sf::Keyboard::Space) {
        m_jetpackActive = true;
      }
      break;

    case sf::Event::KeyReleased:
      if (event.key.code == sf::Keyboard::Space) {
        m_jetpackActive = false;
      }
      break;

    case sf::Event::MouseButtonPressed:
      if (event.mouseButton.button == sf::Mouse::Left) {
        m_jetpackActive = true;
      }
      break;

    case sf::Event::MouseButtonReleased:
      if (event.mouseButton.button == sf::Mouse::Left) {
        m_jetpackActive = false;
      }
      break;

    default:
      break;
    }
  }
}

void GameDisplay::render() {
  m_window.clear(sf::Color(10, 10, 30));

  if (m_gameData.isGameOver()) {
    drawGameOver();
  } else {
    drawParallaxBackgrounds();
    drawMap();
    drawPlayers();
    drawUI();
  }

  m_window.display();
}

void GameDisplay::drawParallaxBackgrounds() {
  sf::RectangleShape darkBackground(
      sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
  darkBackground.setFillColor(sf::Color(20, 20, 50));
  m_window.draw(darkBackground);

  for (size_t i = 0; i < m_parallaxLayers.size(); i++) {
    float parallaxOffset = m_cameraPositionX * m_parallaxSpeeds[i];

    float spriteWidth =
        m_backgroundTexture.getSize().x * m_parallaxLayers[i].getScale().x;
    int repetitions =
        static_cast<int>(std::ceil(m_window.getSize().x / spriteWidth)) + 2;

    float startX = -std::fmod(parallaxOffset, spriteWidth);
    float verticalPosition =
        (m_window.getSize().y -
         m_backgroundTexture.getSize().y * m_parallaxLayers[i].getScale().y) /
        2.0f;

    for (int j = -1; j < repetitions; j++) {
      float posX = startX + j * spriteWidth;
      m_parallaxLayers[i].setPosition(posX, verticalPosition);

      if (posX < m_window.getSize().x && posX + spriteWidth > 0) {
        m_window.draw(m_parallaxLayers[i]);
      }
    }
  }

  sf::RectangleShape gradientOverlay(
      sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
  sf::Color bottomColor(0, 0, 30, 50);
  gradientOverlay.setFillColor(bottomColor);
  m_window.draw(gradientOverlay);
}

void GameDisplay::drawBackground() {
  sf::Sprite background(m_backgroundTexture);

  float scaleX = static_cast<float>(m_window.getSize().x) /
                 m_backgroundTexture.getSize().x;
  float scaleY = static_cast<float>(m_window.getSize().y) /
                 m_backgroundTexture.getSize().y;

  background.setScale(scaleX, scaleY);
  m_window.draw(background);

  if (m_debugMode) {
    sf::RectangleShape topLine(sf::Vector2f(m_window.getSize().x, 2.0f));
    topLine.setPosition(
        0, m_topBoundary *
               (static_cast<float>(m_window.getSize().y) / m_backgroundHeight));
    topLine.setFillColor(sf::Color::Red);
    m_window.draw(topLine);

    sf::RectangleShape bottomLine(sf::Vector2f(m_window.getSize().x, 2.0f));
    bottomLine.setPosition(
        0, m_window.getSize().y -
               (m_bottomBoundary * (static_cast<float>(m_window.getSize().y) /
                                    m_backgroundHeight)));
    bottomLine.setFillColor(sf::Color::Red);
    m_window.draw(bottomLine);
  }
}

void GameDisplay::drawPlayers() {
  Shared::Protocol::GameMap map = m_gameData.getMap();
  std::vector<Shared::Protocol::Player> players = m_gameData.getPlayers();
  int localPlayerId = m_gameData.getLocalPlayerId();

  if (map.width == 0 || map.height == 0) {
    return;
  }

  constexpr float cameraZoom = 2.0f;
  float visibleMapWidth = map.width / cameraZoom;
  float cellWidth = static_cast<float>(m_window.getSize().x) / visibleMapWidth;
  float windowHeight = static_cast<float>(m_window.getSize().y);
  float topOffset = m_topBoundary * (windowHeight / m_backgroundHeight);
  float bottomOffset = m_bottomBoundary * (windowHeight / m_backgroundHeight);
  float playableHeight = windowHeight - topOffset - bottomOffset;

  float cellHeight = playableHeight / map.height;

  for (const auto &player : players) {
    float screenX = (player.getPosition().x - m_cameraPositionX) * cellWidth;

    if (screenX < -cellWidth || screenX > m_window.getSize().x + cellWidth) {
      continue;
    }

    sf::Sprite playerSprite(m_playerSpritesheet);
    if (player.isJetpacking()) {
      playerSprite.setTextureRect(m_playerJetpackFrames[m_jetpackAnimFrame]);
    } else {
      playerSprite.setTextureRect(m_playerRunFrames[m_playerAnimFrame]);
    }

    float scale = 0.4f;
    playerSprite.setScale(scale, scale);

    float spriteWidth = playerSprite.getTextureRect().width * scale;
    float spriteHeight = playerSprite.getTextureRect().height * scale;
    float xPos = screenX + (cellWidth - spriteWidth) / 2;

    float yOffset = 10.0f;
    float relativePos = player.getPosition().y / map.height;
    float yPos = topOffset + (relativePos * playableHeight) +
                 (cellHeight - spriteHeight) / 2 - yOffset;

    yPos =
        std::clamp(yPos, topOffset, windowHeight - bottomOffset - spriteHeight);

    playerSprite.setPosition(xPos, yPos);

    if (player.getId() == localPlayerId) {
      playerSprite.setColor(sf::Color(200, 255, 200));
    } else {
      playerSprite.setColor(sf::Color(255, 200, 200));
    }

    if (m_debugMode) {
      sf::RectangleShape hitbox;
      hitbox.setSize(sf::Vector2f(cellWidth * 0.8f, cellHeight * 0.8f));
      hitbox.setPosition(
          screenX + cellWidth * 0.1f,
          topOffset + (player.getPosition().y / map.height) * playableHeight +
              cellHeight * 0.1f);
      hitbox.setFillColor(sf::Color(0, 0, 0, 0));
      hitbox.setOutlineColor(sf::Color::Red);
      hitbox.setOutlineThickness(1.0f);
      m_window.draw(hitbox);
    }

    m_window.draw(playerSprite);
  }
}

void GameDisplay::drawMap() {
  Shared::Protocol::GameMap map = m_gameData.getMap();
  std::vector<std::vector<int>> coinStates = m_gameData.getCoinStates();
  int localPlayerId = m_gameData.getLocalPlayerId();

  if (map.width == 0 || map.height == 0) {
    return;
  }

  constexpr float cameraZoom = 2.0f;
  float visibleMapWidth = map.width / cameraZoom;
  float cellWidth = static_cast<float>(m_window.getSize().x) / visibleMapWidth;
  float windowHeight = static_cast<float>(m_window.getSize().y);
  float topOffset = m_topBoundary * (windowHeight / m_backgroundHeight);
  float bottomOffset = m_bottomBoundary * (windowHeight / m_backgroundHeight);
  float playableHeight = windowHeight - topOffset - bottomOffset;

  float cellHeight = playableHeight / map.height;

  if (m_debugMode) {
    sf::RectangleShape topBoundary;
    topBoundary.setSize(sf::Vector2f(m_window.getSize().x, 2.0f));
    topBoundary.setPosition(0, topOffset);
    topBoundary.setFillColor(sf::Color::Red);
    m_window.draw(topBoundary);

    sf::RectangleShape bottomBoundary;
    bottomBoundary.setSize(sf::Vector2f(m_window.getSize().x, 2.0f));
    bottomBoundary.setPosition(0, windowHeight - bottomOffset);
    bottomBoundary.setFillColor(sf::Color::Red);
    m_window.draw(bottomBoundary);
  }

  int startCol = static_cast<int>(m_cameraPositionX);
  startCol = std::max(0, startCol);
  int endCol = static_cast<int>(m_cameraPositionX + visibleMapWidth + 1);
  endCol = std::min(endCol, map.width);

  for (int i = 0; i < map.height; i++) {
    for (int j = startCol; j < endCol; j++) {
      float xPos = (j - m_cameraPositionX) * cellWidth;
      float yPos = topOffset + i * cellHeight;

      if (m_debugMode && map.tiles[i][j] != Shared::Protocol::TileType::EMPTY) {
        sf::RectangleShape cellHitbox;
        cellHitbox.setSize(sf::Vector2f(cellWidth * 0.8f, cellHeight * 0.8f));
        cellHitbox.setPosition(xPos + cellWidth * 0.1f,
                               yPos + cellHeight * 0.1f);
        cellHitbox.setFillColor(sf::Color(0, 0, 0, 0));
        cellHitbox.setOutlineColor(sf::Color::Yellow);
        cellHitbox.setOutlineThickness(1.0f);
        m_window.draw(cellHitbox);
      }

      switch (map.tiles[i][j]) {
      case Shared::Protocol::TileType::COIN: {
        sf::Sprite coinSprite(m_coinSpritesheet);
        coinSprite.setTextureRect(m_coinFrames[m_coinAnimFrame]);

        float scale = 0.2f;
        coinSprite.setScale(scale, scale);

        float spriteWidth = m_coinFrames[0].width * scale;
        float spriteHeight = m_coinFrames[0].height * scale;

        coinSprite.setPosition(xPos + (cellWidth - spriteWidth) / 2,
                               yPos + (cellHeight - spriteHeight) / 2);

        if (int coinState = coinStates[i][j];
            localPlayerId == 1 &&
            coinState ==
                static_cast<int>(Shared::Protocol::CoinState::COLLECTED_P1)) {
          coinSprite.setColor(sf::Color(255, 255, 255, 128));
        } else if (localPlayerId == 2 &&
                   coinState ==
                       static_cast<int>(
                           Shared::Protocol::CoinState::COLLECTED_P2)) {
          coinSprite.setColor(sf::Color(255, 255, 255, 128));
        } else {
          coinSprite.setColor(sf::Color(255, 255, 255, 255));
        }

        m_window.draw(coinSprite);
        break;
      }

      case Shared::Protocol::TileType::ELECTRICSQUARE: {
        sf::Sprite zapperSprite(m_zapperSpritesheet);
        zapperSprite.setTextureRect(m_zapperFrames[m_zapperAnimFrame]);

        float scale = 0.6f;
        zapperSprite.setScale(scale, scale);

        float spriteWidth = m_zapperFrames[0].width * scale;
        float spriteHeight = m_zapperFrames[0].height * scale;

        zapperSprite.setPosition(xPos + (cellWidth - spriteWidth) / 2,
                                 yPos + (cellHeight - spriteHeight) / 2);
        m_window.draw(zapperSprite);
        break;
      }

      default:
        break;
      }
    }
  }
}

void GameDisplay::drawUI() {
  std::vector<Shared::Protocol::Player> players = m_gameData.getPlayers();
  int localPlayerId = m_gameData.getLocalPlayerId();

  if (players.empty()) {
    sf::Text waitingText;
    waitingText.setFont(m_gameFont);
    waitingText.setCharacterSize(30);
    waitingText.setString("Waiting for other players...");
    waitingText.setFillColor(sf::Color::White);

    sf::FloatRect textRect = waitingText.getLocalBounds();
    waitingText.setOrigin(textRect.width / 2.0f, textRect.height / 2.0f);
    waitingText.setPosition(m_window.getSize().x / 2.0f,
                            m_window.getSize().y / 2.0f);

    m_window.draw(waitingText);
    return;
  }

  int yOffset = 10;
  for (const auto &player : players) {
    sf::Text scoreText;
    scoreText.setFont(m_gameFont);
    scoreText.setCharacterSize(20);
    scoreText.setOutlineThickness(2.0f);
    scoreText.setOutlineColor(sf::Color::Black);

    if (player.getId() == localPlayerId) {
      scoreText.setString(std::format("You: {}", player.getScore()));
      scoreText.setFillColor(sf::Color::Green);
    } else {
      scoreText.setString(
          std::format("Player {}: {}", player.getId(), player.getScore()));
      scoreText.setFillColor(sf::Color::Red);
    }

    scoreText.setPosition(10, yOffset);
    m_window.draw(scoreText);
    yOffset += 30;
  }
}

void GameDisplay::drawGameOver() {
  int winnerId = m_gameData.getWinnerId();
  int localPlayerId = m_gameData.getLocalPlayerId();

  sf::RectangleShape overlay(
      sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
  overlay.setFillColor(sf::Color(0, 0, 0, 200));
  m_window.draw(overlay);

  sf::Text gameOverText;
  gameOverText.setFont(m_gameFont);
  gameOverText.setCharacterSize(60);
  gameOverText.setString("GAME OVER");
  gameOverText.setFillColor(sf::Color::White);
  gameOverText.setOutlineThickness(3.0f);
  gameOverText.setOutlineColor(sf::Color::Black);

  sf::FloatRect textRect = gameOverText.getLocalBounds();
  gameOverText.setOrigin(textRect.width / 2.0f, textRect.height / 2.0f);
  gameOverText.setPosition(m_window.getSize().x / 2.0f,
                           m_window.getSize().y / 2.0f - 50);

  m_window.draw(gameOverText);

  sf::Text resultText;
  resultText.setFont(m_gameFont);
  resultText.setCharacterSize(40);
  resultText.setOutlineThickness(2.0f);
  resultText.setOutlineColor(sf::Color::Black);

  if (winnerId == localPlayerId) {
    resultText.setString("You win!");
    resultText.setFillColor(sf::Color::Green);
  } else if (winnerId > 0) {
    resultText.setString(std::format("Player {} Wins!", winnerId));
    resultText.setFillColor(sf::Color::Red);
  } else {
    resultText.setString("No winner");
    resultText.setFillColor(sf::Color::Yellow);
  }

  textRect = resultText.getLocalBounds();
  resultText.setOrigin(textRect.width / 2.0f, textRect.height / 2.0f);
  resultText.setPosition(m_window.getSize().x / 2.0f,
                         m_window.getSize().y / 2.0f + 50);
  m_window.draw(resultText);
}

void GameDisplay::updateMap(const Shared::Protocol::GameMap &map) {
  m_gameData.updateMap(map);
  initializeParallaxBackgrounds();
}

void GameDisplay::updateGameState(
    const std::vector<Shared::Protocol::Player> &players) {

  if (m_debugMode) {
    std::cout << std::format("Updating game state with {} players",
                             players.size())
              << std::endl;
  }

  m_gameData.updatePlayers(players);
}

void GameDisplay::handleCoinCollected(int playerId, int x, int y,
                                      int coinState) {
  m_gameData.updateCoinStates(x, y, coinState);
  std::vector<Shared::Protocol::Player> players = m_gameData.getPlayers();
  int localPlayerId = m_gameData.getLocalPlayerId();

  const auto playerIt =
      std::ranges::find_if(players, [playerId](const auto &player) {
        return player.getId() == playerId;
      });

  if (playerIt != players.end()) {
    playerIt->setScore(playerIt->getScore() + 1);

    if (playerId == localPlayerId) {
      m_soundManager.playCoinPickup();
    }
  }
}

void GameDisplay::handlePlayerDeath(int playerId) {
  m_gameData.updatePlayerState(playerId, Shared::Protocol::PlayerState::DEAD);
  int localPlayerId = m_gameData.getLocalPlayerId();

  if (playerId == localPlayerId) {
    m_soundManager.playZapper();
  }
}

void GameDisplay::handleGameOver(int winnerId) {
  m_gameData.updateGameOver(winnerId);
  m_soundManager.stopJetpackLoop();
}

bool GameDisplay::isJetpackActive() const { return m_jetpackActive; }

void GameDisplay::setLocalPlayerId(int id) { m_gameData.setLocalPlayerId(id); }

void GameDisplay::setDebugMode(bool debug) { m_debugMode = debug; }

} // namespace Jetpack::Client
