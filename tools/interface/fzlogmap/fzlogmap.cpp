// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Map specified Log intervals in terms of specified Node groupings.
 * 
 * A specified (or default) interval of Log content is mapped through
 * a set of Node categories.
 * 
 * For more about this, see https://trello.com/c/eSsBEkF0.
 */

//#define USE_COMPILEDPING

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Log:Map"

// std
#include <iostream>
#include <memory>
#include <charconv>
#include <algorithm>
#include <random>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "TimeStamp.hpp"
#include "stringio.hpp"
#include "jsonlite.hpp"

// local
#include "version.hpp"
#include "fzlogmap.hpp"
//#include "render.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzlogmap fzlm;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzlogmap::fzlogmap() : formalizer_standard_program(false), config(*this), flowcontrol(flow_log_interval), ga(*this, add_option_args, add_usage_top),
                         iscale(interval_none), interval(0), noframe(false), calendar(false), recent_format(most_recent_html) {
    add_option_args += "1:2:o:D:H:w:Nc:rRF:T:f:C";
    add_usage_top += " [-1 <time-stamp-1>] [-2 <time-stamp-2>] [-D <days>|-H <hours>|-w <weeks>] [-o <outputfile>] [-N] [-c <num>] [-r] [-R] [-F <raw|txt|html>] [-T <file|'STR:string'>] [-f <groupsfile>] [-C]";
    usage_head.push_back("Generate Mapping of requested Log records.\n");
    usage_tail.push_back(
        "The <time-stamp1> and <time-stamp_2> arguments expect standardized\n"
        "Formalizer time stamps, e.g. 202009140614, but will also accept date stamps\n"
        "of analogous form, e.g. 20200914.\n"
        "The default is:\n"
        "  start from 24 hours before end of interval\n"
        "  end at most recent Log entry\n"
        "Interval start or end specified by time-stamp or relative offset takes\n"
        "precedence over number of chunks or reverse from most recent.\n");
        //"An example of a custom template is:\n"
        //"  'STR:{{ t_chunkopen }} {{ t_diff_mins }} {{ node_id }}\\n'\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzlogmap::usage_hook() {
    ga.usage_hook();
    FZOUT("    -1 start from <time-stamp-1>\n"
          "    -2 end at <time-stamp-2>\n"
          "    -D interval size of <days>\n"
          "    -H interval size of <hours>\n"
          "    -w interval size of <weeks>\n"
          "    -c interval size of <num> Log chunks\n"
          "    -r interval from most recent\n"
          "    -R most recent Log data\n"
          "    -F format of most recent Log data:\n"
          "       raw, txt, html (default)\n"
          "    -T use custom template from file or string (if 'STR:')\n"
          "    -f read category group specifications from <groupsfile>\n"
          "    -C present in calendar format\n"
          "    -o write HTML Log interval to <outputfile> (default=STDOUT)\n"
          "    -N no HTML page frame\n");
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
bool fzlogmap::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs))
            return true;

    switch (c) {

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

    case 'F': {
        if (cargs == "raw") {
            fzlm.recent_format = most_recent_raw;
        } else {
            if (cargs == "txt") {
                fzlm.recent_format = most_recent_txt;
            } else {
                fzlm.recent_format = most_recent_html;
            }
        }
        return true;
    }

    case 'T': {
        fzlm.custom_template = cargs;
        return true;
    }

    case 'f': {
        fzlm.config.categoryfile = cargs;
        return true;
    }

    case 'C': {
        fzlm.calendar = true;
        return true;
    }

    }

    return false;
}

/// Configure configurable parameters.
bool fzlm_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(dest, "outputfile", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(interpret_text, "interpret_text", parlabel, config_parse_text_interpretation(parvalue));
    CONFIG_TEST_AND_SET_PAR(categoryfile, "categoryfile", parlabel, parvalue);
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
void fzlogmap::init_top(int argc, char *argv[]) {
    ERRTRACE;
    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class

}

Graph & fzlogmap::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(edata.graph_ptr)) { 
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *edata.graph_ptr;
}

