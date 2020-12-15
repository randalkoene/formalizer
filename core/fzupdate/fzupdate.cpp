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
    add_option_args += "ruT:";
    add_usage_top += " [-r] [-u] [-T <t_max|full>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("The -T limit overrides the 'map_days' configuration or default parameter.\n");
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
          "    -T update up to and including <t_max> or 'full' update\n");
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
        break;
    }

    case 'u': {
        flowcontrol = flow_update_variable;
        break;
    }

    case 'T': {
        if (cargs=="full") {
            t_limit = RTt_maxtime;
        } else {
            t_limit = time_stamp_time(cargs);
        }
        break;
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

/// Configure configurable parameters.
bool fzu_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(chunk_minutes, "chunk_minutes", parlabel, get_chunk_minutes(parvalue));
    CONFIG_TEST_AND_SET_PAR(map_multiplier, "map_multiplier", parlabel, get_positive_integer(parvalue, "map_multiplier"));
    CONFIG_TEST_AND_SET_PAR(map_days, "map_days", parlabel, get_positive_integer(parvalue, "map_days"));
    CONFIG_TEST_AND_SET_PAR(warn_repeating_too_tight,"warn_repeating_too_tight", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(endofday_priorities,"endofday_priorities", parlabel, (parvalue != "false"));
    CONFIG_TEST_AND_SET_PAR(dolater_endofday,"dolater_endofday", parlabel, time_stamp_time(parvalue));
    CONFIG_TEST_AND_SET_PAR(doearlier_endofday,"doearlier_endofday", parlabel, time_stamp_time(parvalue));
    CONFIG_TEST_AND_SET_PAR(eps_group_offset_mins,"eps_group_offset_mins", parlabel, get_positive_integer(parvalue, "eps_group_offset_mins"));
    CONFIG_TEST_AND_SET_PAR(update_to_earlier_allowed,"update_to_earlier_allowed", parlabel, (parvalue != "false"));
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
        graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
        if (!graphmod_ptr)
            standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);
    }
    return *graphmod_ptr;
}

void fzupdate::prepare_Graphmod_shared_memory(unsigned long _segsize) {
    segsize = _segsize;
    segname = unique_name_Graphmod(); // a unique name to share with `fzserverpq`
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

std::vector<chunks_t> updvar_chunks_required(const targetdate_sorted_Nodes & nodelist) {
    std::vector<chunks_t> chunks_req(nodelist.size(), 0);
    chunks_t suspicious_req = (36*60)/fzu.config.chunk_minutes;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        chunks_req[i] = seconds_to_chunks(node_ptr->seconds_to_complete());
        if (chunks_req[i]>suspicious_req) {
            ADDWARNING(__func__, "Suspiciously large number of chunks ("+std::to_string(chunks_req[i])+") needed to complete Node "+node_ptr->get_id_str());
        }
        ++i;
    }
    return chunks_req;
}

unsigned long updvar_total_chunks_required_nonperiodic(const targetdate_sorted_Nodes & nodelist, const std::vector<chunks_t> & chunks_req) {
    // *** Note: The chunks_req[i] test may be unnecessary, because we're working with a list of incomplete Nodes.
    unsigned long total = 0;
    size_t i = 0;
    for (const auto & [t, node_ptr] : nodelist) {
        if (node_ptr->effective_targetdate()>fzu.t_limit) {
            break;
        }
        if ((chunks_req[i]>0) && (!node_ptr->get_repeats())) {
            total += chunks_req[i];
        }
        ++i;
    }
    return total;
}

/**
 * Note: Much of this is a re-implementation of the Formalizer 1.x process carried out by dil2al when the
 *       function alcomp.cc:generate_AL_CRT() is called.
 */
int update_variable(time_t t_pass) {
    constexpr time_t seconds_per_week = 7*24*60*60;
    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_and_repeating_by_targetdate(fzu.graph());
    Edit_flags editflags;
    editflags.set_Edit_targetdate();

    updvar_set_t_limit(t_pass);

    std::vector<time_t> t_eps(incomplete_repeating.size(), RTt_maxtime);
    std::vector<EPS_flags> epsflags_vec;

    std::vector<chunks_t> chunks_req = updvar_chunks_required(incomplete_repeating);
    unsigned long chunks_req_total = updvar_total_chunks_required_nonperiodic(incomplete_repeating, chunks_req);
    unsigned long chunks_per_week = seconds_per_week / ((unsigned long)fzu.config.chunk_minutes * 60);
    unsigned long weeks_for_nonperiodic = chunks_req_total / chunks_per_week;
    unsigned long days_in_map = weeks_for_nonperiodic * 7 * fzu.config.map_multiplier;
    if (fzu.config.map_days > days_in_map) {
        days_in_map = fzu.config.map_days;
    }

    EPS_map updvar_map(t_pass, days_in_map, incomplete_repeating, chunks_req, epsflags_vec, t_eps);
    updvar_map.place_exact();
    updvar_map.place_fixed();
    updvar_map.group_and_place_movable();

    targetdate_sorted_Nodes eps_update_nodes = updvar_map.get_eps_update_nodes();

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT, this is a wild guess
    fzu.prepare_Graphmod_shared_memory(sizeof(TD_Node_shm)*eps_update_nodes.size() + 10240);

    Batchmod_targetdates_ptr batchmodtd_ptr = fzu.graphmod().request_Batch_Node_Targetdates(eps_update_nodes);
    if (!batchmodtd_ptr) {
        return standard_exit_error(exit_general_error, "Unable to update batch of Node target dates", __func__);
    }

    fzu.graphmod().data.back().set_Edit_flags(editflags.get_Edit_flags());

    auto ret = server_request_with_shared_data(fzu.get_segname(), fzu.graph().get_server_port());
    if (ret != exit_ok) {
        standard_exit_error(ret, "Graph server returned error.", __func__);
    }

    return standard_exit_success("Update variable target date Nodes done.");
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzu.init_top(argc, argv);

    FZOUT("\nThis is a stub.\n\n");
    key_pause();

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

    default: {
        fzu.print_usage();
    }

    }

    return standard.completed_ok();
}
