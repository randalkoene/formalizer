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
#include "Logaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,     /// no recognized request
    flow_make_entry = 1,  /// request: make Log entry
    flow_close_chunk = 2, /// request: close Log chunk
    flow_open_chunk = 3,  /// request: open new Log chunk with associated Node
    flow_NUMoptions
};

class fzl_configurable: public configurable {
public:
    fzl_configurable(formalizer_standard_program & fsp): configurable("fzlog", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string content_file; ///< Optional file path for entry text content.
};


struct fzlog: public formalizer_standard_program {

    fzl_configurable config;

    flow_options flowcontrol;

    entry_data edata;

    std::string newchunk_node_id;

    Graph_access ga; // to include Graph or Log access support

    ReferenceTime reftime; // enables emulated time

    fzlog();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzlog fzl;

#endif // __FZLOG_HPP
