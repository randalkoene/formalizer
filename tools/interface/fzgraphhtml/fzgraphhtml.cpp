// Copyright 2020 Randal A. Koene
// License TBD

/**
 * {{ brief_description }}
 * 
 * {{ long_description }}
 * 
 * For more about this, see {{ doc_reference }}.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Interface:Graph:HTML"

//#define USE_COMPILEDPING

// std
#include <iostream>

// core
#include "error.hpp"
#include "standard.hpp"
#include "stringio.hpp"
#include "Graphtypes.hpp"
#include "Graphinfo.hpp"
#include "apiclient.hpp"

// local
#include "version.hpp"
#include "fzgraphhtml.hpp"
#include "render.hpp"

using namespace fz;

/// The local class derived from `formalizer_standard_program`.
fzgraphhtml fzgh;

/**
 * For `add_option_args`, add command line option identifiers as expected by `optarg()`.
 * For `add_usage_top`, add command line option usage format specifiers.
 */
fzgraphhtml::fzgraphhtml() : formalizer_standard_program(false), config(*this) { //ga(*this, add_option_args, add_usage_top)
    add_option_args += "n:m:Irt:i:L:N:M:D:x:o:s:S:eT:F:uCjc";
    add_usage_top += " [-n <node-ID>|-m <node-ID>|-I|-t <topic-ID|topic-tag|?>|-L <name|?>]"
        " [-r] [-N <num>] [-M <max-YYYYmmddHHMM>] [-D <num-days>] [-x <len>]"
        " [-o <output-path>] [-s <sortkeys>] [-S <NNL>] [-e] [-T <named|node|Node>=<path>]"
        " [-F html|txt|node|desc] [-u] [-C] [-j] [-c]";    //usage_head.push_back("Description at the head of usage information.\n");
    usage_tail.push_back(
        "Notes:\n"
        "1. When no [N <num>] is provided then the configured value is used.\n"
        "2. With '-m new', the '-t' argument is used to provide a comma\n"
        "   delimited list of Topics to associate the new Node with.\n"
        "3. When a custom template path starts with 'STRING:' then it is\n"
        "   interpreted to mean that the custom template is given in the\n"
        "   argument string instead of in a file.\n"
        "4. Without -x, -n output generates embeddable HTML descriptions.\n"
        "   With -x, -n output removes HTML tags in the excerpt.\n"
        "5. The number of elements limit (-N) overrides the limits set\n"
        "   by -D and -M. To ensure that -D or -M can reach their limit\n"
        "   set '-N all'.\n"
        "\n"
        "Options settable through ./formalizer/config/fzgraphhtml/config.json:\n"
        "  num_to_show (int or 'all'): Show data for that many elements.\n"
        "  excerpt_length (int): Length of description excerpts.\n"
        "  rendered_out_path (string or 'STDOUT'): Send output to this target.\n"
        "  embeddable (bool): Embeddable within HTML content.\n"
        "  outputformat ('txt','json','node','desc','html'): Format.\n"
        "  t_max (time-stamp): Up to the specified date.\n"
        "  num_days (int): Number of days from now.\n"
        "  show_still_required (bool): Hours to complete vs. required time.\n"
        "  interpret_text (comma separated list of strings):\n"
        "    One or more of: 'raw','detect_links','emptyline_is_par','full_markdown'\n"
        "  show_current_time (bool): Before/after current time differentiated.\n"
        "  include_daysummary (bool): Show time totals per day.\n"
        "  timezone_offset_hours (int): Local TZ is number of hours ahead.\n"
        "  show_tzadjust (bool): Apply @TZADJUST@ in lists of Nodes.\n"
        "  max_do_links (int): Maximum number of [do] links to place.\n"
        "    A value of 0 means no limit.\n"
        );
}

/**
 * Add FZOUT statements for each line of the usage info to print as
 * help for program specific command line options.
 */
