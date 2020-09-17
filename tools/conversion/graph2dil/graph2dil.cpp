// Copyright 2020 Randal A. Koene
// License TBD

/**
 * graph2dil is a backward compatibility conversion tool from Graph, Node, Edge and Log
 * database stored data structures to HTML-style DIL Files and Detailed-Items-by-ID
 * format of DIL_entry content.
 * 
 */

#define FORMALIZER_MODULE_ID "Formalizer:Conversion:DIL2Graph"

// std
#include <filesystem>
#include <ostream>
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Graphaccess.hpp"
#include "templater.hpp"

// local
#include "log2tl.hpp"
#include "graph2dil.hpp"

/// The Makefile attempts to provide this at compile time based on the source
/// file directory.
#ifdef DEFAULT_TEMPLATE_DIR
    std::string template_dir(DEFAULT_TEMPLATE_DIR "/templates");
#else
    std::string template_dir("./templates");
#endif

// This can be specified in the Makefile. If it is not then the macro
// is set to the empty string, which leads to initialization with time
// stamp in /tmp/graph2dil-<time-stamp>.
#ifndef GRAPH2DIL_OUTPUT_DIR
    #define GRAPH2DIL_OUTPUT_DIR ""
#endif // GRAPH2DIL_OUTPUT_DIR

using namespace fz;

enum flow_options {
    //flow_unknown = 0,   /// no recognized request
    flow_all = 0,       /// default: convert Graph to DIL files and Log to TL files
    flow_log2TL = 1,    /// request: convert Log to TL files
    flow_graph2DIL = 2, /// request: convert Graph to DIL files
    flow_NUMoptions
};

enum template_id_enum {
    DILFile_temp,
    DILFile_entry_temp,
    DILFile_entry_tail_temp,
    DILFile_entry_head_temp,
    DILbyID_temp,
    DILbyID_entry_temp,
    DILbyID_superior_temp,
    NUM_temp
};

const std::vector<std::string> template_ids = {
    "DIL_File_template.html",
    "DIL_File_entry_template.html",
    "DIL_File_entry_tail_template.html",
    "DIL_File_entry_head_template.html",
    "DILbyID_template.html",
    "DILbyID_entry_template.html",
    "DILbyID_superior_template.html"
};

typedef std::map<template_id_enum,std::string> graph2dil_templates;

struct graph2dil: public formalizer_standard_program {

    std::string DILTLdirectory = GRAPH2DIL_OUTPUT_DIR; /// location for converted output files
    std::string DILTLindex = GRAPH2DIL_OUTPUT_DIR "/../graph2dil-lists.html";
    std::vector<std::string> cmdargs; /// copy of command line arguments

    Graph_access ga;

    flow_options flowcontrol;

    render_environment env;
    graph2dil_templates templates;

    std::unique_ptr<Graph> graph;
    std::unique_ptr<Log> log;

    graph2dil(): formalizer_standard_program(true), ga(add_option_args,add_usage_top), flowcontrol(flow_all) { //(flow_unknown) {
        add_option_args += "DL";
        add_usage_top += " [-D] [-L]";
    }

    virtual void usage_hook() {
        ga.usage_hook();
        FZOUT("    -D convert Graph to DIL Files\n");
        FZOUT("    -L convert Log to Task Log files\n");
        FZOUT("\n");
        FZOUT("Default behavior is to convert both Graph to DIL files and Log to Task Log files.\n");
    }

    virtual bool options_hook(char c, std::string cargs) {
        if (ga.options_hook(c,cargs))
            return true;

        switch (c) {

        case 'D':
            flowcontrol = flow_graph2DIL;

        case 'L':
            flowcontrol = flow_log2TL;
            return true;

        }

       return false;
    }

