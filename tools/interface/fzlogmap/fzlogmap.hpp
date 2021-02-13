// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Map specified Log intervals in terms of specified Node groupings.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZLOGMAP_HPP.
 */

#ifndef __FZLOGMAP_HPP
#include "version.hpp"
#define __FZLOGMAP_HPP (__VERSION_HPP)

// core
#include "standard.hpp"
#include "config.hpp"
#include "Graphinfo.hpp"
#include "Graphaccess.hpp"
#include "Logtypes.hpp"
#include "Logaccess.hpp"
#include "html.hpp"

using namespace fz;

// Forward declarations
struct fzlogmap;

enum flow_options {
    flow_unknown = 0,      /// no recognized request
    flow_log_interval = 1, /// request: read and map Log interval
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

class fzlm_configurable: public configurable {
public:
    fzlm_configurable(formalizer_standard_program & fsp): configurable("fzlogmap", fsp), dest("STDOUT") {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string dest;   ///< where to send rendered output (default: "STDOUT")
    text_interpretation interpret_text = text_interpretation::raw;
    std::string categoryfile; ///< a preconfigured file with category group definitions
};

struct fzlogmap: public formalizer_standard_program {

    fzlm_configurable config;

    flow_options flowcontrol;

    Graph_access ga;

    Log_filter filter;

    Set_builder_data categories;

    interval_scale iscale;
    unsigned int interval;
    bool noframe;
    bool calendar;

    std::string custom_template;

    entry_data edata;
    //std::unique_ptr<Log> log;

    most_recent_format recent_format;

    fzlogmap();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    Graph & graph();

    void set_filter();

    bool set_groups(Set_builder_data & groups);

    void get_Log_interval();

    time_t Log_interval_t_start(); // *** You might want to clean this up by combining these three functions and edata in a Log_interval class.

    time_t Log_interval_t_end();

};

extern fzlogmap fzlm;

#endif // __FZLOGMAP_HPP
