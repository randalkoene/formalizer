// Copyright 2020 Randal A. Koene
// License TBD

//#define USE_COMPILEDPING

// std

// core
#include "error.hpp"
#include "Graphinfo.hpp"

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
 * Selects all Nodes that are incomplete and lists them by (inherited)
 * target date.
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

} // namespace fz
