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
#include "Graphtypes.hpp"
#include "Logtypes.hpp"

// local
#include "log2tl.hpp"

// This can be specified in the Makefile. If it is not then the macro
// is set to the empty string, which leads to initialization with time
// stamp in /tmp/graph2dil-<time-stamp>.
#ifndef GRAPH2DIL_OUTPUT_DIR
    #define GRAPH2DIL_OUTPUT_DIR ""
#endif // GRAPH2DIL_OUTPUT_DIR

using namespace fz;

enum flow_options {
    flow_unknown = 0, /// no recognized request
    flow_log2TL = 1,  /// request: convert Log to TL files
    flow_NUMoptions
};

struct graph2dil: public formalizer_standard_program {

    std::string DILTLdirectory = GRAPH2DIL_OUTPUT_DIR; /// location for converted output files
    std::string DILTLindex = GRAPH2DIL_OUTPUT_DIR "/../graph2dil-lists.html";
    std::vector<std::string> cmdargs; /// copy of command line arguments

    Graph_access ga;

    flow_options flowcontrol;

    graph2dil(): ga(add_option_args,add_usage_top), flowcontrol(flow_unknown) {
        add_option_args += "L";
        add_usage_top += " [-L]";
    }

    virtual void usage_hook() {
        ga.usage_hook();
        FZOUT("    -L convert Log to Task Log files\n");
    }

    virtual bool options_hook(char c, std::string cargs) {
        if (ga.options_hook(c,cargs))
            return true;

        switch (c) {

        case 'n':
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

    }

} g2d;

/**
 * Program flow: Handle request to convert Log to Task Log Files.
 */
bool flow_convert_Log2TL() {
    ERRHERE(".prep");
    if (!std::filesystem::create_directories(g2d.DILTLdirectory)) {
        FZERR("\nUnable to create the output directory "+g2d.DILTLdirectory+".\n");
        exit(exit_general_error);
    }

    ERRHERE(".loadGraph");
    std::unique_ptr<Graph> graph = g2d.ga.request_Graph_copy();

    ERRHERE(".loadLog");
    Log log;
    //*** LOAD THE LOG
    FZOUT("\n*** Log loading is still missing! There is no Log in the database yet.\n\n");

    ERRHERE(".goLog2TL");
    Log2TL_conv_params params;
    params.TLdirectory = g2d.DILTLdirectory;
    params.IndexPath = g2d.DILTLindex;
    params.o = &std::cout;
    //params.from_idx = from_section;
    //params.to_idx = to_section;    
    if (!interactive_Log2TL_conversion(*(graph.get()), log, params)) {
        FZERR("\nNeed a database account to proceed. Defaults to $USER.\n");
        exit(exit_general_error);
    }
    
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

    //*** THIS IS JUST A STUB!

    default: {
        g2d.print_usage();
    }

    }

    ERRHERE(".exitok");
    return g2d.completed_ok();
}
