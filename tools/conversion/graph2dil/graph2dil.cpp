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
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "Graphaccess.hpp"
#include "templater.hpp"
#include "Logpostgres.hpp"

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

using namespace fz;

graph2dil g2d;

const std::vector<std::string> template_ids = {
    "DIL_File_template.html",
    "DIL_File_entry_template.html",
    "DIL_File_entry_tail_template.html",
    "DIL_File_entry_head_template.html",
    "DILbyID_template.html",
    "DILbyID_entry_template.html",
    "DILbyID_superior_template.html"
};

/**
 * Configure configurable parameters.
 * 
 * Note that this can throw exceptions, such as std::invalid_argument when a
 * conversion was not poossible. That is a good precaution against otherwise
 * hard to notice bugs in configuration files.
 */
bool g2d_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    ERRTRACE;

    // *** You could also implement try-catch here to gracefully report problems with configuration files.
    CONFIG_TEST_AND_SET_PAR(DILTLdirectory, "DILTLdirectory", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(use_cached_histories, "use_cached_histories", parlabel, (parvalue == "true"));
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

/**
 * Note: Added `true` in the `ga()` initialization to have this treated as a server in the call to
 * load a complete copy of the Log. That supresses a warning message. It should not affect use of
 * the memory-resident Graph.
 */
graph2dil::graph2dil(): formalizer_standard_program(false), config(*this), ga(*this,add_option_args,add_usage_top, true), flowcontrol(flow_all) { //(flow_unknown) {
    add_option_args += "DLo:H";
    add_usage_top += " [-D] [-L] [-o <output-dir>] ([-H])";
    usage_tail.push_back("\n"
                         "Default behavior is to convert both Graph to DIL files and Log to Task Log files.\n");
}

void graph2dil::usage_hook() {
    ga.usage_hook();
    FZOUT("    -D convert Graph to DIL Files\n"
          "    -L convert Log to Task Log files\n"
          "    -o build converted file structure at <output-dir> path\n"
          "    (-H use Node histories cached in database instead of regenerating)\n");
}

bool graph2dil::options_hook(char c, std::string cargs) {
    if (ga.options_hook(c,cargs))
        return true;

    switch (c) {

    case 'D': {
        flowcontrol = flow_graph2DIL;
        return true;
    }

    case 'L': {
        flowcontrol = flow_log2TL;
        return true;
    }

    case 'o': {
        config.DILTLdirectory = cargs;
        return true;
    }

    case 'H': {
        config.use_cached_histories = true;
        return true;
    }

    }

    return false;
}

std::string Graph_summary(Graph & graph) {
    std::string summary;
    summary += "\n\tNumber of Nodes        = "+std::to_string(graph.num_Nodes());
    summary += "\n\tNumber of Edges        = "+std::to_string(graph.num_Edges());
    summary += "\n\tNumber of Topics       = "+std::to_string(graph.num_Topics());
    return summary;
}

std::string Log_summary(Log & log) {
    std::string summary;
    summary += "\n\tNumber of Entries      = "+std::to_string(log.num_Entries());
    summary += "\n\tNumber of Chunks       = "+std::to_string(log.num_Chunks());
    summary += "\n\tNumber of Breakpoints  = "+std::to_string(log.num_Breakpoints());
    return summary;
}

/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void graph2dil::init_top(int argc, char *argv[]) {
    //*************** for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i]; // do this before getopt mucks it up
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    if (config.DILTLdirectory.empty()) {
        config.DILTLdirectory = "/tmp/graph2dil-"+TimeStampYmdHM(ActualTime());
    }
    config.DILTLdirectory += "/lists";
    DILTLindex = config.DILTLdirectory + "/lists.html";

    // For each of the possible program flow control choices that would not already
    // have exited, we need both the Graph and the full Log, so we may as well load them here.
    ERRHERE(".load");
    VERBOSEOUT("\n\nAccessing memory-resident Graph and loading complete Log into memory...\n");
    std::tie(graph,log) = ga.access_shared_Graph_and_request_Log_copy_with_init();

    VERBOSEOUT("\nUsing memory-resident Graph, Log data structure fully loaded:\n"+Graph_summary(*graph)+Log_summary(*log)+"\n\n");

    /*** Presently ignoring the load-from-database option, because generating with explicit_only is probably faster
     *** than loading and then looking for all entries in the Log to remove implicit ones. If the database cache
     *** also remembers if an entry belonged to a Node implicitly or explicitly then that might be faster.
    if (config.use_cached_histories) {
        VERYVERBOSEOUT("Loading Node histories from cache in database...\n\n");
        if (!load_Node_history_cache_table_pq(ga, histories)) {
            VERBOSEOUT("Unable to load Node histories from database, trying to regenerate from Log instead...\n\n");
            histories.init(*(log.get()));
        }
    } else {
    */
        VERYVERBOSEOUT("Regenerating Node histories from Log...\n\n");
        histories.init(*(log.get()), true);
    //}
}

/**
 * Program flow: Handle request to convert Log to Task Log Files.
 */
bool flow_convert_Log2TL() {
    ERRTRACE;
    VERBOSEOUT("\n\nFormalized 1.x HTML Task Log (TL) files will be generated in directory:\n\t"+g2d.config.DILTLdirectory+"\n\n");
    key_pause();
    
    if (g2d.flowcontrol == flow_log2TL) {
        if (std::filesystem::exists(g2d.config.DILTLdirectory)) {
            if (default_choice("The target directory already exists. Overwrite existing files? (y/N) ",'y')) {
                standard_exit_error(exit_file_error, "Please provide a usable path for generated files.", __func__);
            }
        }
    }

    std::error_code ec;
    if (!std::filesystem::create_directories(g2d.config.DILTLdirectory, ec)) {
        if (ec) {
            standard_exit_error(exit_file_error, "\nUnable to create the output directory "+g2d.config.DILTLdirectory+".\n", __func__);
        }
    }

    ERRHERE(".goLog2TL");
    Log2TL_conv_params params;
    params.TLdirectory = g2d.config.DILTLdirectory;
    params.IndexPath = g2d.DILTLindex;
    params.o = &std::cout;
    //params.from_idx = from_section;
    //params.to_idx = to_section;    
    if (!interactive_Log2TL_conversion(*(g2d.graph), *(g2d.log.get()), params)) {
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
    for (std::size_t i = 0; i < graph.num_Topics(); ++i) {
        nit[i] = std::make_unique<Node_Index>();
    }
    
    // Loop through all Nodes and assign each to its major Topic.
    for (auto node_it = graph.begin_Nodes(); node_it != graph.end_Nodes(); ++node_it) {
        Node * nptr = node_it->second.get();
        if (!nptr) {
            standard_exit_error(exit_missing_data, "Graph Node map contains a Node nullptr (should never happen)", __func__);
        }
        Topic_ID maintopic = nptr->main_topic_id();
/*if (nptr->get_id_str() == "20201014220834.1") {
    VERYVERBOSEOUT("'Update dil2graph' Node looks like it has maintopic="+std::to_string(maintopic)+'\n');
}*/

        if (maintopic > graph.num_Topics()) {
            ADDERROR(__func__,"Node ["+nptr->get_id_str()+"] has a main topic index-ID ("+std::to_string(maintopic)+") outside the scope of known Topics ("+std::to_string(graph.num_Topics())+')');
            exit(exit_general_error);
        }

        if (maintopic >= nit.size()) {
            standard_exit_error(exit_missing_data, "Main topic index ("+std::to_string(maintopic)+") of Node "+nptr->get_id_str()+" exceeds number of Topics registered.", __func__);
        }
        Node_Index * ni_ptr = nit[maintopic].get();
        if (!ni_ptr) {
            standard_exit_error(exit_missing_data, "Main topic index ("+std::to_string(maintopic)+") has null Node_Index.", __func__);
        }
        ni_ptr->push_back(nptr);
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
 * This is using a rather slow process right now. The search for `nodeloghead`
 * has to walk back from the most recent Log entry or chunk to the first appearance
 * of the node (if there is any, otherwise all the way to the beginning). Then it
 * does the same thing again for the `nodelogtail`, plus from there follow the
 * `node_prev` chain that was set up in `Graph_access::rapid_access_init()`.
 * 
 * It would probably be much faster to collect Node histories once (or even load
 * them from the database if their cache is up to date), and then just grab the
 * heads and tails from there.
 * 
 * At a bare minimum, you don't need to do the search twice. Just start from
 * Log_chain_target.
 * 
 * Since we're going to do this for all Nodes anyway, the histories appraoch is
 * clearly more efficient.
 * 
 * @param node A Node reference.
 */
std::string render_Node2DILentry(Node & node) {
    template_varvalues varvals;
    varvals.emplace("DIL_ID",node.get_id_str());
    Log_chain_target nodeloghead = g2d.histories.newest(node);
    Log_chain_target nodelogtail = g2d.histories.oldest(node);
    //const Log_chain_target * nodeloghead = g2d.log->newest_Node_chain_element(node.get_id());
    //const Log_chain_target * nodelogtail = g2d.log->oldest_Node_chain_element(node.get_id());
    if (nodelogtail.isnulltarget_byID()) {
        varvals.emplace("tail",""); //"tail"); // the Node has no Log entries yet
        varvals.emplace("head",""); //"head");
    } else {
        varvals.emplace("tail",render_Node_oldest_logged(nodelogtail));
        varvals.emplace("head",render_Node_newest_logged(nodeloghead));
    }
    varvals.emplace("required",to_precision_string(((float)node.get_required())/3600.0,2));
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
std::string Topic_Keywords_to_KeyRel_List(const Topic_KeyRel_Vector & keyrelvec) {
    std::string res;
    for (unsigned int i = 0; i < keyrelvec.size(); ++i) {
        if (i > 0) {
            res += ", ";
        }
        res += keyrelvec[i].keyword + " (";
        res += to_precision_string(keyrelvec[i].relevance,1) + ')';
    }
    return res;
}

#define PROBLEMNODE "20000208085743.1"

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
std::string render_TargetDate(const Node & node) {
bool checkthis = (node.get_id_str() == PROBLEMNODE);
if (checkthis) VERYVERBOSEOUT("INSPECTING NODE "+node.get_id_str()+'\n');
if (checkthis) VERYVERBOSEOUT("VALUE OF TDPROPERTY = "+std::to_string(node.get_tdproperty())+"\n");
    std::string res;
    bool hasYmdHM;
    switch (node.get_tdproperty()) {
    case fixed: {
        res += 'F';
        hasYmdHM = true;
if (checkthis) VERYVERBOSEOUT("WRONG ONE!\n");
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
if (checkthis) VERYVERBOSEOUT("RIGHT ONE!\n");
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
        if (node.get_tdpattern() < patt_nonperiodic) {
            res += periodic_char_code[node.get_tdpattern()];
        }

        if (node.get_tdevery() > 1) {
            res += 'e' + std::to_string(node.get_tdevery()) + '_';
        }

        if (node.get_tdspan() > 0) {
            res += 's' + std::to_string(node.get_tdspan()) + '_';
        }
    }

    if (hasYmdHM) {
        std::string td_str(node.get_targetdate_str());
        if (td_str.empty()) { // being super strict on purpose
            standard_exit_error(exit_conversion_error, "Fixed target date at "+node.get_id_str()+" specified no target date (should probably be inherited instead).\n", __func__);
        }
        res += td_str;
if (checkthis) VERYVERBOSEOUT("TYRING THE WRONG THING! res = "+res+"\n");
    } else {
        res += '?';
if (checkthis) VERYVERBOSEOUT("TYRING THE RIGHT THING! res = "+res+"\n");
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
    if (parval <= 0.0) return "?.?";

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
        if (sup_it != supedges.begin()) {
            combined_superiors += ", ";
        }
        const Edge_ptr e_ptr = (*sup_it).get();
        if (!e_ptr) {
            standard_exit_error(exit_missing_data, "Missing superior Edge pointer at Node "+node.get_id_str()+".\n", __func__);
        }
        combined_superiors += render_Superior(*e_ptr,node);
        bool checkthis = (node.get_id_str() == PROBLEMNODE);
        if (checkthis) VERYVERBOSEOUT("THE SUPERIORS: "+combined_superiors+"\n");
    }
    varvals.emplace("superiors",combined_superiors);
    return g2d.env.render(g2d.templates[DILbyID_entry_temp], varvals);
}

/**
 * Program flow: Handle request to convert Graph to DIL Files and Detailed Items by ID file.
 */
bool flow_convert_Graph2DIL() {
    ERRTRACE;
    VERBOSEOUT("Formalized 1.x HTML Detailed Item Lists (DIL) files will be generated in directory:\n\t"+g2d.config.DILTLdirectory+"\n\n");
    key_pause();

    if (std::filesystem::exists(g2d.config.DILTLdirectory)) {
        if (default_choice("The target directory already exists. Overwrite existing files? (y/N) ",'y')) {
            standard_exit_error(exit_file_error, "Please provide a usable path for generated files.", __func__);
        }
    }

    ERRHERE(".prep");
    std::error_code ec;
    if (!std::filesystem::create_directories(g2d.config.DILTLdirectory, ec)) {
        if (ec) {
            standard_exit_error(exit_file_error, "\nUnable to create the output directory "+g2d.config.DILTLdirectory+".\n", __func__);
        }
    }

    ERRHERE(".goGraph2DIL");
    if (!load_graph2dil_templates(g2d.templates))
        ERRRETURNFALSE(__func__,"unable to load templates for Graph to DIL Files rendering");

    ERRHERE(".DILFiles");
    VERBOSEOUT("Building Topic Index with Nodes allocated to their main topic.\n\n");
    VERYVERBOSEOUT("\tReserving 500 KBytes per DIL file, approximate total 45 MBytes.\n\n");
    Node_Index_by_Topic nit = make_Node_Index_by_Topic(*g2d.graph);
    for (Topic_ID topicid = 0; topicid < nit.size(); ++topicid) {
        Topic * topicptr = g2d.graph->find_Topic_by_id(topicid);
        if (!topicptr) {
            standard_exit_error(exit_missing_data, "Missing Topic with ID ("+std::to_string(topicid)+')', __func__);
        }
        VERYVERBOSEOUT("Generating content for "+std::string(topicptr->get_tag().c_str())+".html\n");

        // Reserve space in the receiving string. At 500 Kilobytes, plan.html has long been by far the largest
        // DIL File. There are (at time of writing) 89 DIL Files for a total of 45 Megabytes pre-allocated space.
        std::string DILFile_str;
        DILFile_str.reserve(512*1024);

        // Render the data of each Node into DIL_entry HTML format.
        Node_Index & nodeindex = *(nit[topicid].get());
        for (const auto & nodeptr : nodeindex) {
            if (!nodeptr) {
                standard_exit_error(exit_missing_data, "Missing Node in Topic list of Nodes", __func__);
            }
            DILFile_str.append(render_Node2DILentry(*nodeptr));
        }

        // Render the combiend DIL_entry set into DIL File HTML format.
        template_varvalues varvals;
        varvals.emplace("topictitle",topicptr->get_title());
        varvals.emplace("keyrel_pairs",Topic_Keywords_to_KeyRel_List(topicptr->get_keyrel()));
        varvals.emplace("topicfile",topicptr->get_tag());
        if (nodeindex.empty()) {
            varvals.emplace("firstYmd","");
        } else {
            varvals.emplace("firstYmd",nodeindex.front()->get_id_str().substr(0,8));
        }
        varvals.emplace("DIL_entries",DILFile_str);
        std::string rendered_DILFile_str = g2d.env.render(g2d.templates[DILFile_temp], varvals);

        std::string DILFile_path = (g2d.config.DILTLdirectory+'/')+topicptr->get_tag().c_str()+".html";
        if (!string_to_file(DILFile_path,rendered_DILFile_str))
            ERRRETURNFALSE(__func__,"unable to write rendered DIL File contents to file "+DILFile_path);

    }

    ERRHERE(".DILbyID");
    VERBOSEOUT("Building DIL-by-ID file.\n\n");
    VERYVERBOSEOUT("\tReserving 3 MBytes.\n\n");
    std::string DILbyID_str;
    DILbyID_str.reserve(3*1024*1024); // The DIL-by-ID file is a bit over 2.5 Megabytes in size (at time of writing).
    for (auto node_it = g2d.graph->begin_Nodes(); node_it != g2d.graph->end_Nodes(); ++node_it) {
        Node_ptr n_ptr = (node_it->second).get();
        if (!n_ptr) {
            standard_exit_error(exit_missing_data, "Graph contains Node nullptr (should never happen).", __func__);
        }
        DILbyID_str.append(render_DILbyID_entry(*n_ptr));
    }
    template_varvalues varvals;
    varvals.emplace("entries",DILbyID_str);
    std::string rendered_DILbyID_str = g2d.env.render(g2d.templates[DILbyID_temp], varvals);

    std::string DILbyID_path = g2d.config.DILTLdirectory+"/detailed-items-by-ID.html";
        if (!string_to_file(DILbyID_path,rendered_DILbyID_str))
            ERRRETURNFALSE(__func__,"unable to write rendered DIL File contents to file "+DILbyID_path);

    FZOUT("\nConverted Graph written to DIL Files in directory:\n  "+g2d.config.DILTLdirectory+"\n\n");

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
