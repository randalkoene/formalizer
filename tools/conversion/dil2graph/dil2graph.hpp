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

#define INCLUDE_DIL2AL
#ifdef INCLUDE_DIL2AL
// dil2al compatibility
#include "dil2al_minimal.hpp"
#endif // INCLUDE_DIL2AL

// core
#include "standard.hpp"
#ifdef INCLUDE_DIL2AL
#include "dilaccess.hpp"
#endif // INCLUDE_DIL2AL
#include "Graphaccess.hpp"

// local
#ifdef INCLUDE_DIL2AL
#include "logtest.hpp"
#include "tl2log.hpp"
#endif // INCLUDE_DIL2AL

/// definte the following to confirm that any combination of fixed + unspecified target date becomes inherit
#define DOUBLE_CHECK_INHERIT

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

#ifdef INCLUDE_DIL2AL
std::string convert_DIL_Topics_file_to_tag(DIL_Topical_List &diltopic);

int convert_DIL_Topics_to_topics(Topic_Tags &topics, Node &node, DIL_entry &entry);

time_t get_Node_Target_Date(DIL_entry &e);

td_property get_Node_tdproperty(DIL_entry &e);

td_pattern get_Node_tdpattern(DIL_entry &e);

Node *convert_DIL_entry_to_Node(DIL_entry &e, Graph &graph, ConversionMetrics &convmet);

Edge *convert_DIL_Superior_to_Edge(DIL_entry &depentry, DIL_entry &supentry, DIL_Superiors &dilsup, Graph &graph, ConversionMetrics &convmet);

Topic_KeyRel_Vector get_DIL_Topics_File_KeyRels(std::string dilfilepath);

unsigned int collect_topic_keyword_relevance_pairs(Topic *topic);

Graph *convert_DIL_to_Graph(Detailed_Items_List *dil, ConversionMetrics &convmet);
#endif // INCLUDE_DIL2AL

std::vector<int> detect_DIL_Topics_Symlinks(const std::vector<std::string> &dilfilepaths, int &num);

void Exit_Now(int status); // needed in dil2al_minimal.cpp and dil2al linked object files

void node_pq_progress_func(unsigned long n, unsigned long ncount);

enum flow_options {
    flow_unknown = 0,  /// no recognized request
    flow_everything = 1, /// load and convert DIL hierarchy to Graph, load and convert Task Log to Log
    flow_load_only = 2,  /// Graph loading test only
    flow_dil_only = 3,   /// load and convert DIL hierarchy to Graph
    flow_tl_only = 4     /// load and convert Task Log to Log
};

class dil2graph: public formalizer_standard_program {
public:
    //std::vector<std::string> cmdargs; /// copy of command line arguments
    //output_format_specifier output_format; /// the format used to deliver query results

    unsigned long proc_from;
    unsigned long proc_to;
    unsigned long from_section;
    unsigned long to_section;

    std::string node_idstr;

    Graph_access ga;

    flow_options flowcontrol;

    bool TL_reconstruction_test;

    dil2graph();

    virtual void usage_hook();

    virtual bool options_hook(char c, std::string cargs);

    /**
     * Initialize configuration parameters.
     * Call this at the top of main().
     * 
     * @param argc command line parameters count forwarded from main().
     * @param argv command line parameters array forwarded from main().
     */
    void init_top(int argc, char *argv[]);

};

extern dil2graph d2g;

#endif // __DIL2GRAPH_HPP
