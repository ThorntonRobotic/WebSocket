#pragma once 

//#define ASIO_STANDALONE
//#define ASIO_WINDOWS
//#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <functional>

#include "logger.hpp"
#include "wsPayload.hpp"

typedef websocketpp::client<websocketpp::config::asio_client> client;


namespace wsClient
{
    // pull out the type of messages sent by our config
    typedef client::message_ptr message_ptr;

    template<class T>
    class Endpoint {
    public:
        typedef std::function<void  (Endpoint* s, websocketpp::connection_hdl hdl, wsPayload::JsonPayload::valueType & p)> messageCallback;
        typedef std::function<void  (Endpoint* s, websocketpp::connection_hdl hdl)> openCallback;  
        typedef std::function<void  (Endpoint* s, websocketpp::connection_hdl hdl)> closeCallback;

    private:
        client endpoint_;
        websocketpp::connection_hdl hdl_;
        T payload_;
        Logger log_;
        std::string status_;
        std::string uri_;
        std::string server_;
        std::string errorReason_;
        websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
    
        // Application Callbacks
        messageCallback messageCallback_;
        openCallback openCallback_;
        closeCallback closeCallback_;
  
    public:
        Endpoint() = delete;

        Endpoint(std::string uri, Logger & log):
        endpoint_(),
        hdl_(),
        payload_(),
        log_(log),
        status_("Idle"),
        uri_(uri),
        server_("N/A"),
        errorReason_(""),
        thread_(),
        messageCallback_(nullptr),
        openCallback_(nullptr),
        closeCallback_(nullptr)
        {
            // Set logging settings
            disableDebug();

            // Initialize Asio
            endpoint_.init_asio();
            endpoint_.start_perpetual();
            thread_ = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &endpoint_);


            // Set the default handlers 
            endpoint_.set_message_handler(std::bind(&Endpoint::onMessage, this, std::placeholders::_1, std::placeholders::_2));
            endpoint_.set_open_handler(std::bind(&Endpoint::onOpen, this,std::placeholders::_1));
            endpoint_.set_fail_handler(std::bind(&Endpoint::onFail, this, websocketpp::lib::placeholders::_1));
            endpoint_.set_close_handler(std::bind(&Endpoint::onClose, this, websocketpp::lib::placeholders::_1));
        }

        ~Endpoint(){
            // Shutdown
            close(websocketpp::close::status::going_away, "");
            thread_->join();
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

        std::string getStatus() const {
            return status_;
        }

        void enableDebug(){
            endpoint_.set_error_channels(websocketpp::log::elevel::all);
            endpoint_.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);
        }

        void disableDebug(){
            endpoint_.set_error_channels(websocketpp::log::elevel::none);
            endpoint_.set_access_channels(websocketpp::log::alevel::none);
        }

        void onMessage(websocketpp::connection_hdl hdl, client::message_ptr msg) {
             if (hdl.lock()){      // Ensure pointer is valid
                 log_.debug("onMessage : %s opcode: %x\n", msg->get_payload().c_str(), msg->get_opcode());
            
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
                status_ = "Idle";
            }
        }

        void onFail(websocketpp::connection_hdl hdl) {
            if (hdl.lock()){
                log_.debug("onFail : called with hdl: %x", hdl); 
                status_ = "Fail"; 
                client::connection_ptr con = endpoint_.get_con_from_hdl(hdl);
                errorReason_ = con->get_ec().message();
                server_ = con->get_response_header("Server");
            }
        }
        
        void onOpen(websocketpp::connection_hdl hdl) {
            if (hdl.lock()){
                log_.debug("onOpen : called with hdl: %x", hdl); 
                status_ = "Open";
                client::connection_ptr con = endpoint_.get_con_from_hdl(hdl);
                errorReason_ = con->get_ec().message();
                server_ = con->get_response_header("Server");
                hdl_ = hdl;
                if (openCallback_){
                        openCallback_(this, hdl);
                }
                else {
                     log_.debug("onOpen : no callback function");  
                 } 
            } 
        }

        bool connect() {
            websocketpp::lib::error_code ec;
            if (status_ != "Idle"){
                log_.debug("connect : connection already attempted");
                return false;  
            }

            client::connection_ptr con = endpoint_.get_connection(uri_, ec);
            if (ec) {
                std::cout << "> Connect initialization error: " << ec.message() << std::endl;
                return false;
            }
            status_ = "Connecting";
            endpoint_.connect(con);

            return true;
        }

        void close(websocketpp::close::status::value code, std::string reason) {
            websocketpp::lib::error_code ec;
            if (hdl_.lock()){
                log_.debug("close : called with hdl: %x", hdl_); 
                endpoint_.close(hdl_, code, reason, ec);
                if (ec) {
                    log_.error("> Error initiating close: %s", ec.message());
                }
            }
            else{
               log_.error("close : called fail, unable to lock hdl: %x\n ", hdl_);  
            }
        }

        void send(std::string  message) {
            websocketpp::lib::error_code ec;
            if (hdl_.lock()){
                std::string encoded = payload_.encode(message);
                endpoint_.send(hdl_, message, websocketpp::frame::opcode::text, ec);
                if (ec) {
                    log_.error("> Error sending message: %s", ec.message());
                }
            }        
            else{
               log_.error("send : message fail, unable to lock hdl: %x", hdl_);  
            }
        }

        void send(typename T::valueType &message) {
            std::string serialized = payload_.serialize(message);
            send(serialized);
        }
    };
};