    /**
     * Initialize configuration parameters.
     * Call this at the top of main().
     * 
     * @param argc command line parameters count forwarded from main().
     * @param argv command line parameters array forwarded from main().
     */
    void init_top(int argc, char *argv[]) {
        //*************** for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i]; // do this before getopt mucks it up
        init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

        if (DILTLdirectory.empty())
            DILTLdirectory = "/tmp/graph2dil-"+TimeStampYmdHM(ActualTime());

        // For each of the possible program flow control choices that would not already
        // have exited, we need both the Graph and the full Log, so we may as well load them here.
        ERRHERE(".load");
        std::tie(graph,log) = ga.request_Graph_and_Log_copies_and_init();

        VERBOSEOUT("\nFormalizer Graph and Log data structures fully loaded:\n\n");
        VERBOSEOUT("  Number of Nodes        = "+std::to_string(graph->num_Nodes())+'\n');
        VERBOSEOUT("  Number of Edges        = "+std::to_string(graph->num_Edges())+'\n');
        VERBOSEOUT("  Number of Topics       = "+std::to_string(graph->num_Topics())+'\n');
        VERBOSEOUT("  Number of Entries      = "+std::to_string(log->num_Entries())+'\n');
        VERBOSEOUT("  Number of Chunks       = "+std::to_string(log->num_Chunks())+'\n');
        VERBOSEOUT("  Number of Breakpoints  = "+std::to_string(log->num_Breakpoints())+"\n\n");
    }

} g2d;

/**
 * Program flow: Handle request to convert Log to Task Log Files.
 */
bool flow_convert_Log2TL() {
    ERRTRACE;
    key_pause();
    
    if (!std::filesystem::create_directories(g2d.DILTLdirectory)) {
        FZERR("\nUnable to create the output directory "+g2d.DILTLdirectory+".\n");
        exit(exit_general_error);
    }

    ERRHERE(".goLog2TL");
    Log2TL_conv_params params;
    params.TLdirectory = g2d.DILTLdirectory;
    params.IndexPath = g2d.DILTLindex;
    params.o = &std::cout;
    //params.from_idx = from_section;
    //params.to_idx = to_section;    
    if (!interactive_Log2TL_conversion(*(g2d.graph.get()), *(g2d.log.get()), params)) {
        FZERR("\nNeed a database account to proceed. Defaults to $USER.\n");
        exit(exit_general_error);
    }
    
    return true;
}

bool load_graph2dil_templates(graph2dil_templates & templates) {
    templates.clear();

    for (int i = 0; i < NUM_temp; ++i) {
        if (!file_to_string(template_dir + "/" + template_ids[i], templates[static_cast<template_id_enum>(i)]))
            ERRRETURNFALSE(__func__, "unable to load " + template_ids[i]);
    }

    return true;
}

/**
 * Arrange lists of pointers to Nodes by their main Topics.
 * 
 * We can obtain main topic by Index-ID directly from the Nodes. So, we
 * make a simple vector arranged by Index-ID for rapid access.
 * 
 * @param graph A valid Graph object containing Nodes.
 */
Node_Index_by_Topic make_Node_Index_by_Topic(Graph & graph) {
    // Pre-seed the vector with pointers to null-size Node_Index objects.
    Node_Index_by_Topic nit(graph.num_Topics());
    for (std::size_t i = 0; i<graph.num_Topics(); ++i)
        nit[i] = std::make_unique<Node_Index>();
    
    // Loop through all Nodes and assign each to its major Topic.
    for (auto node_it = graph.begin_Nodes(); node_it != graph.end_Nodes(); ++node_it) {
        Node * nptr = node_it->second; // *** Slightly risky: Not testing to see if node_it->second is nullptr. Nodes are assumed.
        Topic_ID maintopic = nptr->main_topic_id();

        if (maintopic>graph.num_Topics()) {
            ADDERROR(__func__,"Node ["+nptr->get_id_str()+"] has a main topic index-ID ("+std::to_string(maintopic)+") outside the scope of known Topics ("+std::to_string(graph.num_Topics())+')');
            exit(exit_general_error);
        }

        nit[maintopic]->push_back(nptr);
    }
    return nit;
}

