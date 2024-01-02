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
#include "html.hpp"

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
    text_interpretation interpret_text = text_interpretation::raw;
    size_t node_excerpt_len = 0;
};

struct fzloghtml: public formalizer_standard_program {

    fzlh_configurable config;

    flow_options flowcontrol;

    Graph_access ga;

    Log_filter filter;

    time_t t_center_around = RTt_unspecified;

    interval_scale iscale;
    unsigned int interval;
    bool noframe;

    std::vector<std::string> search_strings;
    bool mustcontainall = false;
    bool caseinsensitive = false;

    std::string custom_template;

    entry_data edata;
    //std::unique_ptr<Log> log;
    bool graph_attempted = false;

    most_recent_format recent_format;

    bool show_total_time_applied = false;
    unsigned long total_minutes_applied = -1;

    fzloghtml();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    void set_filter();

    bool get_Log_interval();

    /// Attempts to find memory-resident Graph but does not exit if it is not found.
    Graph_ptr get_Graph_ptr();

};

extern fzloghtml fzlh;

#endif // __FZLOGHTML_HPP
