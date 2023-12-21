// Copyright 2023 Randal A. Koene
// License TBD

/**
 * Milesontes visualization rendering functions.
 */

#include "error.hpp"
#include "general.hpp"
#include "stringio.hpp"
#include "html.hpp"
#include "Graphtypes.hpp"
//#include "Graphinfo.hpp"
//#include "templater.hpp"
#include "visrender.hpp"
//#include "jsonlite.hpp"

using namespace fz;

#define DEFAULT_OUTPUT_FILE "/var/www/html/formalizer/test_node_graph.sif"
#define DEFAULT_GRAPHML_FILE "/var/www/html/formalizer/test_node_graph.graphml"
fzvismilestones::fzvismilestones():
    formalizer_standard_program(false),
    ga(*this, add_option_args, add_usage_top),
    flowcontrol(flow_unknown),
    //output_path(DEFAULT_OUTPUT_FILE),
    graph_ptr(nullptr) {

    add_option_args += "O:IF:o:";
    add_usage_top += " [-O <output-format>] [-I] [-F <substring>] [-o <output-file|STDOUT>]";
}

void fzvismilestones::usage_hook() {
    ga.usage_hook();
    FZOUT(
        "    -O Output format:\n"
        "       SIF, GraphML, webapp, webview\n"
        "    -I Include completed Nodes.\n"
        "       This also includes Nodes with completion values < 0.\n"
        "    -F Filter to show only Nodes where the first 80 characters contain the\n"
        "       substring.\n"
        "    -o Output to file (or STDOUT).\n"
        "       Default: " DEFAULT_OUTPUT_FILE "\n"
        "\n"
    );
}

bool fzvismilestones::options_hook(char c, std::string cargs) {

    if (ga.options_hook(c,cargs))
        return true;

    
    switch (c) {

        case 'O': {
            if (cargs == "SIF") {
                flowcontrol = flow_SIF;
            } else if (cargs == "GraphML") {
                flowcontrol = flow_GraphML;
            } else if (cargs == "webapp") {
                flowcontrol = flow_webapp;
            } else if (cargs == "webview") {
                flowcontrol = flow_webview;
            } else {
                return false;
            }
            return true;
        }

        case 'I': {
            show_completed = true;
            return true;
        }

        case 'F': {
            filter_substring = cargs;
            uri_encoded_filter_substring = uri_encode(filter_substring);
            return true;
        }

        case 'o': {
            output_path = cargs;
            return true;
        }

    }

    return false;
}