void fzgraphhtml::usage_hook() {
    //ga.usage_hook();
    FZOUT("    -n Show data for Node with <node-ID>.\n"
          "    -m Editing form for Node with <node-ID> ('new' to define new Node).\n"
          "    -I Show data for incomplete Nodes.\n"
          "    -r Show with repeats of repeating Nodes.\n"
          "    -L Show data for Nodes in Named Node List, or show Names if '?'.\n"
          "    -t Show data for Nodes with Topic, or show Topics if '?'.\n"
          "    -i Include 'add-to-node' for <node-id>.\n"
          "    -N Show data for <num> elements (all=no limit), see note 5.\n"
          "    -M Show data up to and including <max-YYYYmmddHHMM>, see note 5.\n"
          "    -D Show data for <num-days> days, see note 5.\n"
          "    -x Excerpt length <len>.\n"
          "    -o Rendered output to <output-path> (\"STDOUT\" is default).\n"
          "    -s Sort by: targetdate.\n"
          "    -S Highlight Nodes within dependency subtrees of Nodes in NNL.\n"
          "    -e Embeddable, no head and tail templates.\n"
          "    -T Use custom template instead of topics, named, node or single Node.\n"
          "    -F Output format: html (default), txt, json, node, desc.\n"
          "    -u Update 'shortlist' Named Node List.\n"
          "    -C (TEST) card output format.\n"
          "    -j no Javascript.\n"
          "    -c include checkboxes.\n");
}

unsigned int parvalue_to_num_to_show(const std::string & parvalue) {
    if (parvalue.empty())
        return 0;

    unsigned int vmax = std::numeric_limits<unsigned int>::max();
    if (parvalue == "all")
        return vmax;

    long val = atol(parvalue.c_str());
    if (val > vmax)
        return vmax;

    return val;
}

bool custom_template(const std::string & cargs) {
    size_t equalpos = cargs.find('=');
    if (equalpos == std::string::npos) {
        return false;
    }
    std::string replace_template = cargs.substr(0,equalpos);
    ++equalpos;
    if (equalpos>=cargs.size()) {
        return false;
    }
    if (replace_template == "named") {
        template_ids[named_node_list_in_list_temp] = cargs.substr(equalpos);
        return true;
    }
    if (replace_template == "node") {
        template_ids[node_pars_in_list_temp] = cargs.substr(equalpos);
        return true;
    }
    if (replace_template == "Node") {
        template_ids[node_temp] = cargs.substr(equalpos);
        return true;
    }
    if (replace_template == "topics") {
        template_ids[topic_pars_in_list_temp] = cargs.substr(equalpos);
        return true;
    }
    return false;
}

