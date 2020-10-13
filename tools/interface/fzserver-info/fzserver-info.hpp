// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Inspect memory resident servers.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZSERVER_INFO_HPP.
 */

#ifndef __FZSERVER_INFO_HPP
#include "version.hpp"
#define __FZSERVER_INFO_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
// #include "Graphaccess.hpp"

/**
 * FORMALIZER_ROOT must be supplied by -D during make.
 * For example: FORMALIZERROOT=-DFORMALIZER_ROOT=\"$(HOME)/.formalizer\"
 */
#ifndef FORMALIZER_ROOT
    #define FORMALIZER_ROOT this_breaks
#endif

using namespace fz;

enum flow_options {
    flow_unknown = 0,      /// no recognized request
    flow_graph_server = 1, /// request: graph server status
    flow_NUMoptions
};

class fzsi_configurable: public configurable {
public:
    fzsi_configurable(formalizer_standard_program & fsp): configurable("fzserver-info", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    //std::string example_par;   ///< example of configurable parameter
};


struct fzserver_info: public formalizer_standard_program {

    fzsi_configurable config;

    flow_options flowcontrol;

    // Graph_access ga; // to include Graph or Log access support

    static constexpr const char * lockfilepath = FORMALIZER_ROOT "/.fzserverpq.lock";

    fzserver_info();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzserver_info fzsi;

#endif // __FZSERVER_INFO_HPP
