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
 * @param httpgetstr A (portion) of an HTTP GET string.
 * @return A vector of token-value pairs.
 */
GET_token_value_vec GET_token_values(const std::string httpgetstr);

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

    fzs_configurable config;

    Graph_access ga;

    flow_options flowcontrol;

    Graph * graph_ptr;

    static constexpr const char * lockfilepath = FORMALIZER_ROOT "/.fzserverpq.lock";

    fzserverpq();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    virtual void handle_request_with_data_share(int new_socket, const std::string & segment_name);

    virtual void handle_special_purpose_request(int new_socket, const std::string & request_str);

};

extern fzserverpq fzs;

/**
 * Find the shared memory segment with the indicated request
 * stack, then process that stack by first carrying out
 * validity checks on all stack elements and then responding
 * to each request.
 * 
 * @param segname The shared memory segment name provided for the request stack.
 * @return 
 */
bool handle_request_stack(std::string segname);

#endif // __FZSERVERPQ_HPP
