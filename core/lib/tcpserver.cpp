// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
//#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <cstring>
#include <map>
#include <filesystem>

// core
#include "standard.hpp"
#include "general.hpp"
#include "tcpserver.hpp"

namespace fz {

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
GET_token_value_vec GET_token_values(const std::string httpgetstr, const char argseparator) {
    auto tokenvalue_strvec = split(httpgetstr, argseparator);
    GET_token_value_vec gtvvec;
    for (const auto & tvstr : tokenvalue_strvec) {
        auto equalpos = tvstr.find('=');
        if ((equalpos != std::string::npos) && (equalpos != 0)) {
            gtvvec.emplace_back(tvstr.substr(0,equalpos), tvstr.substr(equalpos+1));
        }
    }
    return gtvvec;
}

const std::map<std::string, std::string> ext_mimetype = {
    {".html","text/html"},
    {".css","text/css"},
    {".csv","text/csv"},
    {".gif","image/gif"},
    {".ico","image/vnd.microsoft.icon"},
    {".jpg","image/jpeg"},
    {".jpeg","image/jpeg"},
    {".png","image/png"},
    {".svg","image/svg+xml"},
    {".txt","text/plain"},
    {".sh","application/x-sh"},
    {".json","application/json"},
    {".ics","text/calendar"},
    {"_other_","application/octet-stream"}
};

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
const std::string & mimetype_by_extension(const std::string & file_path) {
    std::filesystem::path path(file_path);
    auto it = ext_mimetype.find(path.extension().string());
    //FZOUT("EXTENSION: ["+path.extension().string()+"]\n");
    if (it == ext_mimetype.end()) {
        it = ext_mimetype.find("_other_");
    }
    //FZOUT("MIME Type: "+it->second+'\n');
    return it->second;
}

/// Retrieve or build the header string.
const std::string & http_header_data::str() {
    if (header_str.empty()) {
        header_str = "HTTP/1.1 "+http_response_code_map.at(http_ok);
        if (!server.empty()) {
            header_str += "\nServer: "+server;
        }
        if (!content_type.empty()) {
            header_str += "\nContent-Type: "+content_type;
        }
        if (content_length>0) {
            header_str += "\nContent-Length: "+std::to_string(content_length);
        }
        header_str += "\r\n\r\n";
    }
    return header_str;
}

/// Send text response through socket.
ssize_t server_response_text::respond(int socket) {
    if (code != http_ok) {
        VERYVERBOSEOUT(error_msg+"\nResponding with: "+code_str()+".\n");
        ssize_t header_sent = send(socket, str().c_str(), len(), 0);
        return header_sent;
    } else {
        // str() was already called in the constructor to build the header
        // and combined with the response text. (But note https://trello.com/c/1IIp1vxj.)
        ssize_t text_sent = send(socket, header_str.data(), header_str.size(), 0);
        return text_sent;
    }
}

/// Send binary response through socket.
ssize_t server_response_binary::respond(int socket) {
    ssize_t header_sent = send(socket, str().c_str(), len(), 0);
    if (header_sent>0) {
        ssize_t data_sent = send(socket, data, datalen, 0);
        return data_sent;
    }
    return header_sent;
}

/**
 * Discover this server's IP address from the perspective of a connecting
 * TCP client.
 * 
 * This method should work irrespective of the network device being used.
 * 
 * @param[out] ipaddr_str Reference to string variable that receives the IP address.
 * @return True if successful.
 */
bool find_server_address(std::string & ipaddr_str) {
    ERRTRACE;

    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    sockaddr_in loopback;

    if (sock == -1) {
        return standard_error("Unable to create socket.", __func__);
    }

    std::memset(&loopback, 0, sizeof(loopback));
    loopback.sin_family = AF_INET;
    loopback.sin_addr.s_addr = INADDR_LOOPBACK;   // using loopback ip address
    loopback.sin_port = htons(9);                 // using debug port

    if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == -1) {
        close(sock);
        return standard_error("Unable to connect.", __func__);
    }

