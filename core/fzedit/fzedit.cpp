// Copyright 20201126 Randal A. Koene
// License TBD

/**
 * Edit components of the Graph (Nodes, Edges, Topics).
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:Edit"

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "utf8.hpp"
#include "stringio.hpp"
#include "Graphmodify.hpp"
#include "tcpclient.hpp"
#include "Graphtoken.hpp"

// local
#include "version.hpp"
#include "fzedit.hpp"



using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzedit fze;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzedit::fzedit() : formalizer_standard_program(false), config(*this), flowcontrol(flow_unknown) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "M:L:C:T:f:c:H:a:S:D:t:g:p:r:e:s:Y:G:I:U:P:l:d:uz";
    add_usage_top += " [-M <node-id>|<edge-id>] [-T <text>] [-f <content-file>] [-c <ratio>] [-H <hours>] [-a <val>] [-t <targetdate>] [-g <topics>] [-p <tdprop>] [-r <repeat>] [-e <every>] [-s <span>] [-Y <depcy>] [-G <sig>] [-I <imp>] [-U <urg>] [-P <priority>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back("If a <content-file> is 'DEFAULT' then the path specified in the configuration\n"
                         "file is used. A configured path does not automatically mean that Node text\n"
                         "will be edited. Node text editing is done only if at least one of -T or -f\n"
                         "are included in the list of arguments.\n");
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzedit::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -M modify Node or Edge with <id> (discerned by length of ID)\n"
          "    -T description <text> from the command line\n"
          "    -f description text from <content-file> (\"STDIN\" for stdin until eof, CTRL+D)\n"
          "    -c completion\n"
          "    -H hours required\n"
          "    -a valuation\n"
          "    -t target date time stamp\n"
          "    -g topic tags\n"
          "    -p target date property\n"
          "    -r repeat pattern\n"
          "    -e every\n"
          "    -s span\n"
          "    -Y edge dependency\n"
          "    -G edge significance\n"
          "    -I edge importance\n"
          "    -U edge urgency\n"
          "    -P edge priority\n"
          );
}

/**
 * Handler for command line options that are defined in the derived class
 * as options specific to the program.
 * 
 * Include case statements for each option. Typical handlers do things such
 * as collecting parameter values from `cargs` or setting `flowcontrol` choices.
 * 
 * @param c is the character that identifies a specific option.
 * @param cargs is the optional parameter value provided for the option.
 */
bool fzedit::options_hook(char c, std::string cargs) {
    ERRTRACE;
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'M': {
        // identify by ID length
        if (cargs.size() == NODE_ID_STR_NUMCHARS) {
            flowcontrol = flow_edit_node;
        } else if (cargs.size() == ((2*NODE_ID_STR_NUMCHARS)+1)) {
            flowcontrol = flow_edit_edge;
        } else {
            standard_exit_error(exit_bad_request_data, "Unrecognizable Node or Edge ID ("+cargs+")", __func__);
        }
        idstr = cargs;
        return true;
    }

    case 'T': {
        nd.utf8_text = utf8_safe(cargs);
        editflags.set_Edit_text();
        return true;
    }

    case 'f': {
        config.content_file = cargs;
        editflags.set_Edit_text();
        return true;
    }

    case 'c': {
        nd.completion = std::stof(cargs);
        editflags.set_Edit_completion();
        return true;
    }

    case 'H': {
        nd.hours = std::stof(cargs);
        editflags.set_Edit_required();
        return true;
    }

    case 'a': {
        nd.valuation = std::stof(cargs);
        editflags.set_Edit_valuation();
        return true;
    }

    case 't': {
        nd.targetdate = interpret_config_targetdate(cargs);
        editflags.set_Edit_targetdate();
        return true;
    }

    case 'g': {
        nd.topics = parse_config_topics(cargs);
        editflags.set_Edit_topics();
        return true;
    }

    case 'p': {
        nd.tdproperty = interpret_config_tdproperty(cargs);
        editflags.set_Edit_tdproperty();
        return true;
    }

    case 'r': {
        nd.tdpattern = interpret_config_tdpattern(cargs);
        editflags.set_Edit_tdpattern();
        return true;
    }

    case 'e': {
        nd.tdevery = std::stoi(cargs);
        editflags.set_Edit_tdevery();
        return true;
    }

    case 's': {
        nd.tdspan = std::stoi(cargs);
        editflags.set_Edit_tdspan();
        return true;
    }

    case 'Y': {
        ed.dependency = std::stof(cargs);
        editflags.set_Edit_dependency();
        return true;
    }

    case 'G': {
        ed.significance = std::stof(cargs);
        editflags.set_Edit_significance();
        return true;
    }

    case 'I': {
        ed.importance = std::stof(cargs);
        editflags.set_Edit_importance();
        return true;
    }

    case 'U': {
        ed.urgency = std::stof(cargs);
        editflags.set_Edit_urgency();
        return true;
    }

    case 'P': {
        ed.priority = std::stof(cargs);
        editflags.set_Edit_priority();
        return true;
    }

    }

    return false;
}


