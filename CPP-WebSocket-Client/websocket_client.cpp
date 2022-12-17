#include <websocketpp/client.hpp>
#include <websocketpp/config/debug_asio_no_tls.hpp>

#include <iostream>

typedef websocketpp::client<websocketpp::config::debug_asio> client;

// Store information specific to each WebSocket session
class websocket_connection {
public:
    typedef websocketpp::lib::shared_ptr<websocket_connection> ptr;

    websocket_connection(websocketpp::connection_hdl hdl, std::string uri):
        m_hdl(hdl),
        m_uri(uri)
    {}

    void on_open(client *conn) {
        client::connection_ptr con = conn->get_con_from_hdl(m_hdl);
        is_connected = con->get_state() == websocketpp::session::state::open;
    }

    websocketpp::connection_hdl get_hdl() {
        return m_hdl;
    }

    bool get_state() {
        return is_connected;
    }

private:
    websocketpp::connection_hdl m_hdl;
    std::string m_uri;
    bool is_connected = false;
};

// Create and launch new connection and maintains default settings for those connection
class websocket_endpoint {
public:
    websocket_endpoint() {
        m_endpoint.set_access_channels(websocketpp::log::alevel::all);
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

    // Initiates a new connection
    int connnect(std::string uri) {

        // Create connection request
        // Fyi. connection_ptr allow direct access to information about the connection and
        //      connection configuration. In this case. This library it's not safe for end applications
        //      to use connection_ptr.
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            std::cout << "[ERROR] Can't intialize connection: " << ec.message() << std::endl;
            return -1;
        }

        m_connection.reset(new websocket_connection(con->get_handle(), uri));

        // Add hook/handler/listener
        con->set_open_handler(websocketpp::lib::bind(&websocket_connection::on_open,
            m_connection, &m_endpoint));

        // Add queue of new connection to make
        m_endpoint.connect(con);
    }

    // Retrives connection
    websocket_connection::ptr get_connection() {
        return m_connection;
    }

    // Send message to server
    void send(std::string message) {
        websocketpp::lib::error_code ec;

        m_endpoint.send(m_connection.get()->get_hdl(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "[ERROR] Sending message: " << ec.message() << std::endl;
            return;
        }
    }

private:
    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
    websocket_connection::ptr m_connection;
};

int main()
{
    websocket_endpoint endpoint;
    endpoint.connnect("ws://localhost:8765");
    websocket_connection::ptr conn = endpoint.get_connection();

    while (!conn.get()->get_state());
    endpoint.send("Hello World from WebSocket++ Client");

    while (true);
}
