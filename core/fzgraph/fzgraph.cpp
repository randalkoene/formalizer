// Copyright 2020 Randal A. Koene
// License TBD

/**
 * The fzgraph tool is the authoritative core component with which to
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

#define TESTING_CLIENT_SERVER_SOCKETS
#ifdef TESTING_CLIENT_SERVER_SOCKETS
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#define PORT 8090 
#endif

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
#include "fzgraph.hpp"
#include "addnode.hpp"
#include "addedge.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzgraphedit fzge;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzgraphedit::fzgraphedit() : formalizer_standard_program(false), graph_ptr(nullptr), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "M:T:f:H:a:S:D:t:g:p:r:e:s:Y:G:I:U:P:";
    add_usage_top += " [-M node|edges] [-T <text>] [-f <content-file>] [-H <hours>] [-a <val>] [-S <sups>] [-D <deps>] [-t <targetdate>] [-g <topics>] [-p <tdprop>] [-r <repeat>] [-e <every>] [-s <span>] [-Y <depcy>] [-G <sig>] [-I <imp>] [-U <urg>] [-P <priority>]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "When making a Node, by convention we expect at least one superior, although\n"
        "it is not enforced.\n"
        "When making one or more Edges, the list of superior and dependency nodes\n"
        "are paired up and must be of equal length.\n"
        "Lists of superiors or dependencies, as well as topics, expect comma\n"
        "delimiters.\n");
}

std::string NodeIDs_to_string(const Node_ID_key_Vector & nodeidvec) {
    std::string nodeids_str;
    for (const auto & nodeidkey : nodeidvec) {
        nodeids_str += nodeidkey.str() + ',';
    }
    if (!nodeids_str.empty())
        nodeids_str.pop_back();
    
    return nodeids_str;
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzgraphedit::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -M make 'node' or 'edges'\n");
    FZOUT("    -T description <text> from the command line\n");
    FZOUT("    -f description text from <content-file> (\"STDIN\" for stdin until eof, CTRL+D)\n");
    FZOUT("    -H hours required (default: "+to_precision_string(config.nd.hours)+")\n");
    FZOUT("    -a valuation (default: "+to_precision_string(config.nd.valuation)+")\n");
    FZOUT("    -S list of superior Node IDs (default: "+NodeIDs_to_string(config.superiors)+")\n");
    FZOUT("    -D list of dependency Node IDs (default: "+NodeIDs_to_string(config.dependencies)+")\n");
    FZOUT("    -t target date time stamp (default: "+TimeStampYmdHM(config.nd.targetdate)+")\n");
    FZOUT("    -g topic tags (default: "+join(config.nd.topics,",")+")\n");
    FZOUT("    -p target date property (default: "+td_property_str[config.nd.tdproperty]+")\n");
    FZOUT("    -r repeat pattern (default: "+td_pattern_str[config.nd.tdpattern]+")\n");
    FZOUT("    -e every (default multiplier: "+std::to_string(config.nd.tdevery)+")\n");
    FZOUT("    -s span (default iterations: "+std::to_string(config.nd.tdspan)+")\n");
    FZOUT("    -Y edge dependency (default: "+to_precision_string(config.ed.dependency)+")\n");
    FZOUT("    -G edge significance (default: "+to_precision_string(config.ed.significance)+")\n");
    FZOUT("    -I edge importance (default: "+to_precision_string(config.ed.importance)+")\n");
    FZOUT("    -U edge urgency (default: "+to_precision_string(config.ed.urgency)+")\n");
    FZOUT("    -P edge priority (default: "+to_precision_string(config.ed.priority)+")\n");
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
    //return (td_pattern) 0; // never reaches this
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
bool fzge_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    // *** You could also implement try-catch here to gracefully report problems with configuration files.
    CONFIG_TEST_AND_SET_PAR(content_file, "content_file", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(nd.hours, "hours", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.valuation, "valuation", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(superiors, "superiors", parlabel, parse_config_NodeIDs(parvalue));
    CONFIG_TEST_AND_SET_PAR(dependencies, "dependencies", parlabel, parse_config_NodeIDs(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.topics, "topics", parlabel, parse_config_topics(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.targetdate, "targetdate", parlabel, interpret_config_targetdate(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.tdproperty, "tdproperty", parlabel, interpret_config_tdproperty(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.tdpattern, "tdpattern", parlabel, interpret_config_tdpattern(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.tdevery, "tdevery", parlabel, std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(nd.tdspan, "tdspan", parlabel, std::stoi(parvalue));
    CONFIG_TEST_AND_SET_PAR(ed.dependency, "dependency", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(ed.significance, "significance", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(ed.importance, "importance", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(ed.urgency, "urgency", parlabel, std::stof(parvalue));
    CONFIG_TEST_AND_SET_PAR(ed.priority, "priority", parlabel, std::stof(parvalue));
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
bool fzgraphedit::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'M': {
        if (cargs=="node") {
            flowcontrol = flow_make_node;
            return true;
        }
        if (cargs=="edges") {
            flowcontrol = flow_make_edge;
            return true;
        }
        return false;
    }

    case 'T': {
        config.nd.utf8_text = utf8_safe(cargs);
        return true;
    }

    case 'f': {
        config.content_file = cargs;
        return true;
    }

    case 'H': {
        config.nd.hours = std::stof(cargs);
        return true;
    }

    case 'a': {
        config.nd.valuation = std::stof(cargs);
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
        config.nd.targetdate = interpret_config_targetdate(cargs);
        return true;
    }

    case 'g': {
        config.nd.topics = parse_config_topics(cargs);
        return true;
    }

    case 'p': {
        config.nd.tdproperty = interpret_config_tdproperty(cargs);
        return true;
    }

    case 'r': {
        config.nd.tdpattern = interpret_config_tdpattern(cargs);
        return true;
    }

    case 'e': {
        config.nd.tdevery = std::stoi(cargs);
        return true;
    }

    case 's': {
        config.nd.tdspan = std::stoi(cargs);
        return true;
    }

    case 'Y': {
        config.ed.dependency = std::stof(cargs);
        return true;
    }

    case 'G': {
        config.ed.significance = std::stof(cargs);
        return true;
    }

    case 'I': {
        config.ed.importance = std::stof(cargs);
        return true;
    }

    case 'U': {
        config.ed.urgency = std::stof(cargs);
        return true;
    }

    case 'P': {
        config.ed.priority = std::stof(cargs);
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
void fzgraphedit::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
}

#ifdef TESTING_CLIENT_SERVER_SOCKETS
// ----- begin: Test here then move -----
bool client_socket_message(std::string request_str) {
    //struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char str[100];

    //printf("\nInput the string:");
    //scanf("%[^\n]s", str);
    //char buffer[1024] = { 0 };

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return false;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from
    // text to binary form 127.0.0.1 is local
    // host IP address, this address should be
    // your system local host IP address
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nAddress not supported \n");
        return false;
    }

    // connect the socket
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return false;
    }

    int l = strlen(str);
  
    // send string to server side 
    //send(sock, str, sizeof(str), 0); 
    send(sock, request_str.c_str(), request_str.size()+1, 0);

    // read string sent by server
    ssize_t valread = read(sock, str, l);
    if (valread==0) {
        FZOUT("Server response reached EOF.\n");
    }
    if (valread<0) {
        FZERR("Server response read returned ERROR.\n");
    }

    //printf("%s\n", str);
    std::string response_str(str);

    if (response_str == "RESULTS") {
        FZOUT("Success! Results are in shared 'results' data structure.\n");
    } else {
        if (response_str == "ERROR") {
            FZOUT("An error occurred. See the error message in the shared 'error' data structure.\n");
        } else {
            FZOUT("Unknown response: "+response_str+'\n');
        }
    }

    return true;
}
// ----- end  : Test here then move -----
#endif

int make_node() {
    ERRTRACE;
    auto [exit_code, errstr] = get_content(fzge.config.nd.utf8_text, fzge.config.content_file, "Node description");
    if (exit_code != exit_ok)
        standard_exit_error(exit_code, errstr, __func__);

    // Determine probably memory space needed.
    // *** MORE HERE
    unsigned long segsize = sizeof(fzge.config.nd.utf8_text)+fzge.config.nd.utf8_text.capacity() + 1024; // *** wild guess
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    Node * node_ptr = nullptr;
    if ((node_ptr = add_Node_request(*graphmod_ptr, fzge.config.nd)) == nullptr) {
        standard_exit_error(exit_general_error, "Unable to prepare Add-Node request", __func__);
    }

    for (const auto & supkey : fzge.config.superiors) {
        if (!add_Edge_request(*graphmod_ptr, node_ptr->get_id().key(), supkey, fzge.config.ed)) {
            standard_exit_error(exit_general_error, "Unable to prepare Add-Edge request to superior", __func__);
        }
    }

    for (const auto & depkey : fzge.config.dependencies) {
        if (!add_Edge_request(*graphmod_ptr, depkey, node_ptr->get_id().key(), fzge.config.ed)) {
            standard_exit_error(exit_general_error, "Unable to prepare Add-Edge request from dependency", __func__);
        }
    }

    client_socket_message(segname);

    FZOUT("Here we'd be waiting for the server to respond with the status of our request...\n");
    key_pause();

    return standard.completed_ok();
}

int make_edges() {
    ERRTRACE;

    if (fzge.config.superiors.size() != fzge.config.dependencies.size()) {
        standard_exit_error(exit_general_error, "The list of superiors and list of dependencies must be of equal size", __func__);
    }
    if (fzge.config.superiors.empty()) {
        standard_exit_error(exit_general_error, "At least one superior and dependency pair are needed", __func__);
    }

    // Determine probably memory space needed.
    // *** MORE HERE
    unsigned long segsize = fzge.config.superiors.size()*1024; // *** wild guess
    // Determine a unique segment name to share with `fzserverpq`
    std::string segname(unique_name_Graphmod());
    Graph_modifications * graphmod_ptr = allocate_Graph_modifications_in_shared_memory(segname, segsize);
    if (!graphmod_ptr)
        standard_exit_error(exit_general_error, "Unable to create shared segment for modifications requests (name="+segname+", size="+std::to_string(segsize)+')', __func__);

    for (size_t i = 0; i < fzge.config.superiors.size(); ++i) {
        if (!add_Edge_request(*graphmod_ptr, fzge.config.dependencies[i], fzge.config.superiors[i], fzge.config.ed)) {
            standard_exit_error(exit_general_error, "Unable to prepare Add-Edge request from "+fzge.config.dependencies[i].str()+" to "+fzge.config.superiors[i].str(), __func__);
        }
    }

    client_socket_message(segname);

    FZOUT("Here we'd be waiting for the server to respond with the status of our request...\n");
    key_pause();

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzge.init_top(argc, argv);

    switch (fzge.flowcontrol) {

    case flow_make_node: {
        return make_node();
    }

    case flow_make_edge: {
        return make_edges();
    }

    default: {
        fzge.print_usage();
    }

    }

    return standard.completed_ok();
}