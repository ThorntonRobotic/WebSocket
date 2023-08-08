#include "wsEndpoint.hpp"


// Define a callback to handle incoming messages
void on_message (wsEndpoint::Server<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    std::cout << "decoded: " << p << std::endl; 
    try {
        s->send(hdl, p);
    } catch (websocketpp::exception const & e) {
        std::cout << "Echo failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

int main() {
    wsEndpoint::Server<wsPayload::JsonPayload> s(9002);
    s.setMessageCallback(&on_message);
    s.run();
    return 0;
}