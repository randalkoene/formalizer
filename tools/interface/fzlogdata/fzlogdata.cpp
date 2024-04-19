// Copyright 20240418 Randal A. Koene
// License TBD

/**
 * Log data gathering, inspection, analysis tool
 * 
 * This tool is used to parse the Log to gather information within
 * it, to inspect its integrity, and to analyze data.
 * 
 * For more about this, see ./README.md.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Log:Integrity"

// std
//#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
/* (uncomment to communicate with Graph server)
#include "tcpclient.hpp"
*/
#include "Logtypes.hpp"
#include "Loginfo.hpp"

// local
#include "version.hpp"
#include "fzlogdata.hpp"
#include "render.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzlogdata fzld;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzlogdata::fzlogdata() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top) {
    add_option_args += "IF:o:";
    add_usage_top += " [-I] [-F <raw|txt|html|json>] [-o <outputfile>]";
    usage_head.push_back(
        "Log data gathering, inspection, analysis tool.\n"
        "This tool is used to parse the Log to gather information within\n"
        "it, to inspect its integrity, and to analyze data.\n"
    );
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlogdata::usage_hook() {
    ga.usage_hook();
    FZOUT("    -I collect possible integrity issues in Log.\n"
          "    -F format of most recent Log data:\n"
          "       raw, txt, json, html (default)\n"
          "    -o write rendered Log data to <outputfile> (default=STDOUT)\n"
    );
}

const std::map<std::string, data_format> format_keywords = {
    { "raw", data_format_raw },
    { "txt", data_format_txt },
    { "json", data_format_json },
    { "html", data_format_html },
};

/**
 * Handler for command line options that are defined in the derived class
 * as options specific to the program.
 * 
 * Include case statements for each option. Typical handlers do things such
 * as collecting parameter values from `cargs` or setting `flowcontrol` choices.
 * 
 * @param c is the character that identifies a specific option.
 * @param cargs is the optional parameter value provided for the option.
 */
bool fzlogdata::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs)) return true;

    switch (c) {

    case 'I': {
        flowcontrol = flow_integrity_issues;
        return true;
    }

    case 'F': {
        auto it = format_keywords.find(cargs);
        if (it == format_keywords.end()) {
            fzld.format = data_format_html;
        } else {
            fzld.format = it->second;
        }
        return true;
    }

    case 'o': {
        config.dest = cargs;
        return true;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fzld_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(dest, "outputfile", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(verylargechunk_hours, "verylargechunk_hours", parlabel, std::atoi(parvalue.c_str()));
    //CONFIG_TEST_AND_SET_PAR(example_par, "examplepar", parlabel, parvalue);
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}


/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzlogdata::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

Graph & fzlogdata::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

/**
 * Use the LogIssues class (in Loginfo.hpp) to collect
 * information about possible issues in the Log.
 * This is an integrity test.
 */
bool fzlogdata::collect_LogIssues() {
    // Load the entire Log into memory.
    log = ga.request_Log_copy();
    if (!log) {
        return standard_error("Loading of complete Log failed.", __func__);
    }

    // Use the LogIssues class to inspect the Log.
    LogIssues logissues(*(log.get()));

    logissues.collect_all_issues(graph(), config.verylargechunk_hours*3600);

    return render_integrity_issues(logissues);
}

bool integrity_issues() {
    if (!fzld.collect_LogIssues()) {
        return standard_error("Failed collect_LogIssues.", __func__);
    }
    return true;
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzld.init_top(argc, argv);

    switch (fzld.flowcontrol) {

    case flow_integrity_issues: {
        return standard_exit(integrity_issues(), "Log integrity issues collected.\n", exit_file_error, "Unable to collect Log integrity issues", __func__);
    }

    default: {
        fzld.print_usage();
    }

    }

    return standard.completed_ok();
}
