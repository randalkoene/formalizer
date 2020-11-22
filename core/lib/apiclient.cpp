// Copyright 2020 Randal A. Koene
// License TBD

// std
//#include <>

// core
#include "error.hpp"
#include "tcpclient.hpp"
#include "apiclient.hpp"


namespace fz {

bool ping_server(const std::string server_IPaddr_str, uint16_t port_number) {
    ERRTRACE;

    VERYVERBOSEOUT("Sending PING request to Graph server.\n");
    std::string response_str;

    if (!client_socket_shmem_request("PING", server_IPaddr_str, port_number, response_str)) {
        return standard_error("Communication error.", __func__);
    }

    if (response_str != "LISTENING") {
        return standard_error("Unknown response: "+response_str, __func__);
    }

    VERYVERBOSEOUT("Server response: LISTENING\n");
    return true;
}

bool http_GET(const std::string server_IPaddr_str, uint16_t port_number, const std::string url, std::string & response_str) {
    ERRTRACE;

    VERYVERBOSEOUT("Sending GET request to Graph server: "+url+'\n');

    if (!client_socket_shmem_request("GET "+url, server_IPaddr_str, port_number, response_str)) {
        return standard_error("Communication error.", __func__);
    }

    VERYVERBOSEOUT("Server responded.\n");
    return true;
}

bool NNLreq_update_shortlist(const std::string server_IPaddr_str, uint16_t port_number) {
    ERRTRACE;

    std::string response_str;
    if (!http_GET(server_IPaddr_str, port_number, "/fz/graph/namedlists/_shortlist", response_str)) {
        return false;
    }

    if (response_str.find("updated with") == std::string::npos) {
        return false;
    }

    return true;
}

} // namespace fz
