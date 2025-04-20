/**
 * @file NetworkClient.cpp
 * @brief Implements the NetworkClient class for server communication.
 */

#include "NetworkClient.hpp"
#include "../Shared/Exceptions.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace Jetpack::Client {

NetworkClient::NetworkClient(int serverPort, std::string serverAddress,
                             bool debugMode)
    : m_serverPort(serverPort), m_serverAddress(std::move(serverAddress)),
      m_debugMode(debugMode) {}

NetworkClient::~NetworkClient() {
  m_running = false;

  if (m_networkThread.joinable()) {
    m_networkThread.join();
  }

  if (m_serverSocket != -1) {
    ::close(m_serverSocket);
  }
}

bool NetworkClient::connectToServer() {
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_serverSocket < 0) {
    throw Shared::Exceptions::SocketException("Failed to create socket");
  }

  auto socketGuard = [this]() {
    if (m_serverSocket != -1) {
      ::close(m_serverSocket);
      m_serverSocket = -1;
    }
  };

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(m_serverPort);

  if (inet_pton(AF_INET, m_serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
    socketGuard();
    throw Shared::Exceptions::SocketException("Invalid server address");
  }

  if (::connect(m_serverSocket, reinterpret_cast<sockaddr *>(&serverAddr),
                sizeof(serverAddr)) < 0) {
    socketGuard();
    throw Shared::Exceptions::SocketException("Failed to connect to server");
  }

  const int flags = fcntl(m_serverSocket, F_GETFL, 0);
  fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::CONNECT_REQUEST);
  packet.addByte(static_cast<uint8_t>(0));

  std::vector<std::byte> buffer = packet.serialize();

  if (::send(m_serverSocket, buffer.data(), buffer.size(), 0) !=
      static_cast<ssize_t>(buffer.size())) {
    std::cerr << "Failed to send connection request" << std::endl;
    socketGuard();
    return false;
  }

  return true;
}

void NetworkClient::start() {
  m_display = std::make_shared<GameDisplay>();
  m_networkThread = std::thread(&NetworkClient::networkLoop, this);

  using namespace std::chrono_literals;

  while (m_localPlayerId == -1 && m_running) {
    std::this_thread::sleep_for(50ms);
  }

  if (m_debugMode) {
    m_display->setDebugMode(true);
  }

  if (m_localPlayerId != -1) {
    m_display->setLocalPlayerId(m_localPlayerId);
  }

  m_display->run();
  m_running = false;

  if (m_networkThread.joinable()) {
    m_networkThread.join();
  }
}

void NetworkClient::networkLoop() {
  constexpr size_t BUFFER_SIZE = 1024;
  std::array<std::byte, BUFFER_SIZE> recvBuffer{};
  std::vector<std::byte> accumulatedBuffer;
  accumulatedBuffer.reserve(BUFFER_SIZE * 2);

  using namespace std::chrono_literals;
  constexpr auto inputUpdateInterval = 16ms;

  auto lastInputUpdate = std::chrono::steady_clock::now();

  while (m_running) {
    auto currentTime = std::chrono::steady_clock::now();
    if (currentTime - lastInputUpdate >= inputUpdateInterval) {
      sendPlayerInput();
      lastInputUpdate = currentTime;
    }

    ssize_t bytesRead =
        recv(m_serverSocket, recvBuffer.data(), recvBuffer.size(), 0);
    if (bytesRead > 0) {
      if (m_debugMode) {
        std::cout << std::format("Debug: Received {} from server: ", bytesRead);

        for (ssize_t i = 0; i < bytesRead; i++) {
          std::cout << std::format("{:02X} ",
                                   static_cast<unsigned char>(recvBuffer[i]));
        }

        std::cout << std::endl;
      }

      accumulatedBuffer.insert(accumulatedBuffer.end(), recvBuffer.begin(),
                               recvBuffer.begin() + bytesRead);

      size_t processedBytes = 0;
      while (processedBytes < accumulatedBuffer.size()) {
        size_t packetSize =
            getPacketSize(accumulatedBuffer.data() + processedBytes,
                          accumulatedBuffer.size() - processedBytes);

        if (packetSize == 0) {
          break;
        }

        processPacket(accumulatedBuffer.data() + processedBytes, packetSize);
        processedBytes += packetSize;
      }

      if (processedBytes > 0) {
        accumulatedBuffer.erase(accumulatedBuffer.begin(),
                                accumulatedBuffer.begin() + processedBytes);
      }
    } else if (bytesRead < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Error reading from server: " << strerror(errno)
                  << std::endl;
        break;
      }
    }
  }
}

