#include <cstdio>
#include <string>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

class websocket_handler {
    public:
        websocket_handler () {
        }

        // This message handler will be invoked once for each incoming message. It
        // prints the message and then sends a copy of the message back to the server.
        static void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
            if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                printf("Got payload! %s\n", msg->get_payload().c_str());
            } else {
                printf("Got payload! %s", websocketpp::utility::to_hex(msg->get_payload()).c_str());
            }
            // c->close(hdl, websocketpp::close::status::normal, "done");
        }

        bool connect(std::string const & url) {
            // Create a error code and client endpoint
            websocketpp::lib::error_code errorcode;

            // set all logging to all execpt payload 
            client.set_access_channels(websocketpp::log::alevel::all);
            client.clear_access_channels(websocketpp::log::alevel::frame_payload);
            client.init_asio();

            client.set_message_handler(websocketpp::lib::bind(
                on_message,
                &client,
                websocketpp::lib::placeholders::_1,
                websocketpp::lib::placeholders::_2
            ));

            client::connection_ptr connection = client.get_connection(url, errorcode);
            if (errorcode) {
                fprintf(stderr, "Could not create connection because: %s\n", errorcode.message().c_str());
                return true;
            }
            
            client.connect(connection);
            return false;
        };

        void run() {
            client.run();
        }
    private:
        client client;
};