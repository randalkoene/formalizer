// Copyright 20210501 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the fzserverpq-log program.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZSERVERPQ-LOG_HPP.
 */

#ifndef __FZSERVERPQ-LOG_HPP
#include "version.hpp"
#define __FZSERVERPQ-LOG_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "tcpserver.hpp"
#include "Logaccess.hpp"
// #include "Graphaccess.hpp"

#ifndef FORMALIZER_ROOT
    #define FORMALIZER_ROOT this_breaks
#endif

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_resident_log = 1, /// request: load the Log into shared memory and stay resident
    flow_NUMoptions
};


class fzsl_configurable: public configurable {
public:
    fzsl_configurable(formalizer_standard_program & fsp): configurable("fzserverpq-log", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    static constexpr const char * reqqfilepath = "/tmp/formalizer.core.fzserverpq-log.ReqQ.log";

    uint16_t port_number = 8091;   ///< Default port number to listen on.
    bool persistent_NEL = true;    ///< Default Named Entry Lists are synchronized in-memory and database.
    std::string www_file_root = "/var/www/html"; ///< Root as presented for direct TCP-port API file serving.
    std::string request_log = reqqfilepath;
};


struct fzserverpqlog: public formalizer_standard_program, public shared_memory_server {

    static constexpr const char * lockfilepath = FORMALIZER_ROOT "/.fzserverpq-log.lock";

    fzsl_configurable config;

    flow_options flowcontrol;

    Log * log_ptr;

    // *** A v0.1 simplistic server request log (see https://trello.com/c/dnKYchIu for the better way).
    Errors ReqQ;

    // Graph_access ga; // to include Graph or Log access support

    fzserverpqlog();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    virtual void handle_request_with_data_share(int new_socket, const std::string & segment_name); // see shm_server_handlers.cpp

    virtual void handle_special_purpose_request(int new_socket, const std::string & request_str); // see tcp_server_handlers.cpp

    void log(std::string request, std::string update) { ReqQ.push(request, update); }

    /* (uncomment to include access to memory-resident Graph)
    Graph_ptr graph_ptr = nullptr;
    Graph & graph();
    */

};

extern fzserverpqlog fzsl;

#endif // __FZSERVERPQ-LOG_HPP
