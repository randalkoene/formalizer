// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The fzaddnode tool is the authoritative core component with which to
 * add new Nodes and their initial Edges to the Graph.
 * 
 * {{ long_description }}
 * 
 * For more about this, see https://trello.com/c/FQximby2.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Graph:MakeNode"

// std
#include <iostream>
#include <iterator>

// core
#include "error.hpp"
#include "standard.hpp"
#include "stringio.hpp"
#include "general.hpp"
#include "ReferenceTime.hpp"
#include "Graphbase.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"
#include "utf8.hpp"

// local
#include "version.hpp"
#include "fzaddnode.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzaddnode fzan;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzaddnode::fzaddnode() : formalizer_standard_program(false), graph_ptr(nullptr), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "T:f:H:v:S:D:t:g:p:r:e:s:";
    add_usage_top += " [-T <text>] [-f <content-file>] [-H <hours>] [-v <val>] [-S <sups>] [-D <deps>] [-t <targetdate>] [-g <topics>] [-p <tdprop>] [-r <repeat>] [-e <every>] [-s <span>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    //usage_tail.push_back("Extra usage information.\n");
}

std::string NodeIDs_to_string(const Node_ID_key_Vector & nodeidvec) {
    std::string nodeids_str;
    for (const auto & nodeidkey : nodeidvec) {
        nodeids_str += nodeidkey.str(); + ',';
    }
    if (!nodeids_str.empty())
        nodeids_str.pop_back();
    
    return nodeids_str;
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzaddnode::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -T description <text> from the command line\n");
    FZOUT("    -f description text from <content-file> (\"STDIN\" for stdin until eof, CTRL+D)\n");
    FZOUT("    -H hours required (default: "+to_precision_string(config.hours)+")\n");
    FZOUT("    -v valuation (default: "+to_precision_string(config.valuation)+")\n");
    FZOUT("    -S list of superior Node IDs (default: "+NodeIDs_to_string(config.superiors)+")\n");
    FZOUT("    -D list of dependency Node IDs (default: "+NodeIDs_to_string(config.dependencies)+")\n");
    FZOUT("    -t target date time stamp (default: "+TimeStampYmdHM(config.targetdate)+")\n");
    FZOUT("    -g topic tags (default: "+join(config.topics,",")+")\n");
    FZOUT("    -p target date property (default: "+td_property_str[config.tdproperty]+")\n");
    FZOUT("    -r repeat pattern (default: "+td_pattern_str[config.tdpattern]+")\n");
    FZOUT("    -e every (default multiplier: "+std::to_string(config.tdevery)+")\n");
    FZOUT("    -s span (default iterations: "+std::to_string(config.tdspan)+")\n");
}

time_t interpret_config_targetdate(const std::string & parvalue) {
    if (parvalue.empty())
        return RTt_unspecified;

    if (parvalue == "TODAY") {
        return today_end_time();
    }

    time_t t = time_stamp_time(parvalue);
    if (t == RTt_invalid_time_stamp)
        standard_exit_error(exit_bad_config_value, "Invalid time stamp for configured targetdate default: "+parvalue, __func__);

    return t;
}

std::vector<std::string> parse_config_topics(const std::string & parvalue) {
    std::vector<std::string> topics;
    if (parvalue.empty())
        return topics;

    topics = split(parvalue, ',');
    for (auto & topic_tag : topics) {
        trim(topic_tag);
    }

    return topics;
}

td_property interpret_config_tdproperty(const std::string & parvalue) {
    if (parvalue.empty())
        return variable;

    for (int i = 0; i < _tdprop_num; ++i) {
        if (parvalue == td_property_str[i]) {
            return (td_property) i;
        }
    }

    standard_exit_error(exit_bad_config_value, "Invalid configured td_property default: "+parvalue, __func__);
}

td_pattern interpret_config_tdpattern(const std::string & parvalue) {
    if (parvalue.empty())
        return patt_nonperiodic;

    std::string pval = "patt_"+parvalue;
    for (int i = 0; i <  _patt_num; ++i) {
        if (pval == td_pattern_str[i]) {
            return (td_pattern) i;
        }
    }

    standard_exit_error(exit_bad_config_value, "Invalid configured td_pattern default: "+parvalue, __func__);
}

Node_ID_key_Vector parse_config_NodeIDs(const std::string & parvalue) {
    Node_ID_key_Vector nodekeys;
    std::vector<std::string> nodeid_strings;
    if (parvalue.empty())
        return nodekeys;

    nodeid_strings = split(parvalue, ',');
    for (auto & nodeid_str : nodeid_strings) {
        trim(nodeid_str);
        nodekeys.emplace_back(nodeid_str);
    }

    return nodekeys;
}