    socklen_t addrlen = sizeof(loopback);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == -1) {
        close(sock);
        return standard_error("Unable to get socket name.", __func__);
    }

    close(sock);

    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) {
        return standard_error("Unable to convert network address into character string.", __func__);
    } else {
        ipaddr_str = buf;
    }

    return true;
}


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
exit_status_code server_socket_listen(uint16_t port_number, shared_memory_server & server) {
    #define str_SIZE 1024
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    char str[str_SIZE];
    int addrlen = sizeof(address);
  
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        standard_error("Socket failed.", __func__);
        return exit_communication_error;
    }

    // Let the server bind again to the same address and port in case it crashed and restarted with minimal delay.
    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &enable, sizeof(enable)) < 0) {
        standard_error("The setsockopt(SO_REUSEADDR) call failed.", __func__);
        return exit_communication_error;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, (const char *) &enable, sizeof(enable)) < 0) {
        standard_error("The setsockopt(SO_REUSEPORT) call failed.", __func__);
        return exit_communication_error;
    }
    VERYVERBOSEOUT("Socket initialized at port: "+std::to_string(port_number)+'\n');

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_number);

    // Forcefully attaching socket to the port 8090.
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) { 
        standard_error("Port bind failed.", __func__); 
        return exit_communication_error; 
    } 
  
    // puts the server socket in passive mode 
    if (listen(server_fd, 3) < 0) {
        standard_error("The listen() call returned an error.", __func__); 
        return exit_communication_error;
    }

    while (server.listen) {
        VERYVERBOSEOUT("\nBound and listening to all incoming addresses.\n\n");
        
        // This is receiving the address that is connecting. https://man7.org/linux/man-pages/man2/accept.2.html
        // As described in the man page, if the socket is blocking (as it is here) then it will block,
        // i.e. wait, until a connection request appears. Alternatively, there are several ways
        // described in the Description section of the man page for non-blocking approaches that
        // can involve polling or an interrupt to give attention to a connection request.
        // While blocked, the process consumes no CPU (e.g. see https://stackoverflow.com/questions/23108140/why-do-blocking-functions-not-use-100-cpu).
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            standard_error("The accept() call returned an error.", __func__); 
            return exit_communication_error;
        }
        VERYVERBOSEOUT("Connection accepted from: "+std::string(inet_ntoa(address.sin_addr))+'\n');

        // read string send by client
        memset(str, 0, str_SIZE); // just playing it safe
        valread = read(new_socket, str, sizeof(str)); // *** right now, this read often hangs until some timeout
        if (valread==0) {
            ADDWARNING(__func__, "EOF encountered");
            VERYVERBOSEOUT("Read encountered EOF.\n");
            close(new_socket); // *** possibly remove these... best read up about this some more
            continue;
        }
        if (valread<0) {
            standard_error("Socket read error", __func__);
            close(new_socket);
            continue;
        }

        std::string request_str(str);
        if (request_str.back() == '\n') {
            request_str.pop_back(); // closing newline is optional
        }

        if ((request_str.substr(0,4) == "GET ") || (request_str.substr(0,6) == "PATCH ") || (request_str.substr(0,3) == "FZ ")) { // a special purpose request from a browser interface
            server.handle_special_purpose_request(new_socket, request_str);
            close(new_socket);
            continue;
        }

        if (request_str == "STOP") {
            server.listen = false;
            VERYVERBOSEOUT("STOP request received. Exiting server listen loop.\n");
            std::string response_str("STOPPING");
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            //sleep(1); // this might actually be necessary for the client to receive the whole response
            close(new_socket);
            break;
        }

        if (request_str == "PING") {
            VERYVERBOSEOUT("PING request received. Responding.\n");
            std::string response_str("LISTENING");
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            //sleep(1); // this might actually be necessary for the client to receive the whole response
            close(new_socket);
            continue;           
        }

        // If it was not (one of) the specific requests handled above then it specifies the
        // segment name for a request stack in shared memory.
        server.handle_request_with_data_share(new_socket, request_str);
        close(new_socket);

    }

    close(server_fd);

    return exit_ok;
}

} // namespace fz
