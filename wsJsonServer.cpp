#define ASIO_STANDALONE
//#define ASIO_WINDOWS
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>
#include <map>

#include "json/json.h"

typedef websocketpp::server<websocketpp::config::asio> server;

namespace Payload
{
    class wsJson
    {
    private:
        Json::Value base;
        JSONCPP_STRING err;
    public:
        wsJson(/*logger*/) = default;
        ~wsJson() = default;

         // copy constructor - transferring rights to the compiler to form this constructor
        wsJson(const wsJson&) = delete;
        // move constructor
        wsJson(wsJson&&) = delete;
        // copy assignment operator
        wsJson& operator=(const wsJson&) = delete;
        // move assignment operator
        wsJson& operator=(wsJson&&) = delete;

        bool parse(std::string &data, websocketpp::frame::opcode::value type /* Logger ??*/){
            if (type == websocketpp::frame::opcode::text) {
                Json::CharReaderBuilder builder;
                const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                return reader->parse( data.c_str(),  data.c_str() + data.length(), &base, &err);
            }
            err = "Websocket payload not text";
            return false;
        }
    };
};

class connection_metadata {
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {}

    void on_open(server * c, websocketpp::connection_hdl hdl) {
        m_status = "Open";

        server::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(server * c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";

        server::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }
    
    void on_close(server * c, websocketpp::connection_hdl hdl) {
        m_status = "Closed";
        server::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " (" 
          << websocketpp::close::status::get_string(con->get_remote_close_code()) 
          << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
    }

    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
         std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            m_messages.push_back("<< " + msg->get_payload());
        } else {
            m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
        }
    }

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }
    
    int get_id() const {
        return m_id;
    }
    
    std::string get_status() const {
        return m_status;
    }

    void record_sent_message(std::string message) {
        m_messages.push_back(">> " + message);
    }

    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;
};


// Can we bypass the connection metadata and not worry about storing the handle/index 
// When we return the onMessage we can send the handle <handle, message> if the user needs to respond

template<class T>
class wsJsonServer {
public:
   typedef int HandleId; 
   static HandleId handleIndex;

private:
    server m_endpoint;
    int m_next_id;
 //   typedef struct {
 //      websocketpp::connection_hdl handlePtr;
 //      T message; 
 //   } HandleMetadata;
    typedef std::map<int,connection_metadata::ptr> con_list;
    con_list m_connection_list;

    std::map<HandleId, T> m_messages;

public:
    wsJsonServer():
    m_endpoint(),
    m_next_id(0),
    m_connection_list(),
    m_messages()
    {
         // Set logging settings
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);
        m_endpoint.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        m_endpoint.init_asio();
//TODO think out callbacks
        // Set the default message handler to the echo handler
        m_endpoint.set_message_handler(std::bind(&wsJsonServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
        m_endpoint.set_open_handler(std::bind(&wsJsonServer::onOpen, this,std::placeholders::_1));
        m_endpoint.set_fail_handler(std::bind(&wsJsonServer::onFail, this, websocketpp::lib::placeholders::_1));
        m_endpoint.set_close_handler(std::bind(&wsJsonServer::onClose, this, websocketpp::lib::placeholders::_1));
    }

    void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
        std::cout << "onMessage hdl: " << hdl.lock().get()<< " message: " << msg->get_payload() << " opcode: " << msg->get_opcode()
              << std::endl;
    }
 
    void onClose(websocketpp::connection_hdl hdl) {
       std::cout << "onClose called with hdl: " << hdl.lock().get()<< std::endl;
 //       if (auto c = m_connection_list.find(hdl); c != m_connection_list.end()){
 //           m_connection_list.erase(c);
 //       }
    }

    void onFail(websocketpp::connection_hdl hdl) {
        std::cout << "onFail called with hdl: " << hdl.lock().get()<< std::endl;
 //       if (auto c = m_connection_list.find(hdl); c != m_connection_list.end()){
 //           m_connection_list.erase(c);
 //       }
    }
    
    void onOpen(websocketpp::connection_hdl hdl) {
        std::cout << "onOpen called with hdl: " << hdl.lock().get()<< std::endl;
        if(hdl.lock()){
            // pointer not valid
        }
        if (auto d = hdl.lock()){      // Ensure pointer is valid
           // d->
            int new_id = m_next_id++;
            // TODO: uri
            connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, hdl, "");
            m_connection_list[new_id] = metadata_ptr;
            hdl.

//            std::owner_less<websocketpp::connection_hdl> a = hdl;
//            if (auto c = m_connection_list.find(hdl); c == m_connection_list.end()){
//                m_connection_list[hdl] = handleIndex++;
//            }
//            else{
//                std::cout << "onOpen called with existing handle hdl" <<std::endl;
//            }
        }
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
};

wsJsonServer<Payload::wsJson>::HandleId handleIndex = 0;

int main() {
    wsJsonServer<Payload::wsJson> s;
    s.run();
    return 0;
}