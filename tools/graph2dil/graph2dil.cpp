// Copyright 2020 Randal A. Koene
// License TBD

/**
 * graph2dil is a backward compatibility conversion tool from Graph, Node, Edge and Log
 * database stored data structures to HTML-style DIL Files and Detailed-Items-by-ID
 * format of DIL_entry content.
 * 
 */

#define FORMALIZER_MODULE_ID "Formalizer:Conversion:DIL2Graph"

#include <filesystem>
#include <ostream>
#include <iostream>

#include "error.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "log2tl.hpp"

// This can be specified in the Makefile. If it is not then the macro
// is set to the empty string, which leads to initialization with time
// stamp in /tmp/graph2dil-<time-stamp>.
#ifndef GRAPH2DIL_OUTPUT_DIR
    #define GRAPH2DIL_OUTPUT_DIR ""
#endif // GRAPH2DIL_OUTPUT_DIR

using namespace fz;

struct graph2dil: public formalizer_standard_program {

    std::string DILTLdirectory = GRAPH2DIL_OUTPUT_DIR; /// location for converted output files
    std::string DILTLindex = GRAPH2DIL_OUTPUT_DIR "/../graph2dil-lists.html";
    std::vector<std::string> cmdargs; /// copy of command line arguments

    Graph_access ga;

    graph2dil(): ga(add_option_args,add_usage_top) {

    }

    virtual void usage_hook() {
        ga.usage_hook();
    }

    virtual bool options_hook(char c, char * cargs) {
        if (ga.options_hook(c,cargs))
            return true;

        /*
        switch (c) {

        }
        */

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
        init(argc, argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);
    
        for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i];

        if (DILTLdirectory.empty())
            DILTLdirectory = "/tmp/graph2dil-"+TimeStampYmdHM(ActualTime());

    }

} g2d;

int main(int argc, char *argv[]) {
    ERRHERE(".init");
    g2d.init_top(argc,argv);

    //*** THIS IS JUST A STUB!

    ERRHERE(".prep");
    if (!std::filesystem::create_directories(g2d.DILTLdirectory)) {
        FZERR("\nUnable to create the output directory "+g2d.DILTLdirectory+".\n");
        exit(exit_general_error);
    }

    ERRHERE(".loadGraph");
    std::unique_ptr<Graph> graph = g2d.ga.request_Graph_copy();
    //*** LOAD THE GRAPH

    ERRHERE(".loadLog");
    Log log;
    //*** LOAD THE LOG

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
         
    ERRHERE(".exitok");
    g2d.completed_ok();
}
