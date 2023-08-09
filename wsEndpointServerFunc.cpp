#include "wsServerEndpoint.hpp"
#include "logger.hpp"

// Test program for endpoint server using functions as the callback agent


// Define a callback to handle incoming messages
void on_message (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    std::cout << "decoded: " << p << std::endl; 
    try {
        s->send(hdl, p);
    } catch (websocketpp::exception const & e) {
        std::cout << "Echo failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

void on_open (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {
    std::cout << "onOpen " << std::endl; 
    try {
        std::string msg = "{\"message\":\"open\"}";
        s->send(hdl, msg);
    } catch (websocketpp::exception const & e) {
        std::cout << "Echo failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

using namespace std::placeholders;

int main() {
    Logger log("", "");
    wsServer::Endpoint<wsPayload::JsonPayload> s(9002, log);

    wsServer::Endpoint<wsPayload::JsonPayload>::messageCallback msgCb = std::bind(on_message, _1, _2, _3);
    wsServer::Endpoint<wsPayload::JsonPayload>::openCallback openCb = std::bind(on_open, _1, _2);
    s.setMessageCallback(msgCb);
    s.setOpenCallback(openCb);
    s.run();
    return 0;
}