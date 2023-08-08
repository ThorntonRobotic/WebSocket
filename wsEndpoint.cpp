#define ASIO_STANDALONE
//#define ASIO_WINDOWS
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>
#include <map>

#include "wsPayload.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

namespace wsEndpoint
{

    // pull out the type of messages sent by our config
    typedef server::message_ptr message_ptr;

    // TODO:
    // config info - port, connections allowed
    template<class T>
    class Server {
    public:
        typedef  void (*messageCallback)(server* , websocketpp::connection_hdl, typename T::valueType &);

    private:
        int port_;
        server endpoint_;
        int connections_;
        T payload_;

        // Callbacks
         messageCallback messageCallback_;

  
    public:
        Server() = delete;

        Server(int port /* Logging */):
        port_(port),
        endpoint_(),
        connections_(0),
        payload_(),
        messageCallback_(nullptr)
        {
            // Set logging settings
            endpoint_.set_error_channels(websocketpp::log::elevel::all);
            endpoint_.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

            // Initialize Asio
            endpoint_.init_asio();

            // Set the default handlers 
            endpoint_.set_message_handler(std::bind(&Server::onMessage, this, std::placeholders::_1, std::placeholders::_2));
            endpoint_.set_open_handler(std::bind(&Server::onOpen, this,std::placeholders::_1));
            endpoint_.set_fail_handler(std::bind(&Server::onFail, this, websocketpp::lib::placeholders::_1));
            endpoint_.set_close_handler(std::bind(&Server::onClose, this, websocketpp::lib::placeholders::_1));
        }

        // Override default message handler
        void setMessageCallback( messageCallback cb) {
            messageCallback_ = cb;
        }

        void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
             if (hdl.lock()){      // Ensure pointer is valid
                std::cout << "onMessage hdl: " << hdl.lock().get()<< " message: " << msg->get_payload() << " opcode: " << msg->get_opcode()  << std::endl;
            
                if ( payload_.decode(msg->get_payload())){
                    auto v = payload_.getValue();
                    std::cout << "decoded: " << v << std::endl; 
                    messageCallback_(&endpoint_, hdl, v) ;
                }
                else{
                    std::cout << "decode error in payload" << std::endl;  
                }
             }
        }
    
        void onClose(websocketpp::connection_hdl hdl) {
            std::cout << "onClose called with hdl: " << hdl.lock().get()<< std::endl;
            --connections_ ;
        }

        void onFail(websocketpp::connection_hdl hdl) {
            std::cout << "onFail called with hdl: " << hdl.lock().get()<< std::endl;
        }
        

        void onOpen(websocketpp::connection_hdl hdl) {
            if (auto d = hdl.lock()){      // Ensure pointer is valid
                std::cout << "onOpen called with hdl: " << hdl.lock().get()<< " active connections:" << connections_ << std::endl;
            }
            ++connections_;
            // TODO : create callback with hdl
        }

        void close(websocketpp::connection_hdl & hdl, websocketpp::close::status::value code, std::string reason) {
            websocketpp::lib::error_code ec;
            // TODO : try
            endpoint_.close(hdl, code, reason, ec);
            if (ec) {
                std::cout << "> Error initiating close: " << ec.message() << std::endl;
            }
        }

        void send(websocketpp::connection_hdl & hdl, std::string message) {
            websocketpp::lib::error_code ec;
            // TODO : try
            std::string encoded = payload_.encode(message);
            endpoint_.send(hdl, message, websocketpp::frame::opcode::text, ec);
            if (ec) {
              std::cout << "> Error sending message: " << ec.message() << std::endl;
          }
        }

        void run() {
            endpoint_.listen(port_);

            // Queues a connection accept operation
            endpoint_.start_accept();

            // Start the Asio io_service run loop
            endpoint_.run();
        }
    };
};


// Define a callback to handle incoming messages
void on_message (server* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p) {
    std::cout << "decoded: " << p << std::endl; 
//    try {
//        s->send(hdl, msg->get_payload(), msg->get_opcode());
//    } catch (websocketpp::exception const & e) {
//        std::cout << "Echo failed because: "
//                  << "(" << e.what() << ")" << std::endl;
//    }
}

int main() {
    wsEndpoint::Server<wsPayload::JsonPayload> s(9002);
    s.setMessageCallback(&on_message);
    s.run();
    return 0;
}