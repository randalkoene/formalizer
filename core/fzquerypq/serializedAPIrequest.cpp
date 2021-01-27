// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Functions that serve serialized data API requests for `fzquerypq`.
 * 
 */

// corelib
#include "error.hpp"
#include "standard.hpp"
#include "tcpclient.hpp"

// local
#include "fzquerypq.hpp"
#include "serializedAPIrequest.hpp"

void make_serialized_data_API_request() {

    // *** Wait a second... does this mean we stop keeping fzquerypq separate from fzserverpq now?
    // *** Or should this sort of request be added to a different tool instead (or be in a tool of its own, e.g. fzserial)?
    std::string response_str;
    /*if (!client_socket_shmem_request("FZ "+fzq.request_str, fzq.graph().get_server_IPaddr(), fzq.graph().get_server_port(), response_str)) {
        standard_error("FZ serialized data API request failed.", __func__);
    }*/

    FZOUT(response_str);

}
