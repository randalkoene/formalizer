// Copyright 2020 Randal A. Koene
// License TBD

/** @file Logpostgres.hpp
 * This header file declares Log Postgres types for use with the Formalizer.
 * These define the authoritative version of the data structure for storage in PostgreSQL.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __LOGPOSTGRES_HPP.
 */

/**
 * The functions and classes in this header are used to prepare Log chunk, Log entry and
 * related data of a Log for storage in a Postgres database, and they are used to retrieve
 * that data from the Postgres database.
 * 
 * The data structure in C++ is defined in Logtypes.hpp. The data structure in
 * Postgres is defined here.
 * 
 * Functions are also provided to initialze Log data in Postgres by converting a
 * complete Log with all Chunks and Entries from another storage format to Postgres
 * format. At present, the only other format supported is the original HTML storage
 * format used in dil2al.
 * 
 * Even though this is C++, we use libpq here (not libpqxx) to communicate with the Postgres
 * database. The lipbq is the only standard library for Postgres and is perfectly usable
 * from C++.
 * 
 * This version is focused on simplicity and human readibility, not maximum speed. A
 * complete Log is intialized relatively rarely. Unless automated tools generate new
 * Chunks and Entries very quickly, writing speed should not be a major issue.
 * 
 */

#ifndef __LOGPOSTGRES_HPP
#include "coreversion.hpp"
#define __LOGPOSTGRES_HPP (__COREVERSION_HPP)

// core
#include "Logtypes.hpp"
#include "fzpostgres.hpp"

/**
 * On Ubuntu, to install the libpq libraries, including the libpq-fe.h header file,
 * do: sudo apt-get install libpq-dev
 * You may also have to add /usr/include or /usr/include/postgresql to the CPATH or
 * to the includes in the Makefile, e.g. -I/usr/include/postgresql.
 */
#include <libpq-fe.h>

