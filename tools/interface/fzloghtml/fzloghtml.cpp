// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Generate HTML representation of requested Log records.
 * 
 * A specified (or default) interval of Log content is rendered as HTML.
 * 
 * Note that this program can also work when the memory-resident Graph is
 * not present. Output degrades gracefully.
 * 
 * For more about this, see https://trello.com/c/usj9dcWi.
 */

//#define USE_COMPILEDPING

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Log:HTML"

// std
#include <cmath>
#include <iostream>
#include <memory>
#include <charconv>

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
fzloghtml::fzloghtml() : formalizer_standard_program(false), config(*this), flowcontrol(flow_log_interval), ga(*this, add_option_args, add_usage_top),
                         iscale(interval_none), interval(0), noframe(false), recent_format(most_recent_html) {
    add_option_args += "n:1:2:a:o:D:H:w:Nc:rRf:ACtF:T:";
    add_usage_top += " [-n <node-ID>] [-1 <time-stamp-1>] [-2 <time-stamp-2>] [-a <time-stamp>] [-D <days>|-H <hours>|-w <weeks>] [-o <outputfile>] [-N] [-c <num>] [-r] [-R] [-f <search-text>] [-A] [-C] [-t] [-F <raw|txt|html>] [-T <file|'STR:string'>]";
    usage_head.push_back("Generate HTML representation of requested Log records.\n");
    usage_tail.push_back(
        "The <time-stamp1>, <time-stamp_2> and <time-stamp> arguments expect standardized\n"
        "Formalizer time stamps, e.g. 202009140614, but will also accept date stamps\n"
        "of analogous form, e.g. 20200914.\n"
        "Without a Node specification, the default is:\n"
        "  start from 24 hours before end of interval\n"
        "  end at most recent Log entry\n"
        "With a Node specification but no time stamps, the default is:\n"
        "  complete Node history\n"
        "Interval start or end specified by time-stamp or relative offset takes\n"
        "precedence over number of chunks or reverse from most recent.\n"
        "If '-a' is specified without an interval in days, hours or weeks, then the\n"
        "default interval is from 3 days before to 3 days after the specified time\n"
        "stamp.\n"
        "An example of a custom template is:\n"
        "  'STR:{{ t_chunkopen }} {{ t_diff_mins }} {{ node_id }}\\n'\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzloghtml::usage_hook() {
    ga.usage_hook();
    FZOUT("    -n belongs to <node-ID>\n"
          "    -1 start from <time-stamp-1>\n"
          "    -2 end at <time-stamp-2>\n"
          "    -a centered around <time-stamp>\n"
          "    -D interval size of <days>\n"
          "    -H interval size of <hours>\n"
          "    -w interval size of <weeks>\n"
          "    -c interval size of <num> Log chunks\n"
          "    -r interval from most recent\n"
          "    -R most recent Log data\n"
          "    -f Filter by search text\n"
          "    -A All search terms must be in a Log chunk\n"
          "    -C Case insensitive search\n"
          "    -t Show total time applied\n"
          "    -F format of most recent Log data:\n"
          "       raw, txt, html (default)\n"
          "    -T use custom template from file or string (if 'STR:')\n"
          "    -o write HTML Log interval to <outputfile> (default=STDOUT)\n"
          "    -N no HTML page frame\n");
}

std::vector<std::string> parse_search_strings(const std::string & search_strings_arg) {
    std::vector<std::string> search_strings_vec = split(search_strings_arg, ' ');
    for (unsigned int i = 0; i < search_strings_vec.size(); i++) {
        search_strings_vec[i] = trim(search_strings_vec[i]);
    }
    return search_strings_vec;
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

    case 'a': {
        time_t t = ymd_stamp_time(cargs);
        if (t==RTt_invalid_time_stamp) {
            VERBOSEERR("Invalid 'around' time or date stamp "+cargs+'\n');
            break;
        } else {
            t_center_around = t;
        }
        return true;
    }

    case 'D': {
        iscale = interval_days;
        std::from_chars(cargs.data(), cargs.data()+cargs.size(), interval);
        return true;
    }

    case 'H': {
        iscale = interval_hours;
        std::from_chars(cargs.data(), cargs.data()+cargs.size(), interval);
        return true;
    }

    case 'w': {
        iscale = interval_weeks;
        std::from_chars(cargs.data(), cargs.data()+cargs.size(), interval);
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

    case 'c': {
        std::from_chars(cargs.data(), cargs.data()+cargs.size(), filter.limit);
        return true;
    }

    case 'r': {
        filter.back_to_front = true;
        return true;
    }

    case 'R': {
        flowcontrol = flow_most_recent;
        return true;
    }

    case 'f': {
        search_strings = parse_search_strings(cargs);
        return true;
    }

    case 'A': {
        mustcontainall = true;
        return true;
    }

    case 'C': {
        caseinsensitive = true;
        return true;
    }

    case 't': {
        show_total_time_applied = true;
        return true;
    }

    case 'F': {
        if (cargs == "raw") {
            fzlh.recent_format = most_recent_raw;
        } else {
            if (cargs == "txt") {
                fzlh.recent_format = most_recent_txt;
            } else {
                fzlh.recent_format = most_recent_html;
            }
        }
        return true;
    }

    case 'T': {
        fzlh.custom_template = cargs;
        return true;
    }

    }

    return false;
}

/// Configure configurable parameters.
bool fzlh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(dest, "outputfile", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(interpret_text, "interpret_text", parlabel, config_parse_text_interpretation(parvalue));
    CONFIG_TEST_AND_SET_PAR(node_excerpt_len, "node_excerpt_len", parlabel, std::atoi(parvalue.c_str()));
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

}

void fzloghtml::set_filter() {
    if (t_center_around != RTt_unspecified) {
        if (iscale == interval_none) {
            iscale = interval_days;
            interval = 6; // default is from 3 days before to 3 days after
        }
        unsigned int half_interval = std::ceil(((float)interval)/2.0);
        switch (iscale) {
        case interval_weeks: {
            filter.t_from = t_center_around - half_interval*(7*24*60*60);
        }
        case interval_hours: {
            filter.t_from = t_center_around - half_interval*(60*60);
        }
        default: { // days
            filter.t_from = t_center_around - half_interval*(24*60*60);
        }
        }
        filter.t_to = RTt_unspecified;
    }
    if (!((filter.t_from!=RTt_unspecified) && (filter.t_to!=RTt_unspecified))) { // not a fully absolute time interval
        if (iscale != interval_none) {
            if ((filter.t_from==RTt_unspecified) && (filter.t_to==RTt_unspecified)) { // relative interval from most recent
                get_newest_Log_data(fzlh.ga, fzlh.edata);
                filter.t_to = fzlh.edata.newest_chunk_t;                
            }
            if ((filter.t_to==RTt_unspecified) && (filter.t_from!=RTt_unspecified)) {
                switch (iscale) { // relative interval from a specified time
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
            } else {
                switch (iscale) { // relative interval to a specified time
                case interval_weeks: {
                    filter.t_from = filter.t_to - interval * (7 * 24 * 60 * 60);
                }
                case interval_hours: {
                    filter.t_from = filter.t_to - interval * (60 * 60);
                }
                default: { // days
                    filter.t_from = filter.t_to - interval * (24 * 60 * 60);
                }
                }
            }
        } else {
            if (filter.t_from!=RTt_unspecified) {
                if (filter.limit>0) { // number of chunks from a specified time
                } else { // from specified time (set a limit)
                    filter.t_to = filter.t_from + (24*60*60); // one day default
                }
            } else {
                if (filter.back_to_front) {
                    if (filter.limit>0) {
                        if (filter.t_to!=RTt_unspecified) { // number of chunks earlier from specified time
                            // all good
                        } else { // number of chunks earlier from most recent
                            //get_newest_Log_data(fzlh.ga, fzlh.edata);
                            //filter.t_to = fzlh.edata.newest_chunk_t;
                        }
                    } else {
                        if (filter.t_to!=RTt_unspecified) { // earlier from specified time (set a limit)
                            filter.t_from = filter.t_to - (24*60*60); // one day default
                        } else { // earlier from most recent (set a limit)
                            get_newest_Log_data(fzlh.ga, fzlh.edata);
                            filter.t_to = fzlh.edata.newest_chunk_t;
                            filter.t_from = filter.t_to - (24*60*60); // one day default
                        }
                    }
                } else {
                    if (filter.limit>0) {
                        if (filter.t_to!=RTt_unspecified) { // number of chunks to specified time (flip it!)
                            filter.back_to_front = true;
                        } else { // number of chunks from most recent
                            filter.back_to_front = true;
                        }
                    } else {
                        if (filter.t_to!=RTt_unspecified) { // to specified time (set a limit)
                            filter.t_from = filter.t_to - (24*60*60); // one day default
                        } else { // set default (no Log constraints were set at all)
                            if (filter.nkey.isnullkey()) { // for Node history the default is the full history
                                get_newest_Log_data(fzlh.ga, fzlh.edata);
                                filter.t_to = fzlh.edata.newest_chunk_t;
                                filter.t_from = filter.t_to - (24*60*60); // one day default
                            }
                        }
                    }
                }
            }
        }
    }

    // A filter.nkey is a whole other situation.

    /*
    // Note that RTt_unspecified means not to impose a constraint on the corresponding t variable (see Log_filter in Logtypes.hpp).
    if (filter.nkey.isnullkey() && unlimited) { // with a Node specifier, unbounded is fine, without one, the default should be constrained
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
    */

    //graph = ga.request_Graph_copy();
    //log = ga.request_Log_copy(); *** This has to change... not every request needs the whole log. See https://trello.com/c/O9dcTm9L and https://trello.com/c/EppSyY9Y.
    VERYVERBOSEOUT(filter.info_str());

}

bool fzloghtml::get_Log_interval() {

    edata.log_ptr = ga.request_Log_excerpt(filter);

    if (!edata.log_ptr) {
        standard_error("Missing Log excerpt.", __func__);
        return false;
    }

    VERYVERBOSEOUT("\nfound:\n");
    VERYVERBOSEOUT("  chunks : "+std::to_string(edata.log_ptr->num_Chunks())+'\n');
    VERYVERBOSEOUT("  entries: "+std::to_string(edata.log_ptr->num_Entries())+"\n\n");

    // *** Should we call log.setup_Chain_nodeprevnext() ?

    if (show_total_time_applied) {
        total_minutes_applied = Chunks_total_minutes(edata.log_ptr->get_Chunks());
    }

    return true;
}

Graph_ptr fzloghtml::get_Graph_ptr() {
    ERRTRACE;
    if (!graph_attempted) {
        if (!graphmemman.get_Graph(edata.graph_ptr)) {
            standard_warning("Memory resident Graph not found. Continuing.", __func__);
        }
        graph_attempted = true;
    }
    return edata.graph_ptr;
}

bool read_and_render_Log_interval() {
    fzlh.set_filter();
    if (!fzlh.get_Log_interval()) {
        return false;
    }
    return render_Log_interval();
}

bool most_recent_data() {
    get_newest_Log_data(fzlh.ga, fzlh.edata);
    return render_Log_most_recent();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzlh.init_top(argc, argv);

    // currently, flow_log_interval is the default

    switch (fzlh.flowcontrol) {

    case flow_log_interval: {
        return standard_exit(read_and_render_Log_interval(), "Log interval rendered.\n", exit_file_error, "Unable to render interval", __func__);
    }

    case flow_most_recent: {
        return standard_exit(most_recent_data(), "Most recent Log data obtained.\n", exit_file_error, "Unable to obtain most recent Log data", __func__);
    }

    default: {
        fzlh.print_usage();
    }

    }

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