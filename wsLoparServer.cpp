#include "wsServerEndpoint.hpp"
#include "logger.hpp"
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include<conio.h>
#else
#include <unistd.h>
#endif

// Test program for endpoint server using functions as the callback agent

Logger log_("", "");
int messages = 0;

class DummyAgent{
public:
    // Define a callback to handle incoming messages
    void on_message (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
        log_.debug("decoded: %s", p.toStyledString().c_str());
        printf("\r Messages recieved: %d\r", ++messages); 
        if (p.isMember("steering")){
            float val =  p["steering"].asFloat();
            log_.debug("Remote steering: %f",val);
            //    onJoystickLeftX(val);
        }

        if (p.isMember("throttle")){
            float val =  p["throttle"].asFloat();
            log_.debug("Remote throttle: %f",val);
            //    onJoystickRightY(val);
        }
    }

    void on_open (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {
        log_.debug("on_open"); 
    }

    void on_close (wsServer::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {
        log_.debug("on_close"); 
    }

    void sendMessage(wsServer::Endpoint<wsPayload::JsonPayload>* s,  websocketpp::connection_hdl hdl, std::string key, float val){
        int64_t now = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();  
        Json::Value root; 
        root["timestamp"]= now;
        root[key.c_str()]=val;
        s->send(hdl, root);
    }
};

using namespace std::placeholders;

int main() {

   wsServer::Endpoint<wsPayload::JsonPayload> s(9002, log_);
  
    DummyAgent dummy;
    s.setMessageCallback(std::bind(&DummyAgent::on_message, &dummy, _1, _2, _3));
    s.setOpenCallback(std::bind(&DummyAgent::on_open, &dummy, _1, _2));
    s.setCloseCallback(std::bind(&DummyAgent::on_close, &dummy, _1, _2));
    s.run();
    
    return 0;
}