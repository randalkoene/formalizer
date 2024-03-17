// Copyright 2020 Randal A. Koene
// License TBD

/**
 * nodeboard is a simple prototype for casting Nodes onto dashboard-like web pages
 * in the form of cards.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#define FORMALIZER_MODULE_ID "Formalizer:Visualization:Nodes:HTMLcards"

#include <iostream>

#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"
#include "Graphaccess.hpp"
#include "Graphpostgres.hpp"
#include "nbrender.hpp"

using namespace fz;

nodeboard nb;

bool random_test() {
    FZOUT("\nThis is a test. Let's go find the Graph, so that we have some Nodes to work with.\n\n");
    key_pause();

    FZOUT("\nThe Graph has "+std::to_string(nb.graph().num_Nodes())+" Nodes.\n\n");

    return standard_exit(node_board_render_random_test(nb), "Random test Node board created.\n", exit_general_error, "Unable to map interval.", __func__);
}

bool node_dependencies() {
    if (!nb.node_ptr) {
        return standard_error("Node not found.", __func__);
    }

    return standard_exit(node_board_render_dependencies(nb), "Node dependencies board created.\n", exit_general_error, "Unable to create Node dependencies board.", __func__);
}

bool named_list() {
    Named_Node_List_ptr namedlist_ptr = nb.graph().get_List(nb.list_name);
    if (!namedlist_ptr) {
        return standard_error("Named Node List "+nb.list_name+" not found.", __func__);
    }

    return standard_exit(node_board_render_named_list(namedlist_ptr, nb), "Named Node List board created.\n", exit_general_error, "Unable to create Named Node List board.", __func__);
}

// E.g: nodeboard -l "{milestones_formalizer,procrastination,group_sleep}"
bool list_of_named_lists() {
    return standard_exit(node_board_render_list_of_named_lists(nb), "Kanban board created.\n", exit_general_error, "Unable to create Kanban board.", __func__);
}

// E.g: nodeboard -t "{carboncopies,cc-research,cc-operations}"
bool list_of_topics() {
    return standard_exit(node_board_render_list_of_topics(nb), "Kanban board created.\n", exit_general_error, "Unable to create Kanban board.", __func__);
}

// E.g: nodeboard -m "{carboncopies,NNL:milestones_formalizer}"
bool list_of_topics_and_NNLs() {
    return standard_exit(node_board_render_list_of_topics_and_NNLs(nb), "Kanban board created.\n", exit_general_error, "Unable to create Kanban board.", __func__);
}

// E.g: nodeboard -f /var/www/webdata/formalizer/categories_main2023.json
bool sysmet_categories() {
    return standard_exit(node_board_render_sysmet_categories(nb), "Kanban board created.\n", exit_general_error, "Unable to create Kanban board.", __func__);
}

bool NNL_dependencies() {
    return standard_exit(node_board_render_NNL_dependencies(nb), "NNL dependencies Kanban board created.\n", exit_general_error, "Unable to create NNL dependencies Kanban board.", __func__);
}

bool node_dependencies_tree() {
    if (!nb.node_ptr) {
        return standard_error("Node not found.", __func__);
    }

    return standard_exit(node_board_render_dependencies_tree(nb), "Node dependencies tree created.\n", exit_general_error, "Unable to create Node dependencies tree.", __func__);
}

bool node_superiors_tree() {
    if (!nb.node_ptr) {
        return standard_error("Node not found.", __func__);
    }

    return standard_exit(node_board_render_superiors_tree(nb), "Node superiors tree created.\n", exit_general_error, "Unable to create Node superiors tree.", __func__);
}

bool csv_schedule() {
    if (nb.source_file.empty()) {
        return standard_error("Missing source file argument.", __func__);
    }

    return standard_exit(node_board_render_csv_schedule(nb), "Schedule created from CSV file.\n", exit_general_error, "Unable to create schedule from CSV file.", __func__);
}

int main(int argc, char *argv[]) {
    nb.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    nb.t_now = ActualTime();

    switch (nb.flowcontrol) {

        case flow_node: {
            if (nb.show_dependencies_tree) {
                nb.flowcontrol = flow_dependencies_tree;
                node_dependencies_tree();
            } else if (nb.show_superiors_tree) {
                nb.flowcontrol = flow_superiors_tree;
                node_superiors_tree();
            } else {
                node_dependencies();
            }
            break;
        }

        case flow_named_list: {
            named_list();
            break;
        }

        case flow_random_test: {
            random_test();
            break;
        }

        case flow_listof_NNL: {
            list_of_named_lists();
            break;
        }

        case flow_listof_topics: {
            list_of_topics();
            break;
        }

        case flow_listof_mixed: {
            list_of_topics_and_NNLs();
            break;
        }

        case flow_sysmet_categories: {
            sysmet_categories();
            break;
        }

        case flow_NNL_dependencies: {
            NNL_dependencies();
            break;
        }

        case flow_dependencies_tree: {
            node_dependencies_tree();
            break;
        }

        case flow_superiors_tree: {
            node_superiors_tree();
            break;
        }

        case flow_csv_schedule: {
            csv_schedule();
            break;
        }

        default: {
            nb.print_usage();
        }

    }

    return standard.completed_ok();
}
