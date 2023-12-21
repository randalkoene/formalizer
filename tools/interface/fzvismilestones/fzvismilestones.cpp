// Copyright 2023 Randal A. Koene
// License TBD

/**
 * fzvismilestones is a simple prototype for converting the Graph into
 * visualizable formats.
 */

#define FORMALIZER_MODULE_ID "Formalizer:Visualization:Nodes:Graphed"

#include <iostream>

#include "version.hpp"
#include "error.hpp"
#include "standard.hpp"
#include "Graphtypes.hpp"
//#include "Graphaccess.hpp"
//#include "Graphpostgres.hpp"
#include "visrender.hpp"

using namespace fz;

fzvismilestones vis;

bool convert_to_SIF() {
    FZOUT("\nThe Graph has "+std::to_string(vis.graph().num_Nodes())+" Nodes.\n\n");

    return standard_exit(vis.render_as_SIF(), "Graph converted to SIF file.\n", exit_general_error, "Unable to convert to SIF.", __func__);
}

bool convert_to_GraphML() {
    FZOUT("\nThe Graph has "+std::to_string(vis.graph().num_Nodes())+" Nodes.\n\n");

    return standard_exit(vis.render_as_GraphML(), "Graph converted to GraphML file.\n", exit_general_error, "Unable to convert to GraphML.", __func__);
}

bool convert_to_full_web_app() {
    FZOUT("\nThe Graph has "+std::to_string(vis.graph().num_Nodes())+" Nodes.\n\n");

    return standard_exit(vis.render_as_full_web_app(), "Graph converted to full Web app.\n", exit_general_error, "Unable to convert to full Web app.", __func__);
}

bool convert_to_single_view_web_page() {
    FZOUT("\nThe Graph has "+std::to_string(vis.graph().num_Nodes())+" Nodes.\n\n");

    return standard_exit(vis.render_as_single_view_web_page(), "Graph converted to single view web page.\n", exit_general_error, "Unable to convert to single view web page.", __func__);
}

int main(int argc, char *argv[]) {
    vis.init(argc,argv,version(),FORMALIZER_MODULE_ID,FORMALIZER_BASE_OUT_OSTREAM_PTR,FORMALIZER_BASE_ERR_OSTREAM_PTR);

    switch (vis.flowcontrol) {

        case flow_SIF: {
            convert_to_SIF();
            break;
        }

        case flow_GraphML: {
            convert_to_GraphML();
            break;
        }

        case flow_webapp: {
            convert_to_full_web_app();
            break;
        }

        case flow_webview: {
            convert_to_single_view_web_page();
            break;
        }

        default: {
            vis.print_usage();
        }

    }

    return standard.completed_ok();
}
