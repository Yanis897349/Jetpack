/**
 * @file Server.cpp
 * @brief Implements the GameServer: networking, game loop, and state logic.
 */

#include "Server.hpp"
#include "../Shared/Exceptions.hpp"
#include "Physics.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <vector>

Jetpack::Server::GameServer::GameServer(const int port,
                                        const std::string &mapFile,
                                        const bool debugMode)
    : m_port(port), m_mapFile(mapFile), m_debugMode(debugMode),
      m_broadcaster(m_players, m_debugMode) {
  if (!loadMap()) {
    throw Shared::Exceptions::MapLoaderException("Failed to load map file: " +
                                                 m_mapFile.string());
  }
  initializeSocket();
}

Jetpack::Server::GameServer::~GameServer() {
  for (const auto &[playerSocket, _] : m_players) {
    close(playerSocket);
  }
  close(m_serverSocket);
}

void Jetpack::Server::GameServer::start() {
  while (m_running) {
    const int ready = poll(m_pollfds.data(), m_pollfds.size(), GAME_TICK_MS);

    if (ready < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    handleSocketEvents();
    updateGameState();
  }
}

bool Jetpack::Server::GameServer::loadMap() {
  std::ifstream file(m_mapFile);
  if (!file.is_open()) {
    return false;
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }

  if (lines.empty()) {
    return false;
  }

  m_map.height = lines.size();
  m_map.width = lines[0].length();

  for (const auto &line : lines) {
    if (line.length() != static_cast<size_t>(m_map.width)) {
      return false;
    }
  }

  m_map.tiles.resize(
      m_map.height,
      std::vector(m_map.width, Shared::Protocol::TileType::EMPTY));
  m_map.coinStates.resize(
      m_map.height,
      std::vector(m_map.width, Shared::Protocol::CoinState::AVAILABLE));

  for (int y = 0; y < m_map.height; y++) {
    for (int x = 0; x < m_map.width; x++) {
      switch (lines[y][x]) {
      case '_':
        m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
        break;
      case 'c':
        m_map.tiles[y][x] = Shared::Protocol::TileType::COIN;
        break;
      case 'e':
        m_map.tiles[y][x] = Shared::Protocol::TileType::ELECTRICSQUARE;
        break;
      default:
        m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
        break;
      }
    }
  }

  return true;
}

void Jetpack::Server::GameServer::initializeSocket() {
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_serverSocket < 0) {
    throw Shared::Exceptions::SocketException("Failed to create socket");
  }

  const int flags = fcntl(m_serverSocket, F_GETFL, 0);
  fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

  const int opt = 1;
  setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(m_port);

  if (bind(m_serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    close(m_serverSocket);
    throw Shared::Exceptions::SocketException("Failed to bind socket");
  }

  if (listen(m_serverSocket, MAX_CLIENTS) < 0) {
    close(m_serverSocket);
    throw Shared::Exceptions::SocketException("Failed to listen on socket");
  }

  const pollfd pfd = {m_serverSocket, POLLIN, 0};
  m_pollfds.push_back(pfd);
}

void Jetpack::Server::GameServer::handleSocketEvents() {
  for (size_t i = 0; i < m_pollfds.size(); i++) {
    if (m_pollfds[i].revents & POLLIN) {
      if (m_pollfds[i].fd == m_serverSocket) {
        acceptNewClient();
      } else {
        handleClientData(m_pollfds[i].fd);
      }
    } else if (m_pollfds[i].revents & (POLLHUP | POLLERR)) {
      if (m_pollfds[i].fd != m_serverSocket) {
        handleClientDisconnect(m_pollfds[i].fd);
        i--;
      }
    }
  }
}

void Jetpack::Server::GameServer::acceptNewClient() {
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);

  int clientSocket =
      accept(m_serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (clientSocket < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
    }
    return;
  }

  const int flags = fcntl(clientSocket, F_GETFL, 0);
  fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

  const pollfd pfd = {clientSocket, POLLIN, 0};
  m_pollfds.push_back(pfd);

  const int newPlayerId = m_players.size() + 1;
  m_players.emplace(clientSocket,
                    Shared::Protocol::Player(clientSocket, newPlayerId));

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddr.sin_addr, client_ip, INET_ADDRSTRLEN);

  sendConnectResponse(clientSocket, newPlayerId);
  sendMapData(clientSocket);

  checkGameStart();
}

