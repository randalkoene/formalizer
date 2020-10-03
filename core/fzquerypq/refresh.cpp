// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Functions that handle refresh requests made with `fzquerypq`.
 * 
 */

// corelib
#include "error.hpp"
#include "standard.hpp"
//#include "general.hpp"
//#include "Graphtypes.hpp"
#include "Logpostgres.hpp"

// local
#include "fzquerypq.hpp"
#include "refresh.hpp"

using namespace fz;

void refresh_Node_histories_cache_table() {
    ERRTRACE;
    VERBOSEOUT("Refreshing Node histories cache table...\n");
    if (!refresh_Node_history_cache_pq(fzq.ga)) {
        ADDERROR(__func__, "Unable to refresh Node histories cache table.");
        VERBOSEERR("\nUnable to refresh Node histories cache table.\n\n");
        return;
    }
    VERBOSEOUT("Done.\n");
}
