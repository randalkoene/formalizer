// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to template rendering part of
 * fzgraphhtml.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __RENDER_HPP.
 */

#ifndef __RENDER_HPP
#include "version.hpp"
#define __RENDER_HPP (__VERSION_HPP)


using namespace fz;

enum template_id_enum {
    node_pars_in_list_html_temp,
    node_pars_in_list_head_html_temp,
    node_pars_in_list_tail_html_temp,
    named_node_list_in_list_html_temp,
    node_txt_temp,
    node_html_temp,
    node_pars_in_list_card_temp,
    NUM_temp
};

extern std::vector<std::string> template_ids;

bool render_incomplete_nodes();

bool render_named_node_list();

/**
 * Individual Node data rendering.
 * 
 * Note: We will probably be unifying this with the Node
 * rendering code of `fzquerypq`, even though the data here
 * comes from the memory-resident Graph. See the proposal
 * at https://trello.com/c/jJamMykM. When unifying these, 
 * the output rendering format may be specified by an enum
 * as in the code in fzquerypq:servenodedata.cpp.
 * 
 * The rendering format is specified in `fzq.output_format`.
 * 
 * @param graph A valid Graph.
 * @param node A valid Node object.
 * @param render_format Specifies the rendering output format.
 * @return A string with rendered Node data according to the chosen format.
 */
std::string render_Node_data(Graph & graph, Node & node, unsigned int render_format = 1);

#endif // __RENDER_HPP
