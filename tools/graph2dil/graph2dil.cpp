// Copyright 2020 Randal A. Koene
// License TBD

/**
 * graph2dil is a backward compatibility conversion tool from Graph, Node, Edge and Log
 * database stored data structures to HTML-style DIL Files and Detailed-Items-by-ID
 * format of DIL_entry content.
 * 
 */

#include <filesystem>
#include <ostream>
#include <iostream>

#include "error.hpp"
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

enum exit_status_code { exit_ok,
                        exit_general_error,
                        exit_database_error,
                        exit_unknown_option,
                        exit_cancel,
                        exit_conversion_error,
                        exit_DIL_error };

/**
 * Closing and clean-up actions when exiting the program.
 * 
 * Note that the exit status here needs to be an integer rather than
 * the enumerated exit_status_code type, because it might also
 * become linked into dil2al exit status.
 * 
 * @param status exit status to return to the program caller.
 */
void Exit_Now(int status) {
    ERRWARN_SUMMARY(std::cout);
    Clean_Exit(status);
}

void key_pause() {
    std::cout << "...Presse ENTER to continue (or CTRL+C to exit).\n";
    std::string enterstr;
    std::getline(std::cin, enterstr);
}

struct {
    std::string server_long_id = "Formalizer:Conversion:DIL2Graph v" __VERSION_HPP " (core v" __COREVERSION_HPP ")"; /// Formalizer component identifier
    std::string runnablename; /// the running program name
    std::string DILTLdirectory = GRAPH2DIL_OUTPUT_DIR; /// location for converted output files
    std::string DILTLindex = GRAPH2DIL_OUTPUT_DIR "/../graph2dil-lists.html";
    std::vector<std::string> cmdargs; /// copy of command line arguments

    /**
     * Initialize configuration parameters.
     * Call this at the top of main().
     * 
     * @param argc command line parameters count forwarded from main().
     * @param argv command line parameters array forwarded from main().
     */
    void init(int argc, char *argv[]) {
        for (int i = 0; i < argc; ++i) cmdargs[i] = argv[i];

        runnablename = cmdargs[0];
        auto n = runnablename.find_last_of('/');
        if (n!=std::string::npos)
            runnablename.erase(0,n+1);

        if (DILTLdirectory.empty())
            DILTLdirectory = "/tmp/graph2dil-"+TimeStampYmdHM(ActualTime());

    }

} configpars;

int main(int argc, char *argv[]) {
    ERRHERE(".init");
    configpars.init(argc,argv);

    //*** THIS IS JUST A STUB!

    ERRHERE(".prep");
    if (!std::filesystem::create_directories(configpars.DILTLdirectory)) {
        std::cerr << "\nUnable to create the output directory "+configpars.DILTLdirectory+".\n";
        Exit_Now(exit_general_error);
    }

    ERRHERE(".loadGraph");
    Graph graph;
    //*** LOAD THE GRAPH

    ERRHERE(".loadLog");
    Log log;
    //*** LOAD THE LOG

    ERRHERE(".goLog2TL");
    Log2TL_conv_params params;
    params.TLdirectory = configpars.DILTLdirectory;
    params.IndexPath = configpars.DILTLindex;
    params.o = &std::cout;
    //params.from_idx = from_section;
    //params.to_idx = to_section;    
    if (!interactive_Log2TL_conversion(graph, log, params)) {
        std::cerr << "\nNeed a database account to proceed. Defaults to $USER.\n";
        Exit_Now(exit_general_error);
    }
         
    ERRHERE(".exitok");
    std::cout << configpars.runnablename << " completed.\n";

    Exit_Now(exit_ok);
}
