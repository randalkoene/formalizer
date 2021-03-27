// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Command line and CGI tool to build a time-picker HTML page to call fztask.
 * 
 * See the README.md for examples of fzlogtime calls that work from the local
 * command line or as CGI calls with the necessary variants for different
 * browsers.
 * 
 * For more about this, see https://trello.com/c/I2f2kvmc.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Log:Time"

// std
#include <cstdlib>
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"

// local
#include "version.hpp"
#include "fzlogtime.hpp"
#include "render.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzlogtime fzlt;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzlogtime::fzlogtime() : formalizer_standard_program(false), config(*this), flowcontrol(flow_logtime_page), ga(*this, add_option_args, add_usage_top) {
    add_option_args += "D:neo:C:";
    add_usage_top += " [-D <date>] [-n] [-e] [-o <output-path>] [-C <skip,wrap>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("\nNote that CGI calls can be fickle. For this reason, the -C\n"
                           "option enables variants. For example, the 'wrap' option\n"
                           "is needed with w3m, and the `skip` option is needed with\n"
                           "Chrome.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlogtime::usage_hook() {
    ga.usage_hook();
    FZOUT("    -D print page for <date>\n"
          "    -n generate links with non-local argument\n"
          "    -e embeddable (no HTML head or tail)\n"
          "    -C CGI mode needs:\n"
          "         wrap: to be wrapped in a shell script\n"
          "         skip: to skip the 'Content-type' header\n"
          "    -o Rendered output to <output-path> (\"STDOUT\" is default)\n");
}

bool parse_cgi_variants(const std::string & cargs) {
    auto variant_options = split(cargs,',');
    for (const auto & v_opt : variant_options) {
        if (v_opt == "skip") {
            fzlt.config.skip_content_type = true;
        } else if (v_opt == "wrap") {
            fzlt.config.wrap_cgi_script = true;
        } else {
            return standard_error("Unrecognized CGI mode variant: "+v_opt, __func__);
        }
    }
    return true;
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
bool fzlogtime::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs)) {
        return true;
    }

    switch (c) {

    case 'D': {
        T_page_date = ymd_stamp_time(cargs, false, true);
        return true;
    }

    case 'n': {
        fzlt.nonlocal = true;
        return true;
    }

    case 'e': {
        fzlt.embeddable = true;
        return true;
    }

    case 'o': {
        config.rendered_out_path = cargs;
        return true;
    }

    case 'C': {
        return parse_cgi_variants(cargs);
    }

    }

    return false;
}

const std::map<std::string, char> token_option_map = {
    {"D",'D'},
    {"rendered_out_path",'o'},
    {"cgivar",'C'},
    {"embeddable",'e'},
    {"quiet",'q'},
    {"veryverbose",'V'},
    {"source",'n'} // this assumes that finding the token 'source' means it is set to 'nonlocal'
};

bool cgi_hook(const std::string & token, const std::string & value) {
    auto tom_it = token_option_map.find(token);
    if (tom_it != token_option_map.end()) {
        return fzlt.options_hook(tom_it->second, value);
    } else {
        return standard_error("Unknown CGI token: "+token, __func__);
    }
}

/// Configure configurable parameters.
bool fzlt_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(rendered_out_path, "rendered_out_path", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(skip_content_type, "skip_content_type", parlabel, (parvalue == "true"));
    CONFIG_TEST_AND_SET_PAR(wrap_cgi_script, "wrap_cgi_script", parlabel, (parvalue == "true"));
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
void fzlogtime::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class

    if (cgi.received_data() > 0) {
        for (const auto & [token, value] : cgi.POST) {
            cgi_hook(token, value);
        }
        standard.quiet = true;
    }
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzlt.init_top(argc, argv);

    switch (fzlt.flowcontrol) {

    case flow_logtime_page: {
        return standard_exit(render_logtime_page(), "Log time page generated.\n", exit_file_error, "Unable to generate Log time page.", __func__);
    }

    default: {
        return standard_exit(render_logtime_page(), "Log time page generated.\n", exit_file_error, "Unable to generate Log time page.", __func__);
        //fzlt.print_usage();
    }

    }

    return standard.completed_ok();
}
