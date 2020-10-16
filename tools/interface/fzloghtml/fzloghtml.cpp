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
fzloghtml::fzloghtml() : formalizer_standard_program(false), config(*this), ga(*this, add_option_args, add_usage_top),
                         iscale(interval_none), interval(0), noframe(false) {
    add_option_args += "n:1:2:o:D:H:w:N";
    add_usage_top += " [-n <node-ID>] [-1 <time-stamp-1>] [-2 <time-stamp-2>] [-D <days>|-H <hours>|-w <weeks>] [-o <outputfile>] [-N]";
    usage_head.push_back("Generate HTML representation of requested Log records.\n");
    usage_tail.push_back(
        "The <time-stamp1> and <time-stamp_2> arguments expect standardized\n"
        "Formalizer time stamps, e.g. 202009140614, but will also accept date stamps\n"
        "of analogous form, e.g. 20200914.\n"
        "Without a Node specification, the default is:\n"
        "  start from 24 hours before end of interval\n"
        "  end at most recent Log entry\n"
        "With a Node specification but no time stamps, the default is:\n"
        "  complete Node history\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzloghtml::usage_hook() {
    ga.usage_hook();
    FZOUT("    -n belongs to <node-ID>\n");
    FZOUT("    -1 start from <time-stamp-1>\n");
    FZOUT("    -2 end at <time-stamp-2>\n");
    FZOUT("    -D interval size of <days>\n");
    FZOUT("    -H interval size of <hours>\n");
    FZOUT("    -w interval size of <weeks>\n");
    FZOUT("    -o write HTML Log interval to <outputfile> (default=STDOUT)\n");
    FZOUT("    -N no HTML page frame\n");
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

    case 'n': {
        filter.nkey = Node_ID_key(cargs);
        return true;
    }

    case '1': {
        time_t t = ymd_stamp_time(cargs);
        if (t==RTt_invalid_time_stamp) {
            VERBOSEERR("Invalid 'from' time or date stamp "+cargs+'\n');
            break;
        } else {
            filter.t_from = t;
        }
        return true;
    }

    case '2': {
        time_t t = ymd_stamp_time(cargs);
        if (t==RTt_invalid_time_stamp) {
            VERBOSEERR("Invalid 'before' time or date stamp "+cargs+'\n');
            break;
        } else {
            filter.t_to = t;
        }
        return true;
    }

    case 'D': {
        iscale = interval_days;
        interval = atoi(cargs.c_str());
        return true;
    }

    case 'H': {
        iscale = interval_hours;
        interval = atoi(cargs.c_str());
        return true;
    }

    case 'w': {
        iscale = interval_weeks;
        interval = atoi(cargs.c_str());
        return true;
    }

    case 'o': {
        config.dest = cargs;
        return true;
    }

    case 'N': {
        noframe = true;
        return true;
    }

    }

    return false;
}

/// Configure configurable parameters.
bool fzlh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(dest, "outputfile", parlabel, parvalue);
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
    if ((filter.t_to==RTt_unspecified) && (filter.t_from!=RTt_unspecified) && (iscale!=interval_none)) {
        switch (iscale) {

            case interval_weeks: {
                filter.t_to = filter.t_from + interval*(7*24*60*60);
            }

            case interval_hours: {
                filter.t_to = filter.t_from + interval*(60*60);
            }

            default: { // days
                filter.t_to = filter.t_from + interval*(24*60*60);
            }

        }
    }
    if ((filter.t_from==RTt_unspecified) && (filter.t_to!=RTt_unspecified) && (iscale!=interval_none)) {
        switch (iscale) {

            case interval_weeks: {
                filter.t_from = filter.t_to - interval*(7*24*60*60);
            }

            case interval_hours: {
                filter.t_from = filter.t_to - interval*(60*60);
            }

            default: { // days
                filter.t_from = filter.t_to - interval*(24*60*60);
            }

        }
    }

    // Note that RTt_unspecified means not to impose a constraint on the corresponding t variable (see Log_filter in Logtypes.hpp).
    if (filter.nkey.isnullkey()) { // with a Node specifier, unbounded is fine, without one, the default should be constrained
        // We can leave filter.t_to unspecified, since that automatically means to the most recent.
        // ***But we need to somehow set the start of the default interval. For now, let's just set it to
        //    A day before the current time. If necessary, I can later decide to read the lastest entry to
        //    make it a day from that.
        if (filter.t_from==RTt_unspecified) {
            if (filter.t_to==RTt_unspecified) {
                filter.t_from = ActualTime() - (24*60*60); // Daylight savings time is not taken into account at all.
            } else {
                filter.t_from = filter.t_to - (24*60*60); // Daylight savings time is not taken into account at all.
            }
        }
    }

    //graph = ga.request_Graph_copy();
    //log = ga.request_Log_copy(); *** This has to change... not every request needs the whole log. See https://trello.com/c/O9dcTm9L and https://trello.com/c/EppSyY9Y.
    VERYVERBOSEOUT("\nfilter:\n");
    VERYVERBOSEOUT("  t_from = "+TimeStampYmdHM(filter.t_from)+'\n');
    VERYVERBOSEOUT("  t_to   = "+TimeStampYmdHM(filter.t_to)+'\n');
    VERYVERBOSEOUT("  Node   = "+filter.nkey.str()+"\n\n");

    log = ga.request_Log_excerpt(filter);

    VERYVERBOSEOUT("\nfound:\n");
    VERYVERBOSEOUT("  chunks : "+std::to_string(log->num_Chunks())+'\n');
    VERYVERBOSEOUT("  entries: "+std::to_string(log->num_Entries())+"\n\n");

    // *** Should we call log.setup_Chain_nodeprevnext() ?


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
    FZOUT("  pq_schemaname : "+fzlh.ga.pq_schemaname()+'\n');p
    FZOUT("  testconfig    : "+fzlh.config.testconfig+'\n');
    FZOUT("  errlogpath    : "+ErrQ.get_errfilepath()+'\n');
    FZOUT("  warnlogpath    : "+WarnQ.get_errfilepath()+'\n');

    return standard.completed_ok();
*/