void Jetpack::Server::GameServer::handleClientDisconnect(
    const int clientSocket) {
  const auto it = m_players.find(clientSocket);
  if (it != m_players.end()) {
    m_players.erase(it);
  }

  for (size_t i = 0; i < m_pollfds.size(); i++) {
    if (m_pollfds[i].fd == clientSocket) {
      close(clientSocket);
      m_pollfds.erase(m_pollfds.begin() + i);
      break;
    }
  }

  if (m_gameState == Shared::Protocol::GameState::IN_PROGRESS) {
    int activePlayers = 0;
    for (const auto &[_, player] : m_players) {
      if (player.getState() == Shared::Protocol::PlayerState::PLAYING) {
        activePlayers++;
      }
    }

    if (activePlayers < MIN_PLAYERS) {
      m_gameState = Shared::Protocol::GameState::GAME_OVER;
      m_broadcaster.broadcastGameOver();
    }
  }
}

void Jetpack::Server::GameServer::handleClientData(int clientSocket) {
  uint8_t buffer[BUFFER_SIZE];
  ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);

  if (bytesRead <= 0) {
    if (bytesRead == 0 ||
        (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      handleClientDisconnect(clientSocket);
    }
    return;
  }

  if (m_debugMode) {
    std ::cout << std::format(
        "Debug: Received {} bytes from client {}: ", bytesRead, clientSocket);
    for (ssize_t i = 0; i < bytesRead; i++) {
      std::cout << std::format("{:02X} ", buffer[i]);
    }
    std::cout << std::endl;
  }

  processPacket(clientSocket, buffer, bytesRead);
}

void Jetpack::Server::GameServer::processPacket(const int clientSocket,
                                                const uint8_t *data,
                                                const size_t length) {
  if (length < 1)
    return;

  const Shared::Protocol::PacketType type =
      static_cast<Shared::Protocol::PacketType>(data[0]);

  switch (type) {
  case Shared::Protocol::PacketType::CONNECT_REQUEST:
    break;
  case Shared::Protocol::PacketType::PLAYER_INPUT:
    handlePlayerInput(clientSocket, data, length);
    break;
  case Shared::Protocol::PacketType::PLAYER_DISCONNECT:
    handleClientDisconnect(clientSocket);
    break;
  default:
    break;
  }
}

void Jetpack::Server::GameServer::handlePlayerInput(const int clientSocket,
                                                    const uint8_t *data,
                                                    const size_t length) {
  if (length < 2)
    return;

  const bool isJetpacking = data[1] != 0;

  const auto it = m_players.find(clientSocket);
  if (it != m_players.end() &&
      it->second.getState() == Shared::Protocol::PlayerState::PLAYING) {
    it->second.setJetpacking(isJetpacking);
  }
}

void Jetpack::Server::GameServer::sendConnectResponse(int clientSocket,
                                                      int playerId) const {

  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::CONNECT_RESPONSE);
  packet.addByte(static_cast<uint8_t>(playerId));
  packet.addByte(static_cast<uint8_t>(m_players.size()));

  std::vector<std::byte> buffer = packet.serialize();

  send(clientSocket, buffer.data(), buffer.size(), 0);

  if (m_debugMode) {
    std::cout << std::format("Debug: Sent connection response to client {} "
                             "(Player ID: {}): ",
                             clientSocket, playerId);
    for (size_t i = 0; i < sizeof(buffer); i++) {
      std::cout << std::format("{:02X} ",
                               static_cast<unsigned char>(buffer[i]));
    }
    std::cout << std::endl;
  }
}

