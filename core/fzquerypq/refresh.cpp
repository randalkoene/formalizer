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
#include "Graphpostgres.hpp"
#include "Logpostgres.hpp"

// local
#include "fzquerypq.hpp"
#include "refresh.hpp"

using namespace fz;

const char * histories_refresh_note = R"NOTE(
Done.

Note:
  To enable web based access to Node histories, you may need to
  re-establish ownership/permissions by running
  `fzsetup -1 fzuser`.

)NOTE";

void refresh_Node_histories_cache_table() {
    ERRTRACE;
    VERBOSEOUT("Refreshing Node histories cache table...\n");
    if (!refresh_Node_history_cache_pq(fzq.ga)) {
        standard_error("Unable to refresh Node histories cache table." , __func__);
        return;
    }
    VERBOSEOUT(histories_refresh_note);
}

const char * nnl_refresh_note = R"NOTE(
Done.

Note:
  To enable web based access to Node histories, you may need to
  re-establish ownership/permissions by running
  `fzsetup -1 fzuser`.

)NOTE";

void refresh_Named_Node_Lists_cache_table() {
    ERRTRACE;
    VERBOSEOUT("Refreshing Named Node Lists cache table...\n");
    if (!Init_Named_Node_Lists_pq(fzq.ga.dbname(), fzq.ga.pq_schemaname())) {
        standard_error("Unable to refresh Named Node Lists cache table." , __func__);
        return;
    }
    VERBOSEOUT(nnl_refresh_note);
}
