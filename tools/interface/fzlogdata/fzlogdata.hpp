// Copyright 20240418 Randal A. Koene
// License TBD

/**
 * Log data gathering, inspection, analysis tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZLOGDATA_HPP.
 */

#ifndef __FZLOGDATA_HPP
#include "version.hpp"
#define __FZLOGDATA_HPP (__VERSION_HPP)

// std
#include <set>

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphaccess.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Logaccess.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_integrity_issues = 1,     /// request: collect possible Log integrity issues
    flow_chunk_time_data = 2,      /// request: get time data for list of Log chunks
    flow_NUMoptions
};

enum data_format {
    data_format_raw,
    data_format_txt,
    data_format_html,
    data_format_json,
};

class fzld_configurable: public configurable {
public:
    fzld_configurable(formalizer_standard_program & fsp): configurable("fzlogdata", fsp) {}
    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string dest;   ///< where to send rendered output (default: "STDOUT")
    unsigned int verylargechunk_hours = 24;

    //std::string example_par;   ///< example of configurable parameter
};


struct fzlogdata: public formalizer_standard_program {

    fzld_configurable config;

    flow_options flowcontrol;

    data_format format = data_format_html;

    Log_filter filter;

    std::set<Log_chunk_ID_key> chunk_keys;

    entry_data edata;

    Graph_access ga; // to include Graph or Log access support

    fzlogdata();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    void init_top(int argc, char *argv[]);

    bool get_Log_interval();

    Graph_ptr graph_ptr = nullptr;
    Graph & graph();

    std::unique_ptr<Log> log;

    bool collect_LogIssues();

};

extern fzlogdata fzld;

#endif // __FZLOGDATA_HPP
