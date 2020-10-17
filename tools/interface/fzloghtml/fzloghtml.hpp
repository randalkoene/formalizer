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
#include "config.hpp"
#include "Graphaccess.hpp"
#include "Logtypes.hpp"
#include "Logaccess.hpp"

using namespace fz;

// Forward declarations
struct fzloghtml;

enum flow_options {
    flow_unknown = 0,      /// no recognized request
    flow_log_interval = 1, /// request: read and render Log interval
    flow_most_recent = 2,  /// request: data about most recent Log entry
    flow_NUMoptions
};

enum interval_scale {
    interval_none,
    interval_days,
    interval_hours,
    interval_weeks
};

enum most_recent_format {
    most_recent_raw,
    most_recent_txt,
    most_recent_html
};

class fzlh_configurable: public configurable {
public:
    fzlh_configurable(formalizer_standard_program & fsp): configurable("fzloghtml", fsp), dest("STDOUT") {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string dest;   ///< where to send rendered output (default: "STDOUT")
};

struct fzloghtml: public formalizer_standard_program {

    fzlh_configurable config;

    flow_options flowcontrol;

    Graph_access ga;

    Log_filter filter;

    interval_scale iscale;
    unsigned int interval;
    bool noframe;

    entry_data edata;
    //std::unique_ptr<Log> log;

    most_recent_format recent_format;

    fzloghtml();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    void set_filter();

    void get_Log_interval();

};

extern fzloghtml fzlh;

#endif // __FZLOGHTML_HPP
