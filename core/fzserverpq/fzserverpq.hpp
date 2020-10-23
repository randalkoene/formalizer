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

struct fzserverpq: public formalizer_standard_program {

    Graph_access ga;

    flow_options flowcontrol;

    Graph * graph_ptr;

    static constexpr const char * lockfilepath = FORMALIZER_ROOT "/.fzserverpq.lock";

    fzserverpq();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

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
