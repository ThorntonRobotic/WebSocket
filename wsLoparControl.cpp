#include "wsClientEndpoint.hpp"
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

// Define a callback to handle incoming messages
void on_message (wsClient::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    log_.debug("decoded: %s", p.toStyledString().c_str()); 
    printf("\r Messages recieved: %d\r", ++messages);
}

void on_open (wsClient::Endpoint<wsPayload::JsonPayload>* s, websocketpp::connection_hdl hdl) {

    log_.debug("onOpen"); 
}

using namespace std::placeholders;

void sendMessage(wsClient::Endpoint<wsPayload::JsonPayload> & ws, std::string key, float val){
    int64_t now = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();  
     Json::Value root; 
    root["timestamp"]= now;
    root[key.c_str()]=val;
    ws.send(root);
}

int main() {

    wsClient::Endpoint<wsPayload::JsonPayload>::messageCallback msgCb = std::bind(on_message, _1, _2, _3);
    wsClient::Endpoint<wsPayload::JsonPayload>::openCallback openCb = std::bind(on_open, _1, _2);
    wsClient::Endpoint<wsPayload::JsonPayload> ws("ws://192.168.86.51:9002", log_);    // X2 :33  X1 :51
    ws.setMessageCallback(msgCb);
    ws.setOpenCallback(openCb);
    ws.connect();

    bool done=false;
    std::string input;
    float throttle = 0.0;
    float steering = 0.0;

    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);
 

        if (input == "quit") {
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "st [val -1 - 1]\n"
                << "th [val -1 - 1]\n"
                << "live (A) (S) left-right  (W) (z) accel-decel\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        } else if (input.substr(0,2) == "st") {
            float s = atof(input.substr(3).c_str());
            if (s >= -1.0 && s <= 1.0) {
                std::cout << "> steering " << s << std::endl;
                steering = s;
                sendMessage(ws, "steering", s);
            }
        } else if (input.substr(0,2) == "th") {
            float s = atof(input.substr(3).c_str());
            if (s >= -1.0 && s <= 1.0) {
                std::cout << "> throttle " << s << std::endl;
                throttle = s;
                sendMessage(ws, "throttle", s);
            }
        } else if (input.substr(0,4) == "live"){  
            bool liveMode = true;

            while(liveMode) {
#ifdef _WIN32
                auto c = getch();
#else           
                auto c = getchar();
#endif


                switch (c){
                    case 'a':
                    case 'A':
                        steering = (steering > -1.0 ? steering - 0.1 : steering);
                        sendMessage(ws, "steering", steering);
                        break;
                    case 's':
                    case 'S':
                        steering = (steering < 1.0 ? steering + 0.1 : steering); 
                        sendMessage(ws, "steering", steering);                  
                        break;
                    case 'w':
                    case 'W':
                        throttle = (throttle < 1.0 ? throttle + 0.1 : throttle);
                        sendMessage(ws, "throttle", throttle);
                        break;
                    case 'z':
                    case 'Z':
                        throttle = (throttle > -1.0 ? throttle - 0.1 : throttle);
                        sendMessage(ws, "throttle", throttle);                   
                        break;    
                    case 'x':
                    case 'X':
                        liveMode=false;
                        break;
                    default:
                        std::cout << c << " invalid must be a,s,w,z,x" << std::endl;
                }
            }
        } else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }
    return 0;
}