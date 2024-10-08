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
#include "tcpclient.hpp"
#include "apiclient.hpp"
#include "Graphtoken.hpp"

// local
#include "version.hpp"
#include "fzgraph.hpp"
#include "addnode.hpp"
#include "addedge.hpp"
#include "namednodelist.hpp"


using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzgraphedit fzge;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzgraphedit::fzgraphedit() : formalizer_standard_program(false), graph_ptr(nullptr), config(*this),
                             supdep_from_cmdline(false), nnl_supdep_used(false) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "M:L:C:T:f:H:a:S:D:t:g:p:r:e:s:Y:G:I:U:P:l:d:uzo:m";
    add_usage_top += " [-M node|edges] [-L add|remove|delete] [-C <api-string>] [-T <text>] [-f <content-file>] [-H <hours>] [-a <val>] [-S <sups>] [-D <deps>] [-t <targetdate>] [-g <topics>] [-p <tdprop>] [-r <repeat>] [-e <every>] [-s <span>] [-Y <depcy>] [-G <sig>] [-I <imp>] [-U <urg>] [-P <priority>] [-l <name>] [-d <ask|keep|delete>] [-u] [-z] [-o <outfile>] [-m]";
    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "\n"
        "Please note that the 'superiors', 'dependencies' and 'topics' arguments\n"
        "each expect a list of comma delimited identifiers. Leading and trailing\n"
        "spaces around each element are automatically trimmed.\n"
        "\n"
        "Case notes:\n"
        "\n"
        "A. Making a Node (-M node):\n"
        "   - By convention we expect at least one superior, although that is not\n"
        "     enforced.\n"
        "   - Source preference order to provide superiors and dependencies is:\n"
        "     1. command line, 2. Named Node Lists ('superiors','depdendencies'),\n"
        "     3. configuration file.\n"
        "   - The special targetdate code 'TODAY' is recognized as the end of the\n"
        "     current day.\n"
        "   - In the config.json file, a targetdate empty string explicitly denotes\n"
        "     RTt_unspecified. On the command line, simply leave out the '-t'\n"
        "     argument to accept default values, or explicitly enter '-t -1' to\n"
        "     force RTt_unspecified, but note that the target date property '-p'\n"
        "     has precedence.\n"
        "\n"
        "B. Making one or more Edges (-M edges):\n"
        "   - The list of superior and dependency nodes are paired up and must be\n"
        "     of equal length.\n"
        "\n"
        "C. Modifying a Named Node List (-L):\n"
        "   - The list name is provided with the -l option.\n"
        "   - One or more Node IDs can be provided with the -S and -D options.\n"
        "     (It does not matter which one or both. The lists are concatenated.)\n"
        "\n"
        "D. The -C option provides a way to utilize the Graph server's direct TCP\n"
        "   port API from the command line. This is particularly useful for\n"
        "   server requests that are only supported through that API.\n"
        "   Note that the response is printed only if verbose.\n"
        "\n"
        "   Example a:\n"
        "   -C /fz/graph/namedlists/shortlist?copy=recent&to_max=10&maxsize=10&unique=true\n"
        "\n"
        "   Example b:\n"
        "   -C /fz/graph/namedlists/_reload\n"
        "\n"
        "   Example c:\n"
        "   -C /fz/graph/nodes/20090804075402.1.node -q -o STDOUT\n"
        "\n"
        "   Example d:\n"
        "   -C /fz/graph/nodes/20090804075402.1/targetdate.raw -q -m -o STDOUT\n"
        "\n"
        "   For clean output of just the server API response use both the\n"
        "   options -q and -o <outfile>, e.g. -q -o STDOUT.\n"
        "\n"
        "   For more information about the API see fzserverpq.\n"
        );
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
    FZOUT("    -L modify Named Node List: 'add' to, 'remove' from, or 'delete'\n");
    FZOUT("    -C send any <api-string> to the server port\n");
    FZOUT("    -o send server API output to outfile (can set to STDOUT)\n");
    FZOUT("    -m minimum server API output (empty means error/404)\n")
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
    FZOUT("    -l the <name> of a Named Node List\n");
    FZOUT("    -d after using superiors & dependencies Lists, delete (default), keep, ask\n");
    FZOUT("    -u update 'shortlist' Named Node List\n");
    FZOUT("    -z stop the Graph server\n");
}

Node_ID_key_Vector parse_config_NodeIDs(const std::string & parvalue) {
    ERRTRACE;

    Node_ID_key_Vector nodekeys;
    std::vector<std::string> nodeid_strings;
    if (parvalue.empty())
        return nodekeys;

    nodeid_strings = split(parvalue, ',');
    for (auto & nodeid_str : nodeid_strings) {
        trim(nodeid_str);
        try {
            nodekeys.emplace_back(nodeid_str);
        } catch (ID_exception idexception) {
            standard_exit_error(exit_bad_request_data, "invalid Node ID (" + nodeid_str + ")\n" + idexception.what(), __func__);
        }
    }

    return nodekeys;
}

NNL_after_use interpret_config_supdep_after_use(const std::string & parvalue) {
    if (parvalue == "delete") {
        return nnl_delete;
    }
    if (parvalue == "keep") {
        return nnl_keep;
    }
    if (parvalue == "ask") {
        return nnl_ask;
    }
    standard_exit_error(exit_bad_config_value, "Invalid configured supdep_after_use default: "+parvalue, __func__);
}

