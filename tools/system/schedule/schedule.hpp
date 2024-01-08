// Copyright 2024 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the schedule generating tool.
 * 
 * The output can be used directly in HTML pages or can be provided as CSV data
 * for post-processing, as with nodeboard.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __SCHRENDER_HPP.
 */

#ifndef __SCHRENDER_HPP
#include "version.hpp"
#define __SCHRENDER_HPP (__VERSION_HPP)

#include "Graphtypes.hpp"
#include "Graphaccess.hpp"
#include "Graphinfo.hpp"
#include "templater.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,      ///< no recognized request
    flow_html = 2,         ///< request: generate HTML schedule
    flow_html_for_web = 3, ///< request: genereate HTML schedule for use in web app
    flow_csv = 4,          ///< request: generate CSV schedule
    flow_csv_for_web = 5,  ///< request: generate CSV schedule for use in web app
    flow_NUMoptions
};

enum template_id_enum {
    html_entry_temp,
    NUM_temp
};

typedef std::map<template_id_enum,std::string> schedule_templates;

enum schedule_strategy {
    fixed_late_variable_early_strategy,
    NUMstrategies
};

// struct CSV_Data {
//     std::string start_date;
//     std::string start_time;
//     unsigned int num_minutes;
//     char tdprop;
//     Node * node_ptr;

//     CSV_Data(Graph & graph, const std::string & csv_line);
// };

// struct CSV_Data_Day {
//     std::vector<CSV_Data> day;

// };

struct Day_Entries {
    std::vector<Node *> entries;
    size_t size() const { return entries.size(); }
    void append(Node * entry) { entries.emplace_back(entry); }
};

typedef std::unique_ptr<Day_Entries> day_entries_uptr;

struct Days_Map: public std::vector<Node_ID_key> {
    void fill(unsigned long from_idx, unsigned long before_idx, Node_ID_key nkey) {
        for (unsigned long idx = from_idx; idx < before_idx; idx++) {
            at(idx) = nkey;
        }
    }
};

struct schedule: public formalizer_standard_program {
    Graph_access ga;

    flow_options flowcontrol;
    schedule_strategy strategy = fixed_late_variable_early_strategy;

    time_t thisdatetime;
    std::string thisdate;
    time_of_day_t thistimeofday;
    unsigned long thisminutes;

    time_t t_today_start;

    unsigned int num_days = 1;
    int min_block_size = 1;

    std::string output_path;

    targetdate_sorted_Nodes incomplete_nodes_cache; // Filled once, then used as cached.
    targetdate_sorted_Nodes incnodes_with_repeats;
    unsigned int schedule_size = 0;
    std::map<unsigned long, day_entries_uptr> days;
    time_t t_last_scheduled = 0;

    Days_Map daysmap;

    unsigned long total_minutes = 0;
    unsigned long passed_minutes = 0;
    unsigned long exact_consumed = 0;
    unsigned long fixed_consumed = 0;
    unsigned long variable_consumed = 0;

    //std::vector<CSV_Data_Day> csv_data_vec;

    Graph *graph_ptr;

    render_environment env;
    schedule_templates templates;

    schedule();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    Graph & graph();

    bool to_output(const std::string & rendered_schedule);

    bool render_init();

    bool get_schedule_data(unsigned int get_num_days);

    bool convert_to_data_by_day();

    bool initialize_daymap();

    bool map_exact_target_date_entries();

    bool min_block_available_backwards(unsigned long idx, int next_grab);

    unsigned long set_block_to_node_backwards(unsigned long idx, int next_grab, Node_ID_key nkey);

    bool map_fixed_target_date_entries_late();

    bool min_block_available_forwards(unsigned long idx, int next_grab);

    unsigned long set_block_to_node_forwards(unsigned long idx, int next_grab, Node_ID_key nkey);

    bool map_variable_target_date_entries_early(unsigned int start_at = 0);

    bool get_and_map_more_variable_target_date_entries(unsigned long remaining_minutes);

    bool generate_schedule();

};

bool schedule_render_html(schedule & sch);

bool schedule_render_csv(schedule & sch);

#endif // __SCHRENDER_HPP
