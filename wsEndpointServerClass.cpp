#include "wsServerEndpoint.hpp"
#include "logger.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <random>


// Test program for endpoint server using a class as the callback agent
Logger log_("", "");
int messages=0;

class DummyAgent{
public:
    // Define a callback to handle incoming messages
    void on_message (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
        log_.debug("decoded: %s", p.toStyledString().c_str());
        printf("\r Messages recieved: %d\r", ++messages); 
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

    void on_close (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {
        log_.debug("on_close"); 
    }

};

using namespace std::placeholders;

class Adder{
    public:
    int  add(int a, int b, int c){
        return a+b+c;
    }
};

int  add(int a, int b, int c){
        return a+b+c;
}

int main() {
    wsServer::Endpoint<wsPayload::JsonPayload> s(9002, log_);
    /* Testing code
    
    wsEndpoint::Endpoint<wsPayload::JsonPayload>::messageCallback oCb = std::bind(DummyAgent::on_message, &dummy, _1, _2, _3);

    Adder abc;
   typedef std::function<int ( int, int, int)> oI;  
   oI oIb = std::bind(Adder::add, &abc,  _1, _2, _3);
   int a = oIb(4,5,6);

   oI oJb = std::bind(add, _1, _2, _3);
   int b = oJb(8,9,10);
    */

    DummyAgent dummy;
    s.setMessageCallback(std::bind(DummyAgent::on_message, &dummy, _1, _2, _3));
    s.setOpenCallback(std::bind(DummyAgent::on_open, &dummy, _1, _2));
    s.setCloseCallback(std::bind(DummyAgent::on_close, &dummy, _1, _2));
    s.run();
    
    return 0;
}