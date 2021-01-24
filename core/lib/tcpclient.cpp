// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h> 

// core
#include "standard.hpp"
#include "Graphmodify.hpp"
#include "tcpclient.hpp"

namespace fz {

/**
 * A minimal TCP client communication function used to make the server aware of a request
 * specified by data in shared memory.
 * Although this function was created to communicate shmem requests, it can probably be
 * used for any TCP client communication.
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
bool client_socket_shmem_request(std::string request_str, std::string server_ip_address, uint16_t port_number, std::string & response_str) {
    //struct sockaddr_in address;
    #define str_SIZE 100
    int sock = 0;
    struct sockaddr_in serv_addr;
    char str[str_SIZE];

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return standard_error("Socket creation error.\n", __func__);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_number);

    // Convert IPv4 and IPv6 addresses from text to binary form, where
    // `server_ip_address` is a valid IP address (e.g. 127.0.0.1).
    if (inet_pton(AF_INET, server_ip_address.c_str(), &serv_addr.sin_addr) <= 0) {
        return standard_error("Address not supported: "+server_ip_address, __func__);
    }

    // connect the socket
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        return standard_error("Connection Failed.", __func__);
    }

    VERYVERBOSEOUT("\nConnected to server at "+server_ip_address+':'+std::to_string(port_number)+".\n");

    VERYVERBOSEOUT("Sending request: "+request_str+'\n');

    // send string to server side
    send(sock, request_str.c_str(), request_str.size()+1, 0);

    // read string sent by server
    memset(str, 0, str_SIZE); // just playing it safe
    ssize_t valread = read(sock, str, sizeof(str));
    if (valread==0) {
        VERBOSEOUT("Server response reached EOF.\n");
    }
    if (valread<0) {
        VERBOSEOUT("Server response read returned ERROR.\n");
    }
    close(sock);

    response_str = str;

    return true;
}

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
exit_status_code server_request_with_shared_data(std::string segname, uint16_t port_number) {

    std::string response_str;
    if (!client_socket_shmem_request(segname, "127.0.0.1", port_number, response_str)) {
        VERBOSEERR("Communication error.\n");
        return exit_communication_error;
    }

    if (response_str == "RESULTS") {
        VERBOSEOUT("Successful response received.\n");
        Graphmod_results * resdata = find_results_response_in_shared_memory(segname);
        if (!resdata) {
            standard_error("Unable to find the results data structure in shared memory", __func__);
            return exit_missing_data;
        }
        VERBOSEOUT(resdata->info_str()+'\n');
        return exit_ok;
    }
    
    if (response_str == "ERROR") {
        Graphmod_error * errdata = find_error_response_in_shared_memory(segname);
        if (!errdata) {
            standard_error("The server reported an error but did not return a data structure with additional information.", __func__);
            return exit_general_error;
        }
        standard_error("The server reported this error: "+std::string(errdata->message), __func__);
        return errdata->exit_code;
    }
    
    standard_error("Unknown response: "+response_str, __func__);
    return exit_general_error;
}

} // namespace fz