std::string render_Node_oldest_logged(const Log_chain_target & tail) {
    template_varvalues varvals;
    auto bridx = g2d.log->find_Breakpoint_index_before_chaintarget(tail);
    varvals.emplace("tailYmd",g2d.log->get_Breakpoint_Ymd_str(bridx));
    varvals.emplace("tailTLchainID",tail.str());
    return g2d.env.render(g2d.templates[DILFile_entry_tail_temp], varvals);
}

std::string render_Node_newest_logged(const Log_chain_target & head) {
    template_varvalues varvals;
    auto bridx = g2d.log->find_Breakpoint_index_before_chaintarget(head);
    varvals.emplace("headYmd",g2d.log->get_Breakpoint_Ymd_str(bridx));
    varvals.emplace("headTLchainID",head.str());
    return g2d.env.render(g2d.templates[DILFile_entry_head_temp], varvals);
}

/**
 * Render Node data into a DIL_entry HTML template for inclusion in
 * a DIL File.
 * 
 * @param node A Node reference.
 */
std::string render_Node2DILentry(Node & node) {
    template_varvalues varvals;
    varvals.emplace("DIL_ID",node.get_id_str());
    const Log_chain_target * nodelogtail = g2d.log->oldest_Node_chain_element(node.get_id());
    const Log_chain_target * nodeloghead = g2d.log->newest_Node_chain_element(node.get_id());
    if (!nodelogtail) {
        varvals.emplace("tail","tail"); // the Node has no Log entries yet
        varvals.emplace("head","head");
    } else {
        varvals.emplace("tail",render_Node_oldest_logged(*nodelogtail));
        varvals.emplace("head",render_Node_newest_logged(*nodeloghead));
    }
    varvals.emplace("required",to_precision_string(node.get_required(),2));
    varvals.emplace("completion",to_precision_string(node.get_completion(),1));
    varvals.emplace("valuation",to_precision_string(node.get_valuation(),1));
    varvals.emplace("description",node.get_text());
    return g2d.env.render(g2d.templates[DILFile_entry_temp], varvals);
}

/**
 * Convert a vector of Topic_Keyword structures into an HTML-ready string
 * list of format: "keyword-1 (R.r), keyword-2 (R.r)"".
 * 
 * @param keyrelvec Reference to a vector of Topic_Keyword structures.
 * @return HTML-ready string list.
 */
std::string Topic_Keywords_to_KeyRel_List(const std::vector<Topic_Keyword> & keyrelvec) {
    std::string res;
    for (unsigned int i = 0; i < keyrelvec.size(); ++i) {
        if (i>0) {
            res += ", ";
        }
        res += keyrelvec[i].keyword + " (";
        res += to_precision_string(keyrelvec[i].relevance,1) + ')';
    }
    return res;
}

/**
 * In v2.x of the data structure, the target date data is unified
 * in the Node. The composition of the v1.x HTML format targetdate
 * code is as follows:
 * - Possibly [F]ixed, or [E]xact.
 * - If F or E then possibly periodic [d]aily, work[D]ays, [w]eekly, [b]i-weekly, [m]onthly, end of [M]onth,
 *   [y]early, or old [s]pan daily.
 * - If periodic then possibly [e]very "<num>_" iterations.
 * - And then possibly [s]pan "<num>_" repetitions.
 * - Then, a ? means unspecified targetdate.
 * - Or, a 12 (or 8) digit YmdHM targetdate time stamp.
 */
