#include "wsServerEndpoint.hpp"
#include "logger.hpp"

// Test program for endpoint server using functions as the callback agent
Logger log_("", "");
int messages=0;

// Define a callback to handle incoming messages
void on_message (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    log_.debug("decoded: %s", p.toStyledString().c_str());
    printf("\r Messages recieved: %d\r", ++messages); 
    try {
        s->send(hdl, p);
    } catch (websocketpp::exception const & e) {
        log_.error("failed: %s", e.what()); 
    }
}

void on_open (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {
    log_.debug("on_open");
    try {
        std::string msg = "{\"message\":\"open\"}";
        s->send(hdl, msg);
    } catch (websocketpp::exception const & e) {
        log_.error("failed: %s", e.what()); 
    }
}

using namespace std::placeholders;

int main() {

    wsServer::Endpoint<wsPayload::JsonPayload> s(9002, log_);

    wsServer::Endpoint<wsPayload::JsonPayload>::messageCallback msgCb = std::bind(on_message, _1, _2, _3);
    wsServer::Endpoint<wsPayload::JsonPayload>::openCallback openCb = std::bind(on_open, _1, _2);
    s.setMessageCallback(msgCb);
    s.setOpenCallback(openCb);
    s.run();
    return 0;
}