#pragma once 

#define ASIO_STANDALONE
//#define ASIO_WINDOWS
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>

#include "logger.hpp"
#include "wsPayload.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

namespace wsServer
{
    // pull out the type of messages sent by our config
    typedef server::message_ptr message_ptr;

    template<class T>
    class Endpoint {
    public:
        typedef std::function<void  (Endpoint* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p)> messageCallback;
        typedef std::function<void  (Endpoint* s, websocketpp::connection_hdl hdl)> openCallback;  
        typedef std::function<void  (Endpoint* s, websocketpp::connection_hdl hdl)> closeCallback;

    private:
        int port_;
        server endpoint_;
        int connections_;
        T payload_;
        Logger log_;

        // Application Callbacks
        messageCallback messageCallback_;
        openCallback openCallback_;
        closeCallback closeCallback_;
  
    public:
        Endpoint() = delete;

        Endpoint(int port, Logger & log):
        port_(port),
        endpoint_(),
        connections_(0),
        payload_(),
        log_(log),
        messageCallback_(nullptr),
        openCallback_(nullptr),
        closeCallback_(nullptr)
        {
            // Set logging settings
            disableDebug();

            // Initialize Asio
            endpoint_.init_asio();

            // Set the default handlers 
            endpoint_.set_message_handler(std::bind(&Endpoint::onMessage, this, std::placeholders::_1, std::placeholders::_2));
            endpoint_.set_open_handler(std::bind(&Endpoint::onOpen, this,std::placeholders::_1));
            endpoint_.set_fail_handler(std::bind(&Endpoint::onFail, this, websocketpp::lib::placeholders::_1));
            endpoint_.set_close_handler(std::bind(&Endpoint::onClose, this, websocketpp::lib::placeholders::_1));
        }

        ~Endpoint(){
                // Shutdown
            endpoint_.stop();
        }

        // Allow application callbacks
        void setMessageCallback( messageCallback cb) {
            messageCallback_ = cb;
        }

        void setOpenCallback( openCallback cb) {
            openCallback_ = cb;
        }

        void setCloseCallback( closeCallback cb) {
            closeCallback_ = cb;
        }

        void enableDebug(){
            endpoint_.set_error_channels(websocketpp::log::elevel::all);
            endpoint_.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);
        }

        void disableDebug(){
            endpoint_.set_error_channels(websocketpp::log::elevel::none);
            endpoint_.set_access_channels(websocketpp::log::alevel::none);
        }

        void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
             if (hdl.lock()){      // Ensure pointer is valid
                 log_.debug("onMessage : %s opcode: %x", msg->get_payload().c_str(), msg->get_opcode());
            
                if ( payload_.decode(msg->get_payload())){
                    auto v = payload_.getValue();
                    // There should always be callback 
                    if (messageCallback_){
                        messageCallback_(this, hdl, v);
                    }
                    else {
                         log_.error("onMessage : no callback function");  
                    }
                }
                else{
                    log_.error("onMessage : decode error in payload");  
                }
             }
        }
    
        void onClose(websocketpp::connection_hdl hdl) {
            if (hdl.lock()){
                 log_.debug("onClose : called with hdl: %x", hdl);
                 --connections_ ;        
            }
        }

        void onFail(websocketpp::connection_hdl hdl) {
            if (hdl.lock()){
                 log_.debug("onFail : called with hdl: %x", hdl);      
            }
        }
        
        void onOpen(websocketpp::connection_hdl hdl) {
            if (hdl.lock()){
                 log_.debug("onOpen : called with hdl: %x", hdl); 
                 ++connections_;  
                if (openCallback_){
                        openCallback_(this, hdl);
                    }
                    else {
                         log_.debug("onOpen : no callback function");  
                    } 
            } 
        }

        void close(websocketpp::connection_hdl & hdl, websocketpp::close::status::value code, std::string reason) {
            websocketpp::lib::error_code ec;
            if (hdl.lock()){
                log_.debug("close : called with hdl: %x", hdl); 
                endpoint_.close(hdl, code, reason, ec);
                if (ec) {
                    log_.error("> Error initiating close: %s", ec.message());
                }
            }
            else{
               log_.error("close : called fail, unable to lock hdl: %x\n ", hdl);  
            }
        }

        void send(websocketpp::connection_hdl & hdl, std::string message) {
            websocketpp::lib::error_code ec;
            if (hdl.lock()){
                log_.debug("send : called with hdl: %x", hdl); 
                std::string encoded = payload_.encode(message);
                endpoint_.send(hdl, message, websocketpp::frame::opcode::text, ec);
                if (ec) {
                    log_.error("> Error sending message: %s", ec.message());
                }
            }        
            else{
               log_.error("send : message fail, unable to lock hdl: %x", hdl);  
            }
        }

        void send(websocketpp::connection_hdl & hdl,  typename T::valueType &message) {
            log_.debug("send(object) : called with hdl: %x", hdl); 
            std::string serialized = payload_.serialize(message);
            send(hdl, serialized);
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
