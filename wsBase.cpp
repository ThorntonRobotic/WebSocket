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

#pragma once

namespace RemoteControl {

/// Endpoint role based on the given config
/**
 *
 */
template <typename config>
class remote : public endpoint<connection<config>,config> {
public:
    /// Type of this endpoint
    typedef remote<config> type;

    /// Type of the endpoint concurrency component
    typedef typename config::concurrency_type concurrency_type;
    /// Type of the endpoint transport component
    typedef typename config::transport_type transport_type;

    /// Type of the connections this server will create
    typedef connection<config> connection_type;
    /// Type of a shared pointer to the connections this server will create
    typedef typename connection_type::ptr connection_ptr;

    /// Type of the connection transport component
    typedef typename transport_type::transport_con_type transport_con_type;
    /// Type of a shared pointer to the connection transport component
    typedef typename transport_con_type::ptr transport_con_ptr;

    /// Type of the endpoint component of this server
    typedef endpoint<connection_type,config> endpoint_type;

    friend class connection<config>;

    explicit remote() : endpoint_type(false)
    {
        endpoint_type::m_alog->write(log::alevel::devel, "remote constructor");
    }


    /// Destructor
    ~remote<config>() {}

#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no copy constructor because endpoints are not copyable
    remote<config>(remote<config> &) = delete;

    // no copy assignment operator because endpoints are not copyable
    remote<config> & operator=(remote<config> const &) = delete;
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
    /// Move constructor
    remote<config>(remote<config> && o) : endpoint<connection<config>,config>(std::move(o)) {}

#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no move assignment operator because of const member variables
    remote<config> & operator=(remote<config> &&) = delete;
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#endif // _WEBSOCKETPP_MOVE_SEMANTICS_

    // Client functions

    /// Get a new connection
    /**
     * Creates and returns a pointer to a new connection to the given URI
     * suitable for passing to connect(connection_ptr). This method allows
     * applying connection specific settings before performing the opening
     * handshake.
     *
     * @param [in] location URI to open the connection to as a uri_ptr
     * @param [out] ec An status code indicating failure reasons, if any
     *
     * @return A connection_ptr to the new connection
     */
    connection_ptr get_connection(uri_ptr location, lib::error_code & ec) {
        if (location->get_secure() && !transport_type::is_secure()) {
            ec = error::make_error_code(error::endpoint_not_secure);
            return connection_ptr();
        }

        connection_ptr con = endpoint_type::create_connection();

        if (!con) {
            ec = error::make_error_code(error::con_creation_failed);
            return con;
        }

        con->set_uri(location);

        ec = lib::error_code();
        return con;
    }

    /// Get a new connection (string version)
    /**
     * Creates and returns a pointer to a new connection to the given URI
     * suitable for passing to connect(connection_ptr). This overload allows
     * default construction of the uri_ptr from a standard string.
     *
     * @param [in] u URI to open the connection to as a string
     * @param [out] ec An status code indicating failure reasons, if any
     *
     * @return A connection_ptr to the new connection
     */
    connection_ptr get_connection(std::string const & u, lib::error_code & ec) {
        uri_ptr location = lib::make_shared<uri>(u);

        if (!location->get_valid()) {
            ec = error::make_error_code(error::invalid_uri);
            return connection_ptr();
        }

        return get_connection(location, ec);
    }

    /// Begin the connection process for the given connection
    /**
     * Initiates the opening connection handshake for connection con. Exact
     * behavior depends on the underlying transport policy.
     *
     * @param con The connection to connect
     *
     * @return The pointer to the connection originally passed in.
     */
    connection_ptr connect(connection_ptr con) {
        // Ask transport to perform a connection
        transport_type::async_connect(
            lib::static_pointer_cast<transport_con_type>(con),
            con->get_uri(),
            lib::bind(&type::handle_connect,this,con,lib::placeholders::_1)
        );
        return con;
    }

    private:
    // handle_connect
    void handle_connect(connection_ptr con, lib::error_code const & ec) {
        if (ec) {
            con->terminate(ec);

            endpoint_type::m_elog->write(log::elevel::rerror,
                    "handle_connect error: "+ec.message());
        } else {
            endpoint_type::m_alog->write(log::alevel::connect,
                "Successful connection");
            con->start();
        }
    }

    public:
    // Server functions
    /// Create and initialize a new connection
    /**
     * The connection will be initialized and ready to begin. Call its start()
     * method to begin the processing loop.
     *
     * Note: The connection must either be started or terminated using
     * connection::terminate in order to avoid memory leaks.
     *
     * @return A pointer to the new connection.
     */
    connection_ptr get_connection() {
        return endpoint_type::create_connection();
    }

    /// Starts the server's async connection acceptance loop (exception free)
    /**
     * Initiates the server connection acceptance loop. Must be called after
     * listen. This method will have no effect until the underlying io_service
     * starts running. It may be called after the io_service is already running.
     *
     * Refer to documentation for the transport policy you are using for
     * instructions on how to stop this acceptance loop.
     *
     * @param [out] ec A status code indicating an error, if any.
     */
    void start_accept(lib::error_code & ec) {
        if (!transport_type::is_listening()) {
            ec = error::make_error_code(error::async_accept_not_listening);
            return;
        }
        
        ec = lib::error_code();
        connection_ptr con = get_connection();

        if (!con) {
          ec = error::make_error_code(error::con_creation_failed);
          return;
        }

        transport_type::async_accept(
            lib::static_pointer_cast<transport_con_type>(con),
            lib::bind(&type::handle_accept,this,con,lib::placeholders::_1),
            ec
        );

        if (ec && con) {
            // If the connection was constructed but the accept failed,
            // terminate the connection to prevent memory leaks
            con->terminate(lib::error_code());
        }
    }

    /// Starts the server's async connection acceptance loop
    /**
     * Initiates the server connection acceptance loop. Must be called after
     * listen. This method will have no effect until the underlying io_service
     * starts running. It may be called after the io_service is already running.
     *
     * Refer to documentation for the transport policy you are using for
     * instructions on how to stop this acceptance loop.
     */
    void start_accept() {
        lib::error_code ec;
        start_accept(ec);
        if (ec) {
            throw exception(ec);
        }
    }

    /// Handler callback for start_accept
    void handle_accept(connection_ptr con, lib::error_code const & ec) {
        if (ec) {
            con->terminate(ec);

            if (ec == error::operation_canceled) {
                endpoint_type::m_elog->write(log::elevel::info,
                    "handle_accept error: "+ec.message());
            } else {
                endpoint_type::m_elog->write(log::elevel::rerror,
                    "handle_accept error: "+ec.message());
            }
        } else {
            con->start();
        }

        lib::error_code start_ec;
        start_accept(start_ec);
        if (start_ec == error::async_accept_not_listening) {
            endpoint_type::m_elog->write(log::elevel::info,
                "Stopping acceptance of new connections because the underlying transport is no longer listening.");
        } else if (start_ec) {
            endpoint_type::m_elog->write(log::elevel::rerror,
                "Restarting async_accept loop failed: "+ec.message());
        }
    }

};

}; // namespace remote



