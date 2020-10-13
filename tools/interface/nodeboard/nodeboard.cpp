// Copyright 2020 Randal A. Koene
// License TBD

/**
 * nodeboard is a simple prototype for casting Nodes onto dashboard-like web pages
 * in the form of cards.
 * 
 * For more about this, see the Trello card at https://trello.com/c/w2XnEQcc
 */

#define FORMALIZER_MODULE_ID "Formalizer:Visualization:Nodes:HTMLcards"

#include <iostream>

#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"
#include "Graphaccess.hpp"
#include "Graphpostgres.hpp"
#include "nbrender.hpp"

using namespace fz;

struct nodeboard: public formalizer_standard_program {
    Graph_access ga;

    nodeboard(): formalizer_standard_program(false), ga(*this, add_option_args, add_usage_top) {
        //add_option_args += "n:";
        //add_usage_top += " [-n <Node-ID>]";
    }

    virtual void usage_hook() {
        ga.usage_hook();
    }

    virtual bool options_hook(char c, std::string cargs) {

        if (ga.options_hook(c,cargs))
            return true;

        /*
        switch (c) {

        }
        */

       return false;
    }
} nb;

int main(int argc, char *argv[]) {
    nb.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    FZOUT("\nThis is a test. Let's go find the Graph, so that we have some Nodes to work with.\n\n");
    key_pause();

    //std::unique_ptr<Graph> graph = nb.ga.request_Graph_copy();
    Graph * graph = graphmemman.find_Graph_in_shared_memory();
    if (!graph) {
        ADDERROR(__func__, "Memory resident Graph not found");
        FZERR("Memory resident Graph not found.\n");
        standard.exit(exit_general_error);
    }

    FZOUT("\nThe Graph has "+std::to_string(graph->num_Nodes())+" Nodes.\n\n");

    if (!node_board_render(*graph)) {
        ADDERROR(__func__,"unable to render node board");
        standard.exit(exit_general_error);
    }

    return standard.completed_ok();
}
