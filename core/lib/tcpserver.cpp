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

// core
#include "standard.hpp"
#include "tcpserver.hpp"

namespace fz {

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
    #define str_SIZE 100
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

    while (true) {
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
        valread = read(new_socket, str, sizeof(str)); 
        if (valread==0) {
            ADDWARNING(__func__, "EOF encountered");
            VERYVERBOSEOUT("Read encountered EOF.\n");
            continue;
        }
        if (valread<0) {
            standard_error("Socket read error", __func__);
            continue;
        }

        std::string request_str(str);

        // *** You could identify other types of requests here first.
        //     But for now, let's just assume all requests just deliver the segment name for a request stack.
        if (request_str == "STOP") {
            // *** probably close and free up the bound connection here as well.
            VERYVERBOSEOUT("STOP request received. Exiting server listen loop.\n");
            std::string response_str("STOPPING");
            send(new_socket, response_str.c_str(), response_str.size()+1, 0);
            //sleep(1); // this might actually be necessary for the client to receive the whole response
            break;
        }

        // If it was not (one of) the specific requests handled above then it specifies the
        // segment name for a request stack in shared memory.
        server.handle_request_with_data_share(new_socket, request_str);

    }

    close(server_fd);

    return exit_ok;
}

} // namespace fz