size_t NetworkClient::getPacketSize(const std::byte *data,
                                    size_t maxSize) const {
  if (maxSize < 1) {
    return 0;
  }

  auto packetType = static_cast<Shared::Protocol::PacketType>(data[0]);

  switch (packetType) {
  case Shared::Protocol::PacketType::CONNECT_RESPONSE:
    return (maxSize >= 3) ? 3 : 0;

  case Shared::Protocol::PacketType::MAP_DATA: {
    if (maxSize < 5) {
      return 0;
    }

    const int width = static_cast<unsigned char>(data[1]) |
                      (static_cast<unsigned char>(data[2]) << 8);
    const int height = static_cast<unsigned char>(data[3]) |
                       (static_cast<unsigned char>(data[4]) << 8);
    const size_t expectedSize = 5 + (width * height * 2);

    return (maxSize >= expectedSize) ? expectedSize : 0;
  }

  case Shared::Protocol::PacketType::GAME_START:
    return (maxSize >= 3) ? 3 : 0;

  case Shared::Protocol::PacketType::GAME_STATE_UPDATE: {
    if (maxSize < 2) {
      return 0;
    }

    int playerCount = static_cast<unsigned char>(data[1]);
    constexpr size_t playerDataSize = 10;
    size_t expectedSize = 2 + playerCount * playerDataSize;

    return (maxSize >= expectedSize) ? expectedSize : 0;
  }

  case Shared::Protocol::PacketType::COIN_COLLECTED:
    return (maxSize >= 6) ? 6 : 0;

  case Shared::Protocol::PacketType::PLAYER_DEATH:
    return (maxSize >= 2) ? 2 : 0;

  case Shared::Protocol::PacketType::GAME_OVER:
    return (maxSize >= 3) ? 3 : 0;

  case Shared::Protocol::PacketType::PLAYER_INPUT:
    return (maxSize >= 2) ? 2 : 0;

  default:
    return 0;
  }
}

void NetworkClient::processPacket(const std::byte *data, size_t length) {
  if (length < 1) {
    return;
  }

  auto packetType = static_cast<Shared::Protocol::PacketType>(data[0]);

  switch (packetType) {
  case Shared::Protocol::PacketType::CONNECT_RESPONSE:
    handleConnectResponse(data, length);
    break;
  case Shared::Protocol::PacketType::MAP_DATA:
    handleMapData(data, length);
    break;
  case Shared::Protocol::PacketType::GAME_START:
    handleGameStart(data, length);
    break;
  case Shared::Protocol::PacketType::GAME_STATE_UPDATE:
    handleGameStateUpdate(data, length);
    break;
  case Shared::Protocol::PacketType::COIN_COLLECTED:
    handleCoinCollected(data, length);
    break;
  case Shared::Protocol::PacketType::PLAYER_DEATH:
    handlePlayerDeath(data, length);
    break;
  case Shared::Protocol::PacketType::GAME_OVER:
    handleGameOver(data, length);
    break;
  default:
    break;
  }
}

void NetworkClient::handleConnectResponse(const std::byte *data,
                                          size_t length) {
  if (length < 3) {
    return;
  }

  m_localPlayerId = static_cast<unsigned char>(data[1]);
  m_running = true;
}

void NetworkClient::handleMapData(const std::byte *data, const size_t length) {
  if (length < 5) {
    return;
  }

  const int width = static_cast<unsigned char>(data[1]) |
                    (static_cast<unsigned char>(data[2]) << 8);
  const int height = static_cast<unsigned char>(data[3]) |
                     (static_cast<unsigned char>(data[4]) << 8);

  size_t minRequiredSize = 5 + (width * height * 2);
  if (length < minRequiredSize) {
    return;
  }

  m_map.width = width;
  m_map.height = height;
  m_map.tiles.resize(height, std::vector<Shared::Protocol::TileType>(width));
  m_map.coinStates.resize(height,
                          std::vector<Shared::Protocol::CoinState>(width));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      m_map.tiles[y][x] = static_cast<Shared::Protocol::TileType>(
          static_cast<unsigned char>(data[5 + y * width + x]));
    }
  }

  int coinStatesOffset = 5 + (width * height);
  if (length >= static_cast<size_t>(coinStatesOffset + width * height)) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        m_map.coinStates[y][x] = static_cast<Shared::Protocol::CoinState>(
            static_cast<unsigned char>(data[coinStatesOffset + y * width + x]));
      }
    }
  }

  if (m_display) {
    m_display->updateMap(m_map);
  }
}