Graph & fzvismilestones::graph() { 
    if (graph_ptr) {
        return *graph_ptr;
    }
    graph_ptr = graphmemman.find_Graph_in_shared_memory();
    if (!graph_ptr) {
        standard_exit_error(exit_general_error, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}

bool fzvismilestones::render_init() {
    //return load_templates(templates);
    return true;
}

bool fzvismilestones::to_output(const std::string & rendered) {
    if (output_path=="STDOUT") {
        FZOUT(rendered);
        return true;
    }

    if (!string_to_file(output_path, rendered)) {
        ERRRETURNFALSE(__func__,"unable to write rendered to "+output_path);
    }
    FZOUT("Rendered to "+output_path+".\n");
    return true;
}

bool fzvismilestones::check_inactive(Node_ptr node_ptr) {
    return ((!show_completed) && (!node_ptr->is_active()));
}

bool fzvismilestones::check_filtered(Node_ptr node_ptr) {
    if (filter_substring.empty()) return false;
    return (node_ptr->get_text().substr(0, filter_substring_excerpt_length).find(filter_substring) == std::string::npos);
}

/*

layout apply preferred Execute the preferred layout on a network
layout attribute-circle Execute the Attribute Circle Layout on a network
layout attribute-grid Execute the Attribute Grid Layout on a network
layout attributes-layout Execute the Group Attributes Layout on a network
layout circular Execute the Circular Layout on a network
layout copycat Copy network layout from one network view to another
layout cose Execute the Compound Spring Embedder (CoSE) on a network
layout degree-circle Execute the Degree Sorted Circle Layout on a network
layout force-directed Execute the Prefuse Force Directed Layout on a network
layout fruchterman-rheingold Execute the Edge-weighted Force directed (BioLayout) on a network
layout get preferred Return the current preferred layout
layout grid Execute the Grid Layout on a network
layout hierarchical Execute the Hierarchical Layout on a network
layout isom Execute the Inverted Self-Organizing Map Layout on a network
layout kamada-kawai Execute the Edge-weighted Spring Embedded Layout on a network
layout set preferred Set the preferred layout
layout stacked-node-layout Execute the Stacked Node Layout on a network

*/

#define DEFAULT_GRAPH_FILE "/dev/shm/graph.sif"
#define DEFAULT_GRAPH_JS "/dev/shm/graph.cyjs"
std::string generate_cytoscape_script() {
    std::string content =
        "# Import network\n"
        "network import file file=\"" DEFAULT_GRAPH_FILE "\"\n"
        //"# Import and set style\n"
        //"vizmap load file file=\"[full path to .xml style file]\"\n"
        //"vizmap apply styles=[style name]\n"
        "# Set layout\n"
        "layout force-directed\n"
        "# Set view to fit display\n"
        "view fit content\n"
        "# Save\n"
        "network export options=\"cyjs\" outputFile=\"" DEFAULT_GRAPH_JS "\"\n";

    return content;
}

#define EDGETYPE " dep_on"
#define SCRIPTPATH "/dev/shm/cytoscape.script"
#define DEFAULT_CYTOSCAPE "/home/randalk/local/bin/Cytoscape_v3.10.1/cytoscape.sh"
#define DEFAULT_OUTPUT_DIR "/var/www/html/formalizer"

bool fzvismilestones::render_as_SIF() {
    std::string rendered;

    Node_Index indexed_nodes = graph().get_Indexed_Nodes();

    for (const Node_ptr & node_ptr : indexed_nodes) {

        if (check_inactive(node_ptr)) continue;

        if (check_filtered(node_ptr)) continue;

        const Edges_Set & edges_set = node_ptr->dep_Edges();

        if (!edges_set.empty()) {

            rendered += node_ptr->get_id_str()+EDGETYPE;

            for (const auto & edge_ptr : edges_set) {

                Node_ptr depnode_ptr = edge_ptr->get_dep();

                if (check_inactive(depnode_ptr)) continue;

                if (check_filtered(depnode_ptr)) continue;

                rendered += ' '+edge_ptr->get_dep_str();

            }

            rendered += '\n';

        }

    }

    if (output_path.empty()) {
        output_path = DEFAULT_OUTPUT_FILE;
    }

    return to_output(rendered);
}

bool fzvismilestones::render_as_GraphML() {
    std::string rendered;

    Node_Index indexed_nodes = graph().get_Indexed_Nodes();

    for (const Node_ptr & node_ptr : indexed_nodes) {

        if (check_inactive(node_ptr)) continue;

        if (check_filtered(node_ptr)) continue;

        const Edges_Set & edges_set = node_ptr->dep_Edges();

        if (!edges_set.empty()) {

            rendered += node_ptr->get_id_str()+EDGETYPE;

            for (const auto & edge_ptr : edges_set) { // *** TODO: Modify this to produce GraphML output.

                Node_ptr depnode_ptr = edge_ptr->get_dep();

                if (check_inactive(depnode_ptr)) continue;

                if (check_filtered(depnode_ptr)) continue;

                rendered += ' '+edge_ptr->get_dep_str();

            }

            rendered += '\n';

        }

    }

    if (output_path.empty()) {
        output_path = DEFAULT_GRAPHML_FILE;
    }

    return to_output(rendered);
}

const char * CYTOSCAPEJS_HEAD_0 = R"LITERAL(var networks = {")LITERAL";
const char * CYTOSCAPEJS_HEAD_1 = R"LITERAL(": {
  "format_version" : "1.0",
  "generated_by" : "cytoscape-3.10.1",
  "target_cytoscapejs_version" : "~2.1",
  "data" : {
    "shared_name" : "file:)LITERAL";
const char * CYTOSCAPEJS_HEAD_2 = R"LITERAL(",
    "name" : "file:)LITERAL";
const char * CYTOSCAPEJS_HEAD_3 = R"LITERAL(",
    "SUID" : 35781,
    "__Annotations" : [ ],
    "selected" : true
  },
  "elements" : {
)LITERAL";

std::string fzvismilestones::get_or_add_node_SUID(std::string node_id_str) {
    auto it = node_SUID_map.find(node_id_str);
    if (it == node_SUID_map.end()) {
        node_SUID++;
        node_SUID_map[node_id_str] = node_SUID;
        return std::to_string(node_SUID);
    }
    return std::to_string(it->second);
}

std::string fzvismilestones::get_or_add_edge_SUID() {
    edge_SUID++;
    return std::to_string(edge_SUID);
}

bool fzvismilestones::render_Cytoscape_JSON(std::string & rendered) {
    std::string name = "Graph";
    rendered = CYTOSCAPEJS_HEAD_0;
    rendered += name;
    rendered += CYTOSCAPEJS_HEAD_1;
    rendered += name;
    rendered += CYTOSCAPEJS_HEAD_2;
    rendered += name;
    rendered += CYTOSCAPEJS_HEAD_3;

    std::string nodes("    \"nodes\" : [ ");
    std::string edges("    \"edges\" : [");

    Node_Index indexed_nodes = graph().get_Indexed_Nodes();

    edge_SUID = node_SUID + graph().num_Nodes();

    int num_nodes = 0;
    int num_edges = 0;

    for (const Node_ptr & node_ptr : indexed_nodes) {

        if (check_inactive(node_ptr)) continue;

        if (check_filtered(node_ptr)) continue;

        std::string node_idstr = node_ptr->get_id_str();
        std::string node_SUID_str = get_or_add_node_SUID(node_idstr);

        nodes += 
            "{\n"
            "      \"data\" : {\n"
            "        \"id\" : \"" + node_SUID_str + "\",\n"
            "        \"shared_name\" : \"" + node_idstr + "\",\n"
            "        \"name\" : \"" + node_idstr + "\",\n"
            "        \"SUID\" : " + node_SUID_str + ",\n"
            "        \"selected\" : false\n"
            "      }\n"
            "    }, ";
        num_nodes++;

        const Edges_Set & edges_set = node_ptr->dep_Edges();

        if (!edges_set.empty()) {

            for (const auto & edge_ptr : edges_set) {

                Node_ptr depnode_ptr = edge_ptr->get_dep();

                if (check_inactive(depnode_ptr)) continue;

                if (check_filtered(depnode_ptr)) continue;

                std::string edge_SUID_str = get_or_add_edge_SUID();
                std::string dep_id_str = edge_ptr->get_dep_str();
                std::string dep_SUID_str = get_or_add_node_SUID(dep_id_str);
                std::string edge_name = node_idstr + " (dep_on) " + dep_id_str;

                edges +=
                    "{\n"
                    "      \"data\" : {\n"
                    "        \"id\" : \"" + edge_SUID_str + "\",\n"
                    "        \"source\" : \"" + node_SUID_str + "\",\n"
                    "        \"target\" : \"" + dep_SUID_str + "\",\n"
                    "        \"shared_name\" : \"" + edge_name + "\",\n"
                    "        \"name\" : \"" + edge_name + "\",\n"
                    "        \"interaction\" : \"dep_on\",\n"
                    "        \"SUID\" : " + edge_SUID_str + ",\n"
                    "        \"shared_interaction\" : \"dep_on\",\n"
                    "        \"selected\" : false\n"
                    "      }\n"
                    "    }, ";
                num_edges++;

            }

        }

    }

    if (num_nodes>0) {
        nodes[nodes.size()-2] = ' ';
    }
    if (num_edges>0) {
        edges[edges.size()-2] = ' ';
    }
    nodes += "],\n";
    edges += "]\n";

    rendered += nodes + edges + "  }\n}}\n";

    return true;
}

#define WEBAPP "cytowebapp"
#define WEBAPPSOURCE "/home/randalk/src/formalizer/tools/interface/fzvismilestones/webtemplates/fullapp/" WEBAPP ".tar.gz"
#define WEBAPPTARGETDIR DEFAULT_OUTPUT_DIR "/cytowebapp/data"
#define EXTRACT_WEB_APP_TEMPLATE "tar --overwrite -C " DEFAULT_OUTPUT_DIR " -zxvf " WEBAPPSOURCE
bool fzvismilestones::render_as_full_web_app() {
    std::string rendered;
    if (!render_Cytoscape_JSON(rendered)) {
        return false;
    }

    FZOUT(shellcmd2str(EXTRACT_WEB_APP_TEMPLATE));

    output_path = WEBAPPTARGETDIR "/networks.js";
    return to_output(rendered);
}

#define WEBVIEW "cytosingleview"
#define WEBVIEWSOURCE "/home/randalk/src/formalizer/tools/interface/fzvismilestones/webtemplates/singleview/" WEBVIEW ".tar.gz"
#define WEBVIEWTARGETDIR DEFAULT_OUTPUT_DIR "/cytosingleview"
#define EXTRACT_WEB_VIEW_TEMPLATE "tar --overwrite -C " DEFAULT_OUTPUT_DIR " -zxvf " WEBVIEWSOURCE
bool fzvismilestones::render_as_single_view_web_page() {
    std::string rendered;
    if (!render_Cytoscape_JSON(rendered)) {
        return false;
    }

    FZOUT(shellcmd2str(EXTRACT_WEB_VIEW_TEMPLATE));

    output_path = WEBVIEWTARGETDIR "/networks.js";
    return to_output(rendered);
}
