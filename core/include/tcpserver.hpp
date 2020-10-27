// Copyright 2020 Randal A. Koene
// License TBD

/** @file tcpserver.hpp
 * This header file p[rovides functions for TCP client-server communication.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __TCPSERVER_HPP.
 */

#ifndef __TCPSERVER_HPP
#include "coreversion.hpp"
#define __TCPSERVER_HPP (__COREVERSION_HPP)

// std
//#include <ctime>

// core
#include "error.hpp"

namespace fz {

struct shared_memory_server {
    shared_memory_server() {}
    virtual void handle_request_with_data_share(int new_socket, const std::string & segment_name) = 0;
};

/**
 * Set up an IPv4 TCP socket on specified port and listen for client connections from any address.
 * 
 * See https://man7.org/linux/man-pages/man7/ip.7.html.
 * 
 * Note: Only some errors return a detailed error message in an error data structure. Those
 *       are typically errors that were caught during validation of the request data. Other
 *       errors may not do so, although they will typically still log the error on the
 *       server side.
 * 
 * @param port_number The port number to listen on.
 * @param server A shared_memory_server derived server object to handle requests with data share.
 * @return Server listen outcome, expressed in exit codes (exit_ok, exit_general_error, etc).
 */
exit_status_code server_socket_listen(uint16_t port_number, shared_memory_server & server);

} // namespace fz

#endif // __TCPSERVER_HPP
