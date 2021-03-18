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
 * Returns a vector of Topic tag label strings for a specified vector
 * of known Topic IDs from a specified Graph.
 * 
 * Note: Also see the `Graphtypes:Topic_tags_of_Node()` function.
 */
std::vector<std::string> Topic_IDs_to_Tags(Graph & graph, std::vector<Topic_ID> IDs_vec);

/**
 * Filter class that helps construct filter specifications. This can be
 * used to search for Node subsets, for example.
 */
struct Node_Filter {
    time_t t_created_lowerbound = RTt_unspecified;
    time_t t_created_upperbound = RTt_unspecified;
    Node_data lowerbound; // values must be >= to these thresholds (where it makes sense)
    Node_data upperbound; // values must be <= to these thresholds (where it makes sense)
    bool case_sensitive = true;
    Edit_flags filtermask;

    std::string str();
};

/**
 * Finds all Nodes that match a specified Node_Filter.
 * 
 * @param graph A valid Graph data structure.
 * @param nodefilter A specified Node_Filter.
 * @return A map of pointers to nodes by effective targetdate that match the filter specifications.
 */
targetdate_sorted_Nodes Nodes_subset(Graph & graph, const Node_Filter & nodefilter);

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
 * @param limit_repeats_only If true then apply t_max only to repeating Nodes.
 * @return A target date sorted list of Node pointers with repeats.
 */
targetdate_sorted_Nodes Nodes_with_repeats_by_targetdate(const targetdate_sorted_Nodes & sortednodes, time_t t_max, size_t N_max, bool limit_repeats_only = false);

/**
 * Returns targetdate sorted Nodes that are incomplete, augmented with repeated instances.
 * 
 * This is an application in sequence of the two functions above.
 * 
 * @param graph A valid Graph object.
 * @param t_max Limit to which to generate the resulting list of Node pointers.
 * @param N_max Maximum size of list to return (zero means no size limit).
 * @param limit_repeats_only If true then apply t_max only to repeating Nodes.
 * @return A target date sorted list of Node pointers with repeats.
 */
targetdate_sorted_Nodes Nodes_incomplete_with_repeating_by_targetdate(Graph & graph, time_t t_max, size_t N_max, bool limit_repeats_only = false);

/**
 * Selects all Nodes that are incomplete and repeating and lists them by (inherited)
 * target date. Note that this is NOT the same thing as incomplete with repeating. This is
 * an and operation where both things must be true.
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

/**
 * Returns the total required time for all incomplete repeating Nodes and their instances
 * within a time interval.
 * 
 * Note: This same information can be obtained by creating a Nodes filter, obtaining the
 *       resulting list of Nodes, and then computing the total. This function is provided
 *       for convenience.
 * 
 * Note: There is an issue/question here about how to deal with overlapping exact target
 *       date Nodes. Obviously, they will not actually consume time twice, but taking
 *       that into account correctly requires mapping time. It doesn't need to be a map
 *       with as much flexibility as needed for other mapping applications, but at least
 *       a simple flag per smallest time-interval to allocate (e.g. 1 boolean per minute).
 *       Note that mapping 1 year by minute with boolean would require a map of 525600
 *       bytes. In this sense, fixed and exact target date Node required time totals
 *       should ideally be calculated separately and then combined.
 * 
 * @param graph A valid Graph object.
 * @param from_t Earliest time from which to calculate accumulated required time.
 * @param before_t Limit to which to calculate accumulated required time.
 * @param mapexact Flag to choose exact target dates mapping for more precise totals.
 * @return Total required time calculated in minutes.
 */
size_t total_minutes_incomplete_repeating(Graph & graph, time_t from_t, time_t before_t, bool mapexact = true);

/**
 * Returns the total required time for all incomplete non-repeating Nodes within a time
 * interval.
 * 
 * Note: Adding this to the total for repeating nodes (see function above) is a better
 *       way to estimate total time required for all Scheduled Nodes, where non-
 *       repeating Nodes must be completed in time available between repeating Nodes.
 *       It can be used to provide a projection or estimate for the current time
 *       limit up to which variable or unspecified target date Node Scheduling is
 *       sensible. (Better = better than the estimate produced with `Node_Statistics`.)
 * 
 * @param graph A valid Graph object.
 * @param from_t Earliest time from which to calculate accumulated required time.
 * @param before_t Limit to which to calculate accumulated required time.
 * @return Total required time calculated in minutes.
 */
size_t total_minutes_incomplete_nonrepeating(Graph & graph, time_t from_t, time_t before_t = RTt_maxtime);

typedef std::map<std::string, std::string> cat_translation_map;
typedef cat_translation_map * cat_translation_map_ptr;

/**
 * Data structure that specifies Node grouping categories.
 */
struct Set_builder_data {
    std::map<std::string, std::string> NNL_to_category;
    std::map<std::string, std::string> LV_to_category;
    std::map<std::string, std::string> Topic_to_category;
    std::string default_category;

    std::map<Topic_ID, std::string> cache_topicid_to_category; // set from within node_category()

    /**
     * Assign a category to a Node.
     * 
     * This returns a reference to `cat_cache`, which is where the assigned category
     * string is cached. The categorization rules are applied in the following
     * priority order:
     *   1. pre-existing non-empty cache value
     *   2. category of a Named Node List in NNL_to_category in which node is found
     *   3. category of a Label-Value in LV_to_category for a Label the node has
     *   4. category of a Topic tag in Topic_to_category that is a Topic of the node with
     *      higher relevance value than other Topics of the node found in Topic_to_category
     *   5. the default_category, if not empty
     * 
     * Note that adding a pointer to a cateotry `translation` map can be useful if the
     * `cat_cache` should be used for printing or if a 1, 2, 4, or 8 byte (char) code
     * is desired. It allos allows immediate effective merging of categories.
     * 
     * @param graph Valid Graph in which to find Named Node Lists.
     * @param node The Node for which to find the appropriate category.
     * @param cat_cache Reference to a string cache in which to store a category.
     * @param translation Optional pointer to immediate category translation map.
     * @return Reference to `cat_cache`.
     */
    std::string & node_category(Graph & graph, Node & node, std::string & cat_cache, cat_translation_map_ptr translation = nullptr);

};

} // namespace fz

#endif // __GRAPHINFO_HPP