std::string render_TargetDate(Node & node) {
    std::string res;
    bool hasYmdHM;
    switch (node.get_tdproperty()) {
    case fixed: {
        res += 'F';
        hasYmdHM = true;
        break;
    }
    case exact: {
        res += 'E';
        hasYmdHM = true;
        break;
    }
    case inherit: { // should have no YmdHM digits, used to be shown by F? (see get_Node_tdproperty() comments in dil2graph.cpp)
        res += 'F';
        hasYmdHM = false;
        break;
    }
    case variable: { // does have YmdHM digits
        hasYmdHM = true;
        break;
    }
    default:
        hasYmdHM = false;
    }

    const char periodic_char_code[_patt_num] = { 'd', 'D', 'w', 'b', 'm', 'M', 'y', 's', '\0' };
    if (node.get_repeats()) {
        if (node.get_tdpattern()<patt_nonperiodic) {
            res += periodic_char_code[node.get_tdpattern()];
        }

        if (node.get_tdevery()>1) {
            res += 'e' + std::to_string(node.get_tdevery()) + '_';
        }

        if (node.get_tdspan()>0) {
            res += 's' + std::to_string(node.get_tdspan()) + '_';
        }
    }

    if (hasYmdHM) {
        res += node.get_targetdate_str();
    } else {
        res += '?';
    }

    return res;
}

/**
 * It seems that in dil2graph, most of the unspecified values were
 * set to 0.0. In converting back, that means we cannot distinguish
 * between values that appear as 0.0 or as ?.?. But note that this
 * is also how the 'ME' DIL entry editing function has interpreted
 * those values, showing those marked ?.? as 0.0.
 */
std::string render_Edge_Parameter(float parval) {
    if (parval<=0.0) return "?.?";

    return to_precision_string(parval,1);
}

/**
 * Render HTML formatted Superior data for the DIL-by-ID file.
 * 
 * Note that the data is constructed from a combination of Edge and
 * Node data, because the targetdate provided by Superior connection
 * data in v1.x of the data structures is provided in unified form by
 * Node data in v2.x of the data structures.
 * 
 * @param edge Reference to the Edge that connects to the superior Node.
 * @param node Reference to the (dependency) Node.
 */
std::string render_Superior(Edge & edge, Node & node) {
    template_varvalues varvals;
    varvals.emplace("sup_DIL_ID",edge.get_sup_str());
    varvals.emplace("relevance",render_Edge_Parameter(edge.get_dependency()));
    varvals.emplace("unbounded",render_Edge_Parameter(edge.get_significance()));
    varvals.emplace("bounded",render_Edge_Parameter(edge.get_importance()));
    varvals.emplace("targetdate_code",render_TargetDate(node));
    varvals.emplace("urgency",render_Edge_Parameter(edge.get_urgency()));
    varvals.emplace("priority",render_Edge_Parameter(edge.get_priority()));
    return g2d.env.render(g2d.templates[DILbyID_superior_temp], varvals);
}

std::string render_DILbyID_entry(Node & node) {
    template_varvalues varvals;
    varvals.emplace("DIL_ID",node.get_id_str());
    Topic * topic = g2d.graph->main_Topic_of_Node(node);
    if (topic) {
        varvals.emplace("title",topic->get_title());
        varvals.emplace("DIL_File",topic->get_tag());
        varvals.emplace("topic_relevance","1"); // *** I'm just short-circuiting this here, because it was never used.
    }
    const Edges_Set & supedges = node.sup_Edges();
    std::string combined_superiors;
    for (auto sup_it = supedges.begin(); sup_it != supedges.end(); ++sup_it) {
        if (sup_it != supedges.begin())
            combined_superiors += ", ";
        combined_superiors += render_Superior(*(*sup_it),node);
    }
    varvals.emplace("superiors",combined_superiors);
    return g2d.env.render(g2d.templates[DILbyID_entry_temp], varvals);
}


/**
 * Program flow: Handle request to convert Graph to DIL Files and Detailed Items by ID file.
 */