/**
 * Configure configurable parameters.
 * 
 * Note that this can throw exceptions, such as std::invalid_argument when a
 * conversion was not poossible. That is a good precaution against otherwise
 * hard to notice bugs in configuration files.
 */
bool fzan_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    // *** You could also implement try-catch here to gracefully report problems with configuration files.
    CONFIG_TEST_AND_SET_PAR(content_file, "content_file", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(hours, "hours", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(valuation, "valuation", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(superiors, "superiors", parlabel, parse_config_NodeIDs(parvalue));
    CONFIG_TEST_AND_SET_PAR(dependencies, "dependencies", parlabel, parse_config_NodeIDs(parvalue));
    CONFIG_TEST_AND_SET_PAR(topics, "topics", parlabel, parse_config_topics(parvalue));
    CONFIG_TEST_AND_SET_PAR(targetdate, "targetdate", parlabel, interpret_config_targetdate(parvalue));
    CONFIG_TEST_AND_SET_PAR(tdproperty, "tdproperty", parlabel, interpret_config_tdproperty(parvalue));
    CONFIG_TEST_AND_SET_PAR(tdpattern, "tdpattern", parlabel, interpret_config_tdpattern(parvalue));
    CONFIG_TEST_AND_SET_PAR(tdevery, "tdevery", parlabel, std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(tdspan, "tdspan", parlabel, std::stoi(parvalue));
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
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
bool fzaddnode::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'T': {
        utf8_text = utf8_safe(cargs);
        return true;
    }

    case 'f': {
        config.content_file = cargs;
        return true;
    }

    case 'H': {
        config.hours = std::stof(cargs);
        return true;
    }

    case 'v': {
        config.valuation = std::stof(cargs);
        return true;
    }

    case 'S': {
        config.superiors = parse_config_NodeIDs(cargs);
        return true;
    }

    case 'D': {
        config.dependencies = parse_config_NodeIDs(cargs);
        return true;
    }

    case 't': {
        config.targetdate = interpret_config_targetdate(cargs);
        return true;
    }

    case 'g': {
        config.topics = parse_config_topics(cargs);
        return true;
    }

    case 'p': {
        config.tdproperty = interpret_config_tdproperty(cargs);
        return true;
    }

    case 'r': {
        config.tdpattern = interpret_config_tdpattern(cargs);
        return true;
    }

    case 'e': {
        config.tdevery = std::stoi(cargs);
        return true;
    }

    case 's': {
        config.tdspan = std::stoi(cargs);
        return true;
    }

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
void fzaddnode::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

int make_node() {
    ERRTRACE;
    auto [exit_code, errstr] = get_content(fzan.utf8_text, fzan.config.content_file, "Node description");
    if (exit_code != exit_ok)
        standard_exit_error(exit_code, errstr, __func__);

    // Determine probably memory space needed.
    // *** MORE HERE
    unsigned long segsize = sizeof(fzan.utf8_text)+fzan.utf8_text.capacity() + 1024; // *** wild guess
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    Node * node_ptr = graphmod_ptr->request_add_Node();
    if (!node_ptr)
        standard_exit_error(exit_general_error, "Unable to create new Node object in shared segment ("+graphmemman.get_active_name()+')', __func__);

    VERBOSEOUT("Making Node "+node_ptr->get_id_str()+'\n');

    // Set up Node parameters
    node_ptr->set_text(fzan.utf8_text);
    node_ptr->set_required((unsigned int) fzan.config.hours*3600);
    node_ptr->set_valuation(fzan.config.valuation);
    node_ptr->set_targetdate(fzan.config.targetdate);
    node_ptr->set_tdproperty(fzan.config.tdproperty);
    node_ptr->set_tdpattern(fzan.config.tdpattern);
    node_ptr->set_tdevery(fzan.config.tdevery);
    node_ptr->set_tdspan(fzan.config.tdspan);
    node_ptr->set_repeats((fzan.config.tdpattern != patt_nonperiodic) && (fzan.config.tdproperty != variable) && (fzan.config.tdproperty != unspecified));

    // main topic
    // *** THIS REQUIRES A BIT OF THOUGHT

    // Superior Nodes (you could also establish dependencies)
    // *** THIS REQUIRES ADDITIONAL ITEMS ON THE REQUESTS STACK

    // *** let's find out how much space is consumed in shared memory when a Node is created, improve our estimate!

    FZOUT("Here we'd be waiting for the server to respond with the status of our request...\n");
    key_pause();

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzan.init_top(argc, argv);

    fzan.flowcontrol = flow_make_node; // *** There are no other options yet.

    switch (fzan.flowcontrol) {

    case flow_make_node: {
        return make_node();
    }

    default: {
        fzan.print_usage();
    }

    }

    return standard.completed_ok();
}
