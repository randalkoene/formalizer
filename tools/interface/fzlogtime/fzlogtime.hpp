// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Command line and CGI tool to build a time-picker HTML page to call fztask.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZLOGTIME_HPP.
 */

#ifndef __FZLOGTIME_HPP
#include "version.hpp"
#define __FZLOGTIME_HPP (__VERSION_HPP)

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphaccess.hpp"
#include "Logaccess.hpp"
#include "cgihandler.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,      /// no recognized request
    flow_logtime_page = 1, /// request: render a Log time picker page
    flow_NUMoptions
};

class fzlt_configurable: public configurable {
public:
    fzlt_configurable(formalizer_standard_program & fsp): configurable("fzlogtime", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string rendered_out_path = "STDOUT";
    bool skip_content_type = false;
    bool wrap_cgi_script = false;
};


struct fzlogtime: public formalizer_standard_program {

    fzlt_configurable config;

    flow_options flowcontrol;

    Graph_access ga; // to include Graph or Log access support

    entry_data edata;

    bool nonlocal = false;

    bool embeddable = false;

    time_t T_page_date = RTt_unspecified;
    time_t T_page_build = RTt_unspecified;

    CGI_Token_Values cgi;

    fzlogtime();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

};

extern fzlogtime fzlt;

#endif // __FZLOGTIME_HPP