bool flow_convert_Graph2DIL() {
    ERRTRACE;
    key_pause();

    ERRHERE(".prep");
    if (!std::filesystem::create_directories(g2d.DILTLdirectory)) {
        FZERR("\nUnable to create the output directory "+g2d.DILTLdirectory+".\n");
        exit(exit_general_error);
    }

    ERRHERE(".goGraph2DIL");
    if (!load_graph2dil_templates(g2d.templates))
        ERRRETURNFALSE(__func__,"unable to load templates for Graph to DIL Files rendering");

    ERRHERE(".DILFiles");
    Node_Index_by_Topic nit = make_Node_Index_by_Topic(*g2d.graph);
    for (Topic_ID topicid = 0; topicid < nit.size(); ++topicid) {
        // Reserve space in the receiving string. At 500 Kilobytes, plan.html has long been
        // by far the largest DIL File. There are (at time of writing) 89 DIL Files for a
        // total of 45 Megabytes pre-allocated space.
        std::string DILFile_str;
        DILFile_str.reserve(512*1024);

        // Render the data of each Node into DIL_entry HTML format.
        Node_Index & nodeindex = *(nit[topicid].get());
        for (const auto & nodeptr : nodeindex) {
            DILFile_str.append(render_Node2DILentry(*nodeptr)); // *** slightly risky: assuming nodeptr!=nullptr
        }

        // Render the combiend DIL_entry set into DIL File HTML format.
        Topic * topicptr = g2d.graph->find_Topic_by_id(topicid); // *** slightly risky: assuming topicptr!=nullptr
        template_varvalues varvals;
        varvals.emplace("topictitle",topicptr->get_title());
        varvals.emplace("keyrel_pairs",Topic_Keywords_to_KeyRel_List(topicptr->get_keyrel()));
        varvals.emplace("topicfile",topicptr->get_tag());
        varvals.emplace("firstYmd",nodeindex.front()->get_id_str().substr(0,8)); // *** slightly risky: assuming front()!=nullptr
        varvals.emplace("DIL_entries",DILFile_str);
        std::string rendered_DILFile_str = g2d.env.render(g2d.templates[DILFile_temp], varvals);

        std::string DILFile_path = g2d.DILTLdirectory+'/'+topicptr->get_tag()+".html";
        if (!string_to_file(DILFile_path,rendered_DILFile_str))
            ERRRETURNFALSE(__func__,"unable to write rendered DIL File contents to file "+DILFile_path);

    }

    ERRHERE(".DILbyID");
    std::string DILbyID_str;
    DILbyID_str.reserve(3*1024*1024); // The DIL-by-ID file is a bit over 2.5 Megabytes in size (at time of writing).
    for (auto node_it = g2d.graph->begin_Nodes(); node_it != g2d.graph->end_Nodes(); ++node_it) {
        DILbyID_str.append(render_DILbyID_entry(*(node_it->second)));
    }
    template_varvalues varvals;
    varvals.emplace("entries",DILbyID_str);
    std::string rendered_DILbyID_str = g2d.env.render(g2d.templates[DILbyID_temp], varvals);

    std::string DILbyID_path = g2d.DILTLdirectory+"/detailed-items-by-ID.html";
        if (!string_to_file(DILbyID_path,rendered_DILbyID_str))
            ERRRETURNFALSE(__func__,"unable to write rendered DIL File contents to file "+DILbyID_path);

    FZOUT("\nConverted Graph written to DIL Files in directory:\n  "+g2d.DILTLdirectory+"\n\n");

    return true;
}

int main(int argc, char *argv[]) {
    ERRHERE(".init");
    g2d.init_top(argc,argv);

    switch (g2d.flowcontrol) {

    case flow_log2TL: {
        flow_convert_Log2TL();
        break;
    }

    case flow_graph2DIL: {
        flow_convert_Graph2DIL();
        break;
    }

    default: { // both
        flow_convert_Graph2DIL();
        flow_convert_Log2TL();
    }

    }

    ERRHERE(".exitok");
    return standard.completed_ok();
}
