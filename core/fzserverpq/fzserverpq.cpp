// Copyright 2020 Randal A. Koene
// License TBD

/**
 * fzserverpq is the Postgres-compatible version of the C++ implementation of teh Formalizer data server.
 * 
 * For more information see:
 * https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.tarhfe395l5v
 * https://trello.com/c/S7SZUyeU
 * 
 */

#include <cstdio>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

// Formalizer core
#include "Graphpostgres.hpp"
#include "Graphtypes.hpp"
#include "error.hpp"
#include "general.hpp"

// Program specific
#include "fzserverpq.hpp"

namespace fz {

std::string dbname; /// This is initialized to $USER.

//----------------------------------------------------
// Definitions of functions declared in fzserverpq.hpp:
//----------------------------------------------------

} // namespace fz

//----------------------------------------------------
// Definitions of file-local scope functions:
//----------------------------------------------------

using namespace fz;

/**
 * Closing and clean-up actions when exiting the program.
 * 
 * @param status exit status to return to the program caller.
 */
void Exit_Now(int status) {
    ERRWARN_SUMMARY(std::cout);
    Clean_Exit(status);
}

void print_usage(std::string progname) {
    std::cout << "Usage: " << progname << " [-d <dbname>] [-L]\n\n"
              << "  Options:\n"
              << "    -d store resulting Graph in Postgres account <dbname>\n"
              << "       (default is $USER)\n"
              << "    -L load only (no conversion and storage)\n"
              << "\n";
}

bool load_only = false; /// Alternative call, merely to test database loading.

void process_commandline(int argc, char *argv[]) {
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "d:")) != EOF) {

        switch (c) {
        case 'd':
            dbname = optarg;
            break;

        default:
            print_usage(argv[0]);
            Exit_Now(0);
        }
    }
}

int main(int argc, char *argv[]) {
    ERRHERE(".1");

    char *username = std::getenv("USER");
    if (username)
        dbname = username;
    process_commandline(argc, argv);
    if (dbname.empty()) {
        ADDERROR(__func__, "Need a database account to proceed. Defaults to $USER.");
        Exit_Now(1);
    }

    std::cout << "Formalizer Graph Server:Postgres (C++ implementation) v" << version() << " starting.\n";
    std::cout << "Postgres database account selected: " << dbname << '\n';

    ERRHERE(".2");
    Graph graph;
    if (!load_Graph_pq(graph, dbname)) {
        ADDERROR(__func__, "Unable to load Graph from Postgres database.");
        Exit_Now(1);
    }

#define VERBOSE_INITIAL_LOAD
#ifdef VERBOSE_INITIAL_LOAD
    std::cout << "Graph data loaded:\n";
    std::cout << "  Number of Topics = " << graph.get_topics().get_topictags().size() << '\n';
    std::cout << "  Number of Nodes  = " << graph.num_Nodes() << '\n';
    std::cout << "  Number of Edges  = " << graph.num_Edges() << "\n\n";
#endif

    Exit_Now(0);

    return 0;
}
