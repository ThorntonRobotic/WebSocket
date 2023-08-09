#include "wsClientEndpoint.hpp"
#include "logger.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// Test program for endpoint server using functions as the callback agent

Logger log_("", "");
int messages = 0;

// Define a callback to handle incoming messages
void on_message (wsClient::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    log_.debug("decoded: %s", p.toStyledString().c_str()); 
    printf("\r Messages recieved: %d\r", ++messages);
}

void on_open (wsClient::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {

    log_.debug("onOpen"); 
}

using namespace std::placeholders;

int main() {

    wsClient::Endpoint<wsPayload::JsonPayload>::messageCallback msgCb = std::bind(on_message, _1, _2, _3);
    wsClient::Endpoint<wsPayload::JsonPayload>::openCallback openCb = std::bind(on_open, _1, _2);
    wsClient::Endpoint<wsPayload::JsonPayload> s("ws://127.0.0.1:9002", log_);    
    s.setMessageCallback(msgCb);
    s.setOpenCallback(openCb);
    s.connect();

    wsClient::Endpoint<wsPayload::JsonPayload> t("ws://127.0.0.1:9002", log_);    
    t.setMessageCallback(msgCb);
    t.setOpenCallback(openCb);
    t.connect();

    int count=0;
    char f[1024];

    while(1){
        usleep(1000);
        sprintf(f, "{\"Message\":\"Client test s(%d)\"}", ++count);
        s.send(std::string(f));
        t.send("{\"Message\":\"Client test t\"}");
    }
    return 0;
}