void NetworkClient::handleGameStart(const std::byte *data,
                                    const size_t length) {
  if (length < 3) {
    return;
  }

  int playerCount = static_cast<unsigned char>(data[1]);

  m_players.clear();
  if (m_players.empty()) {
    for (int i = 1; i <= playerCount; i++) {
      m_players.emplace_back(-1, i);
      m_players.back().setState(Shared::Protocol::PlayerState::PLAYING);
    }

    if (m_display) {
      m_display->updateGameState(m_players);
    }
  }
}

void NetworkClient::handleGameStateUpdate(const std::byte *data,
                                          const size_t length) {
  if (length < 2) {
    return;
  }

  const int playerCount = static_cast<unsigned char>(data[1]);
  constexpr size_t playerDataSize = 10;

  if (length < 2 + playerCount * playerDataSize) {
    return;
  }

  for (int i = 0; i < playerCount; i++) {
    const int offset = 2 + i * playerDataSize;
    const int playerId = static_cast<unsigned char>(data[offset]);
    const auto state = static_cast<Shared::Protocol::PlayerState>(
        static_cast<unsigned char>(data[offset + 1]));

    const int16_t xFixedPrecision =
        static_cast<unsigned char>(data[offset + 2]) |
        (static_cast<unsigned char>(data[offset + 3]) << 8);
    const int16_t yFixedPrecision =
        static_cast<unsigned char>(data[offset + 4]) |
        (static_cast<unsigned char>(data[offset + 5]) << 8);
    const float x = xFixedPrecision / 100.0f;
    const float y = yFixedPrecision / 100.0f;

    const int score = static_cast<unsigned char>(data[offset + 6]) |
                      (static_cast<unsigned char>(data[offset + 7]) << 8);
    bool isJetpacking = static_cast<unsigned char>(data[offset + 8]) != 0;

    auto playerIt = std::find_if(
        m_players.begin(), m_players.end(),
        [playerId](const auto &player) { return player.getId() == playerId; });

    if (playerIt != m_players.end()) {
      playerIt->setState(state);
      playerIt->setPosition(x, y);
      playerIt->setScore(score);
      playerIt->setJetpacking(isJetpacking);
    } else {
      m_players.emplace_back(-1, playerId);
      m_players.back().setState(state);
      m_players.back().setPosition(x, y);
      m_players.back().setScore(score);
      m_players.back().setJetpacking(isJetpacking);
    }
  }

  if (m_display) {
    m_display->updateGameState(m_players);
  }
}

void NetworkClient::handleCoinCollected(const std::byte *data,
                                        const size_t length) const {
  if (length < 6) {
    return;
  }

  const int playerId = static_cast<unsigned char>(data[1]);
  const int x = static_cast<unsigned char>(data[2]);
  const int y = static_cast<unsigned char>(data[3]);
  const int coinState = static_cast<unsigned char>(data[5]);

  if (m_display) {
    m_display->handleCoinCollected(playerId, x, y, coinState);
  }
}

void NetworkClient::handlePlayerDeath(const std::byte *data,
                                      const size_t length) const {
  if (length < 2) {
    return;
  }

  int playerId = static_cast<unsigned char>(data[1]);

  if (m_display) {
    m_display->handlePlayerDeath(playerId);
  }
}

void NetworkClient::handleGameOver(const std::byte *data,
                                   const size_t length) const {
  if (length < 3) {
    return;
  }

  bool hasWinner = static_cast<unsigned char>(data[1]) != 0;
  int winnerId = hasWinner ? static_cast<unsigned char>(data[2]) : -1;

  if (m_display) {
    m_display->handleGameOver(winnerId);
  }
}

void NetworkClient::sendPlayerInput() const {
  if (m_serverSocket < 0 || !m_display) {
    return;
  }

  bool jetpackActive = m_display->isJetpackActive();

  Shared::Protocol::NetworkPacket packet(
      Shared::Protocol::PacketType::PLAYER_INPUT);
  packet.addByte(static_cast<uint8_t>(jetpackActive ? 1 : 0));

  std::vector<std::byte> buffer = packet.serialize();

  if (m_debugMode) {
    std::cout << std::format("Debug: Sent {} bytes to server: ", buffer.size());

    for (size_t i = 0; i < buffer.size(); i++) {
      std::cout << std::format("{:02X} ",
                               static_cast<unsigned char>(buffer[i]));
    }

    std::cout << std::endl;
  }

  send(m_serverSocket, buffer.data(), buffer.size(), 0);
}

int NetworkClient::getLocalPlayerId() const { return m_localPlayerId; }

} // namespace Jetpack::Client
