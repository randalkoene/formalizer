// Copyright 2020 Randal A. Koene
// License TBD

//#define USE_COMPILEDPING

// std

// core
#include "error.hpp"
#include "Graphinfo.hpp"

namespace fz {

std::string Graph_Info(Graph & graph) {
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

} // namespace fz
