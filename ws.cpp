#define ASIO_STANDALONE
//#define ASIO_WINDOWS
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "json/json.h"

//typedef websocketpp::client<websocketpp::config::asio_client> client;

    template <class client>
    class websocket_endpoint {
    public:

        class connection {
        public:
            typedef websocketpp::lib::shared_ptr<websocket_endpoint::connection> ptr;

            connection(int id, websocketpp::connection_hdl hdl, std::string uri)
            : m_id(id)
            , m_hdl(hdl)
            , m_status("Connecting")
            , m_uri(uri)
            , m_server("N/A")
            {}

            void on_open(client * c, websocketpp::connection_hdl hdl) {
                m_status = "Open";

                typename client::connection_ptr con = c->get_con_from_hdl(hdl);
                m_server = con->get_response_header("Server");
            }

            void on_fail(client * c, websocketpp::connection_hdl hdl) {
                m_status = "Failed";

                typename client::connection_ptr con = c->get_con_from_hdl(hdl);
                m_server = con->get_response_header("Server");
                m_error_reason = con->get_ec().message();
            }
            
            void on_close(client * c, websocketpp::connection_hdl hdl) {
                m_status = "Closed";
                typename client::connection_ptr con = c->get_con_from_hdl(hdl);
                std::stringstream s;
                s << "close code: " << con->get_remote_close_code() << " (" 
                << websocketpp::close::status::get_string(con->get_remote_close_code()) 
                << "), close reason: " << con->get_remote_close_reason();
                m_error_reason = s.str();
            }

            void on_message(websocketpp::connection_hdl hdl, typename client::message_ptr msg) {
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

            //friend std::ostream & operator<< (std::ostream & out, connection const & data);
        private:
            int m_id;
            websocketpp::connection_hdl m_hdl;
            std::string m_status;
            std::string m_uri;
            std::string m_server;
            std::string m_error_reason;
            std::vector<std::string> m_messages;
        };

    public:
        websocket_endpoint () : m_next_id(0) {
            m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
            m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

            m_endpoint.init_asio();
            m_endpoint.start_perpetual();

            m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
        }

        ~websocket_endpoint() {
            m_endpoint.stop_perpetual();
            
            for (typename con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
                if (it->second->get_status() != "Open") {
                    // Only close open connections
                    continue;
                }
                
                std::cout << "> Closing connection " << it->second->get_id() << std::endl;
                
                websocketpp::lib::error_code ec;
                m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
                if (ec) {
                    std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                            << ec.message() << std::endl;
                }
            }
            
            m_thread->join();
        }

        int connect(std::string const & uri) {
            websocketpp::lib::error_code ec;

            typename client::connection_ptr con = m_endpoint.get_connection(uri, ec);

            if (ec) {
                std::cout << "> Connect initialization error: " << ec.message() << std::endl;
                return -1;
            }

            int new_id = m_next_id++;
            typename websocket_endpoint::connection::ptr metadata_ptr = websocketpp::lib::make_shared<connection>(new_id, con->get_handle(), uri);
            m_connection_list[new_id] = metadata_ptr;

            con->set_open_handler(websocketpp::lib::bind(
                &websocket_endpoint::connection::on_open,
                metadata_ptr,
                &m_endpoint,
                websocketpp::lib::placeholders::_1
            ));
            con->set_fail_handler(websocketpp::lib::bind(
                &websocket_endpoint::connection::on_fail,
                metadata_ptr,
                &m_endpoint,
                websocketpp::lib::placeholders::_1
            ));
            con->set_close_handler(websocketpp::lib::bind(
                &websocket_endpoint::connection::on_close,
                metadata_ptr,
                &m_endpoint,
                websocketpp::lib::placeholders::_1
            ));
            con->set_message_handler(websocketpp::lib::bind(
                &websocket_endpoint::connection::on_message,
                metadata_ptr,
                websocketpp::lib::placeholders::_1,
                websocketpp::lib::placeholders::_2
            ));

            m_endpoint.connect(con);

            return new_id;
        }

        void close(int id, websocketpp::close::status::value code, std::string reason) {
            websocketpp::lib::error_code ec;
            
            typename con_list::iterator metadata_it = m_connection_list.find(id);
            if (metadata_it == m_connection_list.end()) {
                std::cout << "> No connection found with id " << id << std::endl;
                return;
            }
            
            m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
            if (ec) {
                std::cout << "> Error initiating close: " << ec.message() << std::endl;
            }
        }

        void send(int id, std::string message) {
            websocketpp::lib::error_code ec;
            
            typename con_list::iterator metadata_it = m_connection_list.find(id);
            if (metadata_it == m_connection_list.end()) {
                std::cout << "> No connection found with id " << id << std::endl;
                return;
            }
            
            m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
            if (ec) {
                std::cout << "> Error sending message: " << ec.message() << std::endl;
                return;
            }
            
            metadata_it->second->record_sent_message(message);
        }

        typename websocket_endpoint::connection::ptr get_metadata(int id) const {
            typename con_list::const_iterator metadata_it = m_connection_list.find(id);
            if (metadata_it == m_connection_list.end()) {
                return typename connection::ptr();
            } else {
                return metadata_it->second;
            }
        }
    private:
        typedef std::map<int,typename websocket_endpoint::connection::ptr> con_list;

        client m_endpoint;
        websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

        con_list m_connection_list;
        int m_next_id;
    };

   // std::ostream & operator<< (std::ostream & out, websocket_endpoint::connection const & data) {
   //         out << "> URI: " << data.m_uri << "\n"
   //             << "> Status: " << data.m_status << "\n"
   //             << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
   //             << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
   //         out << "> Messages Processed: (" << data.m_messages.size() << ") \n";
   //         std::vector<std::string>::const_iterator it;
   //         for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
   //             out << *it << "\n";
   //         }
//
 //           return out;
  //      }


int main() {
    bool done = false;
    std::string input;

    typedef websocketpp::client<websocketpp::config::asio_client> c;
    //websocket_endpoint<websocketpp::client<websocketpp::config::asio_client>> endpoint;
    websocket_endpoint<c> endpoint;
    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit") {
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "connect <ws uri>\n"
                << "send <connection id> <message>\n"
                << "close <connection id> [<close code:default=1000>] [<close reason>]\n"
                << "show <connection id>\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        } else if (input.substr(0,7) == "connect") {
            int id = endpoint.connect(input.substr(8));
            if (id != -1) {
                std::cout << "> Created connection with id " << id << std::endl;
            }
        } else if (input.substr(0,4) == "send") {
            std::stringstream ss(input);
            
            std::string cmd;
            int id;
            std::string message;
            
            ss >> cmd >> id;
            std::getline(ss,message);
            
            Json::Value root;
            Json::StreamWriterBuilder builder;
            root["Message"] = message;
            const std::string jsonMessage = Json::writeString(builder, root);

            std::cout << "Sending:" << jsonMessage << std::endl;
            endpoint.send(id, jsonMessage);
            
        } else if (input.substr(0,5) == "close") {
            std::stringstream ss(input);
            
            std::string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            std::string reason;
            
            ss >> cmd >> id >> close_code;
            std::getline(ss,reason);
            
            endpoint.close(id, close_code, reason);
        } else if (input.substr(0,4) == "show") {
            int id = atoi(input.substr(5).c_str());

            websocket_endpoint<c>::connection::ptr metadata = endpoint.get_metadata(id);
            if (metadata) {
 //               std::cout << *metadata << std::endl;
            } else {
                std::cout << "> Unknown connection id " << id << std::endl;
            }
        } else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}