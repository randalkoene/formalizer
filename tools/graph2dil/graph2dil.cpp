// Copyright 2020 Randal A. Koene
// License TBD

/**
 * graph2dil is a backward compatibility conversion tool from Graph, Node, Edge and Log
 * database stored data structures to HTML-style DIL Files and Detailed-Items-by-ID
 * format of DIL_entry content.
 * 
 */

#include <filesystem>
#include <iostream>

#include "error.hpp"
#include "Graphtypes.hpp"
#include "Logtypes.hpp"
#include "log2tl.hpp"

using namespace fz;

enum exit_status_code { exit_ok,
                        exit_general_error,
                        exit_database_error,
                        exit_unknown_option,
                        exit_cancel,
                        exit_conversion_error,
                        exit_DIL_error };

std::string DILTLdirectory;

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

void initialize() {
    DILTLdirectory = "/tmp/graph2dil-"+TimeStampYmdHM(ActualTime()); // default location for converted output files
}

int main(int argc, char *argv[]) {
    initialize();

    //*** THIS IS JUST A STUB!

    if (!std::filesystem::create_directories(DILTLdirectory)) {
        std::cerr << "\nUnable to create the output directory "+DILTLdirectory+".\n";
        Exit_Now(exit_general_error);
    }

    Graph graph;
    //*** LOAD THE GRAPH

    Log log;
    //*** LOAD THE LOG

    if (!interactive_Log2TL_conversion(graph, log, DILTLdirectory)) {
        std::cerr << "\nNeed a database account to proceed. Defaults to $USER.\n";
        Exit_Now(exit_general_error);
    }
         
    Exit_Now(exit_ok);
}
