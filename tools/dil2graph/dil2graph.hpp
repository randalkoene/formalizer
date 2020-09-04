// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the dil2graph tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __DIL2GRAPH_HPP.
 */

#ifndef __DIL2GRAPH_HPP
#include "version.hpp"
#define __DIL2GRAPH_HPP (__VERSION_HPP)

// dil2al compatibility
#include "dil2al_minimal.hpp"

// core
#include "standard.hpp"
#include "dilaccess.hpp"

// local
#include "logtest.hpp"
#include "tl2log.hpp"

using namespace fz;

struct ConversionMetrics {
    int nullnodes;
    int skippedentries;
    int duplicates;
    int unknownnodeerror;
    int notopics;
    int nulledges;
    int duplicateedges;
    int selfconnections;
    int missingdepnode;
    int missingsupnode;
    int unknownedgeerror;
    int topicsanskeyrel;
    ConversionMetrics() : nullnodes(0), skippedentries(0), duplicates(0), unknownnodeerror(0), notopics(0), nulledges(0), duplicateedges(0), selfconnections(0), missingdepnode(0), missingsupnode(0), unknownedgeerror(0), topicsanskeyrel(0) {}
    int conversion_problems_sum() { // These can be problematic.
        return nullnodes + skippedentries + duplicates + unknownnodeerror + notopics + nulledges + duplicateedges + missingdepnode + missingsupnode + unknownedgeerror;
    }
    int conversion_metrics_sum() { // This is the simple sum of counts, problematic or not.
        return conversion_problems_sum() + selfconnections + topicsanskeyrel;
    }
};

std::string convert_DIL_Topics_file_to_tag(DIL_Topical_List &diltopic);

int convert_DIL_Topics_to_topics(Topic_Tags &topics, Node &node, DIL_entry &entry);

time_t get_Node_Target_Date(DIL_entry &e);

td_property get_Node_tdproperty(DIL_entry &e);

td_pattern get_Node_tdpattern(DIL_entry &e);

Node *convert_DIL_entry_to_Node(DIL_entry &e, Graph &graph, ConversionMetrics &convmet);

Edge *convert_DIL_Superior_to_Edge(DIL_entry &depentry, DIL_entry &supentry, DIL_Superiors &dilsup, Graph &graph, ConversionMetrics &convmet);

std::vector<Topic_Keyword> get_DIL_Topics_File_KeyRels(std::string dilfilepath);

unsigned int collect_topic_keyword_relevance_pairs(Topic *topic);

std::vector<int> detect_DIL_Topics_Symlinks(const std::vector<std::string> &dilfilepaths, int &num);

std::vector<int> detect_DIL_Topics_Symlinks(const std::vector<std::string> &dilfilepaths, int &num);

Graph *convert_DIL_to_Graph(Detailed_Items_List *dil, ConversionMetrics &convmet);

void Exit_Now(int status); // needed in dil2al_minimal.cpp and dil2al linked object files

enum flow_options {
    flow_unknown = 0,  /// no recognized request
    flow_everything = 1, /// load and convert DIL hierarchy to Graph, load and convert Task Log to Log
    flow_load_only = 2,  /// Graph loading test only
    flow_dil_only = 3,   /// load and convert DIL hierarchy to Graph
    flow_tl_only = 4     /// load and convert Task Log to Log
};

struct dil2graph: public formalizer_standard_program {

    //std::vector<std::string> cmdargs; /// copy of command line arguments
    //output_format_specifier output_format; /// the format used to deliver query results

    unsigned long from_section;
    unsigned long to_section;

    std::string node_idstr;

    Graph_access ga;

    flow_options flowcontrol;

    dil2graph() : from_section(0), to_section(9999999), ga(add_option_args, add_usage_top), flowcontrol(flow_everything) {
        COMPILEDPING(std::cout, "PING-dil2graph().1\n");
        add_option_args += "LDTmo:1:2:";
        add_usage_top += " [-m] [-L|-D|-T] [-o <testfile>] [-1 <num1>] [-2 <num2>]";
    }

    virtual void usage_hook() {
        ga.usage_hook();
        FZOUT("    -m manual decisions (no automatic fixes)\n");
        FZOUT("    -L load only (no conversion and storage)\n");
        FZOUT("    -D DIL hierarchy conversion only\n");
        FZOUT("    -T Task Log conversion only\n");
        FZOUT("    -o specify path of test output file\n");
        FZOUT("       (default: "+testfilepath+")\n");
        FZOUT("    -1 1st section to reconstruct is <num1>\n");
        FZOUT("    -2 last section to reconstruct is <num2>\n");
    }

    virtual bool options_hook(char c, std::string cargs) {
        if (ga.options_hook(c,cargs))
            return true;

        switch (c) {

        case 'm':
            manual_decisions = true; // *** this variable is still outside of a struct (see tl2log.hpp/cpp)
            return true;
        
        case 'L':
            flowcontrol = flow_load_only;
            return true;

        case 'D':
            flowcontrol = flow_dil_only;
            return true;

        case 'T':
            flowcontrol = flow_tl_only;
            return true;

        case 'o':
            testfilepath = cargs;
            return true;

        case '1':
            from_section = std::atoi(cargs.c_str());
            return true;

        case '2':
            to_section = std::atoi(cargs.c_str());
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

        add_to_exit_stack(&exit_postop); // include exit steps needed for dil2al code
        add_to_exit_stack(&exit_report);
    }

};

extern dil2graph d2g;

#endif // __DIL2GRAPH_HPP
