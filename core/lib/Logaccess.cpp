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
 * This function sets the following `entry_data` structure variables:
 * 
 *   log_ptr           Unique_ptr receives and owns a Log object.
 *   c_newest          Receives pointer to most recent Log_chunk object.
 *                     It the Log is completely empty (a new install)
 *                     then this is nullptr.
 *   c_newest_chunk_t  Set to open time (==chunk ID) of most recent Log
 *                     chunk, or `RTt_unspecified` if there are none.
 *   is_open           True if the most recent Log chunk is open, false
 *                     otherwise (including when the Log is empty).
 *   e_newest          Receives pointer to the most recent Log_entry
 *                     object in the Log chunk, nullptr if there is none,
 *                     which is true both if the Log chunk is empty and
 *                     if the Log is empty.
 *   newest_minor_id   Set to the minor ID of the entry at e_newest, or
 *                     0 if there is none.
 * 
 * Note that the behavior of the call to `Log::get_newest_Entry()` that
 * sets e_newest is different here than when called on other Log
 * intervals. Here, only the last Log chunk is loaded from the database,
 * so that there are no entries if that chunk has none. Under other
 * circumstances it would be possible that an empty most recent Log
 * chunk would return as newest Log entry an object for the last entry
 * in the preceding Log chunk.
 * 
 * @param[in] pa A valid initialization object for database access.
 * @param[out] edata A convenient structure for the related data.
 */
void get_newest_Log_data(Postgres_access & pa, entry_data & edata) {

    if (!edata.log_ptr) { // make an empty one if it does not exist yet
        edata.log_ptr = std::make_unique<Log>();
    }

    if (!load_last_chunk_and_entry_pq(*(edata.log_ptr.get()), pa)) {
        standard_exit_error(exit_database_error, "Unable to read the newest Log chunk", __func__);
    }
    
    edata.c_newest = edata.log_ptr->get_newest_Chunk();
    if (edata.c_newest) {
        edata.newest_chunk_t = edata.log_ptr->newest_chunk_t();
        edata.is_open = edata.c_newest->is_open();
    } else {
        edata.newest_chunk_t = RTt_unspecified;
        edata.is_open = false;
    }

    edata.e_newest = edata.log_ptr->get_newest_Entry();
    if (edata.e_newest) {
        edata.newest_minor_id = edata.e_newest->get_minor_id();
    } else {
        edata.newest_minor_id = 0;
    }
}

} // namespace fz
