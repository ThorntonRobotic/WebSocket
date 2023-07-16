#define ASIO_STANDALONE
//#define ASIO_WINDOWS
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>

#include "json/json.h"


typedef websocketpp::server<websocketpp::config::asio> server;

class wsJsonServer {
public:
    wsJsonServer() {
         // Set logging settings
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);
        m_endpoint.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        m_endpoint.init_asio();

        // Set the default message handler to the echo handler
        m_endpoint.set_message_handler(std::bind(&wsJsonServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
        m_endpoint.set_open_handler(std::bind(&wsJsonServer::onOpen, this,std::placeholders::_1));
        m_endpoint.set_fail_handler(std::bind(&wsJsonServer::onFail, this, websocketpp::lib::placeholders::_1));
        m_endpoint.set_close_handler(std::bind(&wsJsonServer::onClose, this, websocketpp::lib::placeholders::_1));
    }

    void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
        std::cout << "onMessage called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

        // write a new message      
        m_endpoint.send(hdl, msg->get_payload(), msg->get_opcode());

        Json::Value root;
        JSONCPP_STRING err;
 
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse( msg->get_payload().c_str(),  msg->get_payload().c_str() + msg->get_payload().length(), &root,&err)) {
            std::cout << "error" << std::endl;
        }
        else {  
            const std::string msg = root["Message"].asString();
            std::cout << "JSON Payload[message]" << msg << std::endl;
        }
    }

    void onClose(websocketpp::connection_hdl hdl) {
        std::cout << "onClose called with hdl: " << hdl.lock().get()<< std::endl;
    }

    void onFail(websocketpp::connection_hdl hdl) {
        std::cout << "onFail called with hdl: " << hdl.lock().get()<< std::endl;
    }
    
    void onOpen(websocketpp::connection_hdl hdl) {
        std::cout << "onOpen called with hdl: " << hdl.lock().get()<< std::endl;
    }

    void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;
//        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
//        if (ec) {
//            std::cout << "> Error initiating close: " << ec.message() << std::endl;
//        }
    }

    void send(int id, std::string message) {
        websocketpp::lib::error_code ec;
        
  //      con_list::iterator metadata_it = m_connection_list.find(id);
  //      if (metadata_it == m_connection_list.end()) {
  //          std::cout << "> No connection found with id " << id << std::endl;
  //          return;
  //      }
  //      
  //      m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
  //      if (ec) {
  //          std::cout << "> Error sending message: " << ec.message() << std::endl;
  //          return;
  //      }
  //      
  //      metadata_it->second->record_sent_message(message);
    }


    void run() {
        // Listen on port 9002
        m_endpoint.listen(9002);

        // Queues a connection accept operation
        m_endpoint.start_accept();

        // Start the Asio io_service run loop
        m_endpoint.run();
    }
private:
    server m_endpoint;
};

int main() {
    wsJsonServer s;
    s.run();
    return 0;
}