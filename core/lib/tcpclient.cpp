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
#include "general.hpp"
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
    #define str_SIZE 1000 // was 100, but that caused trouble when fzserverpq /fz API requests gave HTML responses to fzgraph -C
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

std::string strip_HTTP_header(const std::string& response_str) {
    auto rnrn_loc = response_str.find("\r\n\r\n");
    if (rnrn_loc != std::string::npos) {
        return response_str.substr(rnrn_loc+4);
    } else {
        auto nn_loc = response_str.find("\n\n");
        if (nn_loc == std::string::npos) {
            return response_str;
        } else {
            return response_str.substr(nn_loc+2);
        }
    }
}

http_GET_long::http_GET_long(const std::string& ipaddrstr, uint16_t port_number, const std::string url) {
    VERYVERBOSEOUT("Sending GET request to "+ipaddrstr+": "+url+'\n');

    std::string propergetstr = "GET "+url+" HTTP/1.1\r\nHost: "+ipaddrstr+"\r\n\r\n";

    valid_response = http_client_socket_request(propergetstr, ipaddrstr, port_number);
    if (!valid_response) {
        standard_error("Communication error.", __func__);
    } else {
        VERYVERBOSEOUT("Server responded.\n");
    }
}

/**
 * C sockets request function that can handle both Content-Length
 * and Transfer-Encoding: chunked HTTP responses.
 */
bool http_GET_long::http_client_socket_request(const std::string& request_str, const std::string& server_ip_address, uint16_t port_number) {
    #define str_SIZE 1000 // was 100, but that caused trouble when fzserverpq /fz API requests gave HTML responses to fzgraph -C
    int sock = 0;
    struct sockaddr_in serv_addr;
    char str[str_SIZE+1];

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

    while (response_str.size() < content_length) {
        // read data returned by server
        memset(str, 0, str_SIZE+1); // just playing it safe
        ssize_t valread = read(sock, str, str_SIZE);
        if (valread==0) {
            VERBOSEOUT("Server response reached EOF.\n");
            break;
        }
        if (valread<0) {
            VERBOSEOUT("Server response read returned ERROR.\n");
            close(sock);
            return standard_error("Server returned error with code: "+std::to_string(valread)+'\n', __func__);
        }

        if (!header_detected) {
            response_str += str;
            data_start = http_find_header();
            header_detected = (data_start >= 0);
            if (header_detected) {
                // Remove header from data
                response_str = response_str.substr(data_start);
                if (chunked_transfer) {
                    // Remove chunk size from data
                    auto n_loc = response_str.find('\n');
                    if (n_loc != std::string::npos) {
                        response_str = response_str.substr(n_loc+1);
                    }
                }
            }
        } else {

            if (chunked_transfer) {
                ssize_t chunklen_lineend = find_in_char_array('\n', str, valread);
                long int chunk_length = strtol(str, nullptr, 16);
                if (chunk_length == 0) break; // Last chunk
                response_str += (str+chunklen_lineend+1);
            } else {
                response_str += str;
            }
        }
    }

    close(sock);

    return true;
}

ssize_t http_GET_long::http_find_header() {
    // Find empty line
    auto rnrn_loc = response_str.find("\r\n\r\n");
    ssize_t data_start = -1;
    if (rnrn_loc != std::string::npos) {
        data_start = rnrn_loc+4;
    } else {
        rnrn_loc = response_str.find("\n\n");
        if (rnrn_loc != std::string::npos) {
            data_start = rnrn_loc+2;
        } else {
            return -1;
        }
    }

    // Confirm that it contains an HTML header
    std::string headerstr(response_str.substr(0, data_start));
    if (headerstr.find("HTTP/") == std::string::npos) {
        return -1;
    }

    // Check for chunked transfer
    chunked_transfer = headerstr.find("chunked") != std::string::npos;

    // Check for transfer length
    if (!chunked_transfer) {
        auto length_loc = headerstr.find("Content-Length:");
        content_length = atoi(headerstr.substr(length_loc+15).c_str());
    }

    // Check if html content
    html_content = headerstr.find("text/html") != std::string::npos;

    return data_start;
}

} // namespace fz
