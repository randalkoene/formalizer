// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Generate HTML representation of requested Log records.
 * 
 * A specified (or default) interval of Log content is rendered as HTML.
 * 
 * For more about this, see https://trello.com/c/usj9dcWi.
 */

//#define USE_COMPILEDPING

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Log:HTML"

// std
#include <iostream>
#include <memory>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "TimeStamp.hpp"

// local
#include "version.hpp"
#include "fzloghtml.hpp"
#include "render.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzloghtml fzlh;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzloghtml::fzloghtml(): formalizer_standard_program(false), config(*this), ga(*this, add_option_args,add_usage_top), t_from(RTt_unspecified), t_before(RTt_unspecified), iscale(interval_none), interval(0) {
    add_option_args += "1:2:o:d:H:w:";
    add_usage_top += " [-1 <time-stamp-1>] [-2 <time-stamp-2>] [-d <days>|-H <hours>|-w <weeks>] [-o <outputfile>]";
    usage_head.push_back("Generate HTML representation of requested Log records.\n");
    usage_tail.push_back(
        "The <time-stamp1> and <time-stamp_2> arguments expect standardized\n"
        "Formalizer time stamps, e.g. 202009140614, but will also accept date stamps\n"
        "of analogous form, e.g. 20200914.\n"
        "The default is:\n"
        "  start from 24 hours before end of interval\n"
        "  end at current time\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzloghtml::usage_hook() {
    ga.usage_hook();
    FZOUT("    -1 start from <time-stamp-1>\n");
    FZOUT("    -2 end before <time-stamp-2>\n");
    FZOUT("    -d interval size of <days>\n");
    FZOUT("    -H interval size of <hours>\n");
    FZOUT("    -w interval size of <weeks>\n");
    FZOUT("    -o write HTML Log interval to <outputfile> (otherwise to STDOUT)\n");
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
bool fzloghtml::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs))
            return true;

    switch (c) {

    case '1': {
        time_t t = ymd_stamp_time(cargs);
        if (t==RTt_invalid_time_stamp) {
            VERBOSEERR("Invalid 'from' time or date stamp "+cargs+'\n');
        } else {
            t_from = t;
        }
        break;
    }

    case '2': {
        time_t t = ymd_stamp_time(cargs);
        if (t==RTt_invalid_time_stamp) {
            VERBOSEERR("Invalid 'before' time or date stamp "+cargs+'\n');
        } else {
            t_before = t;
        }
        break;
    }

    case 'd': {
        iscale = interval_days;
        interval = atoi(cargs.c_str());
        break;
    }

    case 'H': {
        iscale = interval_hours;
        interval = atoi(cargs.c_str());
        break;
    }

    case 'w': {
        iscale = interval_weeks;
        interval = atoi(cargs.c_str());
        break;
    }

    case 'o': {
        dest = cargs;
        return true;
    }

    }

    return false;
}

/// Configure configurable parameters.
bool fzlh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(testconfig, "testconfig", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * Note: If we wanted to take daylight savings time into account then we could use Howard
 *       Hinnant's `date.h` library instead. See the card at https://trello.com/c/ANI2Bxei.
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzloghtml::init_top(int argc, char *argv[]) {
    ERRTRACE;
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class

    // If only t_before or t_from is specified, see if we use a specified interval
    if ((t_before==RTt_unspecified) && (t_from!=RTt_unspecified) && (iscale!=interval_none)) {
        switch (iscale) {

            case interval_weeks: {
                t_before = t_from + interval*(7*24*60*60);
            }

            case interval_hours: {
                t_before = t_from + interval*(60*60);
            }

            default: { // days
                t_before = t_from + interval*(24*60*60);
            }

        }
    }
    if ((t_from==RTt_unspecified) && (t_before!=RTt_unspecified) && (iscale!=interval_none)) {
        switch (iscale) {

            case interval_weeks: {
                t_from = t_before - interval*(7*24*60*60);
            }

            case interval_hours: {
                t_from = t_before - interval*(60*60);
            }

            default: { // days
                t_from = t_before - interval*(24*60*60);
            }

        }
    }

    // If necessary, set the default interval
    if (t_before==RTt_unspecified) {
        t_before = ActualTime();
    }
    if (t_from==RTt_unspecified) {
        t_from = t_before - (24*60*60); // Daylight savings time is not taken into account at all.
    }

    //graph = ga.request_Graph_copy();
    log = ga.request_Log_copy();

}

int main(int argc, char *argv[]) {
    ERRTRACE;
    fzlh.init_top(argc, argv);

    render();

    /*
    switch (fzlh.flowcontrol) {

    default: {
        fzlh.print_usage();
    }

    }
    */

    return standard.completed_ok();
}

/*
    FZOUT("TESTING CONFIGURATION LOADING!\n");
    FZOUT("Configuration parameters were set to:\n");
    FZOUT("  dbname        : "+fzlh.ga.dbname()+'\n');
    FZOUT("  pq_schemaname : "+fzlh.ga.pq_schemaname()+'\n');
    FZOUT("  testconfig    : "+fzlh.config.testconfig+'\n');
    FZOUT("  errlogpath    : "+ErrQ.get_errfilepath()+'\n');
    FZOUT("  warnlogpath    : "+WarnQ.get_errfilepath()+'\n');

    return standard.completed_ok();
*/