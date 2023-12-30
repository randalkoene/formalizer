// Copyright 2020 Randal A. Koene
// License TBD

//#define USE_COMPILEDPING

// std

// core
#include "error.hpp"
#include "Graphinfo.hpp"
#include "GraphLogxmap.hpp"

namespace fz {

void Graph_Info(Graph & graph, Graph_info_label_value_pairs & graphinfo) {
    graphinfo["num_topics"] = std::to_string(graph.get_topics().get_topictags().size());
    graphinfo["num_nodes"] = std::to_string(graph.num_Nodes());
    graphinfo["num_edges"] = std::to_string(graph.num_Edges());
}

std::string Graph_Info_str(Graph & graph) {
    std::string info_str("Graph info:");
    info_str += "\n  number of Topics = " + std::to_string(graph.get_topics().get_topictags().size());
    info_str += "\n  number of Nodes  = " + std::to_string(graph.num_Nodes());
    info_str += "\n  number of Edges  = " + std::to_string(graph.num_Edges()) + '\n';
    return info_str;
}

/**
 * This collector of Topic tags purposely obtains them from the Topic objects
 * instead of from the `topicbytag` map, so that it can be used to confirm that
 * the structure is correctly formed.
 */
std::string List_Topics(Graph & graph, std::string delim) {
    const Topic_Tags & topics = graph.get_topics();
    const Topic_Tags_Vector & topictags = topics.get_topictags();
    std::string listoftopics;
    listoftopics.reserve((50+delim.size())*topictags.size());

    for (size_t i = 0; i < topictags.size(); ++i) {
        if (i != 0) {
            listoftopics += delim;
        }

        Topic * topic_ptr = topictags[i].get();

        if (topic_ptr) {
            listoftopics += topic_ptr->get_tag().c_str(); // converting from bi::basic_string
        }
    }
    return listoftopics;
}

Nodes_Stats Nodes_statistics(Graph & graph) {
    Nodes_Stats nstats;
    for (auto n_it = graph.begin_Nodes(); n_it != graph.end_Nodes(); ++n_it) {
        Node * node_ptr = n_it->second.get();
        if (node_ptr) {
            float completion = node_ptr->get_completion();
            float required = node_ptr->get_required();
            if (completion < 0.0) {
                ++nstats.num_other;
            } else {
                if (completion >= 1.0) {
                    ++nstats.num_completed;
                    nstats.sum_required_completed += required;
                } else {
                    ++nstats.num_open;
                    nstats.sum_required_open += required;
                }
            }
        }
    }
    return nstats;
}

void Nodes_statistics_pairs(const Nodes_Stats & nstats, Graph_info_label_value_pairs & nodesinfo) {
    nodesinfo["nodes_comp"] = std::to_string(nstats.num_completed);
    nodesinfo["nodes_open"] = std::to_string(nstats.num_open);
    nodesinfo["nodes_other"] = std::to_string(nstats.num_other);
    nodesinfo["sec_comp"] = std::to_string(nstats.sum_required_completed);
    nodesinfo["sec_open"] = std::to_string(nstats.sum_required_open);
}

std::string Nodes_statistics_string(const Nodes_Stats & nstats) {
    std::string nstats_str("Nodes statistics:");
    nstats_str += "\n\tNodes completed = " + std::to_string(nstats.num_completed);
    nstats_str += "\n\tNodes open      = " + std::to_string(nstats.num_open);
    nstats_str += "\n\tNodes other     = " + std::to_string(nstats.num_other);
    nstats_str += "\n\tSum of seconds required for all completed = " + std::to_string(nstats.sum_required_completed);
    nstats_str += "\n\tSum of seconds required for all open      = " + std::to_string(nstats.sum_required_open) + '\n';
    return nstats_str;
}

unsigned long Edges_with_data(Graph & graph) {
    unsigned long withdata = 0;
    for (auto e_it = graph.begin_Edges(); e_it != graph.end_Edges(); ++e_it) {
        Edge *edge_ptr = e_it->second.get();
        if (edge_ptr) {
            if ((edge_ptr->get_importance() > 0.0) ||
                (edge_ptr->get_priority() > 0.0) ||
                (edge_ptr->get_significance() > 0.0) ||
                (edge_ptr->get_urgency() > 0.0) ||
                (edge_ptr->get_dependency() > 0.0)) {
                ++withdata;
            }
        }
    }
    return withdata;
}

/**
 * Returns a vector of Topic tag label strings for a specified vector
 * of known Topic IDs from a specified Graph.
 * 
 * Note: Also see the `Graphtypes:Topic_tags_of_Node()` function.
 */
std::vector<std::string> Topic_IDs_to_Tags(Graph & graph, std::vector<Topic_ID> IDs_vec) {
    std::vector<std::string> res;
    for (const auto & topic_id : IDs_vec) {
        Topic * topic_ptr = graph.find_Topic_by_id(topic_id);
        if (!topic_ptr) {
            ADDERROR(__func__, "List of Topic IDs contains unknown Topic with ID "+std::to_string(topic_id));
        } else {
            res.emplace_back(topic_ptr->get_tag().c_str());
        }
    }
    return res;
}

void append_filter_bit_status(std::string & s, bool bitflag, std::string label, std::string lower_str, std::string upper_str) {
    static const char bitflagged[2][6] = {"no [", "Yes ["};
    s += (label + ": ") + (bitflagged[(int) bitflag] + lower_str) + '-' + upper_str + "]\n";
}

std::string Node_Filter::str() {
    static const char repeats_value[2][6] = {"false", "true"};
    std::string s;
    append_filter_bit_status(s, filtermask.Edit_tcreated(), "t_created", TimeStampYmdHM(t_created_lowerbound), TimeStampYmdHM(t_created_upperbound));
    append_filter_bit_status(s, filtermask.Edit_text(), "text", lowerbound.utf8_text.substr(0,10), upperbound.utf8_text.substr(0,10));
    append_filter_bit_status(s, filtermask.Edit_completion(), "completion", to_precision_string(lowerbound.completion,3), to_precision_string(upperbound.completion,3));
    append_filter_bit_status(s, filtermask.Edit_required(), "hours", to_precision_string(lowerbound.hours,3), to_precision_string(upperbound.hours,3));
    append_filter_bit_status(s, filtermask.Edit_valuation(), "valuation", to_precision_string(lowerbound.valuation,3), to_precision_string(upperbound.valuation,3));
    append_filter_bit_status(s, filtermask.Edit_targetdate(), "targetdate", TimeStampYmdHM(lowerbound.targetdate), TimeStampYmdHM(upperbound.targetdate));
    append_filter_bit_status(s, filtermask.Edit_tdproperty(), "tdproperty", td_property_str[lowerbound.tdproperty], td_property_str[upperbound.tdproperty]);
    append_filter_bit_status(s, filtermask.Edit_repeats(), "repeats", repeats_value[(int) lowerbound.repeats], repeats_value[(int) upperbound.repeats]);
    append_filter_bit_status(s, filtermask.Edit_tdpattern(), "tdpattern", td_pattern_str[lowerbound.tdpattern], td_pattern_str[upperbound.tdpattern]);
    append_filter_bit_status(s, filtermask.Edit_tdevery(), "tdevery", std::to_string(lowerbound.tdevery), std::to_string(upperbound.tdevery));
    append_filter_bit_status(s, filtermask.Edit_tdspan(), "tdspan", std::to_string(lowerbound.tdspan), std::to_string(upperbound.tdspan));
    append_filter_bit_status(s, filtermask.Edit_topics(), "topics", join(lowerbound.topics,","), join(upperbound.topics,","));
    if (case_sensitive) {
        s += "case sensitive\n";
    } else {
        s += "not case sensitive\n";
    }
    return s;
}

/**
 * Collect all unique Nodes in the dependencies tree of a Node.
 * 
 * @param node_ptr A valid pointer to Node.
 * @param fulldepth_dependencies A Node_Set container for the resulting set of dependencies.
 * @return True if successful.
 */
bool Node_Dependencies_fulldepth(const Node* node_ptr, base_Node_Set & fulldepth_dependencies) {
    if (!node_ptr) return false;

    // Get dependencies edges.
    for (const auto & edge_ptr : node_ptr->dep_Edges()) {
        if (!edge_ptr) continue;

        // Try to add dependency Node to set.
        bool is_new;
        std::tie (std::ignore, is_new) = fulldepth_dependencies.emplace(edge_ptr->get_dep_key());

        // If the Node is new to the set then seek its dependencies.
        if (is_new) {
            if (!Node_Dependencies_fulldepth(edge_ptr->get_dep(), fulldepth_dependencies)) {
                standard_error("Recursive dependencies collection failed, skipping", __func__);
                continue;
            }
        }
    }

    return true;
}

void Node_Subtree::build_targetdate_sorted(Graph & graph) {
    for (const auto & nkey : set_by_key) {
        Node * node_ptr = graph.Node_by_id(nkey);
        tdate_node_pointers.emplace(node_ptr->effective_targetdate(), node_ptr);
    }
};

/**
 * Collect the subtrees that are the full dependencies of all Nodes in a
 * Named Nodes List.
 * 
 * @param nnl_str Named Nodes List.
 * @return A map in which the keys are the Node IDs of Nodes in the NNL and
 *         the values are each a set of unique Nodes that are the dependencies.
 */
map_of_subtrees_t Threads_Subtrees(Graph & graph, const std::string & nnl_str, bool sort_by_targetdate) {
    map_of_subtrees_t map_of_subtrees;

    Named_Node_List_ptr namedlist_ptr = graph.get_List(nnl_str);
    if (!namedlist_ptr) {
        standard_error("Named Node List "+nnl_str+" not found.", __func__);
        return map_of_subtrees;
    }

    for (const auto & nkey : namedlist_ptr->list) {

        Node * node_ptr = graph.Node_by_id(nkey);

        //base_Node_Set fulldepth_dependencies;
        Node_Subtree fulldepth_dependencies;
        if (!Node_Dependencies_fulldepth(node_ptr, fulldepth_dependencies.set_by_key)) {
            standard_error("Full depth dependencies collection failed for Node "+nkey.str()+", skipping", __func__);
            continue;
        }

        if (sort_by_targetdate) {
            fulldepth_dependencies.build_targetdate_sorted(graph);
        }

        map_of_subtrees[nkey] = fulldepth_dependencies;

    }

    return map_of_subtrees;
}

bool Map_of_Subtrees::collect(Graph & graph, const std::string & list_name) {
    if (list_name.empty()) return false;
    subtrees_list_name = list_name;
    map_of_subtrees = Threads_Subtrees(graph, subtrees_list_name, sort_by_targetdate);
    has_subtrees = !map_of_subtrees.empty();
    return true;
}

bool Map_of_Subtrees::is_subtree_head(Node_ID_key subtree_key) const {
    if (!has_subtrees) return false;
    return map_of_subtrees.find(subtree_key) != map_of_subtrees.end();
}

const Node_Subtree & Map_of_Subtrees::get_subtree_set(Node_ID_key subtree_key) const {
    return map_of_subtrees.at(subtree_key);
}

bool Map_of_Subtrees::node_in_subtree(Node_ID_key subtree_key, Node_ID_key node_key) const {
    if (!has_subtrees) return false;
    auto subtree = map_of_subtrees.find(subtree_key);
    if (subtree == map_of_subtrees.end()) return false;
    const base_Node_Set & subtree_ref = subtree->second.set_by_key;
    if (subtree_ref.find(node_key) == subtree_ref.end()) return false;
    return true;
}

bool Map_of_Subtrees::node_in_any_subtree(Node_ID_key node_key) const {
    if (!has_subtrees) return false;
    for (const auto & [subtree_key, subtree_ref]: map_of_subtrees) {
        if (subtree_ref.set_by_key.find(node_key) != subtree_ref.set_by_key.end()) return true;
    }
    return false;
}

bool Map_of_Subtrees::node_in_heads_or_any_subtree(Node_ID_key node_key) const {
    if (!has_subtrees) return false;
    for (const auto & [subtree_key, subtree_ref]: map_of_subtrees) {
        if (subtree_key == node_key) return true;
        if (subtree_ref.set_by_key.find(node_key) != subtree_ref.set_by_key.end()) return true;
    }
    return false;
}

/**
 * Functions for working with @PREREQS:...@ and @PROVIDES:...@ data in Nodes.
 */

void Prerequisite::update(const std::vector<std::string> & provided_vec, Node * provider) {
    if (_state == fulfilled) {
        return;
    }
    for (const auto & provided : provided_vec) {
        if (prereq == provided) {
            if (provider->get_completion() >= 1.0) {
                _state = fulfilled;
                provided_by = provider;
                return;
            }
            _state = unfulfilled;
            provided_by = provider;
            return;
        }
    }
}

std::vector<Prerequisite> get_prerequisites(const Node & node, bool check_prerequisites) {
    std::vector<Prerequisite> prereqs;
    std::string text(node.get_text().c_str());
    auto prereqs_start = text.find("@PREREQS:");
    if (prereqs_start == std::string::npos) {
        return prereqs;
    }
    prereqs_start += 9;
    auto prereqs_end = text.find('@', prereqs_start);
    if (prereqs_end == std::string::npos) {
        return prereqs;
    }
    std::string prereqs_str = text.substr(prereqs_start, prereqs_end - prereqs_start);
    auto prereqs_strvec = split(prereqs_str, ',');
    for (const auto & prereq_str : prereqs_strvec) {
        prereqs.emplace_back(Prerequisite(prereq_str));
    }

    if (check_prerequisites && (!prereqs.empty())) {
        check_prerequisites_provided_by_dependencies(node, prereqs);
    }

    return prereqs;
}

std::vector<std::string> get_provides_capabilities(const Node & node) {
    std::vector<std::string> provides;
    std::string text(node.get_text().c_str());
    auto provides_start = text.find("@PROVIDES:");
    if (provides_start == std::string::npos) {
        return provides;
    }
    provides_start += 10;
    auto provides_end = text.find('@', provides_start);
    if (provides_end == std::string::npos) {
        return provides;
    }
    std::string provides_str = text.substr(provides_start, provides_end - provides_start);
    provides = split(provides_str, ',');
    return provides;
}

void check_prerequisites_provided_by_dependencies(const Node & node, std::vector<Prerequisite> & prereqs, int go_deeper) {
    for (const auto & edge_ptr : node.dep_Edges()) {
        if (edge_ptr) {
            Node * dep_ptr = edge_ptr->get_dep();
            if (dep_ptr) {
                auto dep_provides = get_provides_capabilities(*dep_ptr);
                for (auto & prereq : prereqs) {
                    prereq.update(dep_provides, dep_ptr);
                }
                if (go_deeper>0) {
                    check_prerequisites_provided_by_dependencies(*dep_ptr, prereqs, go_deeper-1);
                }
            }
        }
    }
}

/**
 * Finds all Nodes that match a specified Node_Filter.
 * 
 * @param graph A valid Graph data structure.
 * @param nodefilter A specified Node_Filter.
 * @return A map of pointers to nodes by effective targetdate that match the filter specifications.
 */
targetdate_sorted_Nodes Nodes_subset(Graph & graph, const Node_Filter & nodefilter) {
    targetdate_sorted_Nodes nodes;
    if (nodefilter.filtermask.None()) {
        return nodes;
    }

    std::string uppersearchtext;
    if (nodefilter.filtermask.Edit_text() && (!nodefilter.case_sensitive)) {
        uppersearchtext = nodefilter.lowerbound.utf8_text;
        std::transform (uppersearchtext.begin(), uppersearchtext.end(), uppersearchtext.begin(), ::toupper);
    }

    for (const auto & [nkey, node_ptr] : graph.get_nodes()) {

        if (nodefilter.filtermask.Edit_text()) {
            if (!nodefilter.case_sensitive) {
                std::string uppertext(node_ptr->get_text().c_str());
                std::transform (uppertext.begin(), uppertext.end(), uppertext.begin(), ::toupper);
                if (uppertext.find(uppersearchtext) == std::string::npos) {
                    continue;
                }
            } else {
                if (node_ptr->get_text().find(nodefilter.lowerbound.utf8_text.c_str()) == Node_utf8_text::npos) {
                    continue;
                }
            }
        }
        if (nodefilter.filtermask.Edit_completion()) {
            if ((node_ptr->get_completion() < nodefilter.lowerbound.completion) || (node_ptr->get_completion() > nodefilter.upperbound.completion)) {
                continue;
            }
        }
        if (nodefilter.filtermask.Edit_required()) {
            if ((node_ptr->get_required_hours() < nodefilter.lowerbound.hours) || (node_ptr->get_required_hours() > nodefilter.upperbound.hours)) {
                continue;
            }
        }
        if (nodefilter.filtermask.Edit_targetdate()) {
            if ((node_ptr->get_targetdate() < nodefilter.lowerbound.targetdate) || (node_ptr->get_targetdate() > nodefilter.upperbound.targetdate)) {
                continue;
            }
        }
        if (nodefilter.filtermask.Edit_tdproperty()) {
            if ((node_ptr->get_tdproperty() != nodefilter.lowerbound.tdproperty) && (node_ptr->get_tdproperty() != nodefilter.upperbound.tdproperty)) {
                continue;
            }
        }
        if (nodefilter.filtermask.Edit_repeats()) {
            if ((node_ptr->get_repeats() != nodefilter.lowerbound.repeats) && (node_ptr->get_repeats() != nodefilter.upperbound.repeats)) {
                continue;
            }
        }
        if (nodefilter.filtermask.Edit_tdpattern()) {
            if ((node_ptr->get_tdpattern() != nodefilter.lowerbound.tdpattern) && (node_ptr->get_tdpattern() != nodefilter.upperbound.tdpattern)) {
                continue;
            }
        }
        if (nodefilter.filtermask.Edit_tcreated()) {
            if ((nodefilter.t_created_lowerbound > RTt_unspecified) && (node_ptr->t_created() < nodefilter.t_created_lowerbound)) {
                continue;
            }
            if ((nodefilter.t_created_upperbound > RTt_unspecified) && (node_ptr->t_created() > nodefilter.t_created_upperbound)) {
                continue;
            }            
        }
        // matched all filter requirements
        nodes.emplace(node_ptr->effective_targetdate(), node_ptr.get());

    }
    return nodes;
}

/**
 * Selects all Nodes that are incomplete and lists them by (inherited)
 * target date.
 * 
 * For example, see how this is used in `fzgraphhtml`.
 * 
 * @param graph A valid Graph data structure.
 * @return A map of pointers to nodes by effective targetdate.
 */
targetdate_sorted_Nodes Nodes_incomplete_by_targetdate(Graph & graph) {
    targetdate_sorted_Nodes nodes;
    for (const auto & [nkey, node_ptr] : graph.get_nodes()) {
        float completion = node_ptr->get_completion();
        if ((completion>=0.0) && (completion<1.0) && (node_ptr->get_required()>0.0)) {
            nodes.emplace(node_ptr->effective_targetdate(), node_ptr.get());
        }
    }
    return nodes; // automatic copy elision std::move(nodes);
}

/**
 * Add virtual Nodes to produce a list where repeating Nodes appear at their
 * pattern-specified repeat target dates.
 * 
 * Note that `t_max < 0 ` is interpreted as `t_max = RTt_maxtime`.
 * 
 * If you want a list with repeats of only incomplete Nodes then `sortednodes` can
 * be prepared with `Nodes_incomplete_by_targetdate()`. Otherwise, use an alternative
 * preparation. See `Nodes_incomplete_with_repeating_by_targetdate()` below.
 * 
 * BEWARE: As implemented presently, the N_max limit is only applied by cropping
 *         after collecting a potentially much larger set.
 *         As implemented presently, the combination N_max==0 and t_max<0 leads to
 *         an attempt to create a set with infinite repeated Nodes of unlimited span.
 * 
 * @param sortednodes A list of target date sorted Node pointers.
 * @param t_max Limit to which to generate the resulting list of Node pointers.
 * @param N_max Maximum size of list to return (zero means no size limit).
 * @param limit_repeats_only If true then apply t_max only to repeating Nodes.
 * @return A target date sorted list of Node pointers with repeats.
 */
targetdate_sorted_Nodes Nodes_with_repeats_by_targetdate(const targetdate_sorted_Nodes & sortednodes, time_t t_max, size_t N_max, bool limit_repeats_only) {
    targetdate_sorted_Nodes withrepeats;
    if (t_max < 0) {
        t_max = RTt_maxtime;
    }
    for (const auto & [t, node_ptr] : sortednodes) {
        if ((t > t_max) && (!limit_repeats_only)) {
            break;
        }
        // Note that you cannot immediately apply N_max here, because the first repeated Nodes
        // might fill up all the space even though others will have earlier target dates.
        if (node_ptr->get_repeats()) {
            if (t <= t_max) {
                auto tdwithrepeats = node_ptr->repeat_targetdates(t_max, N_max, t);
                for (const auto & t_repeat : tdwithrepeats) {
                    withrepeats.emplace(t_repeat, node_ptr);
                }
            }
        } else {
            withrepeats.emplace(t, node_ptr);
        }
    }
    if (N_max > 0) {
        if (withrepeats.size() > N_max) {
            withrepeats.erase(std::next(withrepeats.begin(), N_max), withrepeats.end());
        }
    }
    return withrepeats;
}

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
targetdate_sorted_Nodes Nodes_incomplete_with_repeating_by_targetdate(Graph & graph, time_t t_max, size_t N_max, bool limit_repeats_only) {
    targetdate_sorted_Nodes incomplete = Nodes_incomplete_by_targetdate(graph);
    return Nodes_with_repeats_by_targetdate(incomplete, t_max, N_max, limit_repeats_only);
}

/**
 * Returns the total required time for all incompelte repeating Nodes and their instances
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
size_t total_minutes_incomplete_repeating(Graph & graph, time_t from_t, time_t before_t, bool mapexact) {
    size_t minutes = 0;
    Byte_Map exactmap(from_t, before_t, 60, false, !mapexact); // a map of minutes
    for (const auto & [nkey, node_ptr] : graph.get_nodes()) {
        if (!node_ptr) {
            continue;
        }
        float completion = node_ptr->get_completion();
        time_t td_effective = node_ptr->effective_targetdate();
        time_t required = node_ptr->get_required();
        if ((completion>=0.0) && (completion<1.0) && (required>0) && node_ptr->get_repeats() && (td_effective >= from_t) && (td_effective < before_t)) {
            if (mapexact && (node_ptr->td_exact())) {
                auto td_withrepeats = node_ptr->repeat_targetdates(before_t - 1, 0, td_effective);
                for (const auto & td : td_withrepeats) {
                    exactmap.set(1, td - required, td);
                }
            } else {
                minutes += node_ptr->minutes_to_complete();
            }
        }
    }
    if (mapexact) {
        minutes += exactmap.sum();
    }
    return minutes;
}

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
size_t total_minutes_incomplete_nonrepeating(Graph & graph, time_t from_t, time_t before_t) {
    size_t minutes = 0;
    for (const auto & [nkey, node_ptr] : graph.get_nodes()) {
        if (!node_ptr) {
            continue;
        }
        float completion = node_ptr->get_completion();
        if ((completion>=0.0) && (completion<1.0)) {
            if (!node_ptr->get_repeats()) {
                time_t td_effective = node_ptr->effective_targetdate();
                if ((td_effective >= from_t) && (td_effective < before_t)) {
                    minutes += node_ptr->minutes_to_complete();
                }
            }
        }
    }
    return minutes;
}

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
targetdate_sorted_Nodes Nodes_incomplete_and_repeating_by_targetdate(Graph & graph) {
    targetdate_sorted_Nodes nodes;
    for (const auto & [nkey, node_ptr] : graph.get_nodes()) {
        float completion = node_ptr->get_completion();
        if ((completion>=0.0) && (completion<1.0) && (node_ptr->get_required()>0.0) && node_ptr->get_repeats()) {
            nodes.emplace(node_ptr->effective_targetdate(), node_ptr.get());
        }
    }
    return nodes; // automatic copy elision std::move(nodes);
}


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
key_sorted_Nodes Nodes_created_in_time_interval(Graph & graph, time_t earliest, time_t before) {
    key_sorted_Nodes nodes;
    if (before <= earliest)
        return nodes;

    const Node_ID_key key_earliest(earliest, 1);
    const Node_ID_key key_before(before, 1);
    auto it_same_or_later = graph.get_nodes().lower_bound(key_earliest);
    auto it_before = graph.get_nodes().upper_bound(key_before);
    for (auto it = it_same_or_later; it != it_before; ++it) {
        nodes.emplace(it->first, it->second.get());
    }

    return nodes;
}

/**
 * Selects all Nodes that include a specific Topic ID and lists them by (inherited)
 * target date.
 * 
 * @param graph A valid Graph data structure.
 * @return A map of pointers to nodes by effective targetdate.
 */
targetdate_sorted_Nodes Nodes_with_topic_by_targetdate(Graph & graph, Topic_ID id) {
    targetdate_sorted_Nodes nodes;
    for (const auto & [nkey, node_ptr] : graph.get_nodes()) {
        auto node_topics = node_ptr->get_topics();
        if (node_topics.find(id) != node_topics.end()) {
            nodes.emplace(node_ptr->effective_targetdate(), node_ptr.get());
        }
    }
    return nodes; // automatic copy elision std::move(nodes);
}

/**
 * Selects all Nodes that appear in a specific Named Node Lilst and lists them by
 * effective target date.
 * 
 * @param graph A valid Graph data structure.
 * @param namedlist_ptr A valid Named Node List pointer.
 * @return A map of pointers to nodes by effective targetdate.
 */
targetdate_sorted_Nodes Nodes_in_list_by_targetdate(Graph & graph, Named_Node_List_ptr namedlist_ptr) {
    targetdate_sorted_Nodes nodes;
    if (!namedlist_ptr) {
        standard_error("Named Node List not found.", __func__);
        return nodes;
    }

    for (const auto & nkey: namedlist_ptr->list) {
        Node * node_ptr = graph.Node_by_id(nkey);
        if (!node_ptr) {
            standard_error("Node "+nkey.str()+" not found in Graph, skipping", __func__);
            continue;
        }

        nodes.emplace(node_ptr->effective_targetdate(), node_ptr);
    }
    return nodes; // automatic copy elision std::move(nodes);
}

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
std::string & Set_builder_data::node_category(Graph & graph, Node & node, std::string & cat_cache, cat_translation_map_ptr translation) {
    if (!cat_cache.empty()) {
        return cat_cache;
    }

    for (const auto & [list_name, category] : NNL_to_category) {
        Named_Node_List_ptr nnl_ptr = graph.get_List(list_name);
        if (nnl_ptr) {
            if (nnl_ptr->contains(node.get_id().key())) {
                if (translation) {
                    cat_cache = (*translation)[category];
                } else {
                    cat_cache = category;
                }
                return cat_cache;
            }
        }
    }

    // *** for (const auto & [label, category] : LV_to_category) {
        // *** to be implemented, look for label in node's Label-Value pairs
    // ***}

    /*  3 ways to do this:
          1. For each Node, for each topic-category pair, search all a Node's Topic-IDs to see if
             they refer to a tag that corresponds and compare the relevance.
          2. For each Node, build a tagstring-relevance pair map (usually just containing one
             entry), and for each entry in that map, try to find the string in the topic-category map.
          3. Convert topictag-category map to Topic_ID-category map, for each Node get its
             Topic_ID-relevance map, for each element in that check if it is in the Tc map and compare relevance.
        The 3rd is probably the overall most efficient.
     */
    if (!Topic_to_category.empty()) {
        if (cache_topicid_to_category.empty()) {
            std::vector<std::string> topiclabels;
            for (const auto & [topicstr, category] : Topic_to_category) {
                topiclabels.push_back(topicstr);
            }
            auto topicindexids = graph.get_topics().tags_to_indices(topiclabels);
            //FZOUT("tidxsize="+std::to_string(topicindexids.size())+'\n');
            //for (auto & tt : topicindexids) { FZOUT(std::to_string(tt)+','); }
            //FZOUT('\n');
            for (size_t i = 0; i < topiclabels.size(); ++i) {
                //FZOUT(std::to_string(topicindexids[i])+' '+topiclabels[i]+' '+Topic_to_category[topiclabels[i]]+'\n');
                cache_topicid_to_category.emplace(topicindexids[i], Topic_to_category[topiclabels[i]]);
            }
        }
        float maxrel = -1.0;
        const Topics_Set & topicsset = node.get_topics();
        for (const auto & [topic_id, topic_rel] : topicsset) {
            auto it = cache_topicid_to_category.find(topic_id);
            if (it != cache_topicid_to_category.end()) {
                if (topic_rel > maxrel) {
                    maxrel = topic_rel;
                    //FZOUT(node.get_id_str()+' '+it->second+'\n');
                    if (translation) {
                        cat_cache = (*translation)[it->second];
                    } else {
                        cat_cache = it->second;
                    }
                }
            }
        }
        if (!cat_cache.empty()) {
            return cat_cache;
        }
    }

    if (!default_category.empty()) {
        if (translation) {
            cat_cache = (*translation)[default_category];
        } else {
            cat_cache = default_category;
        }
    }
    return cat_cache;
}

} // namespace fz