namespace fz {

/**
 * Log Breakpoint fields:
 * - `pqlb_id`: a Log chunk ID identifies each Breakpoint.
 */
enum pq_LBfields { pqlb_id, _pqlb_NUM };

/**
 * Log chunk fields:
 * - `pqlc_id`: ID that also specifies the start time of a chunk.
 * - `pqlc_nid`: Node ID of the Node to which the chunk belongs.
 * - `pqlc_tclose`: Chunk close time (or infinity if the cuhnk is still open).
 */
enum pq_LCfields { pqlc_id, pqlc_nid, pqlc_tclose, _pqlc_NUM };

/**
 * Log entry fields:
 *  - `pqle_id`: ID that corresponds to a Log chunk ID, with an index position.
 *  - `pqle_nid`: possible Node ID when the entry does not belong to the same Node as the chunk.
 *  - `pqle_text`: Entry text content.
 */
enum pq_LEfields { pqle_id, pqle_nid, pqle_text, _pqle_NUM };

//bool create_Enum_Types_pq(const active_pq & apq);

bool create_Breakpoints_table_pq(const active_pq & apq);

bool create_Logchunks_table_pq(const active_pq & apq);

bool create_Logentries_table_pq(const active_pq & apq);

bool add_Breakpoint_pq(const active_pq & apq, const Log_chunk_ID_key & bptopid);

bool add_Logchunk_pq(const active_pq & apq, const Log_chunk & chunk);

std::string entry_minor_id_pq(unsigned int minor_id);

bool add_Logentry_pq(const active_pq & apq, const Log_entry & entry);

bool modify_Logentry_pq(const active_pq & apq, const Log_entry & entry);

bool store_Log_pq(const Log & log, Postgres_access & pa, void (*progressfunc)(unsigned long, unsigned long) = NULL);

/**
 * Append Entry to existing table in schema of PostgreSQL database.
 * 
 * @param entry A valid Log entry object.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log entry was successfully stored in the database.
 */
bool append_Log_entry_pq(const Log_entry & entry, Postgres_access & pa);

/**
 * Update Entry in existing table in schema of PostgreSQL database.
 * 
 * @param entry A valid Log entry object.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log entry was successfully updated in the database.
 */
bool update_Log_entry_pq(const Log_entry & entry, Postgres_access & pa);

/**
 * Close the Chunk specified, which must already exist within a table in
 * schema of PostgreSQL database.
 * 
 * @param chunk A valid Log chunk object with valid t_close time.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log chunk was successfully updated to closed status.
 */
bool close_Log_chunk_pq(const Log_chunk & chunk, Postgres_access & pa);

/**
 * Append Chunk to existing table in schema of PostgreSQL database.
 * 
 * Note: Please make sure that you close any open Log chunk before appending
 *       a new one!
 * 
 * @param chunk A valid Log chunk object.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log chunk was successfully stored in the database.
 */
bool append_Log_chunk_pq(const Log_chunk & chunk, Postgres_access & pa);

/**
 * Change the Node to which the Chunk specified belongs. The chunk must
 * already exist within a table in schema of PostgreSQL database.
 * 
 * Note:
 * - After changing Chunk ownership the Node histories cache should
 *   be refreshed.
 * 
 * @param chunk A valid Log chunk object with the updated node_id.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log chunk nid was successfully updated.
 */
bool modify_Log_chunk_nid_pq(const Log_chunk & chunk, Postgres_access & pa);

bool load_Log_pq(Log & log, Postgres_access & pa);

bool load_partial_Log_pq(Log & log, Postgres_access & pa, const Log_filter & filter);

/**
 * Load the last Log chunk and last Log entry in the table.
 * 
 * This function uses a `Log_filter` that specifies end-to-front reading
 * limited to 1 chunk and calls `load_partial_Log_pq()`.
 * 
 * @param log A Log for the Chunks and Entries, can be empty or may be added to.
 * @param pa Access object with database name and schema name.
 * @return True if the last chunk and last entry were successfully loaded from the database.
 */
bool load_last_chunk_and_entry_pq(Log & log, Postgres_access & pa);

/**
 * Store a Node_history object to a cache table in the database to
 * speed up generation of a Node-specific Log history.
 * 
 * @param nodehist A Node_history object.
 * @param pa Access object with valid database and schema identifiers.
 * @return True if cache table storage was successful.
 */
bool store_Node_history_pq(const Node_histories & nodehist, Postgres_access & pa);

/**
 * Refresh the Node history cache table.
 * 
 * @param pa Access object with valid database and schema identifiers.
 * @return True if the refresh was successful.
 */
bool refresh_Node_history_cache_pq(Postgres_access & pa);

/**
 * Load the cached Log history of a specific Node.
 * 
 * @param[in] apq Access object with database name and schema name.
 * @param[in] nkey A Node ID key.
 * @param[out] nodehist A Node_history object that receives the resulting data.
 */
bool load_Node_history_cache_entry_pq(active_pq & apq, const Node_ID_key & nkey, Node_history & nodehist);

/**
 * Load a Node_histories object from a complete cache table in the database.
 * 
 * @param[in] pa Access object with valid database and schema identifiers.
 * @param[out] nodehistories A Node_histories object.
 * @return True if cache table loading was successful.
 */
bool load_Node_history_cache_table_pq(Postgres_access & pa, Node_histories & nodehistories);

/**
 * A data types conversion helper class that can deliver the Postgres Breakpoints table
 * equivalent INSERT value expression for all data content in a Breakpoints.
 * 
 * This helper class does not modify the Breakpoints in any way.
 */
class Breakpoint_pq {
protected:
    const Log_chunk_ID_key* chunkkey; // pointer to an element of the Log_chunk_ID_key_deque
public:
    Breakpoint_pq(const Log_chunk_ID_key* _chunkkey): chunkkey(_chunkkey) {} // See Trello card about (const type)* pointers.

    std::string id_pqstr();
    std::string All_Breakpoint_Data_pqstr();
};

/**
 * A data types conversion helper class that can deliver the Postgres Log chunk table
 * equivalent INSERT value expression for all data content in a Log chunk.
 * 
 * This helper class does not modify the Log chunk in any way.
 */
class Logchunk_pq {
protected:
    const Log_chunk* chunk; // pointer to a (const Log chunk), i.e. *chunk is treated as constant
public:
    Logchunk_pq(const Log_chunk* _chunk): chunk(_chunk) {} // See Trello card about (const type)* pointers.

    std::string id_pqstr();
    std::string nid_pqstr();
    std::string tclose_pqstr();
    std::string All_Logchunk_Data_pqstr();
};

/**
 * A data types conversion helper class that can deliver the Postgres Log entries table
 * equivalent INSERT value expression for all data content in a Log entry.
 * 
 * This helper class does not modify the Log entry in any way.
 */
class Logentry_pq {
protected:
    const Log_entry* entry; // pointer to a (const Log_entry), i.e. *entry is treated as constant
public:
    Logentry_pq(const Log_entry* _entry): entry(_entry) {} // See Trello card about (const type)* pointers.

    std::string id_pqstr();
    std::string nid_pqstr();
    std::string text_pqstr();
    std::string All_Logentry_Data_pqstr();
};


} // namespace fz

#endif // __LOGPOSTGRES_HPP
