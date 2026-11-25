/**
 * @file Server.hpp
 * @brief TCP server that manages lobbies and multiplayer game sessions.
 */

#pragma once

#include "network/GameSession.hpp"
#include <asio.hpp>
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief Lifetime of an unjoined lobby before it is expired.
 */
constexpr unsigned int DEFAULT_LOBBY_TTL_S = 60;

/**
 * @brief Server that accepts connections, creates lobbies, and runs Battleship matches.
 */
class GameServer {
  public:
    /**
     * @brief Creates a server instance with the given lobby expiration timeout.
     * @param lobby_ttl Lifetime of an unjoined lobby in seconds before expiration.
     */
    explicit GameServer(unsigned int lobby_ttl = DEFAULT_LOBBY_TTL_S);

    /**
     * @brief Destroys the server and stops active I/O operations.
     */
    ~GameServer();

    GameServer(const GameServer &) = delete;
    GameServer &operator=(const GameServer &) = delete;

    /**
     * @brief Binds to the specified TCP port and runs the I/O event loop.
     * @param port Port number to listen on.
     */
    void run(int port);

    /**
     * @brief Stops the server and closes active connections and lobbies.
     */
    void stop();

    /**
     * @brief Returns the TCP port bound by the acceptor.
     * @return Bound port number, or 0 if not yet bound.
     */
    int boundPort() const { return m_bound_port; }

  private:
    /**
     * @brief Connection state and I/O buffers for a connected client.
     */
    struct Client : std::enable_shared_from_this<Client> {
        /**
         * @brief Constructs a client wrapper around an accepted socket.
         * @param s Connected TCP socket.
         */
        explicit Client(asio::ip::tcp::socket s) : socket(std::move(s)) {}

        asio::ip::tcp::socket socket;        ///< Active client socket.
        asio::streambuf read_buf;            ///< Incoming line-delimited read buffer.
        std::deque<std::string> write_queue; ///< Queue of outbound lines to write.
        bool writing = false;                ///< True while an async write is in progress.
        bool close_after_writes = false;     ///< True to close once queued writes finish.

        /**
         * @brief Protocol lifecycle states for a client.
         */
        enum class State { AwaitingRequest,
                           InLobby,
                           InSession,
                           Closed };
        State state = State::AwaitingRequest; ///< Current client state.

        std::string lobby_code;                          ///< Active lobby code if in a lobby.
        int player_idx = -1;                             ///< Player index (0 or 1) in a match.
        std::unique_ptr<asio::steady_timer> lobby_timer; ///< Expiration timer for a created lobby.
    };
    using ClientPtr = std::shared_ptr<Client>; ///< Shared pointer to a client.

    /**
     * @brief Active match state between two connected clients.
     */
    struct Session {
        /**
         * @brief Constructs a session associated with an I/O context.
         */
        explicit Session(asio::io_context &) {}
        int id = 0;           ///< Unique session identifier.
        GameSession game;     ///< Underlying Battleship game state.
        ClientPtr clients[2]; ///< Connected players for index 0 and 1.
        bool over = false;    ///< True once the match has ended.
    };
    using SessionPtr = std::shared_ptr<Session>; ///< Shared pointer to a session.

    asio::io_context m_ioc;             ///< Asio I/O execution context.
    asio::ip::tcp::acceptor m_acceptor; ///< Listener for incoming TCP connections.
    int m_bound_port = 0;               ///< Actual port bound by the acceptor.

    unsigned int m_lobby_ttl; ///< Lobby timeout in seconds.

    // all below are accessed only from the io thread.
    std::unordered_map<std::string, ClientPtr> m_registry;      ///< Open lobbies mapped by code.
    std::unordered_map<Client *, SessionPtr> m_client_sessions; ///< Active session lookup per client.
    std::unordered_set<ClientPtr> m_all_clients;                ///< All currently tracked clients.
    int m_next_session_id = 1;                                  ///< Next session ID to assign.

    /**
     * @brief Generates a random 6-digit lobby code.
     * @return Generated lobby code string.
     */
    static std::string make_code();

    /**
     * @brief Starts an asynchronous accept for the next client connection.
     */
    void accept_next();

    /**
     * @brief Handles completion of an asynchronous accept operation.
     * @param ec Error code from the accept operation.
     * @param sock Newly accepted client socket.
     */
    void on_accept(asio::error_code ec, asio::ip::tcp::socket sock);

    /**
     * @brief Starts an asynchronous read until newline for a client.
     * @param c Target client.
     */
    void read_message(ClientPtr c);

    /**
     * @brief Handles completion of an asynchronous read operation.
     * @param c Target client.
     * @param ec Error code from the read operation.
     */
    void on_read(ClientPtr c, asio::error_code ec);

    /**
     * @brief Routes a received message line based on client state.
     * @param c Source client.
     * @param line Received message line without trailing newline.
     */
    void dispatch_message(ClientPtr c, const std::string &line);

    /**
     * @brief Handles initial lobby creation or join requests.
     * @param c Requesting client.
     * @param line Raw protocol JSON line.
     */
    void handle_request(ClientPtr c, const std::string &line);

    /**
     * @brief Handles messages sent while a client is waiting in a lobby.
     * @param c Client in lobby state.
     * @param line Raw protocol JSON line.
     */
    void handle_lobby_message(ClientPtr c, const std::string &line);

    /**
     * @brief Handles in-game actions during an active session.
     * @param c Client sending the action.
     * @param line Raw protocol JSON line.
     */
    void handle_session_message(ClientPtr c, const std::string &line);

    /**
     * @brief Creates a new lobby and starts its expiration timer.
     * @param c Client creating the lobby.
     */
    void handle_create(ClientPtr c);

    /**
     * @brief Joins an existing lobby and starts a match if valid.
     * @param c Client joining the lobby.
     * @param code Target lobby code.
     */
    void handle_join(ClientPtr c, const std::string &code);

    /**
     * @brief Initializes and starts a game session between two players.
     * @param p0 First player client.
     * @param p1 Second player client.
     */
    void start_session(ClientPtr p0, ClientPtr p1);

    /**
     * @brief Cleans up lobby or session state when a client disconnects.
     * @param c Disconnected client.
     */
    void handle_disconnect(ClientPtr c);

    /**
     * @brief Enqueues a newline-terminated message for asynchronous sending.
     * @param c Destination client.
     * @param line Message payload without trailing newline.
     */
    void send_line(ClientPtr c, std::string line);

    /**
     * @brief Writes the next queued line to the client socket.
     * @param c Target client.
     */
    void write_next(ClientPtr c);

    /**
     * @brief Marks a client to close after pending writes finish.
     * @param c Target client.
     */
    void graceful_close(ClientPtr c);

    /**
     * @brief Immediately closes a client socket and removes it from tracking.
     * @param c Target client.
     */
    void close_client(ClientPtr c);

    /**
     * @brief Sends current game state snapshots to both players in a session.
     * @param s Target session.
     */
    void push_state(SessionPtr s);
};