/// Configure configurable parameters.
bool fze_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(content_file, "content_file", parlabel, parvalue);
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

Graph_modifications & fzedit::graphmod() {
    if (!graphmod_ptr) {
        if (segname.empty()) {
            standard_exit_error(exit_general_error, "Unable to allocate shared segment before generating segment name and size. The prepare_Graphmod_shared_memory() function has to be called first.", __func__);
        }
        graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
        if (!graphmod_ptr)
            standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);
    }
    return *graphmod_ptr;
}

void fzedit::prepare_Graphmod_shared_memory(unsigned long _segsize) {
    segsize = _segsize;
    segname = unique_name_Graphmod(); // a unique name to share with `fzserverpq`
}

std::string verbose_edit_node_info() {
    std::string venstr("Modifying Node "+fze.idstr+", editing the following parameters:");
    if (fze.editflags.Edit_valuation()) {
        venstr += "\n\tvaluation = " + to_precision_string(fze.nd.valuation);
    }
    if (fze.editflags.Edit_completion()) {
        venstr += "\n\tcompletion = " + to_precision_string(fze.nd.completion);
    }
    if (fze.editflags.Edit_required()) {
        venstr += "\n\trequired (hrs) = " + to_precision_string(fze.nd.hours);
    }
    if (fze.editflags.Edit_text()) {
        venstr += "\n\ttext = " + fze.nd.utf8_text;
    }
    if (fze.editflags.Edit_targetdate()) {
        venstr += "\n\ttargetdate = " + TimeStampYmdHM(fze.nd.targetdate);
    }
    if (fze.editflags.Edit_tdproperty()) {
        venstr += "\n\ttdproperty = " + td_property_str[fze.nd.tdproperty];
    }
    /* if (fze.editflags.Edit_repeats()) {
        venstr += "\n\trepeats = ";
    } */
    if (fze.editflags.Edit_tdpattern()) {
        venstr += "\n\ttdpattern = " + td_pattern_str[fze.nd.tdpattern] + " (also edits repeats)";
    }
    if (fze.editflags.Edit_tdevery()) {
        venstr += "\n\ttdevery = " + std::to_string(fze.nd.tdevery);
    }
    if (fze.editflags.Edit_tdspan()) {
        venstr += "\n\ttdspan = " + std::to_string(fze.nd.tdspan);
    }
    if (fze.editflags.Edit_topics()) {
        venstr += "\n\ttopics =";
        for (const auto & topic_tag : fze.nd.topics) {
            venstr += ' ' + topic_tag;
        }
    }
    venstr += '\n';
    return venstr;
}

