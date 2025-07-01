#include <cassert>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/frame.hpp>
#include <websocketpp/utilities.hpp>
#include "socket.h"

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

static int clientSocket;
static client c;

void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
    websocketpp::lib::error_code errorcode;

    // start sending packets first
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("Message from client: %s\n", buffer);

    c->send(hdl, buffer, websocketpp::frame::opcode::binary, errorcode);

    if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        printf("Got payload! %s | %zu\n", msg->get_payload().c_str(), msg->get_payload().size());

        send(clientSocket, msg->get_payload().data(), msg->get_payload().size(), 0);
    }


    // if (msg->get_opcode() == websocketpp::frame::opcode::text) {
    //     printf("Got payload! %s\n", msg->get_payload().c_str());
    // } else {
    //     printf("Got payload! %s\n", websocketpp::utility::to_hex(msg->get_payload()).c_str());
    // }
    
    // c->send(hdl, "cock", websocketpp::frame::opcode::binary, errorcode);
    // c->close(hdl, websocketpp::close::status::normal, "done");
}

void on_connect(client* c, websocketpp::connection_hdl hdl) {
    printf("Websocket conection ready! Now starting local socket connection...\n");
    clientSocket = socket_listen(socket_bind(INADDR_ANY, 8080));
    printf("done!\n");
}

void connect(std::string url) {
    websocketpp::lib::error_code errorcode;
    
    // set all logging to all execpt payload 
    c.set_access_channels(websocketpp::log::alevel::all);
    c.clear_access_channels(websocketpp::log::alevel::frame_payload);
    c.init_asio();
    
    c.set_message_handler(websocketpp::lib::bind(
        on_message,
        &c,
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2
    ));

    c.set_open_handler(websocketpp::lib::bind(
        &on_connect,
        &c,
        websocketpp::lib::placeholders::_1
    ));
    
    // make a connection 
    client::connection_ptr connection = c.get_connection(url, errorcode);
    if (errorcode) {
        fprintf(stderr, "Could not create connection because: %s\n", errorcode.message().c_str());
        exit(1);
    }

    c.connect(connection);
};

void run() {
    c.run();
}