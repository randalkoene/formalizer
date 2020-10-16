// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ header_description }}
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZLOG_HPP.
 */

#ifndef __FZLOG_HPP
#include "version.hpp"
#define __FZLOG_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,    /// no recognized request
    flow_make_entry = 1, /// request: make Log entry
    flow_NUMoptions
};

struct entry_data {
    std::string specific_node_id; ///< if empty then same as Log chunk
    Node * node_ptr = nullptr;
    Graph * graph_ptr = nullptr;
    Log log; // *** Maybe you should turn this into unique_ptr<Log> and do make_unique<Log>() after main().
    Log_chunk * c_newest = nullptr;
    time_t newest_chunk_t = -1;
    bool is_open = false;
    Log_entry * e_newest = nullptr;
    uint8_t newest_minor_id = 0;
    std::string utf8_text;
};

class fzl_configurable: public configurable {
public:
    fzl_configurable(formalizer_standard_program & fsp): configurable("fzlog", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


struct fzlog: public formalizer_standard_program {

    fzl_configurable config;

    flow_options flowcontrol;

    entry_data edata;

    Graph_access ga; // to include Graph or Log access support

    fzlog();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzlog fzl;

#endif // __FZLOG_HPP
