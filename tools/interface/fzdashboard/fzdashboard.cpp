// Copyright 20210226 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Board:HTML"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "stringio.hpp"
/* (uncomment to communicate with Graph server)
#include "tcpclient.hpp"
*/

// local
#include "version.hpp"
#include "fzdashboard.hpp"
#include "render.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzdashboard fzdsh;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzdashboard::fzdashboard() : formalizer_standard_program(false), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "D:f:o:";
    add_usage_top += " [-D index|admin|legacy|custom:<template-dir>] [-f <json-file>] [-o <output-dir>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzdashboard::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -D render a predefined dashboard, or render with custom templates at\n"
          "       <template-dir>\n"
          "    -f JSON dashboard definition file path\n"
          "    -o output directory or STDOUT (default in config or current dir)\n"
          "\n"
          "    Note that using the NOTITLE title for a buttons subset in the JSON\n"
          "    file continues the same subset category on another line before\n"
          "    automated wrapping of lines.\n"
          "\n"
          "    The path to JSON definitions for predefined index, admin or legacy dashboards\n"
          "    is specified in the fzdashboard configuration file. It is presently\n"
          "    set to: "+fzdsh.config.json_path+'\n');
}

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
bool fzdashboard::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'D': {
        flowcontrol = flow_dashboard;
        target = cargs;
        return true;
    }

    case 'f': {
        json_path_from_arg = true;
        config.json_path = cargs;
        return true;
    }

    case 'o': {
        top_path_from_arg = true;
        config.top_path = cargs;
        return true;
    }

    }

    return false;
}

/// Configure configurable parameters.
bool fzdsh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(top_path, "top_path", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(json_path, "json_path", parlabel, parvalue);
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
void fzdashboard::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

Graph & fzdashboard::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

int generate_dashboard() {
    if (fzdsh.target.substr(0,7) == "custom:") {
        if ((!fzdsh.json_path_from_arg) || (!fzdsh.top_path_from_arg)) {
            return standard_exit_error(exit_command_line_error, "Explicit JSON source (-f) and output path (-o) must be provided with custom template directory (-D custom:<template-dir>).", __func__);
        }
    } else {
        if (!fzdsh.json_path_from_arg) {
            // The JSON path directory should already be in json_path, just add the target.
            if (fzdsh.config.json_path.empty()) {
                return standard_exit_error(exit_command_line_error, "Missing JSON dashboard specification file", __func__);
            }
            fzdsh.config.json_path += '/' + fzdsh.target+".json";
        }
    }

    std::string json_str;
    std::ifstream::iostate readstate;
    if (!file_to_string(fzdsh.config.json_path, json_str, &readstate)) {
        return standard_exit_error(exit_file_error, "Unable to read file at "+fzdsh.config.json_path, __func__);
    }

    if (render(json_str, dynamic_html)) {
        if (render(json_str, static_html)) {
            return standard.completed_ok();;
        }
    }
    return standard_exit_error(exit_general_error, "Rendering error.", __func__);
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzdsh.init_top(argc, argv);

    switch (fzdsh.flowcontrol) {

    case flow_dashboard: {
        generate_dashboard();
        break;
    }

    default: {
        fzdsh.print_usage();
    }

    }

    return standard.completed_ok();
}
