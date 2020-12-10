// Copyright 2020 Randal A. Koene
// License TBD

/** @file Graphinfo.hpp
 * This header file declares basic information gathering functions for use with
 * Graph data structures. The functions collected in this header are (mostly) of
 * the sort that collect information for multi-element subsets of the Graph.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPHINFO_HPP.
 */

#ifndef __GRAPHINFO_HPP
#include "coreversion.hpp"
#define __GRAPHINFO_HPP (__COREVERSION_HPP)

// std

// core
#include "Graphtypes.hpp"

namespace fz {

void Graph_Info(Graph & graph, Graph_info_label_value_pairs & graphinfo);

std::string Graph_Info_str(Graph & graph);

std::string List_Topics(Graph & graph, std::string delim);

struct Nodes_Stats {
    unsigned long num_completed = 0; ///< marked completion >= 1.0
    unsigned long num_open = 0;      ///< marked 0.0 <= completion < 1.0
    unsigned long num_other = 0;     ///< marked completion < 0.0
    unsigned long sum_required_completed = 0;
    unsigned long sum_required_open = 0;
};

Nodes_Stats Nodes_statistics(Graph & graph);

void Nodes_statistics_pairs(const Nodes_Stats & nstats, Graph_info_label_value_pairs & nodesinfo);

std::string Nodes_statistics_string(const Nodes_Stats & nstats);

unsigned long Edges_with_data(Graph & graph);

/**
 * Selects all Nodes that are incomplete and lists them by (inherited)
 * target date.
 * 
 * @param graph A valid Graph data structure.
 * @return A map of pointers to nodes by effective targetdate.
 */
targetdate_sorted_Nodes Nodes_incomplete_by_targetdate(Graph & graph);

/**
 * Add virtual Nodes to produce a list where repeating Nodes appear at their
 * pattern-specified repeat target dates.
 * 
 * Note that `t_max < 0 ` is interpreted as `t_max = RTt_maxtime`.
 * 
 * If you want a list with repeats of only incomplete Nodes then `sortednodes` can
 * be prepared with `Nodes_incomplete_by_targetdate()`. Otherwise, use an alternative
 * preparation.
 * 
 * @param sortednodes A list of target date sorted Node pointers.
 * @param t_max Limit to which to generate the resulting list of Node pointers.
 * @param N_max Maximum size of list to return (zero means no size limit).
 * @return A target date sorted list of Node pointers with repeats.
 */
targetdate_sorted_Nodes Nodes_with_repeats_by_targetdate(const targetdate_sorted_Nodes & sortednodes, time_t t_max, size_t N_max);

/**
 * Selects all Nodes that are incomplete and repeating and lists them by (inherited)
 * target date.
 * 
 * For example, see how this is used in `fzupdate` together with `Graphmodify:Update_repeating_Nodes()`.
 * 
 * @param graph A valid Graph data structure.
 * @return A map of pointers to nodes by effective targetdate.
 */
targetdate_sorted_Nodes Nodes_incomplete_and_repeating_by_targetdate(Graph & graph);

/**
 * Selects all Nodes that have Node IDs (i.e. creation times) within
 * a specified time interval.
 * 
 * For example, see how this is used in `fzaddnode`.
 * 
 * @param graph A valid Graph data structure.
 * @param earliest The earliest epoch-time equivalent Node-ID.
 * @param before The epoch-time equivalent beyond the Node-ID interval.
 * @return A map of pointers to nodes by Node_ID_key.
 */
key_sorted_Nodes Nodes_created_in_time_interval(Graph & graph, time_t earliest, time_t before);

/**
 * Selects all Nodes that include a specific Topic ID and lists them by (inherited)
 * target date.
 * 
 * @param graph A valid Graph data structure.
 * @return A map of pointers to nodes by effective targetdate.
 */
targetdate_sorted_Nodes Nodes_with_topic_by_targetdate(Graph & graph, Topic_ID id);

} // namespace fz

#endif // __GRAPHINFO_HPP
