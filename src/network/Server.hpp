/**
 * @file Server.hpp
 * @brief TCP server that manages lobbies and multiplayer game sessions.
 */

#pragma once

#include "network/GameSession.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_socket.h>
#include <unordered_map>

/**
 * @brief Maximum size of a network read buffer.
 */
constexpr int BUFFER_SIZE = 4096;

/**
 * @brief Lifetime of an unjoined lobby before it is expired.
 */
constexpr int LOBBY_TTL_S = 60;

/**
 * @brief Server that accepts connections, creates lobbies, and runs Battleship matches.
 */
class GameServer {
  public:
    /**
     * @brief Creates an empty server instance.
     */
    GameServer() = default;

    /**
     * @brief Starts the server and listens on the specified TCP port.
     * @param port Port to bind and accept incoming connections on.
     */
    void run(int port);

  private:
    /**
     * @brief Client connection state tracked by the server.
     */
    struct Client {
        sockpp::tcp_socket socket; ///< Active client socket.
        std::mutex write_mutex;   ///< Serializes outbound writes to this socket.
    };

    using ClientPtr = std::shared_ptr<Client>; ///< Shared pointer to a connected client.

    /**
     * @brief Registry entry for an open lobby waiting for a second player.
     */
    struct LobbyEntry {
        ClientPtr socket; ///< Waiting client for the lobby.
        std::chrono::steady_clock::time_point created_at; ///< Time when the lobby was created.
    };

    std::mutex m_lobby_mutex; ///< Protects the lobby registry.
    std::unordered_map<std::string, LobbyEntry> m_registry; ///< All currently open lobbies awaiting a match.

    /**
     * @brief Generates a new lobby code.
     * @return A unique lobby identifier string.
     */
    static std::string make_code();

    /**
     * @brief Trims leading and trailing whitespace from a string.
     * @param s Input string.
     * @return Trimming result.
     */
    static std::string trim(std::string s);

    /**
     * @brief Reads one newline-terminated line from a socket.
     * @param sock Socket to read from.
     * @return The next message line received from the client.
     */
    std::string readLine(sockpp::tcp_socket &sock);

    /**
     * @brief Sends one newline-terminated line to a client.
     * @param c Client socket to write to.
     * @param line Message to send.
     */
    static void sendLine(Client &c, const std::string &line);

    /**
     * @brief Removes stale lobbies whose lifetime exceeds the TTL.
     */
    void reaper();

    /**
     * @brief Handles a client connection during the initial handshake phase.
     * @param sock Socket provided by the accept loop.
     */
    void handle_client(sockpp::tcp_socket sock);

    /**
     * @brief Handles creation of a new lobby for a first client.
     * @param client Client who requested a lobby.
     */
    void handle_new(ClientPtr client);

    /**
     * @brief Attempts to join an existing lobby using a code.
     * @param client Client requesting the join.
     * @param code Lobby identifier to join.
     */
    void handle_join(ClientPtr client, const std::string &code);

    /**
     * @brief Runs a full match between two clients.
     * @param p0 First player socket.
     * @param p1 Second player socket.
     */
    void runSession(ClientPtr p0, ClientPtr p1);

    /**
     * @brief Processes one incoming protocol line for a live match.
     * @param session Active game session.
     * @param sockets Player sockets for the current match.
     * @param playerIdx Index of the player who sent the message.
     * @param line Incoming serialized protocol payload.
     */
    void handleLine(GameSession &session, ClientPtr sockets[2], int playerIdx, const std::string &line);

    /**
     * @brief Broadcasts the current session state to both players.
     * @param session Active game session.
     * @param sockets Player sockets for the current match.
     */
    void pushToAll(GameSession &session, ClientPtr sockets[2]);
};
