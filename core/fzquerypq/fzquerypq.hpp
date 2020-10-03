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

    fzquerypq() : formalizer_standard_program(true), output_format(output_txt), ga(*this, add_option_args, add_usage_top, true), flowcontrol(flow_unknown) {
        COMPILEDPING(std::cout, "PING-fzquerypq().1\n");
        add_option_args += "n:F:R:";
        add_usage_top += " [-n <Node-ID>] [-F txt|html] [-R histories]";
    }

    virtual void usage_hook() {
        ga.usage_hook();
        FZOUT("    -n request data for Node <Node-ID>\n");
        FZOUT("    -F specify output format\n");
        FZOUT("       available formats:\n");
        FZOUT("       txt = basic ASCII text template\n")
        FZOUT("       html = HTML template\n");
        FZOUT("    -R refresh:\n");
        FZOUT("       histories = Node histories cache table\n");
    }

    bool set_output_format(const std::string & cargs) {
        if (cargs == "html") {
            output_format = output_html;
            return true;
        }
        if (cargs == "txt") {
            output_format = output_txt;
            return true;
        }
        ADDERROR(__func__,"unknown output format: "+cargs);
        FZERR("unknown output format: "+cargs);
        exit(exit_command_line_error);
        return false; // never gets here
    }

    virtual bool options_hook(char c, std::string cargs) {
        if (ga.options_hook(c,cargs))
            return true;

        switch (c) {

        case 'n': {
            flowcontrol = flow_node;
            node_idstr = cargs;
            return true;
        }
        
        case 'F': {
            set_output_format(cargs);
            return true;
        }

        case 'R': {
            if (cargs=="histories") {
                flowcontrol = flow_refresh_histories;
                return true;
            }
            ADDERROR(__func__, "Unknown option -R "+cargs);
            VERBOSEERR("Unknown option -R "+cargs+'\n');
            return false;
        }

        }

       return false;
    }

    /**
     * Initialize configuration parameters.
     * Call this at the top of main().
     * 
     * @param argc command line parameters count forwarded from main().
     * @param argv command line parameters array forwarded from main().
     */
    void init_top(int argc, char *argv[]) {
        //*************** for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i]; // do this before getopt mucks it up
        init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    }

};

extern fzquerypq fzq;

#endif // __FZQUERYPQ_HPP
