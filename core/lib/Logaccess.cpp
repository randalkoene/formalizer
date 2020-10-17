// Copyright 2020 Randal A. Koene
// License TBD

// std

// core
#include "error.hpp"
#include "standard.hpp"
#include "Logpostgres.hpp"
#include "Logaccess.hpp"

namespace fz {

/**
 * Query the Log to find most recent entry points.
 * 
 * The results are returned in an `entry_data` structure, which includes
 * information about the most recent Log chunk and about the most recent
 * Log entry. It also makes the in-memory Graph available via pointer.
 * 
 * @param[in] pa A valid initialization object for database access.
 * @param[out] edata A convenient structure for the related data.
 */
void get_newest_Log_data(Postgres_access & pa, entry_data & edata) {

    if (!edata.log_ptr) { // make an empty one if it does not exist yet
        edata.log_ptr = std::make_unique<Log>();
    }

    if (!load_last_chunk_and_entry_pq(*(edata.log_ptr.get()), pa)) {
        std::string errstr("Unable to read the newest Log chunk");
        ADDERROR(__func__, errstr);
        VERBOSEERR(errstr+'\n');
        standard.exit(exit_database_error);
    }
    
    edata.c_newest = edata.log_ptr->get_newest_Chunk();
    if (edata.c_newest) {
        edata.newest_chunk_t = edata.log_ptr->newest_chunk_t();
        edata.is_open = edata.c_newest->is_open();
    }

    edata.e_newest = edata.log_ptr->get_newest_Entry();
    if (edata.e_newest) {
        edata.newest_minor_id = edata.e_newest->get_minor_id();
    }
}

} // namespace fz
