/**
 * @file Client.hpp
 * @brief Client-side networking interface for lobby and gameplay communication.
 */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "model/Protocol.hpp"
#include "network/NetEvent.hpp"
#include <atomic>
#include <mutex>
#include <optional>
#include <sockpp/tcp_connector.h>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief TCP client used to communicate with the Battleship server.
 *
 * The client maintains a background listening thread, serializes outbound
 * messages, and exposes queued network events for the main game loop.
 */
class NetworkClient {
  public:
    /**
     * @brief Creates a disconnected client.
     */
    NetworkClient() = default;

    /**
     * @brief Disconnects from the server and cleans up background threads.
     */
    ~NetworkClient() noexcept;

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    /**
     * @brief Connects synchronously to the server.
     * @param ip Server IP address or hostname.
     * @param port Server port number.
     * @return true if the connection succeeded, false otherwise.
     */
    bool connect(const std::string& ip, int port) noexcept;

    /**
     * @brief Starts an asynchronous connection attempt.
     *
     * A worker thread performs the connection and pushes a
     * Network::ConnectionResult into the inbox when complete.
     * @param ip Server IP address or hostname.
     * @param port Server port number.
     */
    void connectAsync(const std::string& ip, int port);

    /**
     * @brief Closes the active connection if one exists.
     *
     * Safe to call during shutdown or after a failed/incomplete connection.
     */
    void disconnect() noexcept;

    // Lobby
    /**
     * @brief Requests creation of a new lobby on the server.
     */
    void createLobby();

    /**
     * @brief Joins an existing lobby using its access code.
     * @param code Lobby code to join.
     */
    void joinLobby(const std::string& code);

    // Gameplay
    /**
     * @brief Sends a full ship placement payload to the server.
     * @param payload Ship positions to submit.
     */
    void sendPlaceShips(const std::vector<Protocol::ShipPlacement>& payload);

    /**
     * @brief Sends a single shot command to the server.
     * @param x Column index.
     * @param y Row index.
     */
    void sendFireShot(int x, int y);

    /**
     * @brief Checks whether the client is currently connected.
     * @return true if connected to the server.
     */
    [[nodiscard]] bool isConnected() const noexcept {
        return m_connected.load();
    }

    /**
     * @brief Retrieves one pending network event, if available.
     * @return The next queued event, or std::nullopt when the inbox is empty.
     */
    std::optional<Network::NetEvent> poll();

  private:
    /**
     * @brief Sends a serialized message over the socket.
     * @param line Message payload to send.
     */
    void sendMsg(const std::string& line);

    /**
     * @brief Background listener loop that reads incoming server traffic.
     */
    void listenLoop() noexcept;

    /**
     * @brief Parses a received line and queues the corresponding event.
     * @param line Raw message received from the server.
     */
    void dispatch(const std::string& line);

    /**
     * @brief Adds an event to the inbox.
     * @param ev Event to queue.
     */
    void pushEvent(Network::NetEvent ev) noexcept;

    sockpp::tcp_socket m_sock; ///< Connected socket for communication.
    std::atomic<bool> m_connected{false}; ///< Current connection state.
    std::thread m_listenThread; ///< Thread that listens for incoming messages.
    std::thread m_connectThread; ///< Thread used by connectAsync().

    std::mutex m_sendMutex; ///< Protects outbound writes.

    std::mutex m_inboxMutex; ///< Protects incoming event queue.
    std::vector<Network::NetEvent> m_inbox; ///< Pending events for the main thread.
};

#endif