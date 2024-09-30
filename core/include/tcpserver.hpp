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
    bool listen;
    shared_memory_server() : listen(true) {}
    virtual void handle_request_with_data_share(int new_socket, const std::string & segment_name) = 0;
    virtual void handle_special_purpose_request(int new_socket, const std::string & request_str) = 0;
};

struct GET_token_value {
    std::string token;
    std::string value;
    GET_token_value(const std::string _token, const std::string _value) : token(_token), value(_value) {}
};
typedef std::vector<GET_token_value> GET_token_value_vec;

/**
 * Convert portion of a HTTP GET string into a vector of
 * token-value pairs.
 * 
 * Note: For a more complete token-value extraction that includes decoding of
 *       %-codes, see the functions in the `cgihandler` library.
 * 
 * @param httpgetstr A (portion) of an HTTP GET string.
 * @param argseparator Separator character (normally '&').
 * @return A vector of token-value pairs.
 */
GET_token_value_vec GET_token_values(const std::string httpgetstr, const char argseparator = '&');

/**
 * Returns a MIME type string reference that corresponds with the extension of a
 * file path.
 * 
 * Note: This function does not 'sniff' the file content to determine if the extension
 * correctly points out the MIME type.
 * 
 * @param file_path The path to a file.
 * @return The MIME type string reference.
 */
const std::string & mimetype_by_extension(const std::string & file_path);

enum http_response_code {
    http_ok = 200,
    http_bad_request = 400,
    http_not_found = 404,
};

const std::map<http_response_code, std::string> http_response_code_map = {
    {http_ok, "200 OK"},
    {http_bad_request, "400 Bad Request"},
    {http_not_found, "404 Not Found"}
};

/**
 * A structure to collect HTTP header data and generate HTTP header strings.
 * 
 * Optionally, set `header_str` directly. Otherwise, use constructors and set
 * structure data, then call `ok_str()` and `ok_len()` to send
 * HTTP header info.
 */
struct http_header_data {
    std::string server;
    std::string content_type;
    size_t content_length;

    http_response_code code = http_ok;
    std::string header_str; // generated from the component data or set directly
    std::string error_msg;

    http_header_data() : content_type("text/html") {}
    http_header_data(size_t len) : content_type("text/html"), content_length(len) {}
    http_header_data(std::string file_path) : content_type(mimetype_by_extension(file_path)) {}
    http_header_data(std::string file_path, size_t len) : content_type(mimetype_by_extension(file_path)), content_length(len) {}
    http_header_data(http_response_code _code, std::string resp_str) : code(_code), header_str("HTTP/1.1 "+http_response_code_map.at(_code)+"\r\n\r\n"), error_msg(resp_str) {}

    const std::string & str(); // this builds the header string
    size_t len() { return str().size(); }
    const std::string & code_str() { return http_response_code_map.at(code); }
};

/**
 * A helpful class for server responses with text data.
 * 
 * See fzserverpq:tcp_server_handlers:direct_tcpport_api_file_serving() as an
 * example where this is used.
 */
class server_response_text: public http_header_data {
public:
    server_response_text(const std::string & resp_str) : http_header_data(resp_str.size()) { str(); header_str += resp_str; }
    server_response_text(http_response_code _code, std::string resp_str) : http_header_data(_code, resp_str) {}
    ssize_t respond(int socket);
};

/**
 * A helpful class for server responses with plain text data.
 * 
 * See fzserverpq:tcp_server_handlers:direct_tcpport_api_file_serving() as an
 * example where this is used.
 */
class server_response_plaintext: public server_response_text {
public:
    server_response_plaintext(const std::string & resp_str) : server_response_text(resp_str) { content_type = "text/plain"; header_str.clear(); str(); header_str += resp_str; }
};

/**
 * A helpful class for server responses with binary data.
 * 
 * See fzserverpq:tcp_server_handlers:direct_tcpport_api_file_serving() as an
 * example where this is used.
 */
class server_response_binary: public http_header_data {
protected:
    const void * data;
    size_t datalen;
public:
    server_response_binary(std::string file_path, const void * _data, size_t _datalen) : http_header_data(file_path, _datalen), data(_data), datalen(_datalen) {}
    ssize_t respond(int socket);
};

/**
 * Discover this server's IP address from the perspective of a connecting
 * TCP client.
 * 
 * This method should work irrespective of the network device being used.
 * 
 * @param[out] ipaddr_str Reference to string variable that receives the IP address.
 * @return True if successful.
 */
bool find_server_address(std::string & ipaddr_str);

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
