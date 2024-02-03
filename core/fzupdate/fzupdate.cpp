// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Update the Node Schedule.
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:Update"

//#define USE_COMPILEDPING
//#define DEBUG_SKIP

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "ReferenceTime.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
#include "tcpclient.hpp"

// local
#include "version.hpp"
#include "fzupdate.hpp"
#include "epsmap.hpp"

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzupdate fzu;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzupdate::fzupdate() : formalizer_standard_program(false), config(*this),
                 reftime(add_option_args, add_usage_top) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "rubRNT:D:P:d";
    add_usage_top += " [-r|-u|-b|-R|-N] [-T <t_max|full>] [-D <days>] [-P <pack_interval>] [-d]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "The -T limit overrides the 'map_days' configuration or default parameter.\n"
        "\n"
        "Please note that fzupdate is primarily about Schedule updating. To 'skip'\n"
        "or 'update' instances of a single repeating Node, please see the TCP API\n"
        "requests available in fzserverpq.\n"
        "\n"
        "The required time data returned with -R provides 4 values:\n"
        "  1. The total for the time from now to -T in minutes.\n"
        "  2. Corresponding projected annual ratio of available time.\n"
        "  3. Corresponding projected average weekly hours.\n"
        "  4. Corresponding projected average daily hours.\n"
    );
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzupdate::usage_hook() {
    //ga.usage_hook();
    reftime.usage_hook();
    FZOUT("    -r update repeating Nodes\n"
          "    -u update variable target date Nodes\n"
          "    -b break up EPS group of Nodes with the variable target date in -T\n"
          "    -R calculate time required for incomplete repeating Nodes to -T\n"
          "    -N calculate the minutes required for incomplete non-repeating Nodes to -T\n"
          "    -T update up to and including <t_max> or 'full' update\n"
          "    -D number of days to map with -u (default in config, or 14)\n"
          "    -P interval seconds for moveable packing beyond map (or 'none')\n"
          "    -d dry-run, do not modify Graph\n");
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
bool fzupdate::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;
    if (reftime.options_hook(c, cargs)) {
        if (reftime.Time() == RTt_invalid_time_stamp) {
            standard_exit_error(exit_general_error, "Invalid emulated time specification ("+cargs+')', __func__);
        }
        return true;
    }

    switch (c) {

    case 'r': {
        flowcontrol = flow_update_repeating;
        return true;
    }

    case 'u': {
        flowcontrol = flow_update_variable;
        return true;
    }

    case 'b': {
        flowcontrol = flow_break_eps_groups;
        return true;
    }

    case 'R': {
        flowcontrol = flow_required_repeated;
        return true;
    }

    case 'N': {
        flowcontrol = flow_required_nonrepeating;
        return true;
    }

    case 'T': {
        if (cargs=="full") {
            t_limit = RTt_maxtime;
        } else {
            t_limit = time_stamp_time(cargs);
        }
        return true;
    }

    case 'D': {
        config.map_days = std::atoi(cargs.c_str());
        return true;
    }

    case 'P': {
        if (cargs=="none") {
            config.pack_moveable = false;
        } else {
            config.pack_moveable = true;
            config.pack_interval_beyond = std::atoi(cargs.c_str());
        }
        return true;
    }

    case 'd': {
        dryrun = true;
        return true;
    }

    }

    return false;
}

unsigned int get_chunk_minutes(const std::string & parvalue) {
    int mins = std::atoi(parvalue.c_str());
    if (mins<=0) {
        standard_exit_error(exit_bad_config_value, "Invalid chunk_minutes in configuration file", __func__);
    }
    fzu.config.chunk_seconds = (unsigned)mins*60;
    return (unsigned)mins;
}

unsigned int get_positive_integer(const std::string & parvalue, std::string errmsg) {
    int v = std::atoi(parvalue.c_str());
    if (v<=0) {
        standard_exit_error(exit_bad_config_value, "Invalid "+errmsg+" in configuration file", __func__);
    }
    return (unsigned)v;
}