void Jetpack::Server::GameServer::sendMapData(int clientSocket) {
  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::MAP_DATA);

  packet.addShort(static_cast<uint16_t>(m_map.width));
  packet.addShort(static_cast<uint16_t>(m_map.height));

  for (int y = 0; y < m_map.height; y++) {
    for (int x = 0; x < m_map.width; x++) {
      packet.addByte(static_cast<uint8_t>(m_map.tiles[y][x]));
    }
  }
  for (int y = 0; y < m_map.height; y++) {
    for (int x = 0; x < m_map.width; x++) {
      packet.addByte(static_cast<uint8_t>(m_map.coinStates[y][x]));
    }
  }
  std::vector<std::byte> buffer = packet.serialize();
  send(clientSocket, buffer.data(), buffer.size(), 0);

  if (m_debugMode) {
    std::cout << std::format("Debug: Sent map data to client {}: ",
                             clientSocket);
    for (size_t i = 0; i < sizeof(buffer); i++) {
      std::cout << std::format("{:02X} ",
                               static_cast<unsigned char>(buffer[i]));
    }
    std::cout << std::endl;
  }
}

void Jetpack::Server::GameServer::checkGameStart() {
  if (m_gameState != Shared::Protocol::GameState::WAITING_FOR_PLAYERS) {
    return;
  }

  const int readyPlayersCount = m_players.size();

  if (readyPlayersCount >= MIN_PLAYERS) {
    m_gameState = Shared::Protocol::GameState::IN_PROGRESS;

    for (auto &[_, player] : m_players) {
      player.setState(Shared::Protocol::PlayerState::READY);
      player.setPosition(1.0f, m_map.height - 2.0f);
    }

    m_broadcaster.broadcastGameStart();
    m_broadcaster.broadcastGameState();
  }
}

void Jetpack::Server::GameServer::updateGameState() {
  if (m_gameState != Shared::Protocol::GameState::IN_PROGRESS) {
    return;
  }

  bool allReady = true;
  bool anyPlaying = false;

  for (const auto &[_, player] : m_players) {
    if (player.getState() == Shared::Protocol::PlayerState::PLAYING) {
      anyPlaying = true;
    } else if (player.getState() == Shared::Protocol::PlayerState::READY) {
    } else {
      allReady = false;
    }
  }

  if (allReady && !anyPlaying) {
    for (auto &[_, player] : m_players) {
      if (player.getState() == Shared::Protocol::PlayerState::READY) {
        player.setState(Shared::Protocol::PlayerState::PLAYING);
      }
    }
    m_broadcaster.broadcastGameState();
    return;
  }

  updatePlayers();
  checkCollisions();
  m_broadcaster.broadcastGameState();
  checkGameEnd();
}

void Jetpack::Server::GameServer::updatePlayers() {
  for (auto &[_, player] : m_players) {
    if (player.getState() != Shared::Protocol::PlayerState::PLAYING) {
      continue;
    }

    Physics::applyPhysics(player);
    Physics::checkBounds(player, m_map);

    if (player.getPosition().x >= m_map.width) {
      player.setState(Shared::Protocol::PlayerState::FINISHED);
    }
  }
}

