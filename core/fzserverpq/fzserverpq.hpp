// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the fzserverpq program.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZSERVERPQ_HPP.
 */

#ifndef __FZSERVERPQ_HPP
#include "version.hpp"
#define __FZSERVERPQ_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "tcpserver.hpp"
#include "Graphaccess.hpp"

/**
 * FORMALIZER_ROOT must be supplied by -D during make.
 * For example: FORMALIZERROOT=-DFORMALIZER_ROOT=\"$(HOME)/.formalizer\"
 */
#ifndef FORMALIZER_ROOT
    #define FORMALIZER_ROOT this_breaks
#endif

using namespace fz;

enum flow_options {
    flow_unknown = 0,        /// no recognized request
    flow_resident_graph = 1, /// request: load the Graph into shared memory and stay resident
    flow_NUMoptions
};

class fzs_configurable : public configurable {
public:
    fzs_configurable(formalizer_standard_program &fsp) : configurable("fzserverpq", fsp) {}
    bool set_parameter(const std::string &parlabel, const std::string &parvalue);

    uint16_t port_number = 8090;   ///< Default port number to listen on.
    bool persistent_NNL = true;    ///< Default Named Node Lists are synchronized in-memory and database.
};

struct fzserverpq: public formalizer_standard_program, public shared_memory_server {

    static constexpr const char * lockfilepath = FORMALIZER_ROOT "/.fzserverpq.lock";
    static constexpr const char * reqqfilepath = "/tmp/formalizer.core.fzserverpq.ReqQ.log";

    fzs_configurable config;

    Graph_access ga;

    flow_options flowcontrol;

    Graph * graph_ptr;

    // *** A v0.1 simplistic server request log (see https://trello.com/c/dnKYchIu for the better way).
    Errors ReqQ;

    fzserverpq();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    virtual void handle_request_with_data_share(int new_socket, const std::string & segment_name); // see shm_server_handlers.cpp

    virtual void handle_special_purpose_request(int new_socket, const std::string & request_str); // see tcp_server_handlers.cpp

    void log(std::string request, std::string update) { ReqQ.push(request, update); }

};

extern fzserverpq fzs;

#endif // __FZSERVERPQ_HPP
