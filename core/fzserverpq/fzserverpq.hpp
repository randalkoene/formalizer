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
#include "standard.hpp"
#include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,        /// no recognized request
    flow_resident_graph = 1, /// request: load the Graph into shared memory and stay resident
    flow_NUMoptions
};

struct fzserverpq: public formalizer_standard_program {

    Graph_access ga;

    flow_options flowcontrol;

    fzserverpq();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzserverpq fzs;

#endif // __FZSERVERPQ_HPP