void fzlogmap::set_filter() {
    if (!((filter.t_from!=RTt_unspecified) && (filter.t_to!=RTt_unspecified))) { // not a fully absolute time interval
        if (iscale != interval_none) {
            if ((filter.t_from==RTt_unspecified) && (filter.t_to==RTt_unspecified)) { // relative interval from most recent
                get_newest_Log_data(fzlm.ga, fzlm.edata);
                filter.t_to = fzlm.edata.newest_chunk_t;                
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
                            //get_newest_Log_data(fzlm.ga, fzlm.edata);
                            //filter.t_to = fzlm.edata.newest_chunk_t;
                        }
                    } else {
                        if (filter.t_to!=RTt_unspecified) { // earlier from specified time (set a limit)
                            filter.t_from = filter.t_to - (24*60*60); // one day default
                        } else { // earlier from most recent (set a limit)
                            get_newest_Log_data(fzlm.ga, fzlm.edata);
                            filter.t_to = fzlm.edata.newest_chunk_t;
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
                                get_newest_Log_data(fzlm.ga, fzlm.edata);
                                filter.t_to = fzlm.edata.newest_chunk_t;
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

    //log = ga.request_Log_copy(); *** This has to change... not every request needs the whole log. See https://trello.com/c/O9dcTm9L and https://trello.com/c/EppSyY9Y.
    VERYVERBOSEOUT(filter.info_str());

}

void fzlogmap::get_Log_interval() {

    edata.log_ptr = ga.request_Log_excerpt(filter);

    VERYVERBOSEOUT("\nfound:\n");
    VERYVERBOSEOUT("  chunks : "+std::to_string(edata.log_ptr->num_Chunks())+'\n');
    VERYVERBOSEOUT("  entries: "+std::to_string(edata.log_ptr->num_Entries())+"\n\n");

    // *** Should we call log.setup_Chain_nodeprevnext() ?

}

time_t fzlogmap::Log_interval_t_start() {
    if (edata.log_ptr) {
        return edata.log_ptr->oldest_chunk_t();
    } else {
        return RTt_unspecified;
    }
}

time_t fzlogmap::Log_interval_t_end() {
    if (edata.log_ptr) {
        return edata.log_ptr->newest_chunk_t();
    } else {
        return RTt_unspecified;
    }
}

/*
  *** Build this here and move it to GraphLogxmap afterwards...

  This one will probably be even more basic and fundamental than fzupdate:epsmap.hpp:EPS_map.
*/

constexpr size_t minutes_day = 24*60;
constexpr size_t bytes_per_pointer = sizeof(Node_ptr);
constexpr size_t fzmr_bytes_per_day = bytes_per_pointer*minutes_day; // about 11.5k per day on 64 bit system (4.2MBytes per mapped year, i.e. 80 MBytes for 20 years)
constexpr size_t minutes_per_week = 7*minutes_day;
constexpr time_t seconds_per_week = minutes_per_week*60;

/**
 * The smallest allocation time unit for Formalizer Log and Schedule purposes is a minute.
 * Consequently, minutes are used in full resolution Log (or Schedule) mapping.
 */
typedef std::vector<Node_ptr> fz_minute_record_t;
//typedef std::map<time_t, Node_ptr> fz_epoch_to_minute_record_map_t; // *** not sure if we'll ever this type

/**
 * 
 * Note A: The populate() function assigns Node pointers to the record minutes in accordance with
 *         a specified Log. It is possible to develop similar functions that can assign according
 *         to other specifications, e.g. when using various approaches to Scheduling.
 *         *** EPS_Map could be refactored to use Minute_Record_Map at its base if that makes sense.
 * 
 * Note B: *** We may want to switch this to using tai_clock time points (defined in chrono) once we
 *             can reliably use those (new in C++20), because those are supposed to be unique, without
 *             leap seconds and therefore without any ambiguity. See the considerations noted in
 *             the Standardiztion Trello card at https://trello.com/c/T4nTOVWy.
 */
struct Minute_Record_Map {
    fz_minute_record_t minuterecord;
    time_t t_start;
    time_t t_end;

    Minute_Record_Map(time_t from_t, time_t before_t, bool inclusive = false) : t_start(from_t), t_end(before_t) { init(inclusive); }
    Minute_Record_Map(Graph & graph, Log & log) {
        t_start = log.oldest_chunk_t();
        t_end = log.newest_chunk_t();
        init(true);
        populate(graph, log);
    }

    void init(bool inclusive = false) {
        if (inclusive) {
            t_end += 60;
        }
        size_t approx_size = ((t_end - t_start) / 60) + 1; // This hopefully hops over leap seconds and includes an extra minute.
        minuterecord.resize(approx_size, nullptr);
    }

    size_t minutes() const {
        return minuterecord.size();
    }

    Node_ptr at(size_t idx) const {
        if (idx < minuterecord.size()) {
            return minuterecord.at(idx);
        }
        return nullptr;
    }

    ssize_t record_index(time_t t) const {
        return (t - t_start) / 60;
    }

    Node_ptr at_t(time_t t) const {
        ssize_t idx = record_index(t);
        if (idx >= 0) {
            return at(idx);
        }
        return nullptr;
    }


    void populate(Graph & graph, Log & log) {
        Log_chunks_Map & chunks = log.get_Chunks();
        for (const auto & [chunk_id, chunk_ptr] : chunks) {
            if (chunk_ptr) {
                time_t t_open = chunk_ptr->get_open_time();
                time_t t_close = chunk_ptr->get_close_time(); // If FZ_TCHUNK_OPEN then there is no record of consumed time for this chunk yet.
                if (t_close != FZ_TCHUNK_OPEN) {
                    Node_ptr nptr = chunk_ptr->get_Node(graph);
                    ssize_t ridx_from = record_index(t_open);
                    if (ridx_from < 0) {
                        ridx_from = 0; // skip any before the mapped record
                    }
                    ssize_t ridx_before = record_index(t_close);
                    std::fill (minuterecord.begin()+ridx_from, minuterecord.begin()+ridx_before, nptr);
                }
            }
        }
    }

};

// *** probably move this to a library as well
typedef std::set<std::string> category_set_t;
typedef std::map<Node_ptr, std::string> node_category_caches_t;
struct Node_Category_Cache_Map {
    node_category_caches_t nodecatcache;

    void add(Node & node) {
        nodecatcache.emplace(&node, "");
    }

    std::string & cat_cache(Node & node) {
        return nodecatcache[&node];
    }

    size_t num_nodes() {
        return nodecatcache.size();
    }

    /**
     * Parse the cached Node categories and collect the full set of
     * unique categories.
     * 
     * @param categories Set of strings to receive the unique set of categories found.
     */
    void category_set(category_set_t & categories) {
        VERYVERBOSEOUT("Categories: ")
        for (const auto & [nptr, cat_cache] : nodecatcache) {
            if (nptr) {
                categories.emplace(cat_cache);
                VERYVERBOSEOUT(cat_cache+',');
            }
        }
        VERYVERBOSEOUT('\n');
        VERYVERBOSEOUT("Unique: ");
        for (const auto & catstr : categories) {
            VERYVERBOSEOUT(catstr+',');
        }
        VERYVERBOSEOUT('\n');
    }

    void random_cache_chars() {
        const std::string charsymbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abzdefghijklmnopqrstuvwxyz";
        std::default_random_engine generator;
        std::uniform_int_distribution<int> distribution(0,charsymbols.size()-1);
        for (auto & [n_ptr, cat_cache] : nodecatcache) {
            cat_cache = charsymbols[distribution(generator)];
        }
    }
};

// *** this is pretty Log specific, so perhaps keep this here in fzlogmap
void build_Node_category_cache_map(Log & log, Node_Category_Cache_Map & nccmap) {
    Log_chunks_Map & chunks = log.get_Chunks();
    Graph & graph = fzlm.graph();
    for (const auto & [chunk_id, chunk_ptr] : chunks) {
        if (chunk_ptr) {
            Node_ptr nptr = chunk_ptr->get_Node(graph);
            if (nptr) {
                nccmap.add(*nptr);
            }
        }
    }
}

std::string map2str(const Minute_Record_Map & mrmap, Node_Category_Cache_Map & nccmap, Set_builder_data & groups, bool singlechar = true) {
    std::string mapstr;
    Graph & graph = fzlm.graph();
    time_t t_daystart = day_start_time(mrmap.t_start);
    size_t skip_minutes = (mrmap.t_start - t_daystart)/60;
    if (skip_minutes > 0) {
        for (size_t i = 0; i < skip_minutes; ++i) {
            if ((i % 60) == 0) { //((i % minutes_day) == 0) {
                mapstr += '\n';
            }
            mapstr += '>';
        }
    }
    size_t char_count = skip_minutes;
    for (size_t i = 0; i < mrmap.minutes(); ++i) {
        if ((char_count % 60) == 0) { //((char_count % minutes_day) == 0) {
            mapstr += '\n';
            if ((char_count % minutes_day) == 0) {
                mapstr += "[DAY ---]\n";
            }
        }
        Node_ptr nptr = mrmap.at(i);
        if (nptr) {
            if (singlechar) {
                // *** beware, this is not testing if empty
                mapstr += groups.node_category(graph, *nptr, nccmap.cat_cache(*nptr))[0];
            } else {
                mapstr += groups.node_category(graph, *nptr, nccmap.cat_cache(*nptr));
            }
        } else {
            mapstr += '_';
        }
        ++char_count;
    }
    mapstr += '\n';
    return mapstr;
}

constexpr ssize_t minsperrow = 30;
constexpr ssize_t rowsperhour = 60/minsperrow;

std::string map2cal(const Minute_Record_Map & mrmap, Node_Category_Cache_Map & nccmap, Set_builder_data & groups, bool singlechar = true, cat_translation_map_ptr translation = nullptr) {
    std::string mapstr;
    Graph & graph = fzlm.graph();
    time_t t_daystart = day_start_time(mrmap.t_start);
    size_t i_start = (t_daystart - mrmap.t_start)/60;
    for (ssize_t dayrow = 0; dayrow < (rowsperhour*24); ++dayrow) {
        for (ssize_t daycol = 0; daycol < 7; ++daycol) {
            for (ssize_t row_minute = 0; row_minute < minsperrow; ++row_minute) {
                ssize_t i = i_start + (minsperrow*dayrow) + (24*60*daycol) + row_minute;
                if ((i >= 0) && (i < (ssize_t)mrmap.minutes())) {
                    Node_ptr nptr = mrmap.at(i);
                    if (nptr) {
                        if (singlechar) {
                            // *** beware, this is not testing if empty
                            mapstr += groups.node_category(graph, *nptr, nccmap.cat_cache(*nptr), translation)[0];
                        } else {
                            mapstr += groups.node_category(graph, *nptr, nccmap.cat_cache(*nptr), translation);
                        }
                    } else {
                        mapstr += '_';
                    }
                } else {
                    mapstr += '.';
                }
                if (row_minute == (minsperrow-1)) {
                    if (daycol == 6) {
                        if ((dayrow % rowsperhour) == 0) {
                            mapstr += to_precision_string((dayrow / rowsperhour), 0, ' ', 3) + '\n';
                        } else {
                            mapstr += '\n';
                        }
                    } else {
                        mapstr += "  ";
                    }
                }
            }
        }
    }
    return mapstr;
}

/*
Another alternative:Provide 7 times 48 strings. Fill them with substrings the size of the number of minutes of each mapped Node.
Add color in front and back. Put them together as a calendar. Might want to switch that into place of the mapping function,
since there's no point having to reconstitute from pointers.
*/

// *** extremely inefficient, even for incrementing from a map!
//     at the least, you should just build a map with category label and a totals structure,
//     where the totals structure has a place for grant total and a vector for day totals
struct Minute_Totals {
    std::map<std::string, size_t> mintotals;
    Minute_Totals(category_set_t & categories) {
        for (const auto & catstr : categories) {
            mintotals.emplace(catstr,0);
        }
    }
};
typedef std::unique_ptr<Minute_Totals> Minute_Totals_ptr;
typedef std::vector<Minute_Totals_ptr> Minute_Totals_vec_t;
//typedef std::unique_ptr<Minute_Totals_vec_t> Minute_Totals_vec_ptr;

// *** The following is not the most efficient way to do this. It is more efficient
//     not to make the whole maps when you can just add the intervals to the totals.
Minute_Totals_vec_t map2totals(const Minute_Record_Map & mrmap, Node_Category_Cache_Map & nccmap, Set_builder_data & groups, category_set_t & categories) {
    //Minute_Totals_vec_ptr mintotvec = std::make_unique<Minute_Totals_vec_t>();
    Minute_Totals_vec_t mintotvec;
    Graph & graph = fzlm.graph();
    time_t t_daystart = day_start_time(mrmap.t_start);
    size_t skip_minutes = (mrmap.t_start - t_daystart)/60;
    size_t mins_count = skip_minutes;
    std::unique_ptr<Minute_Totals> mintot_ptr = std::make_unique<Minute_Totals>(categories);
    for (size_t i = 0; i < mrmap.minutes(); ++i) {
        if ((mins_count % minutes_day) == 0) {
            if (i > 0) {
                mintotvec.push_back(std::move(mintot_ptr));
                mintot_ptr = std::make_unique<Minute_Totals>(categories);
            }
        }
        Node_ptr nptr = mrmap.at(i);
        if (nptr) {
            auto it = mintot_ptr->mintotals.find(groups.node_category(graph, *nptr, nccmap.cat_cache(*nptr)));
            if (it != mintot_ptr->mintotals.end()) {
                it->second++;
            }
        }
        ++mins_count;
    }
    mintotvec.push_back(std::move(mintot_ptr));
    return mintotvec;
}

std::string totalsheader(const cat_translation_map & decode, unsigned int colwidth = 7, bool hours = true) {
    std::string headerstr;
    for (const auto & [code, catstr] : decode) {
        if (catstr.size() >= colwidth) {
            headerstr += catstr.substr(0,colwidth) + ' ';
        } else {
            std::string catfill(colwidth - catstr.size(),' ');
            headerstr += catfill + catstr + ' ';
        }        
    }
    headerstr.back() = '\n';
    return headerstr;
}

std::string legend(const cat_translation_map & decode) {
    std::string legendstr("\n");
    for (const auto & [code, catstr] : decode) {
        legendstr += code + " = " + catstr + ", ";
    }
    if (!legendstr.empty()) {
        legendstr.pop_back();
        legendstr.back() = '\n';
    }
    legendstr += '\n';
    return legendstr;
}

/**
 * ...
 * 
 * @param header Include categories header or not. (See how this is used to avoid printing map print codes.)
 * ...
 */
std::string totals2str(Minute_Totals_vec_t & mintotvec, category_set_t & categories, bool header = true, unsigned int colwidth = 7, bool hours = true) {
    std::string totstr;
    if (header) {
        for (const auto & catstr : categories) {
            if (catstr.size() >= colwidth) {
                totstr += catstr.substr(0,colwidth) + ' ';
            } else {
                std::string catfill(colwidth - catstr.size(),' ');
                totstr += catfill + catstr + ' ';
            }
        }
        totstr.back() = '\n';
    }
    Minute_Totals grandtotals(categories);
    for (const auto & mintot_ptr : mintotvec) {
        for (const auto & [categorystr, minutes] : mintot_ptr->mintotals) {
            if (hours) {
                totstr += to_precision_string(minutes/60.0, 2, ' ', colwidth) + ' ';
            } else {
                totstr += to_precision_string(minutes, 0, ' ', colwidth) + ' '; // totstr += std::to_string(minutes) + ' ';
            }
            grandtotals.mintotals[categorystr] += minutes;
        }
        totstr += '\n';
    }
    totstr += '\n';
    for (const auto & [categorystr, minutes] : grandtotals.mintotals) {
        if (hours) {
            totstr += to_precision_string(minutes/60.0, 2, ' ', colwidth) + ' ';
        } else {
            totstr += to_precision_string(minutes, 0, ' ', colwidth) + ' ';
        }
    }
    totstr += '\n';   
    return totstr;
}

std::size_t add_category_specifications(const std::string jsoncontent, size_t search_from, Set_builder_data & groups) {
    // find JSON end bracket
    size_t end_bracket_pos = jsoncontent.find('}', search_from);
    if (end_bracket_pos == std::string::npos) {
        return end_bracket_pos;
    }
    // from there, find opening bracket
    size_t start_bracket_pos = jsoncontent.rfind('{', end_bracket_pos);
    if ((start_bracket_pos == std::string::npos) || (start_bracket_pos < search_from)) {
        return std::string::npos;
    }
    // find the category label
    size_t end_quotes_pos = jsoncontent.rfind('"', start_bracket_pos);
    if ((end_quotes_pos == std::string::npos) || (end_quotes_pos < search_from)) {
        return std::string::npos;
    }
    size_t start_quotes_pos = jsoncontent.rfind('"', end_quotes_pos - 1);
    if ((start_quotes_pos == std::string::npos) || (start_quotes_pos < search_from)) {
        return std::string::npos;
    }
    std::string category = jsoncontent.substr(start_quotes_pos+1, end_quotes_pos - start_quotes_pos - 1);
    if (category.empty()) {
        return std::string::npos;
    }
    VERYVERBOSEOUT("Category: "+category+'\n');
    // find the configlines in the interval
    std::string group_jsoncontent = jsoncontent.substr(start_bracket_pos, end_bracket_pos - start_bracket_pos + 1);
    VERYVERBOSEOUT("Section: "+group_jsoncontent+'\n');
    auto configlines = json_get_param_value_lines(group_jsoncontent);
    // get parameters and values in the lines
    for (const auto& it : configlines) {
        VERYVERBOSEOUT("Line: "+it+'\n');
        auto [parlabel, parvalue] = json_param_value(it);
        if ((!parlabel.empty()) && (!is_json_comment(parlabel))) {
            // from those, for the labeled category, specify NNLs, Label-Values, Topics
            if (parlabel == "NNLs") {
                auto nnls = split(parvalue,';');
                for (const auto & list_name : nnls) {
                    groups.NNL_to_category[list_name] = category;
                    VERYVERBOSEOUT("  NNL: "+list_name+'\n');
                }
            } else if (parlabel == "LVs") {
                auto lvs = split(parvalue, ';');
                for (const auto & label : lvs) {
                    groups.LV_to_category[label] = category;
                    VERYVERBOSEOUT("  LV: "+label+'\n');
                }
            } else if (parlabel == "Topics") {
                auto topics = split(parvalue, ';');
                for (const auto & topictag : topics) {
                    groups.Topic_to_category[topictag] = category;
                    VERYVERBOSEOUT("Topic: "+topictag+'\n');
                }
            } else {
                VERBOSEERR("Unrecognized specifier: "+parlabel+'\n');
            }
        }
    }
    // return position past end bracket
    return end_bracket_pos+1;
}

bool fzlogmap::set_groups(Set_builder_data & groups) {
    if (config.categoryfile.empty()) {
        return false;
    }
    std::string jsoncontent;
    if (!file_to_string(config.categoryfile, jsoncontent)) {
        ERRRETURNFALSE(__func__, "Unable to load configuration file "+config.categoryfile);
    }

    size_t search_from = 0;
    while ((search_from = add_category_specifications(jsoncontent, search_from, groups)) != std::string::npos);
    
    // find possible default group
    size_t default_pos = jsoncontent.find("\"DEFAULT\"");
    if (default_pos != std::string::npos) {
        size_t end_quotes = jsoncontent.rfind('"', default_pos - 1);
        if (end_quotes != std::string::npos) {
            size_t start_quotes = jsoncontent.rfind('"', end_quotes-1);
            if (start_quotes != std::string::npos) {
                std::string default_category = jsoncontent.substr(start_quotes+1, end_quotes - start_quotes - 1);
                groups.default_category = default_category;
                VERYVERBOSEOUT("Default category: "+groups.default_category+'\n');
            }
        }
    }

    return true;
}

void setup_print_codes(cat_translation_map & printcodes) {
    // ** You could get this from another file, or, you could integrate
    //    parsing and setting this up as part of geting the JSON file content,
    //    in which case printcodes could just become another parameter of the
    //    same struct.
    // ** Here, this is hardcoded just for demonstration purposes.
    printcodes["*SLEEP"] = "z";
    printcodes["IPAB"] = "\u001b[42mI\u001b[0m"; //"I";
    printcodes["DAYINT"] = "\u001b[44mD\u001b[0m"; //"D";
    printcodes["BUILDSYSTEM"] = "\u001b[43mB\u001b[0m";
    printcodes["SYSTEM"] = "\u001b[43mS\u001b[0m";
    printcodes["HOBBIES"] = "\u001b[41mH\u001b[0m";
    printcodes["MEALS"] = "\u001b[45mM\u001b[0m";
    printcodes["%SOLO"] = "\u001b[41ms\u001b[0m";
    printcodes["@SOCIAL"] = "\u001b[41m@\u001b[0m";
    printcodes["CHORES"] = "\u001b[40;1mC\u001b[0m";
    printcodes["TRAVEL"] = "\u001b[45mT\u001b[0m";
    printcodes["WELLBEING"] = "\u001b[45mW\u001b[0m";
    printcodes["other"] = "\u001b[46mo\u001b[0m";
}

/**
 * Get the set of unique category codes and a decoding map.
 * 
 * Note: Where the decoding map produced here may be used, you could
 *       instead use any other map from the unique set of codes to
 *       transformed category labels.
 * 
 * @param encode A cat_translation_map that encodes from groups to another set of code strings.
 * @param uniquecodes Receives the unique set of transformation codes used in a cat_translation_map.
 * @param decode Receives a decoding map.
 */
void unique_encode_decode(const cat_translation_map & encode, category_set_t & uniquecodes, cat_translation_map & decode) {
    for (const auto & [key, code] : encode) {
        uniquecodes.emplace(code);
        decode[code] = key;
    }
}


bool make_map() {
    // Get Log interval according to filter.
    fzlm.set_filter();
    fzlm.get_Log_interval();
    if (!fzlm.edata.log_ptr) {
        return false;
    }
    Log & logref = *(fzlm.edata.log_ptr);

    // Map Log interval to Nodes.
    Minute_Record_Map mrmap(fzlm.graph(), logref);
    VERYVERBOSEOUT("Mapped "+std::to_string(mrmap.minutes())+" minutes to Nodes.\n");

    // Map Nodes to Category groups.
    Node_Category_Cache_Map nccmap;
    build_Node_category_cache_map(logref, nccmap);
    VERYVERBOSEOUT("Found "+std::to_string(nccmap.num_nodes())+ " Nodes in Log interval.\n");

    Set_builder_data groups;
    if (fzlm.config.categoryfile.empty()) {
        nccmap.random_cache_chars();
        groups.default_category = "?";
    } else {
        if (!fzlm.set_groups(groups)) {
            return false;
        }
    }
    //nccmap.category_set(categories);
    //VERYVERBOSEOUT("Mapping Nodes to "+std::to_string(categories.size())+" categories.");

    // Cross-map Log interval to Category groups.
    if (fzlm.calendar) {
        // Add a translation map frpm groups to print codes. Note that there do not
        // need to be as many print codes as groups. Some groups may have no print
        // code, and multiple groups can be mapped to the same print code.
        // See below how that affects computed totals!
        cat_translation_map printcodes;
        setup_print_codes(printcodes);

        // Let's prepare the set of unique printcodes, as well as a decoding
        // map. Note that you could use some other map for decode instead.
        category_set_t uniquecodes;
        cat_translation_map decode;
        unique_encode_decode(printcodes, uniquecodes, decode);

        FZOUT(legend(decode));
        FZOUT(map2cal(mrmap, nccmap, groups, false, &printcodes));
        // Get the unique set of categories from the Node Category Cache Map.
        // Note that this set can be smaller than the number of printcodes keys if
        // not all were represented by Nodes in the time interval.
        category_set_t categories;
        nccmap.category_set(categories); // categories is a subset of uniquecodes
        VERYVERBOSEOUT("Mapped Nodes to "+std::to_string(categories.size())+" categories.\n");

        // If a calendar was made then the set of categories is full of printcodes. We need
        // those to compute totals, but we should decode them to display category names.
        // Let's collect totals for all unique print codes in case there are some that
        // are not in the categories found.
        auto totals = map2totals(mrmap, nccmap, groups, uniquecodes);

        // Print the decoded print code categories here rather than the transformed
        // print codes used in the map.
        FZOUT('\n'+totalsheader(decode));
        FZOUT(totals2str(totals, uniquecodes, false));

    } else {
        FZOUT(map2str(mrmap, nccmap, groups));
        // Get the unique set of categories from the Node Category Cache Map.
        // Note that this set can be smaller than the number of groups if
        // not all were represented by Nodes in the time interval.
        category_set_t categories;
        nccmap.category_set(categories);
        VERYVERBOSEOUT("Mapped Nodes to "+std::to_string(categories.size())+" categories.\n");
        auto totals = map2totals(mrmap, nccmap, groups, categories);
        FZOUT('\n'+totals2str(totals, categories));
    }

    return true;
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzlm.init_top(argc, argv);

    // currently, flow_log_interval is the default

    switch (fzlm.flowcontrol) {

    case flow_log_interval: {
        return standard_exit(make_map(), "Log interval mapped.\n", exit_file_error, "Unable to map interval", __func__);
    }

    //case flow_most_recent: {
    //    return standard_exit(most_recent_data(), "Most recent Log data obtained.\n", exit_file_error, "Unable to obtain most recent Log data", __func__);
    //}

    default: {
        fzlm.print_usage();
    }

    }

    return standard.completed_ok();
}