void Jetpack::Server::GameServer::checkCollisions() {
  for (auto &[_, player] : m_players) {
    if (player.getState() != Shared::Protocol::PlayerState::PLAYING) {
      continue;
    }

    const int cell_x = static_cast<int>(player.getPosition().x);
    const int cell_y = static_cast<int>(player.getPosition().y);

    if (cell_x >= 0 && cell_x < m_map.width && cell_y >= 0 &&
        cell_y < m_map.height) {
      const Shared::Protocol::TileType tile = m_map.tiles[cell_y][cell_x];

      if (tile == Shared::Protocol::TileType::COIN) {
        const Shared::Protocol::CoinState currentState =
            m_map.coinStates[cell_y][cell_x];
        const int playerId = player.getId();
        bool alreadyCollected = false;

        if (playerId == 1 &&
            (currentState == Shared::Protocol::CoinState::COLLECTED_P1 ||
             currentState == Shared::Protocol::CoinState::COLLECTED_BOTH)) {
          alreadyCollected = true;
        } else if (playerId == 2 &&
                   (currentState == Shared::Protocol::CoinState::COLLECTED_P2 ||
                    currentState ==
                        Shared::Protocol::CoinState::COLLECTED_BOTH)) {
          alreadyCollected = true;
        }

        if (!alreadyCollected) {
          player.setScore(player.getScore() + 1);
          if (currentState == Shared::Protocol::CoinState::AVAILABLE) {
            m_map.coinStates[cell_y][cell_x] =
                (playerId == 1) ? Shared::Protocol::CoinState::COLLECTED_P1
                                : Shared::Protocol::CoinState::COLLECTED_P2;
          } else if (currentState ==
                         Shared::Protocol::CoinState::COLLECTED_P1 &&
                     playerId == 2) {
            m_map.coinStates[cell_y][cell_x] =
                Shared::Protocol::CoinState::COLLECTED_BOTH;
            m_map.tiles[cell_y][cell_x] = Shared::Protocol::TileType::EMPTY;
          } else if (currentState ==
                         Shared::Protocol::CoinState::COLLECTED_P2 &&
                     playerId == 1) {
            m_map.coinStates[cell_y][cell_x] =
                Shared::Protocol::CoinState::COLLECTED_BOTH;
            m_map.tiles[cell_y][cell_x] = Shared::Protocol::TileType::EMPTY;
          }
          m_broadcaster.broadcastCoinCollected(
              player.getId(), cell_x, cell_y,
              static_cast<int>(m_map.coinStates[cell_y][cell_x]));
        }
      } else if (tile == Shared::Protocol::TileType::ELECTRICSQUARE) {
        player.setState(Shared::Protocol::PlayerState::DEAD);
        m_broadcaster.broadcastPlayerDeath(player.getId());
      }
    }
  }
}

void Jetpack::Server::GameServer::checkGameEnd() {
  bool allFinished = true;
  bool anyDead = false;
  int activePlayersCount = 0;

  for (const auto &[_, player] : m_players) {
    if (player.getState() == Shared::Protocol::PlayerState::PLAYING) {
      allFinished = false;
      activePlayersCount++;
    } else if (player.getState() == Shared::Protocol::PlayerState::FINISHED) {
      activePlayersCount++;
    } else if (player.getState() == Shared::Protocol::PlayerState::DEAD) {
      anyDead = true;
    }
  }

  if ((allFinished && activePlayersCount > 0) || anyDead ||
      (activePlayersCount < MIN_PLAYERS && m_players.size() >= MIN_PLAYERS)) {
    m_gameState = Shared::Protocol::GameState::GAME_OVER;

    int winnerId = -1;
    int highestScore = -1;

    for (const auto &[_, player] : m_players) {
      if (anyDead && player.getState() != Shared::Protocol::PlayerState::DEAD) {
        winnerId = player.getId();
        break;
      }

      if (player.getScore() > highestScore) {
        highestScore = player.getScore();
        winnerId = player.getId();
      }
    }

    m_broadcaster.broadcastGameOver(winnerId);
    resetGame();
  }
}

void Jetpack::Server::GameServer::resetGame() {
  for (auto const &[sock, _] : m_players) {
    ::close(sock);
  }
  m_players.clear();

  m_pollfds.clear();
  m_pollfds.push_back({m_serverSocket, POLLIN, 0});

  if (!loadMap()) {
    throw Shared::Exceptions::MapLoaderException("Failed to load map file: " +
                                                 m_mapFile.string());
  }

  m_gameState = Shared::Protocol::GameState::WAITING_FOR_PLAYERS;
}
