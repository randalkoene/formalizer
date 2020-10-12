// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the graph2dil tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZQUERYPQ_HPP.
 */

#ifndef __FZQUERYPQ_HPP
#include "version.hpp"
#define __FZQUERYPQ_HPP (__VERSION_HPP)

#include <vector>
#include <iostream>

#include "standard.hpp"
#include "Graphaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_node = 1,    /// serve requested Node data
    flow_refresh_histories = 2, /// refresh Node histories cache table
    flow_NUMoptions
};

enum output_format_specifier {
    output_txt,
    output_html,
    output_NUMENUMS
};

struct fzquerypq: public formalizer_standard_program {

    std::vector<std::string> cmdargs; /// copy of command line arguments

    output_format_specifier output_format; /// the format used to deliver query results

    std::string node_idstr;

    Graph_access ga;

    flow_options flowcontrol;

    fzquerypq();

    virtual void usage_hook();

    bool set_output_format(const std::string & cargs);

    virtual bool options_hook(char c, std::string cargs);

    /**
     * Initialize configuration parameters.
     * Call this at the top of main().
     * 
     * @param argc command line parameters count forwarded from main().
     * @param argv command line parameters array forwarded from main().
     */
    void init_top(int argc, char *argv[]);

};

extern fzquerypq fzq;

#endif // __FZQUERYPQ_HPP
