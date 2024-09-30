// Copyright 2020 Randal A. Koene
// License TBD

/** @file tcpclient.hpp
 * This header file p[rovides functions for TCP client-server communication.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __TCPCLIENT_HPP.
 */

#ifndef __TCPCLIENT_HPP
#include "coreversion.hpp"
#define __TCPCLIENT_HPP (__COREVERSION_HPP)

// std
//#include <ctime>

// core
#include "error.hpp"

namespace fz {

/**
 * A minimal TCP client communication function used to make the server aware of a request
 * specified by data in shared memory.
 * 
 * Note: This client-server messaging function is only meant to be used with short pre-defined
 *       message strings, while data is shared through shared memory. A different function
 *       should be used for generic client-server stream communication.
 * 
 * The return value of this function communication success or failure on the client side.
 * According to the protocol, problems on the server side are signaled by a response
 * of "ERROR". In that case, check the shared memory for a possible "error" data structure
 * with additional information.
 * 
 * @param[in] request_str A string that identifies the shared memory segment for request data.
 * @param[in] server_ip_address Normally this is "127.0.0.1" for shared memory exchanges.
 * @param[in] port_number An agreed port number for the communication.
 * @param[out] response_str The server response string.
 * @return True if communication succeeded.
 */
bool client_socket_shmem_request(std::string request_str, std::string server_ip_address, uint16_t port_number, std::string & response_str);

/**
 * Contact the server with the unique label of shared memory
 * containing data for a Graph modification request.
 * 
 * Prints information about results unless `-q` was specified.
 * 
 * Note: The address of the server is hard coded to 127.0.0.1 here,
 *       because this modification request method uses shared
 *       memory, which is (outside of shared-memory clusters) only
 *       possible on the same machine.
 * 
 * @param segname The unique shared memory segment label.
 * @param port_number The port number at which the server is expected to be listening.
 * @return The request outcome, expressed in exit codes (exit_ok, exit_general_error, etc).
 */
exit_status_code server_request_with_shared_data(std::string segname, uint16_t port_number);

std::string strip_HTTP_header(const std::string& response_str);

class http_GET_long {
protected:
    ssize_t data_start = -1;
    size_t content_length = 99999999;

public:
    bool valid_response = false;
    std::string response_str;

    bool header_detected = false;
    bool chunked_transfer = false;
    bool html_content = false;

public:
    http_GET_long(const std::string& ipaddrstr, uint16_t port_number, const std::string url);

    /**
     * C sockets request function that can handle both Content-Length
     * and Transfer-Encoding: chunked HTTP responses.
     */
    bool http_client_socket_request(const std::string& request_str, const std::string& server_ip_address, uint16_t port_number);

    ssize_t http_find_header();
};

} // namespace fz

#endif // __TCPCLIENT_HPP
