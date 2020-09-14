// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Generate HTML representation of requested Log records.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZLOGHTML_HPP.
 */

#ifndef __FZLOGHTML_HPP
#include "version.hpp"
#define __FZLOGHTML_HPP (__VERSION_HPP)

// core
#include "standard.hpp"
#include "Graphaccess.hpp"
#include "Logtypes.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    //flow_something = 1,     /// request: make boilerplate for C++ program
    flow_NUMoptions
};

enum interval_scale {
    interval_none,
    interval_days,
    interval_hours,
    interval_weeks
};

struct fzloghtml: public formalizer_standard_program {

    flow_options flowcontrol;

    Graph_access ga;

    time_t t_from;
    time_t t_before;
    interval_scale iscale;
    unsigned int interval;

    std::string dest;   ///< where to send rendered output (empty means STDOUT)

    std::unique_ptr<Log> log;

    fzloghtml();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzloghtml fzlh;

#endif // __FZLOGHTML_HPP