output_format parse_output_format(const std::string & parvalue) {
    if (parvalue == "txt") {
        VERYVERBOSEOUT("TEXT output\n");
        return output_txt;
    } if (parvalue == "json") {
        VERYVERBOSEOUT("JSON output\n");
        return output_json;
    } if (parvalue == "node") {
        VERYVERBOSEOUT("NODE output\n");
        return output_node;
    } if (parvalue == "desc") {
        VERYVERBOSEOUT("DESCRIPTION output\n");
        return output_desc;
    } else {
        VERYVERBOSEOUT("HTML output\n");
        return output_html;
    }
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
bool fzgraphhtml::options_hook(char c, std::string cargs) {
    //if (ga.options_hook(c,cargs))
    //        return true;

    switch (c) {

    case 'n': {
        flowcontrol = flow_node;
        node_idstr = cargs;
        return true;
    }

    case 'm': {
        flowcontrol = flow_node_edit;
        node_idstr = cargs;
        return true;
    }

    case 'I': {
        if (with_repeats) {
            flowcontrol = flow_incomplete_with_repeats;
        } else {
            flowcontrol = flow_incomplete;
        }
        return true;
    }

    case 'r': {
        with_repeats = true;
        if (flowcontrol == flow_incomplete) {
            flowcontrol = flow_incomplete_with_repeats;
        }
        return true;
    }

    case 't': {
        if (flowcontrol != flow_node_edit) {
            flowcontrol = flow_topics;
        }
        list_name = cargs;
        return true;
    }

    case 'i': {
        node_idstr = cargs;
        add_to_node = true;
        return true;
    }

    case 'L': {
        flowcontrol = flow_named_list;
        list_name = cargs;
        return true;
    }

    case 'N': {
        config.num_to_show = parvalue_to_num_to_show(cargs);
        return true;
    }

    case 'M': {
        config.t_max = time_stamp_time(cargs);
        return true;
    }

    case 'D': {
        num_days = std::atoi(cargs.c_str());
        if (num_days<=0) {
            standard_exit_error(exit_command_line_error, "Invalid number of days.", __func__);
        }
        config.t_max = ActualTime() + num_days*24*60*60;
        return true;
    }

    case 'x': {
        config.excerpt_length = atoi(cargs.c_str());
        config.excerpt_requested = true;
        return true;
    }

    case 'o': {
        config.rendered_out_path = cargs;
        return true;
    }

    case 's': {
        if (cargs == "targetdate") {
            config.sort_by_targetdate = true;
        }
        return true;
    }

    case 'S': {
        subtrees_list_name = cargs;
        return true;
    }

    case 'e': {
        config.embeddable = true;
        return true;
    }

    case 'T': {
        return custom_template(cargs);
    }

    case 'F': {
        config.outputformat = parse_output_format(cargs);
        return true;
    }

    case 'u': {
        update_shortlist = true;
        return true;
    }

    case 'C': {
        test_cards = true;
        return true;
    }

    case 'j': {
        no_javascript = true;
        return true;
    }

    case 'c': {
        config.include_checkboxes = true;
        return true;
    }
   
    }

    return false;
}

bool parvalue_to_bool(const std::string & parvalue) {
    if (parvalue == "true") {
        return true;
    } else {
        return false;
    }
}

time_t set_t_max_days(const std::string & parvalue) {
    int numdays = std::atoi(parvalue.c_str());
    if (numdays <= 0) {
        standard_exit_error(exit_bad_config_value, "Invalid number of days in configuration file.", __func__);
    }
    return ActualTime() + numdays*24*60*60;
}

/// Configure configurable parameters.
bool fzgh_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(num_to_show, "num_to_show", parlabel, parvalue_to_num_to_show(parvalue));
    CONFIG_TEST_AND_SET_PAR(excerpt_length, "excerpt_length", parlabel, atoi(parvalue.c_str()));
    CONFIG_TEST_AND_SET_PAR(rendered_out_path, "rendered_out_path", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(embeddable, "embeddable", parlabel, parvalue_to_bool(parvalue));
    CONFIG_TEST_AND_SET_PAR(outputformat, "outputformat", parlabel, parse_output_format(parvalue));
    CONFIG_TEST_AND_SET_PAR(t_max, "t_max", parlabel, time_stamp_time(parvalue));
    CONFIG_TEST_AND_SET_PAR(t_max, "num_days", parlabel, set_t_max_days(parvalue));
    CONFIG_TEST_AND_SET_PAR(show_still_required, "show_still_required", parlabel, parvalue_to_bool(parvalue));
    CONFIG_TEST_AND_SET_PAR(interpret_text, "interpret_text", parlabel, config_parse_text_interpretation(parvalue));
    CONFIG_TEST_AND_SET_PAR(show_current_time, "show_current_time", parlabel, parvalue_to_bool(parvalue));
    CONFIG_TEST_AND_SET_PAR(include_daysummary, "include_daysummary", parlabel, parvalue_to_bool(parvalue));
    CONFIG_TEST_AND_SET_PAR(timezone_offset_hours, "timezone_offset_hours", parlabel, atoi(parvalue.c_str()));
    CONFIG_TEST_AND_SET_PAR(tzadjust_day_separators, "tzadjust_day_separators", parlabel, parvalue_to_bool(parvalue));
    CONFIG_TEST_AND_SET_PAR(show_tzadjust, "show_tzadjust", parlabel, parvalue_to_bool(parvalue));
    CONFIG_TEST_AND_SET_PAR(max_do_links, "max_do_links", parlabel, atoi(parvalue.c_str()));
    CONFIG_TEST_AND_SET_PAR(include_checkboxes, "include_checkboxes", parlabel, parvalue_to_bool(parvalue));
    //CONFIG_TEST_AND_SET_FLAG(example_flagenablefunc, example_flagdisablefunc, "exampleflag", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}


/**
 * Initialize configuration parameters.
 * Call this at the top of main().
 * 
 * @param argc command line parameters count forwarded from main().
 * @param argv command line parameters array forwarded from main().
 */
void fzgraphhtml::init_top(int argc, char *argv[]) {
    ERRTRACE;

    // *** add any initialization here that has to happen before standard initialization
    init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    // *** add any initialization here that has to happen once in main(), for the derived class
    if (config.t_max < 0) {
        config.t_max = ActualTime() + (100*seconds_per_day); // the default is 100 days
    }
}

Graph & fzgraphhtml::graph() {
    ERRTRACE;
    if (!graphmemman.get_Graph(graph_ptr)) {
        standard_exit_error(exit_resident_graph_missing, "Memory resident Graph not found.", __func__);
    }
    return *graph_ptr;
}


void test_other_graph_info(Graph & graph) {

    //VERYVERBOSEOUT(List_Topics(graph, "\n"));

    VERYVERBOSEOUT('\n'+Nodes_statistics_string(Nodes_statistics(graph)));

    VERYVERBOSEOUT("\nNumber of Edges with non-zero edge data = "+std::to_string(Edges_with_data(graph))+'\n');

}

void get_node_info() {
    ERRTRACE;

    auto [node_ptr, graph_ptr] = find_Node_by_idstr(fzgh.node_idstr, fzgh.graph_ptr);

    if (graph_ptr) {

        VERYVERBOSEOUT(Graph_Info_str(*graph_ptr));

        test_other_graph_info(*graph_ptr);

    } else {
        standard_exit_error(exit_resident_graph_missing, "Unable to access memory-resident Graph.", __func__);
    }

    ERRHERE(".render");
    if (node_ptr) {

        std::string rendered_node_data(render_Node_data(*graph_ptr, *node_ptr));

        if (fzgh.config.rendered_out_path == "STDOUT") {
            FZOUT(rendered_node_data);
        } else {
            if (!string_to_file(fzgh.config.rendered_out_path, rendered_node_data))
                standard_exit_error(exit_file_error, "Unable to write rendered page to file.", __func__);
        }

    } else {
        standard_exit_error(exit_bad_request_data, "Invalid Node ID: "+fzgh.node_idstr, __func__);
    }

}

void get_topic_info() {
    ERRTRACE;

    if (fzgh.list_name == "?") {
        fzgh.flowcontrol = flow_topics;
    } else {
        fzgh.flowcontrol = flow_topic_nodes;
        if ((fzgh.list_name[0] >= '0') && (fzgh.list_name[0] <= '9')) {
            fzgh.topic_id = std::atoi(fzgh.list_name.c_str());
            if (!fzgh.graph().find_Topic_by_id(fzgh.topic_id)) {
                standard_exit_error(exit_bad_request_data, "Invalid Topic ID: "+std::to_string(fzgh.topic_id), __func__);
            }
        } else {
            Topic * topic_ptr = fzgh.graph().find_Topic_by_tag(fzgh.list_name);
            if (!topic_ptr) {
                standard_exit_error(exit_bad_request_data, "Invalid Topic tag: "+fzgh.list_name, __func__);
            }
            fzgh.topic_id = topic_ptr->get_id();
        }
    }

    switch (fzgh.flowcontrol) {

    case flow_topics: {
        render_topics();
        break;
    }

    case flow_topic_nodes: {
        render_topic_nodes();
        break;
    }

    default: {
        fzgh.print_usage();
    }

    }
}

int main(int argc, char *argv[]) {
    ERRTRACE;

    fzgh.init_top(argc, argv);

    if (fzgh.update_shortlist) {
        NNLreq_update_shortlist(fzgh.graph().get_server_IPaddr(), fzgh.graph().get_server_port());
    }
    fzgh.replacements.emplace_back(fzgh.graph().get_server_full_address()); // [fzserverpq_address]

    switch (fzgh.flowcontrol) {

    case flow_node: {
        get_node_info();
        break;
    }

    case flow_node_edit: {
        render_node_edit();
        break;
    }

    case flow_incomplete: {
        render_incomplete_nodes();
        break;
    }

    case flow_incomplete_with_repeats: {
        render_incomplete_nodes_with_repeats();
        break;
    }

    case flow_named_list: {
        return standard_exit(render_named_node_list(), "Named Node List rendered.", exit_general_error, "Unable to render Named Node List.", __func__);
    }

    case flow_topics: {
        get_topic_info();
        break;
    }

    default: {
        fzgh.print_usage();
    }

    }

    return standard.completed_ok();
}
