// Copyright 2023 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the visrender part of the
 * fzvismilestones tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __FZVISMILSESTONES_HPP.
 */

#ifndef __FZVISMILSESTONES_HPP
#include "version.hpp"
#define __FZVISMILSESTONES_HPP (__VERSION_HPP)

#include <string>
#include <map>

#include "Graphtypes.hpp"
#include "Graphaccess.hpp"
//#include "templater.hpp"

using namespace fz;

enum flow_options {
    flow_unknown = 0,                 ///< no recognized request
    flow_SIF = 1,                     ///< request: generate output in SIF format
    flow_GraphML = 2,                 ///< request: generate output in GraphML format
    flow_webapp = 3,                  ///< request: generate output as full Cytoscape Web Application
    flow_webview = 4,                 ///< request: generate output as singe view Cytoscape Web Page
    flow_NUMoptions
};

struct fzvismilestones: public formalizer_standard_program {
    Graph_access ga;

    flow_options flowcontrol;

    //Node * node_ptr = nullptr;

    int excerpt_length = 160;

    bool show_completed = false;

    std::string filter_substring;
    int filter_substring_excerpt_length = 80;
    std::string uri_encoded_filter_substring;

    std::string output_path;

    Graph *graph_ptr;

    int node_SUID = 35782;
    int edge_SUID = 0;

    std::map<std::string, int> node_SUID_map;

    //render_environment env;
    //nodeboard_templates templates;

    fzvismilestones();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    Graph & graph();

    bool render_init();

    bool to_output(const std::string & rendered_board);

    bool check_inactive(Node_ptr node_ptr);

    bool check_filtered(Node_ptr node_ptr);

    bool render_as_SIF();

    bool render_as_GraphML();

    std::string get_or_add_node_SUID(std::string node_id_str);

    std::string get_or_add_edge_SUID();

    bool render_Cytoscape_JSON(std::string & rendered);

    bool render_as_full_web_app();

    bool render_as_single_view_web_page();

};

#endif // __FZVISMILSESTONES_HPP
