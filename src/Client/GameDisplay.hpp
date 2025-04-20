/**
 * @file GameDisplay.hpp
 * @brief Handles game rendering, animation, and user input for the Jetpack client.
 */

#pragma once

#include "../Shared/Protocol.hpp"
#include "GameData.hpp"
#include "SoundManager.hpp"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <vector>

namespace Jetpack::Client {

/**
 * @class GameDisplay
 * @brief Manages rendering, animations, and user input for the Jetpack game client.
 * 
 * GameDisplay handles the visual presentation of the game world, including rendering
 * the map, players, and UI elements. It also processes user input for jetpack control
 * and manages sound effects and animations.
 */
class GameDisplay {
public:
  /**
   * @brief Constructs a new GameDisplay.
   * @param windowWidth Width of the game window in pixels.
   * @param windowHeight Height of the game window in pixels.
   * 
   * Creates the game window, initializes graphics resources, and sets up the rendering
   * boundaries for the playable area.
   */
  explicit GameDisplay(int windowWidth = 1920, int windowHeight = 1080);
  
  /**
   * @brief Destructor that ensures proper cleanup of resources.
   * 
   * Closes the window and stops music playback.
   */
  ~GameDisplay();

  /**
   * @brief Deleted copy constructor.
   */
  GameDisplay(const GameDisplay &) = delete;
  
  /**
   * @brief Deleted copy assignment operator.
   */
  GameDisplay &operator=(const GameDisplay &) = delete;
  
  /**
   * @brief Deleted move constructor.
   */
  GameDisplay(GameDisplay &&) = delete;
  
  /**
   * @brief Deleted move assignment operator.
   */
  GameDisplay &operator=(GameDisplay &&) = delete;

  /**
   * @brief Enters the main rendering loop.
   * 
   * Processes events, updates animations and camera position, and renders the scene
   * until the window is closed.
   */
  void run();

  /**
   * @brief Updates the game map with data received from the server.
   * @param map New map data to render.
   */
  void updateMap(const Shared::Protocol::GameMap &map);
  
  /**
   * @brief Updates player states with data received from the server.
   * @param players List of updated player information.
   */
  void updateGameState(const std::vector<Shared::Protocol::Player> &players);
  
  /**
   * @brief Handles a coin collection event.
   * @param playerId ID of the player who collected the coin.
   * @param x X-coordinate of the collected coin.
   * @param y Y-coordinate of the collected coin.
   * @param coinState New state of the coin after collection.
   */
  void handleCoinCollected(int playerId, int x, int y, int coinState);
  
  /**
   * @brief Handles a player death event.
   * @param playerId ID of the player who died.
   */
  void handlePlayerDeath(int playerId);
  
  /**
   * @brief Handles the game over event.
   * @param winnerId ID of the winning player, or -1 if no winner.
   */
  void handleGameOver(int winnerId);

  /**
   * @brief Sets the ID of the local player.
   * @param id Player ID assigned by the server.
   */
  void setLocalPlayerId(int id);
  
  /**
   * @brief Enables or disables the debug display mode.
   * @param debug If true, shows debugging visuals like hitboxes.
   */
  void setDebugMode(bool debug);

  /**
   * @brief Checks if the jetpack control is currently active.
   * @return True if the jetpack button is currently pressed.
   */
  [[nodiscard]] bool isJetpackActive() const;

private:
  sf::RenderWindow m_window;

  sf::Texture m_backgroundTexture;
  sf::Texture m_playerSpritesheet;
  sf::Texture m_coinSpritesheet;
  sf::Texture m_zapperSpritesheet;

  sf::Font m_gameFont;

  std::vector<sf::IntRect> m_playerRunFrames;
  std::vector<sf::IntRect> m_playerJetpackFrames;
  std::vector<sf::IntRect> m_coinFrames;
  std::vector<sf::IntRect> m_zapperFrames;

  sf::Clock m_animationClock;
  int m_playerAnimFrame = 0;
  int m_jetpackAnimFrame = 0;
  int m_coinAnimFrame = 0;
  int m_zapperAnimFrame = 0;

  bool m_wasJetpacking = false;
  bool m_debugMode = false;

  SoundManager m_soundManager;
  GameData m_gameData;

  bool m_jetpackActive = false;

  float m_topBoundary = 0.0f;
  float m_bottomBoundary = 0.0f;
  float m_backgroundHeight = 0.0f;
  float m_playableHeight = 0.0f;

  /**
   * @brief Processes system and input events.
   * 
   * Handles window close events and input for jetpack controls.
   */
  void processEvents();
  
  /**
   * @brief Updates animation frames based on elapsed time.
   */
  void updateAnimations();
  
  /**
   * @brief Manages jetpack sound effects based on player state.
   */
  void handleJetpackSounds();

  /**
   * @brief Loads and initializes graphical and audio resources.
   */
  void loadResources();
  
  /**
   * @brief Sets up animation frame rectangles from sprite sheets.
   */
  void initializeAnimations();

  std::vector<sf::Sprite> m_parallaxLayers;
  std::vector<float> m_parallaxSpeeds;
  float m_backgroundScrollPosition = 0.0f;
  float m_cameraPositionX = 0.0f;
  float m_visibleMapWidth = 0.0f;
  float m_cameraZoom = 2.0f;

  /**
   * @brief Initializes the parallax background layers.
   */
  void initializeParallaxBackgrounds();
  
  /**
   * @brief Updates the position of parallax backgrounds based on camera movement.
   * @param deltaTime Time elapsed since last frame in seconds.
   */
  void updateParallaxBackgrounds(float deltaTime);
  
  /**
   * @brief Renders the parallax background layers.
   */
  void drawParallaxBackgrounds();

  /**
   * @brief Main rendering function called each frame.
   */
  void render();
  
  /**
   * @brief Renders the static game background.
   */
  void drawBackground();
  
  /**
   * @brief Renders the game map tiles (coins, zappers).
   */
  void drawMap();
  
  /**
   * @brief Renders the player sprites.
   */
  void drawPlayers();
  
  /**
   * @brief Renders the UI elements (scores, status).
   */
  void drawUI();
  
  /**
   * @brief Renders the game over screen.
   */
  void drawGameOver();
};

} // namespace Jetpack::Client