/**
 * Configure configurable parameters.
 * 
 * Note that this can throw exceptions, such as std::invalid_argument when a
 * conversion was not poossible. That is a good precaution against otherwise
 * hard to notice bugs in configuration files.
 */
bool fzge_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    ERRTRACE;

    // *** You could also implement try-catch here to gracefully report problems with configuration files.
    CONFIG_TEST_AND_SET_PAR(port_number, "port_number", parlabel, std::stoi(parvalue));
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
    CONFIG_TEST_AND_SET_PAR(supdep_after_use, "supdep_after_use", parlabel, interpret_config_supdep_after_use(parvalue));
    CONFIG_TEST_AND_SET_PAR(listname, "listname", parlabel, parvalue);
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
    ERRTRACE;

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

    case 'L': {
        if (cargs=="add") {
            flowcontrol = flow_add_to_list;
            return true;
        }
        if (cargs=="remove") {
            flowcontrol = flow_remove_from_list;
            return true;
        }
        if (cargs=="delete") {
            flowcontrol = flow_delete_list;
            return true;
        }
        return false;
    }

    case 'C': {
        flowcontrol = flow_port_api;
        api_string = cargs;
        return true;
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
        supdep_from_cmdline = true;
        return true;
    }

    case 'D': {
        config.dependencies = parse_config_NodeIDs(cargs);
        supdep_from_cmdline = true;
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

    case 'l': {
        config.listname = cargs;
        return true;
    }

    case 'd': {
        config.supdep_after_use = interpret_config_supdep_after_use(cargs);
        return true;
    }

    case 'o': {
        outfile = cargs;
        return true;
    }

    case 'm': {
        minimum_API_output = true;
        return true;
    }

    case 'z': {
        flowcontrol = flow_stop_server;
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

int stop_server() {
    VERBOSEOUT("Sending STOP request to Graph server.\n");
    std::string response_str;
    if (!client_socket_shmem_request("STOP", "127.0.0.1", fzge.config.port_number, response_str)) {
        standard_exit_error(exit_communication_error, "Communication error.", __func__);
    }

    if (response_str == "STOPPING") {
        VERBOSEOUT("Server stopping.\n");
        standard.completed_ok();
    }

    return standard_exit_error(exit_general_error, "Unknown response: "+response_str, __func__);
}

int port_API_request() {
    VERBOSEOUT("Sending API request to Server port.\n");
    std::string response_str;
    if (!http_GET(fzge.get_Graph()->get_server_IPaddr(), fzge.get_Graph()->get_server_port(), fzge.api_string, response_str)) {
        return standard_exit_error(exit_communication_error, "API request to Server port failed: "+fzge.api_string, __func__);
    }

    VERYVERBOSEOUT("Server response:\n\n"+response_str);

    if (fzge.minimum_API_output) {
        // Detect success or failure
        auto first_line_end = response_str.find('\n');
        if (first_line_end == std::string::npos) {
            VERYVERBOSEOUT("No line end found in server response.\n");
            response_str = "";
        } else {
            if (response_str.substr(0, first_line_end).find("200 OK") == std::string::npos) {
                VERYVERBOSEOUT("Server response was not OK.\n");
                response_str = "";
            } else {
                // Handle both \r\n\r\n and \n\n empty line formats
                auto empty_line_nn = response_str.find("\n\n");
                auto empty_line_rnrn = empty_line_nn == std::string::npos ? response_str.find("\r\n\r\n") : empty_line_nn;
                if (empty_line_rnrn == std::string::npos) {
                    VERYVERBOSEOUT("No empty line found in server response.\n");
                    response_str = "";
                } else {
                    if (empty_line_nn != std::string::npos) {
                        response_str = response_str.substr(empty_line_nn+2);
                    } else {
                        response_str = response_str.substr(empty_line_rnrn+4);
                    }
                }
            }
        }
    }

    if (!fzge.outfile.empty()) {
        if (fzge.outfile == "STDOUT") {
            FZOUT(response_str);
        } else {
            if (!string_to_file(fzge.outfile, response_str)) {
                return standard_exit_error(exit_file_error, "Failed to write API response to file at "+fzge.outfile, __func__);
            }
        }
    }

    return standard.completed_ok();
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzge.init_top(argc, argv);

    if (!fzge.get_Graph()) { // this initializes fzge.graph_ptr (which is subsequently returned by get_Graph())
        return standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }

    if (fzge.update_shortlist) {
        NNLreq_update_shortlist(fzge.get_Graph()->get_server_IPaddr(), fzge.get_Graph()->get_server_port());
    }

    switch (fzge.flowcontrol) {

    case flow_make_node: {
        return make_node();
    }

    case flow_make_edge: {
        return make_edges();
    }

    case flow_stop_server: {
        return stop_server();
    }

    case flow_add_to_list: {
        return add_to_list();
    }

    case flow_remove_from_list: {
        return remove_from_list();
    }

    case flow_delete_list: {
        return delete_list();
    }

    case flow_port_api: {
        return port_API_request();
    }

    default: {
        fzge.print_usage();
    }

    }

    return standard.completed_ok();
}