time_t get_time_of_day_seconds(const std::string & HM_or_seconds) {
    if (HM_or_seconds.size() == 5) {
        if (HM_or_seconds[2] == ':') {
            long hour = std::atol(HM_or_seconds.substr(0,2).c_str());
            long min = std::atol(HM_or_seconds.substr(3,2).c_str());
            if ((hour < 0) || (min < 0) || (hour > 23) || (min > 59)) {
                standard_exit_error(exit_bad_config_value, "Invalid time of day in configuration file", __func__);
            }
            if ((hour == 0) && (min == 0)) {
                return 86400;
            }
            return (hour*3600) + (min*60);
        }
    }
    time_t seconds = std::atol(HM_or_seconds.c_str());
    if ((seconds<0) || (seconds>86400)) {
        standard_exit_error(exit_bad_config_value, "Invalid time of day in configuration file", __func__);
    }
    return seconds;
}

/// Configure configurable parameters.
bool fzu_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(chunk_minutes, "chunk_minutes", parlabel, get_chunk_minutes(parvalue));
    CONFIG_TEST_AND_SET_PAR(map_multiplier, "map_multiplier", parlabel, get_positive_integer(parvalue, "map_multiplier"));
    CONFIG_TEST_AND_SET_PAR(map_days, "map_days", parlabel, get_positive_integer(parvalue, "map_days"));
    CONFIG_TEST_AND_SET_PAR(warn_repeating_too_tight,"warn_repeating_too_tight", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(endofday_priorities,"endofday_priorities", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(dolater_endofday,"dolater_endofday", parlabel, get_time_of_day_seconds(parvalue));
    CONFIG_TEST_AND_SET_PAR(doearlier_endofday,"doearlier_endofday", parlabel, get_time_of_day_seconds(parvalue));
    CONFIG_TEST_AND_SET_PAR(eps_group_offset_mins,"eps_group_offset_mins", parlabel, get_positive_integer(parvalue, "eps_group_offset_mins"));
    CONFIG_TEST_AND_SET_PAR(update_to_earlier_allowed,"update_to_earlier_allowed", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(pack_moveable,"pack_moveable", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(pack_interval_beyond, "pack_interval_beyond", parlabel, get_positive_integer(parvalue, "pack_interval_beyond"));
    CONFIG_TEST_AND_SET_PAR(fetch_days_beyond_t_limit, "fetch_days_beyond_t_limit", parlabel, get_positive_integer(parvalue, "fetch_days_beyond_t_limit"));
    CONFIG_TEST_AND_SET_PAR(showmaps,"showmaps", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(timezone_offset_hours, "timezone_offset_hours", parlabel, std::atoi(parvalue.c_str()));
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
void fzupdate::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

Graph & fzupdate::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

Graph_modifications & fzupdate::graphmod() {
    if (!graphmod_ptr) {
        if (segname.empty()) {
            standard_exit_error(exit_general_error, "Unable to allocate shared segment before generating segment name and size. The prepare_Graphmod_shared_memory() function has to be called first.", __func__);
        }
        VERYVERBOSEOUT("\nAllocating shared memory block "+segname+" with "+std::to_string(segsize)+" bytes.\n");
        graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
        if (!graphmod_ptr)
            standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);
    }
    return *graphmod_ptr;
}

void fzupdate::prepare_Graphmod_shared_memory(unsigned long _segsize) {
    segsize = _segsize;
    segname = unique_name_Graphmod(); // a unique name to share with `fzserverpq`
    VERYVERBOSEOUT("Unique shared memory block name generated: "+segname+"\n");
}

/// See implementation decisions in https://trello.com/c/eUjjF1yZ/222-how-graph-components-are-edited#comment-5fd8fed424188014cb31a937.
int update_repeating(time_t t_pass) {
    //targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_and_repeating_by_targetdate(fzu.graph());

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT, this is a wild guess
    //fzu.prepare_Graphmod_shared_memory(sizeof(TD_Node_shm)*incomplete_repeating.size() + 10240);
    fzu.prepare_Graphmod_shared_memory(10240);


    Batchmod_tpass_ptr batchmodtpass_ptr = fzu.graphmod().request_Batch_Node_Tpass(t_pass); //, incomplete_repeating);
    if (!batchmodtpass_ptr) {
        return standard_exit_error(exit_general_error, "Unable to update batch of Nodes past t_pass", __func__);
    }

    //fzu.graphmod().data.back().set_Edit_flags(editflags.get_Edit_flags());

    if (fzu.dryrun) {
        return standard_exit_success("Dryrun - update repeating done.");
    }

    auto ret = server_request_with_shared_data(fzu.get_segname(), fzu.graph().get_server_port());
    if (ret != exit_ok) {
        standard_exit_error(ret, "Graph server returned error.", __func__);
    }

    return standard_exit_success("Update repeating done.");
}

void updvar_set_t_limit(time_t t_pass) {
    if (fzu.t_limit > 0) {
        if (fzu.t_limit <= t_pass) {
            standard_exit_error(exit_command_line_error, "Specified T limit "+TimeStampYmdHM(fzu.t_limit)+" is smaller than (emulated) current time "+TimeStampYmdHM(t_pass), __func__);
        }
        return; // command line limit overrides map_days
    }
    fzu.t_limit = time_add_day(t_pass,fzu.config.map_days);
}

struct update_constraints {
    time_t t_fetchlimit = RTt_maxtime;
    unsigned long chunks_per_week = seconds_per_week / (20*60);
    size_t num_nodes_with_repeating = 0;
    unsigned long chunks_req_total = 0;
    unsigned long weeks_for_nonperiodic = 0;
    unsigned long days_in_map = 0;
    update_constraints() {
        t_fetchlimit = fzu.t_limit + (fzu.config.fetch_days_beyond_t_limit*seconds_per_day);
        chunks_per_week = seconds_per_week / ((unsigned long)fzu.config.chunk_minutes * 60);
    }
    void set(size_t num_nodes, unsigned long _chunks_req_total) {
        num_nodes_with_repeating = num_nodes;
        chunks_req_total = _chunks_req_total;
        weeks_for_nonperiodic = chunks_req_total / chunks_per_week;
        if (weeks_for_nonperiodic == 0) {
            weeks_for_nonperiodic = 1;
        }
        days_in_map = weeks_for_nonperiodic * 7 * fzu.config.map_multiplier;
        if (fzu.config.map_days > days_in_map) {
            days_in_map = fzu.config.map_days;
        }
    }
    std::string str() {
        std::string s("Limits: t_limit ("+TimeStampYmdHM(fzu.t_limit)+") + fetch_days_beyond_t_limit ("+std::to_string(fzu.config.fetch_days_beyond_t_limit)+") = t_fetchlimit = "+TimeStampYmdHM(t_fetchlimit));
        s += "\nNumber of Nodes fetched to map (including repeats)     : "+std::to_string(num_nodes_with_repeating);
        s += "\nChunks required for non-repeating Nodes (up to t_limit): "+std::to_string(chunks_req_total);
        s += "\nNumber of chunks in a week: "+std::to_string(chunks_per_week);
        s += "\nNumber of days in map     : "+std::to_string(days_in_map)+'\n';
        return s;
    }
};

std::string show_shm_request(Batchmod_targetdates_ptr batchmodreq_ptr) {
    std::string s("Shared memory segment                                              : "+fzu.get_segname());
    s += "\nPointer to shared memory location in fzupdate memory mapping       : "+std::to_string((uint64_t)graphmemman.get_segmem());
    s += "\nPointer to Graph_modifications object                              : "+std::to_string((uint64_t)(&fzu.graphmod()));
    s += "\nPointer to Data at tail of request stack                           : "+std::to_string((uint64_t)(&(fzu.graphmod().data.back())));
    s += "\nPointer retrieved from data.back().batchmodtd_ptr                  : "+std::to_string((uint64_t)(fzu.graphmod().data.back().batchmodtd_ptr.get()));
    s += "\nEquivalent offset from data.back().batchmodtd_ptr variable location: "+std::to_string((uint64_t)(fzu.graphmod().data.back().batchmodtd_ptr.get_offset()));
    s += "\nPointer reported directly in local batchmodreq_ptr variable        : "+std::to_string((uint64_t)batchmodreq_ptr);
    s += "\ntdnkeys_num reported by object in shared memory                    : "+std::to_string(fzu.graphmod().data.back().batchmodtd_ptr.get()->tdnkeys_num);
    s += "\ntdnkeys offset pointer get()                                       : "+std::to_string((uint64_t)(fzu.graphmod().data.back().batchmodtd_ptr.get()->tdnkeys.get()));
    s += "\ntdnkeys[0] location via offset pointer                             : "+std::to_string((uint64_t)(&(fzu.graphmod().data.back().batchmodtd_ptr.get()->tdnkeys[0])));
    s += "\nNode ID key content at tdnkeys[0]                                  : "+fzu.graphmod().data.back().batchmodtd_ptr.get()->tdnkeys[0].nkey.str();
    s += '\n';
    return s;
}

/**
 * This can be used by several update functions that change the target dates of a specified set of
 * Nodes.
 * 
 * @param update_nodes A map of new target dates and Node pointers.
 * @param editflags A valid Edit_flags bitmask (typically, targetdate is set).
 * @return True if the server request was successful.
 */
bool request_batch_targetdates_modifications(const targetdate_sorted_Nodes & update_nodes, const Edit_flags & editflags) {
    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT, this is a wild guess
    fzu.prepare_Graphmod_shared_memory(sizeof(TD_Node_shm)*update_nodes.size()*4 + 102400);

    Batchmod_targetdates_ptr batchmodtd_ptr = fzu.graphmod().request_Batch_Node_Targetdates(update_nodes);
    if (!batchmodtd_ptr) {
        return standard_exit_error(exit_general_error, "Unable to update batch of Node target dates", __func__);
    }
    VERBOSEOUT("Prepared server request with a batch of "+std::to_string(batchmodtd_ptr->tdnkeys_num)+" Node and target date elements\n");

    if (fzu.graphmod().data.empty()) {
        return standard_exit_error(exit_missing_data, "Missing server request data", __func__);
    }
    fzu.graphmod().data.back().set_Edit_flags(editflags.get_Edit_flags());

    VERYVERBOSEOUT(show_shm_request(batchmodtd_ptr));

    if (fzu.dryrun) {
        return standard_exit_success("Dryrun - update batch of Nodes done.");
    }

    auto ret = server_request_with_shared_data(fzu.get_segname(), fzu.graph().get_server_port());
    if (ret != exit_ok) {
        return standard_exit_error(ret, "Graph server returned error.", __func__);
    }

    return true;
}

/**
 * Note: Much of this is a re-implementation of the Formalizer 1.x process carried out by dil2al when the
 *       function alcomp.cc:generate_AL_CRT() is called.
 * 
 * Outline of steps:
 * 1. Determine the time limit (fzu.t_limit) up to which mapping is to be done.
 * 2. Initialize constraints, including constraints.t_fetchlimit (which is beyond fzu.t_limit).
 * 3. Collect all incomplete Nodes + Repeats up to constraints.t_fetchlimit.
 * 4. Determine EPS data, find out how many chunks are required for each.
 * 5. Set constraints based on the number of chunks needed for non-repeating Nodes.
 * 6. Create an EPS map for constraints.days_in_map. Set map time stamps, create list of 5 minute slots, create Node key to EPS data index lookup table.
 * 7. Place exact target date Nodes. Up to fzu.t_limit, map exact target date Nodes (plus collect some additional information).
 * 8. Place fixed target date Nodes. Up to fzu.t_limit, map fixed and inherit target date Nodes (plus collect some additional information).
 * 9. Group and place moveable (variable and unspecified) Nodes.
 * 10. From the map, collect the Nodes to update and their new target dates.
 * 11. Request modification of that batch.
 * 
 * The fzu.config.pack_moveable mode:
 *   The optional alternative update method for variable target date Nodes is to map them carefully
 *   up to the indicated time, and to move the rest to a pre-determined series of offsets beyond that.
 * 
 * Revised moveable treatment as per Trello card https://trello.com/c/zWqexJ9g:
 *   The even more alternative method is not to modify variable target date Nodes at all, but to treat
 *   them as a priority-sorted list to choose from or fill in where time permits.
 */
int update_variable(time_t t_pass) {
    updvar_set_t_limit(t_pass);

    if (fzu.config.pack_moveable) {
        VERBOSEOUT("Pack-moveable mode.\n");
    }

    if (fzu.t_limit == RTt_maxtime) {
        // Let's keep this case from exploding: Set the limit to time needed to complete non-repeating Nodes.
        size_t annual_repeating_minutes = total_minutes_incomplete_repeating(fzu.graph(), t_pass, t_pass+(seconds_per_day*365), true);  
        float minutes_per_year = 60*24*365;
        float year_ratio = (float)annual_repeating_minutes / minutes_per_year;
        if (year_ratio >= 1.0) {
            return standard_exit_error(exit_general_error, "Unable to project time needed to complete Nodes.", __func__);
        }
        size_t available_minutes_per_year = (1.0-year_ratio)*minutes_per_year;
        size_t minutes = total_minutes_incomplete_nonrepeating(fzu.graph(), t_pass, fzu.t_limit);
        size_t num_years = (minutes / available_minutes_per_year) + 1;
        VERBOSEOUT("Updating "+std::to_string(num_years)+" years.\n");
        minutes_per_year *= (60*num_years); // years int seconds
        fzu.t_limit = t_pass + (size_t)minutes_per_year;
    }

    update_constraints constraints;
    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_with_repeating_by_targetdate(fzu.graph(), constraints.t_fetchlimit, 0, fzu.config.pack_moveable);
    Edit_flags editflags;
    editflags.set_Edit_targetdate();

    eps_data_vec epsdata(incomplete_repeating);

    constraints.set(incomplete_repeating.size(), epsdata.updvar_total_chunks_required_nonperiodic(incomplete_repeating));  
    VERBOSEOUT(constraints.str());

    EPS_map updvar_map(t_pass, constraints.days_in_map, incomplete_repeating, epsdata);
    updvar_map.place_exact();
    updvar_map.place_fixed();
    updvar_map.group_and_place_movable();

    targetdate_sorted_Nodes eps_update_nodes = updvar_map.get_eps_update_nodes();

    if (eps_update_nodes.empty()) {
        VERBOSEOUT("No variable target dates within examined range to update.\n");
    } else {
        request_batch_targetdates_modifications(eps_update_nodes, editflags);
    }

    return standard_exit_success("Update variable target date Nodes done.");
}

/**
 * Break up a group of Nodes with the same variable target date.
 * 
 * @param t Target date for which to break up a Node group if one exists.
 * @return Exit code.
 */
int break_eps_group(time_t t) {
    if (t<0) {
        return standard_exit_error(exit_command_line_error, "Unable to break up group with negative target date.", __func__);
    }

    // set up filter
    Node_Filter nodefilter;
    nodefilter.lowerbound.completion = 0.0;
    nodefilter.upperbound.completion = 0.99999;
    nodefilter.lowerbound.tdproperty = variable;
    nodefilter.upperbound.tdproperty = variable;
    nodefilter.lowerbound.targetdate = t;
    nodefilter.upperbound.targetdate = t;
    nodefilter.filtermask.set_Edit_completion();
    nodefilter.filtermask.set_Edit_tdproperty();
    nodefilter.filtermask.set_Edit_targetdate();

    // find subset of Nodes
    targetdate_sorted_Nodes epsgroup = Nodes_subset(fzu.graph(), nodefilter);

    // break up group
    targetdate_sorted_Nodes update_nodes;
    if (epsgroup.size()>1) {
        epsgroup.erase(epsgroup.begin()); // one gets to keep the target date
        time_t t_new = t - 60; // offset down by a minute
        for (auto & [t_old, n_ptr] : epsgroup) {
            update_nodes.emplace(t_new, n_ptr);
            t_new -= 120;
            if (t_new < 0) {
                t_new = 0;
            }
        }
    }
    Edit_flags editflags;
    editflags.set_Edit_targetdate();
    request_batch_targetdates_modifications(update_nodes, editflags);

    return standard_exit_success("Breaking up group of Nodes with variable target date done.");
}

int required_time_for_repeated_Nodes() {
    time_t t_start = ActualTime();
    if (fzu.t_limit < t_start) {
        return standard_exit_error(exit_command_line_error, "Needs specified time limit later than current time.", __func__);
    }
    size_t minutes = total_minutes_incomplete_repeating(fzu.graph(), t_start, fzu.t_limit, true);
    float minutes_per_year = 60*24*365;
    float interval_years = (float)(fzu.t_limit - t_start) / (60.0*minutes_per_year);
    float year_ratio = (float)minutes / (interval_years*minutes_per_year);
    float day_hours = 24.0 * year_ratio;
    float week_hours = 7.0 * day_hours;
    VERBOSEOUT("Time required for repeating Nodes between "+TimeStampYmdHM(t_start)+" and "+TimeStampYmdHM(fzu.t_limit)+":\n");
    VERBOSEOUT("[total minutes] [annual ratio] [hours per week] [hours per day]\n");
    FZOUT(std::to_string(minutes)+' '+to_precision_string(year_ratio, 2)+' '+to_precision_string(week_hours, 2)+' '+to_precision_string(day_hours, 2)+'\n');
    return standard_exit_success("Required time for repeating Nodes calculated.");
}

int required_time_for_nonrepeating_Nodes() {
    time_t t_start = ActualTime();
    if (fzu.t_limit < t_start) {
        return standard_exit_error(exit_command_line_error, "Needs specified time limit later than current time.", __func__);
    }
    size_t minutes = total_minutes_incomplete_nonrepeating(fzu.graph(), t_start, fzu.t_limit);
    VERBOSEOUT("Time minutes for non-repeating Nodes between "+TimeStampYmdHM(t_start)+" and "+TimeStampYmdHM(fzu.t_limit)+":\n");
    FZOUT(std::to_string(minutes)+'\n');
    return standard_exit_success("Required time for non-repeating Nodes calculated.");
}

/**
 * Change tdproperty of Nodes in specified Named Node List to specified property.
 * 
 * For example, note how this function is used by `fztask.py` and `earlywiz.py`.
 * 
 * @param list_name The specified Named Node List.
 * @param tdproperty The `td_property` enumerated type to set to.
 * @return Exit code.
 */
/*int update_NNL_tdproperty(const std::string list_name, td_property tdproperty = td_property::variable) {
    updvar_set_t_limit(t_pass);
    update_constraints constraints;

    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_with_repeating_by_targetdate(fzu.graph(), constraints.t_fetchlimit, 0);
    Edit_flags editflags;
    editflags.set_Edit_targetdate();

    eps_data_vec epsdata(incomplete_repeating);

    constraints.set(incomplete_repeating.size(), epsdata.updvar_total_chunks_required_nonperiodic(incomplete_repeating));  
    VERBOSEOUT(constraints.str());

    EPS_map updvar_map(t_pass, constraints.days_in_map, incomplete_repeating, epsdata);
    updvar_map.place_exact();
    updvar_map.place_fixed();
    updvar_map.group_and_place_movable();

    targetdate_sorted_Nodes eps_update_nodes = updvar_map.get_eps_update_nodes();

    request_batch_targetdates_modifications(eps_update_nodes, editflags);

    return standard_exit_success("Update variable target date Nodes done.");
}*/

void test_time(time_t t) {
    FZOUT("t = "+std::to_string(t)+' ');
    FZOUT(TimeStampYmdHM(t)+' ');
    FZOUT(TimeStamp("%Y%m%d%H%M%S",t)+'\n')
}

void time_conversion_tests() {
    time_t t = -1;
    test_time(t);
    t = 0;
    test_time(t);
    t = ActualTime();
    test_time(t);
    t = RTt_maxtime;
    test_time(t);
    standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzu.init_top(argc, argv);

    if (fzu.dryrun) {
        VERBOSEOUT("Dryrun mode.\n");
    }

    switch (fzu.flowcontrol) {

    case flow_update_repeating: {
        return update_repeating(fzu.reftime.Time());
    }

    case flow_update_variable: {
        return update_variable(fzu.reftime.Time());
    }

    // *** NOTE: If we combine and carry out both repeating and variable updates then there is no need to
    //     write changes to database until all targetdates have been updated by both methods and the full
    //     list of Nodes with set Edit_flags is known.
    //     We also don't need to collect the incomplete_repeating list twice.

    case flow_break_eps_groups: {
        return break_eps_group(fzu.t_limit);
    }

    case flow_required_repeated: {
        return required_time_for_repeated_Nodes();
    }

    case flow_required_nonrepeating: {
        return required_time_for_nonrepeating_Nodes();
    }

    default: {
        fzu.print_usage();
    }

    }

    return standard.completed_ok();
}