/// Minimize the number of edits requested to those that actually change values.
void reduce_edits(Graph & graph) {
    Node_ptr node_ptr = graph.Node_by_idstr(fze.idstr);
    Edit_flags editflags;
    if (fze.editflags.Edit_valuation() && (fze.nd.valuation != node_ptr->get_valuation())) {
        editflags.set_Edit_valuation();
    }
    if (fze.editflags.Edit_completion() && (fze.nd.completion != node_ptr->get_completion())) {
        editflags.set_Edit_completion();
    }
    if (fze.editflags.Edit_required() && ((3600.0*(float)fze.nd.hours) != node_ptr->get_required())) {
        editflags.set_Edit_required();
    }
    if (fze.editflags.Edit_text() && (fze.nd.utf8_text != node_ptr->get_text().c_str())) {
        editflags.set_Edit_text();
    }
    if (fze.editflags.Edit_targetdate()) {
        VERYVERBOSEOUT("***"+TimeStampYmdHM(fze.nd.targetdate)+"\n");
        VERYVERBOSEOUT("***"+TimeStampYmdHM(node_ptr->get_targetdate())+"\n");
    }
    if (fze.editflags.Edit_targetdate() && (fze.nd.targetdate != node_ptr->get_targetdate())) {
        VERYVERBOSEOUT("**Edit targetdate\n");
        editflags.set_Edit_targetdate();
    }
    if (fze.editflags.Edit_tdproperty() && (fze.nd.tdproperty != node_ptr->get_tdproperty())) {
        editflags.set_Edit_tdproperty();
    }
    if (fze.editflags.Edit_tdpattern() && (fze.nd.tdpattern != node_ptr->get_tdpattern())) {
        editflags.set_Edit_tdpattern();
    }
    if (fze.editflags.Edit_tdevery() && (fze.nd.tdevery != node_ptr->get_tdevery())) {
        editflags.set_Edit_tdevery();
    }
    if (fze.editflags.Edit_tdspan() && (fze.nd.tdspan != node_ptr->get_tdspan())) {
        editflags.set_Edit_tdspan();
    }
    if (fze.editflags.Edit_topics()) {
       editflags.set_Edit_topics();
    }
    fze.editflags.set_Edit_flags(editflags.get_Edit_flags());
}

int edit_node() {
    ERRTRACE;
    VERYVERBOSEOUT("Initial Edit requests:\n");
    VERYVERBOSEOUT(verbose_edit_node_info());
    VERYVERBOSEOUT("\n");

    if (fze.editflags.Edit_text()) {
        auto [exit_code, errstr] = get_content(fze.nd.utf8_text, fze.config.content_file, "Node description");
        if (exit_code != exit_ok)
            standard_exit_error(exit_code, errstr, __func__);
    }
    if (fze.editflags.Edit_tdpattern()) {
        fze.editflags.set_Edit_repeats(); // the repeats value is set automatically in Graphmodify:Node_data::copy()
    }

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT, this is a wild guess
    fze.prepare_Graphmod_shared_memory(sizeof(fze.nd.utf8_text)+fze.nd.utf8_text.capacity() + 10240);

    Node_ptr node_ptr = fze.graphmod().request_edit_Node(fze.idstr);
    if (!node_ptr) {
        return standard_exit_error(exit_general_error, "Unable to edit Node "+fze.idstr, __func__);
    }

    fze.graphmod().data.back().set_Edit_flags(fze.editflags.get_Edit_flags());
    Graph_ptr graph_ptr = fze.graphmod().get_reference_Graph(); // points to the in-memory Graph

    reduce_edits(*graph_ptr);
    VERYVERBOSEOUT(verbose_edit_node_info());

    fze.nd.copy(*graph_ptr, *node_ptr);

    auto ret = server_request_with_shared_data(fze.get_segname(), graph_ptr->get_server_port());
    standard.exit(ret);
}

int edit_edge() {
    ERRTRACE;

    // Determine probable memory space needed.
    // *** MORE HERE TO BETTER ESTIMATE THAT, this is a wild guess
    fze.prepare_Graphmod_shared_memory(10240);

    Edge_ptr edge_ptr = fze.graphmod().request_edit_Edge(fze.idstr);
    if (!edge_ptr) {
        return standard_exit_error(exit_general_error, "Unable to edit Edge "+fze.idstr, __func__);
    }

    fze.graphmod().data.back().set_Edit_flags(fze.editflags.get_Edit_flags());
    Graph_ptr graph_ptr = fze.graphmod().get_reference_Graph();
    fze.ed.copy(*edge_ptr);

    auto ret = server_request_with_shared_data(fze.get_segname(), graph_ptr->get_server_port());
    standard.exit(ret);
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzedit::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class

}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fze.init_top(argc, argv);

    switch (fze.flowcontrol) {

    case flow_edit_node: {
        return edit_node();
    }

    case flow_edit_edge: {
        return edit_edge();
    }

    default: {
        fze.print_usage();
    }

    }

    return standard.completed_ok();
}
