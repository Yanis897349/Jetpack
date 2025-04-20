/**
 * @file Exceptions.hpp
 * @brief Defines exception types for the Jetpack project.
 */

#pragma once

#include <exception>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace Jetpack::Shared::Exceptions {

/**
 * @class Exception
 * @brief Base class for all Jetpack exceptions.
 */
class Exception : public std::exception {
public:
  /**
   * @brief Construct with an error message.
   * @param message The explanatory message.
   */
  explicit Exception(std::string message) : m_message(std::move(message)) {}

  /**
   * @brief Retrieve the exception message.
   * @return C‑string describing the error.
   */
  [[nodiscard]] const char *what() const noexcept override {
    return m_message.c_str();
  }

protected:
  std::string m_message;
};

/**
 * @class SocketException
 * @brief Exception thrown on socket creation/bind/listen errors.
 */
class SocketException : public Exception {
public:
  /**
   * @brief Construct from errno and custom message.
   * @param message    Description of the failure.
   */
  explicit SocketException(const std::string_view message)
      : Exception(std::string("Socket error: ").append(message)) {
    m_errorCode = errno;
  }

  /**
   * @brief Construct from given error code and message.
   * @param message    Description of the failure.
   * @param errorCode  Numeric system error code.
   */
  explicit SocketException(const std::string_view message, const int errorCode)
      : Exception(std::string("Socket error: ").append(message)),
        m_errorCode(errorCode) {}

  /**
   * @brief Get the stored error code.
   * @return The POSIX errno or provided errorCode.
   */
  [[nodiscard]] int getErrorCode() const noexcept { return m_errorCode; }

  /**
   * @brief Get a human‑readable string for the error code.
   * @return System‑provided error message.
   */
  [[nodiscard]] std::string getErrorMessage() const {
    return std::system_category().message(m_errorCode);
  }

private:
  int m_errorCode = 0;
};

/**
 * @class GameServerException
 * @brief General exception for GameServer failures.
 */
class GameServerException : public Exception {
public:
  /**
   * @brief Construct with a descriptive message.
   * @param message Explanation of the error.
   */
  explicit GameServerException(const std::string_view message)
      : Exception(std::string("Game server error: ").append(message)) {}
};

/**
 * @class MapLoaderException
 * @brief Exception for failures loading or parsing a map file.
 */
class MapLoaderException : public GameServerException {
public:
  /**
   * @brief Construct with custom message.
   * @param message Explanation of the map loader failure.
   */
  explicit MapLoaderException(const std::string_view message)
      : GameServerException(std::string("Map loader error: ").append(message)) {
  }

  /**
   * @brief Construct citing a specific file path and error.
   * @param path    Filesystem path that failed to load.
   * @param message Detail of the failure.
   */
  explicit MapLoaderException(const std::filesystem::path &path,
                              const std::string_view message)
      : GameServerException(std::string("Map loader error: Failed to load '")
                                .append(path.string())
                                .append("': ")
                                .append(message)) {}
};

/**
 * @class ProtocolException
 * @brief Exception for protocol framing or parsing errors.
 */
class ProtocolException : public Exception {
public:
  /**
   * @brief Construct with protocol error detail.
   * @param message Explanation of protocol violation.
   */
  explicit ProtocolException(const std::string_view message)
      : Exception(std::string("Protocol error: ").append(message)) {}
};

/**
 * @class ResourceException
 * @brief Exception for failures loading external resources.
 */
class ResourceException : public Exception {
public:
  /**
   * @brief Construct with a message.
   * @param message Explanation of the resource failure.
   */
  explicit ResourceException(const std::string_view message)
      : Exception(std::string("Resource error: ").append(message)) {}

  /**
   * @brief Construct citing a specific file path and error.
   * @param path    Filesystem path that failed to load.
   * @param message Detail of the failure.
   */
  explicit ResourceException(const std::filesystem::path &path,
                             const std::string_view message)
      : Exception(std::string("Resource error: Failed to load '")
                      .append(path.string())
                      .append("': ")
                      .append(message)) {}
};

} // namespace Jetpack::Shared::Exceptions
