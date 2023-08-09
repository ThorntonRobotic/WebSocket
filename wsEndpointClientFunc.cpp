#include "wsClientEndpoint.hpp"
#include "logger.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// Test program for endpoint server using functions as the callback agent


// Define a callback to handle incoming messages
void on_message (wsClient::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    std::cout << "decoded: " << p << std::endl; 
}

void on_open (wsClient::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {
    std::cout << "onOpen " << std::endl; 
}

using namespace std::placeholders;

int main() {
    Logger log("", "");
 
    wsClient::Endpoint<wsPayload::JsonPayload>::messageCallback msgCb = std::bind(on_message, _1, _2, _3);
    wsClient::Endpoint<wsPayload::JsonPayload>::openCallback openCb = std::bind(on_open, _1, _2);
    wsClient::Endpoint<wsPayload::JsonPayload> s("ws://127.0.0.1:9002", log);    
    s.setMessageCallback(msgCb);
    s.setOpenCallback(openCb);
    s.connect();

    wsClient::Endpoint<wsPayload::JsonPayload> t("ws://127.0.0.1:9002", log);    
    t.setMessageCallback(msgCb);
    t.setOpenCallback(openCb);
    t.connect();

    while(1){
        sleep(1);
        s.send("{\"Message\":\"Client test s\"}");
        t.send("{\"Message\":\"Client test t\"}");
    }
    return 0;
}