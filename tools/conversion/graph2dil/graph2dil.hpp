// Copyright 2020 Randal A. Koene
// License TBD

/** @file graph2dil.hpp
 * This header file is used for declarations specific to the graph2dil tool.
 * 
 * Functions and classes available here are typically useful to create the data structures
 * needed when producing DIL Hierarchy compatible output from Graph data.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPH2DIL_HPP.
 */

#ifndef __GRAPH2DIL_HPP
#include "version.hpp"
#define __GRAPH2DIL_HPP (__VERSION_HPP)

// std
#include <memory>
#include <map>
#include <vector>

// core
#include "config.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"


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

//typedef std::map<Topic*, std::unique_ptr<Node_Index>> Node_Index_by_Topic; //*** by Index-ID is faster
typedef std::vector<std::unique_ptr<Node_Index>> Node_Index_by_Topic;

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

typedef std::map<template_id_enum,std::string> graph2dil_templates;

class g2d_configurable : public configurable {
public:
    g2d_configurable(formalizer_standard_program &fsp) : configurable("graph2dil", fsp) {}
    bool set_parameter(const std::string &parlabel, const std::string &parvalue);

    std::string DILTLdirectory =  GRAPH2DIL_OUTPUT_DIR; /// location for converted output files
    bool use_cached_histories = false; ///< If True, use cached Node histories table from database, otherwise generate Node_histories.
};

struct graph2dil: public formalizer_standard_program {

    g2d_configurable config;

    std::string DILTLindex; // initialized in init_top() after config and args have been processed
    std::vector<std::string> cmdargs; /// copy of command line arguments

    Graph_access ga;

    flow_options flowcontrol;

    render_environment env;
    graph2dil_templates templates;

    Graph * graph;
    std::unique_ptr<Log> log;
    Node_histories histories;

    graph2dil();

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

extern graph2dil g2d;

#endif // __GRAPH2DIL_HPP
