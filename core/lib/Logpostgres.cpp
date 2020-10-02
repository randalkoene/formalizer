// Copyright 2020 Randal A. Koene
// License TBD

#include "Logpostgres.hpp"

#include "error.hpp"
#include "general.hpp"
#include "Logtypes.hpp"

namespace fz {

/**
 * Notes about the Postgres Log layout:
 * 
 * - About the Log entry `minor_id`: Where Node elements are concerned, we made the assumption in the
 *   2.x data structure that there would never be more than 9 created in the same second. Hence, the
 *   minor_id required only 1 digit to store. The situation is different for Log entries. First of
 *   all, the major part of the ID is only YYYYmmddHHMM (without seconds). Secondly, it is entirely
 *   possible that a Log chunk could contain many more than 9 Log entries. Therefore, Log entry ID
 *   storage requires more digit space for the `minor_id`. In the current version of the data
 *   structure (see coreversion), we are assuming that, because the `minor_id` is encoded as a
 *   `uint8_t`, there can be a maximum of 255 Log entries in a Log chunk. The `minor_id` is given
 *   a 3 digit space in the `.logentries` Postgres table.
 * 
 */

std::string pq_LBlayout(
    "id timestamp (0) PRIMARY KEY" // `pqlb_id`
);

std::string pq_LClayout(
    "id timestamp (0) PRIMARY KEY," // `pqlc_id`
    "nid char(16),"                 // `pqlc_nid`
    "tclose timestamp (0)"          // `pqlc_tclose`
);

std::string pq_LElayout(
    "id char(16),"  // pqle_id (see notes at top)
    "nid char(16)," // pqle_nid
    "text text"     // pqle_text
);

//bool create_Enum_Types_pq(const active_pq & apq) {}

/**
 * Create a new database table for Breakpoints.
 * 
 * @param apq active database connection.
 * @return true if table was successfully created.
 */
bool create_Breakpoints_table_pq(const active_pq & apq) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    std::string pq_maketable("CREATE TABLE "+apq.pq_schemaname+".Breakpoints ("+pq_LBlayout+')');
    return simple_call_pq(apq.conn,pq_maketable);
}

/**
 * Create a new database table for Log chunks.
 * 
 * @param apq active database connection.
 * @return true if table was successfully created.
 */
bool create_Logchunks_table_pq(const active_pq & apq) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    std::string pq_maketable("CREATE TABLE "+apq.pq_schemaname+".Logchunks ("+pq_LClayout+')');
    return simple_call_pq(apq.conn,pq_maketable);
}

/**
 * Create a new database table for Log entries.
 * 
 * @param apq active database connection.
 * @return true if table was successfully created.
 */
bool create_Logentries_table_pq(const active_pq & apq) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    std::string pq_maketable("CREATE TABLE "+apq.pq_schemaname+".Logentries ("+pq_LElayout+')');
    return simple_call_pq(apq.conn,pq_maketable);
}

bool add_Breakpoint_pq(const active_pq & apq, const Log_chunk_ID_key & bptopid) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    Breakpoint_pq bpq(&bptopid);
    std::string tstr("INSERT INTO "+apq.pq_schemaname + ".Breakpoints VALUES "+ bpq.All_Breakpoint_Data_pqstr());
    return simple_call_pq(apq.conn,tstr);
}

bool add_Logchunk_pq(const active_pq & apq, const Log_chunk & chunk) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    Logchunk_pq lpq(&chunk);
    std::string tstr("INSERT INTO "+apq.pq_schemaname + ".Logchunks VALUES "+ lpq.All_Logchunk_Data_pqstr());
    return simple_call_pq(apq.conn,tstr);
}

bool add_Logentry_pq(const active_pq & apq, const Log_entry & entry) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    Logentry_pq epq(&entry);
    std::string tstr("INSERT INTO "+apq.pq_schemaname + ".Logentries VALUES "+ epq.All_Logentry_Data_pqstr());
    return simple_call_pq(apq.conn,tstr);
}

/**
 * Store all the Chunks and Entries of the Log in the PostgreSQL database.
 * 
 * @param log a Log containing all of the Chunks and Entries.
 * @param pa access object with database name and Formalizer schema name.
 * @param progress_func points to an optional progress indicator function.
 * @returns true if the Log was successfully stored in the database.
 */
bool store_Log_pq(const Log & log, Postgres_access & pa, void (*progressfunc)(unsigned long, unsigned long)) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_LOG_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".schema");
    if (!create_Formalizer_schema_pq(apq.conn, apq.pq_schemaname)) STORE_LOG_PQ_RETURN(false);

    //ERRHERE(".enum");
    //if (!create_Enum_Types_pq(conn, schemaname)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".bptable");
    if (!create_Breakpoints_table_pq(apq)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".bps");
    unsigned long n = log.num_Breakpoints();
    unsigned long ncount = 0;
    for (Log_chunk_ID_key_deque::size_type bpidx = 0; bpidx<n; ++bpidx) {
        Log_chunk_ID_key & bptopidkey = const_cast<Log *>(&log)->get_Breakpoint_first_chunk_id_key(bpidx);
        if (!add_Breakpoint_pq(apq, bptopidkey)) STORE_LOG_PQ_RETURN(false);
        ncount++;
        if (progressfunc) (*progressfunc)(n,ncount);
    }

    ERRHERE(".chunktable");
    if (!create_Logchunks_table_pq(apq)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".chunks");
    n = log.num_Chunks();
    ncount = 0;
    for (Log_chunk_ptr_deque::size_type cidx = 0; cidx<n; ++cidx) {
        Log_chunk * chunkptr = const_cast<Log *>(&log)->get_chunk(cidx); // *** slightly risky, untested pointer
        if (!add_Logchunk_pq(apq, *chunkptr)) STORE_LOG_PQ_RETURN(false);
        ncount++;
        if (progressfunc) (*progressfunc)(n,ncount);
    }

    ERRHERE(".entriestable");
    if (!create_Logentries_table_pq(apq)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".entries");
    n = log.num_Entries();
    ncount = 0;
    for (auto eit = const_cast<Log *>(&log)->get_Entries().begin(); eit != const_cast<Log *>(&log)->get_Entries().end(); ++eit) {
        Log_entry * entryptr = eit->second.get(); // *** slightly risky, untested pointer
        if (!add_Logentry_pq(apq, *entryptr)) STORE_LOG_PQ_RETURN(false);
        ncount++;
        if (progressfunc) (*progressfunc)(n,ncount);
    }

    STORE_LOG_PQ_RETURN(true);
}


// ======================================
// Definitions of class member functions:
// ======================================

/// Return the Breakpoint top chunk id as Postgres time stamp.
std::string Breakpoint_pq::id_pqstr() {
    return TimeStamp_pq(chunkkey->get_epoch_time()); // *** slightly risky, untested pointer
}

/// Return the Postgres VALUES set for all Breakpoint data between brackets.
std::string Breakpoint_pq::All_Breakpoint_Data_pqstr() {
    return "(" + id_pqstr() + ')';
}

/// Return the Log chunk id (tbegin) as Postgres time stamp.
std::string Logchunk_pq::id_pqstr() {
    return TimeStamp_pq(chunk->get_open_time()); // *** slightly risky, untested pointer
}

/// Return the Log chunk Node ID between apostrophes.
std::string Logchunk_pq::nid_pqstr() {
    return "'" + chunk->get_NodeID().str() + "'";
}

/// Return the Log chunk close time as Postgres time stamp.
std::string Logchunk_pq::tclose_pqstr() {
    return TimeStamp_pq(chunk->get_close_time()); // *** slightly risky, untested pointer
}

/// Return the Postgres VALUES set for all Log chunk data between brackets.
std::string Logchunk_pq::All_Logchunk_Data_pqstr() {
    return "(" + id_pqstr() + ',' +
           nid_pqstr() + ',' +
           tclose_pqstr() + ')';
}

/// Return the Log entry ID between apostrophes.
std::string Logentry_pq::id_pqstr() {
    return "'"+entry->get_id_str()+"'";
}

/// Return the Log entry Node ID between apostrophes.
std::string Logentry_pq::nid_pqstr() {
    return "'" + entry->get_nodeidkey().str() + "'";
}

/// Return the text between dollar-quoted tags that prevent any issues with characters in the text.
std::string Logentry_pq::text_pqstr() {
    // Using the Postgres dollar-quoted tag method means no need to escape any characters within the text!
    return "$txt$" + const_cast<Log_entry *>(entry)->get_entrytext() + "$txt$"; // *** slightly risky, untested pointer
}

/// Return the Postgres VALUES set for all Log entry data between brackets.
std::string Logentry_pq::All_Logentry_Data_pqstr() {
    return "(" + id_pqstr() + ',' +
           nid_pqstr() + ',' +
           text_pqstr() + ')';
}

/**
 * Load full Breakpoints table into Log::breakpoints.
 * 
 * We can only add Breakpoints by adding actual existing Log_chunk objects
 * (which is a good safety mechanism). This means, that this function can
 * really only be successfully called after calling `read_Chunks_pq()`.
 * 
 * @param apq data structure with active database connection pointer and schema name.
 * @param log a Log object, typically with an existing chunks list but empty Breakpoints list.
 */
bool read_Breakpoints_pq(active_pq & apq, Log & log) {
    ERRTRACE;
    if (!query_call_pq(apq.conn,"SELECT * FROM "+apq.pq_schemaname+".Breakpoints ORDER BY id",false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);

    PGresult *res;

    while ((res = PQgetResult(apq.conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<1) ERRRETURNFALSE(__func__,"not enough fields in Breakpoints table");

        //if (!get_Breakpoints_pq_field_numbers(res)) return false; //*** there is only one field at this time

        for (int r = 0; r < rows; ++r) {

            std::string tag = PQgetvalue(res, r, 0);
            time_t bp_t = epochtime_from_timestamp_pq(PQgetvalue(res, r, 0));
            if (bp_t<0)
                ERRRETURNFALSE(__func__,"stored Breakpoint has undefined top Log chunk start time");

            Log_chunk_ID_key chunkkey(bp_t);
            Log_chunk * chunk = log.get_chunk(chunkkey);
            if (!chunk)
                ERRRETURNFALSE(__func__,"stored Breakpoint refers to Log chunk not found in Log");

            log.get_Breakpoints().add_later_Breakpoint(*chunk); // append
        }

        PQclear(res);
    }

    return true;
}

const std::string pq_chunk_fieldnames[_pqlc_NUM] = {"id",
                                                   "nid",
                                                   "tclose"};

const std::string pq_entry_fieldnames[_pqle_NUM] = {"id",
                                                   "nid",
                                                   "text"};

unsigned int pq_chunk_field[_pqlc_NUM];
unsigned int pq_entry_field[_pqle_NUM];

/**
 * Retrieve field column numbers for chunks query to make sure the
 * correct field numbers are used. This is an extra safety measure
 * in case formats are changed in the future and in case of potential
 * database version mismatch.
 * 
 * This function updates the field numbers in `pq_chunk_field`.
 * The field names that this version assumes are in the variable
 * `pq_chunk_fieldnames`. The fields are enumerated with `pq_LCfields`.
 * 
 * @param res a valid pointer obtained by `PQgetResult()`.
 * @return true if all the field names were found.
 */
bool get_Chunk_pq_field_numbers(PGresult *res) {
    if (!res) return false;

    for (auto i = 0; i<_pqlc_NUM; i++) {
        if ((pq_chunk_field[i] = PQfnumber(res,pq_chunk_fieldnames[i].c_str())) < 0) {
            ERRRETURNFALSE(__func__,"field '"+pq_chunk_fieldnames[i]+"' not found in database chunks table");
        }
    }
    return true;
}

bool get_Entry_pq_field_numbers(PGresult *res) {
    if (!res) return false;

    for (auto i = 0; i<_pqle_NUM; i++) {
        if ((pq_entry_field[i] = PQfnumber(res,pq_entry_fieldnames[i].c_str())) < 0) {
            ERRRETURNFALSE(__func__,"field '"+pq_entry_fieldnames[i]+"' not found in database entries table");
        }
    }
    return true;
}

/**
 * Load full Chunks table into Log::chunks.
 * 
 * With optional WHERE statement.
 */
bool read_Chunks_pq(active_pq & apq, Log & log, std::string wherestr = "") {
    ERRTRACE;
    if (!query_call_pq(apq.conn,"SELECT * FROM "+apq.pq_schemaname+".Logchunks"+wherestr+" ORDER BY "+pq_chunk_fieldnames[pqlc_id],false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);
  
    PGresult *res;

    while ((res = PQgetResult(apq.conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<_pqlc_NUM) ERRRETURNFALSE(__func__,"not enough fields in chunks table");

        if (!get_Chunk_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            time_t chopen_t = epochtime_from_timestamp_pq(PQgetvalue(res, r, pq_chunk_field[pqlc_id]));
            if (chopen_t<0)
                ERRRETURNFALSE(__func__,"stored Chunk has undefined start time");

            try {
                Log_TimeStamp chunkstamp(chopen_t);

                std::string nidstr(PQgetvalue(res, r, pq_chunk_field[pqlc_nid]));
                try {
                    Node_ID nid(nidstr);

                    time_t chunkclose_t = epochtime_from_timestamp_pq(PQgetvalue(res, r, pq_chunk_field[pqlc_tclose])); // it might be open!

                    log.add_later_Chunk(chunkstamp,nid,chunkclose_t);
                } catch (ID_exception idexception) {
                    ERRRETURNFALSE(__func__,"Invalid Node ID ["+nidstr+"], "+idexception.what());
                }
            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__,"Invalid Chunk ID ["+TimeStampYmdHM(chopen_t)+"], "+idexception.what());
            }

        }

        PQclear(res);
    }

    return true;
}

/**
 * Load full Entries table into Log::entries.
 * 
 * We can only add Entries while referring to existing Log_chunks
 * (which is a good safety mechanism). This means, that this function can
 * really only be successfully called after calling `read_Chunks_pq()`.
 * 
 * EXPERIMENTING: With optional WHERE statement! Just in case this function can be reused
 * for load_Log_interval()!
 * 
 * @param apq data structure with active database connection pointer and schema name.
 * @param log a Log object, typically with an existing chunks list but empty Breakpoints list.
 * @param wherestr An optional WHERE string to constrain which records are retrieved.
 * @return true if successful.
 */
bool read_Entries_pq(active_pq & apq, Log & log, std::string wherestr = "") {
    ERRTRACE;
    if (!query_call_pq(apq.conn,"SELECT * FROM "+apq.pq_schemaname+".Logentries"+wherestr+" ORDER BY "+pq_entry_fieldnames[pqle_id],false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);
  
    PGresult *res;

    while ((res = PQgetResult(apq.conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<_pqle_NUM) ERRRETURNFALSE(__func__,"not enough fields in entries table");

        if (!get_Entry_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            std::string entryid_str(PQgetvalue(res, r, pq_entry_field[pqle_id]));
            std::string nodeid_str(PQgetvalue(res, r, pq_entry_field[pqle_nid]));
            std::string entrytext(PQgetvalue(res, r, pq_entry_field[pqle_text]));
            rtrim(entryid_str);
            rtrim(nodeid_str);

            // attempt to build a Log_entry_ID object
            try {
                const Log_entry_ID entryid(entryid_str);

                const Log_chunk_ID_key chunkkey(entryid.key()); // no need to try, this one has to be valid if the entry ID was valid
                Log_chunk * chunk = log.get_chunk(chunkkey);
                if (!chunk)
                    ERRRETURNFALSE(__func__,"stored Entry ("+entryid_str+") refers to Log chunk not found in Log");

                std::unique_ptr<Log_entry> entry;
                if (nodeid_str.empty() || (nodeid_str=="{null-key}")) { // make Log_entry object without Node specifier
                    entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                    chunk->add_Entry(*entry); // add to chunk.entries
                    log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                } else {
                    // attempt to build a Node_ID object
                    try {
                        const Node_ID_key nodeidkey(nodeid_str);
                        // const Node_ID nodeid(nodeid_str); // *** not sure why we were doing this

                        // make Log_entry object with Node specifier
                        entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, nodeidkey, chunk);
                        chunk->add_Entry(*entry);
                        log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                    } catch (ID_exception idexception) {
                        ERRRETURNFALSE(__func__, "invalid Node ID (" + nodeid_str + ") at Log entry [" + entryid_str + "], " + idexception.what()); // *** alternative: +",\ntreating as chunk-relative");
                        /* only use the below if you use the ADDERROR() alternative:
                        entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                        chunk->add_Entry(*entry);
                        log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr
                        */
                    }
                }
            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__, "entry with invalid Log entry ID (" + entryid_str + "), " + idexception.what());
            }

        }

        PQclear(res);
    }

    return true;
}

/**
 * First-Pass Load of Entries into Log::entries.
 * 
 * This version of the database table query loads entries data and creates
 * Log_entry objects but does not yet associate Log_chunks. It can be used
 * to prepare Log entries in advance where a sequence of passes is needed.
 * 
 * Note that this functions DOES NOT do `chunk->add_Entry()`. A second pass
 * is needed to accomplish that.
 * 
 * See for example how this is used in Load_Node_history_pq().
 * 
 * With optional WHERE statement.
 * 
 * @param apq data structure with active database connection pointer and schema name.
 * @param log a Log object that is added to.
 * @param wherestr An optional WHERE string to constrain which records are retrieved.
 * @return true if successful.
 */
bool read_Entries_first_pass_pq(active_pq & apq, Log & log, std::string wherestr = "") {
    ERRTRACE;
    if (!query_call_pq(apq.conn,"SELECT * FROM "+apq.pq_schemaname+".Logentries"+wherestr+" ORDER BY "+pq_entry_fieldnames[pqle_id],false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);
  
    PGresult *res;

    while ((res = PQgetResult(apq.conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<_pqle_NUM) ERRRETURNFALSE(__func__,"not enough fields in entries table");

        if (!get_Entry_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            std::string entryid_str(PQgetvalue(res, r, pq_entry_field[pqle_id]));
            std::string nodeid_str(PQgetvalue(res, r, pq_entry_field[pqle_nid]));
            std::string entrytext(PQgetvalue(res, r, pq_entry_field[pqle_text]));
            rtrim(entryid_str);
            rtrim(nodeid_str);

            // attempt to build a Log_entry_ID object
            try {
                const Log_entry_ID entryid(entryid_str);
                // Note that this version does NOT require a corresponding Log chunk!

                std::unique_ptr<Log_entry> entry;
                if (nodeid_str.empty() || (nodeid_str=="{null-key}")) { // make Log_entry object without Node specifier
                    entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext);
                    log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                } else {
                    // attempt to build a Node_ID object
                    try {
                        const Node_ID_key nodeidkey(nodeid_str);

                        // make Log_entry object with Node specifier
                        entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, nodeidkey);
                        log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                    } catch (ID_exception idexception) {
                        ERRRETURNFALSE(__func__, "invalid Node ID (" + nodeid_str + ") at Log entry [" + entryid_str + "], " + idexception.what()); // *** alternative: +",\ntreating as chunk-relative");
                        /* only use the below if you use the ADDERROR() alternative:
                        entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                        log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr
                        */
                    }
                }
            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__, "entry with invalid Log entry ID (" + entryid_str + "), " + idexception.what());
            }

        }

        PQclear(res);
    }

    return true;
}

/**
 * Load the full Log with all Log chunks, Log entries and Breakpoints from the PostgresSQL
 * database.
 * 
 * @param log a Log for the Chunks, Entries and Breakpoints, typically empty.
 * @param pa access object with database name and schema name.
 * @return true if the Log was succesfully loaded from the database.
 */
bool load_Log_pq(Log & log, Postgres_access & pa) {
    ERRTRACE;
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_LOG_PQ_RETURN(r) { PQfinish(conn); return r; }
    active_pq apq(conn,pa.pq_schemaname());

    ERRHERE(".chunks");
    if (!read_Chunks_pq(apq,log)) LOAD_LOG_PQ_RETURN(false);

    ERRHERE(".entries");
    if (!read_Entries_pq(apq,log)) LOAD_LOG_PQ_RETURN(false);

    ERRHERE(".breakpoints");
    if (!read_Breakpoints_pq(apq,log)) LOAD_LOG_PQ_RETURN(false);

    LOAD_LOG_PQ_RETURN(true);
}

/**
 * Find all of the Log chunks and Log entries (within the interval) that belong to
 * a specified Node, in two phases.
 * 
 * This function is called by load_partial_Log_pq() if the filter specifies a Node.
 * 
 * @param apq Access object with database name and schema name.
 * @param log Log object where Chunks and Entries are added.
 * @param chunkwherestr A prepared PQ WHERE string specifying Node and possibly time interval.
 * @param entrywherestr A prepared PQ WHERE string specifying Node.
 */
bool load_Node_history_pq(active_pq & apq, Log & log, const std::string & chunkwherestr, const std::string & entrywherestr) {
    ERRTRACE;
    // Phase 1: Collect all of the chunks and entries that explicitly belong to the specified Node.
    if (!read_Chunks_pq(apq,log,chunkwherestr)) return false;
    if (!read_Entries_first_pass_pq(apq,log,entrywherestr)) return false; // Typically entries with NID different from their chunk.

    // Phase 2: Collect all remaining entries within the collected chunks and remaining chunks that collected entries are in.
    Log_chunk_ID_key_set chunkkeyset = log.chunk_key_list_from_entries();
    if (chunkkeyset.size()>0) {
        // converting set of keys to WHERE IN query (fortunately, the command length limit is huge, see https://stackoverflow.com/a/4937695)
        std::string whereinstr(" WHERE id IN (");
        whereinstr.reserve(20+chunkkeyset.size()*(20+3));
        for (const auto & chunkkey : chunkkeyset) {
            whereinstr += TimeStamp_pq(chunkkey.get_epoch_time()) + ','; // '\'' + chunkkey.str() + "',";
        }
        whereinstr.back() = ')'; // *** you might need to add ORDER BY
        if (!read_Chunks_pq(apq,log,whereinstr)) return false; // This can handle making a duplicate, but we should prune those.
        log.prune_duplicate_chunks();
        log.add_entries_to_chunks(); // completes read_Entries_first_pass_pq() by doing the second part.
    }
    if (log.num_Chunks()>0) {
        std::string whereinstr(" WHERE SUBSTRING(id,1,12) IN (");
        whereinstr.reserve(40+log.num_Chunks()*(20+3));
        for (const auto & chunkptr : log.get_Chunks()) {
            //whereinstr += TimeStamp_pq(chunkptr.get()->get_open_time()) + ',';
            whereinstr += '\'' + chunkptr.get()->get_tbegin_str() + "',";
        }
        whereinstr.back() = ')'; // *** you might need to add ORDER BY
        if (!read_Entries_pq(apq,log,whereinstr)) return false; // inserting into map automatically deals with duplicates
    }
    return true;
}

/**
 * Load the Log chunks and Log entries that satisfy a specific filter.
 * 
 * The filter may specify a time interval and may specify a Node to which the
 * Log chunks and Log entries must belong.
 * 
 * This can also be used by smart on-demand Log caching modes.
 * 
 * @param log A Log for the Chunks and Entries, can be empty or may be added to.
 * @param pa Access object with database name and schema name.
 * @param filter A selective Log reading filter structure.
 * @return True if the Log was succesfully loaded from the database.
 */
bool load_partial_Log_pq(Log & log, Postgres_access & pa, const Log_filter & filter) {
    ERRTRACE;
    bool use_t_from = filter.t_from != RTt_unspecified;
    bool use_t_to = filter.t_to != RTt_unspecified;
    bool use_nkey = !filter.nkey.isnullkey();
    if (use_t_from && use_t_to && (filter.t_from > filter.t_to))
        return false;
    if (use_t_from && (filter.t_from > ActualTime()))
        return false;

    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_LOG_PQ_RETURN(r) { PQfinish(conn); return r; }
    active_pq apq(conn,pa.pq_schemaname());

    // Create Postgres WHERE statement.
    std::string chunkwherestr;
    std::string entrywherestr;
    if (use_t_from || use_t_to || use_nkey) {
        chunkwherestr += " WHERE ";
        if (use_t_from) {
            chunkwherestr += "id >= "+TimeStamp_pq(filter.t_from);
            if ((use_t_to) || (use_nkey))
                chunkwherestr += " AND ";
        }
        if (use_t_to) {
            chunkwherestr += "id <= "+TimeStamp_pq(filter.t_to);
            if (use_nkey)
                chunkwherestr += " AND ";
        }
    }

    if (use_nkey) { // if Node specified then take this branch
        std::string nidstr = "nid == '"+filter.nkey.str()+"'";
        chunkwherestr += nidstr;
        entrywherestr += " WHERE "+nidstr;
        if (load_Node_history_pq(apq,log,chunkwherestr,entrywherestr)) {
            LOAD_LOG_PQ_RETURN(true);
        } else {
            LOAD_LOG_PQ_RETURN(false);
        }
    }

    ERRHERE(".chunks");
    if (!read_Chunks_pq(apq,log,chunkwherestr)) LOAD_LOG_PQ_RETURN(false);

    if (use_t_from) {
        if (use_t_to) {
            entrywherestr += " WHERE SUBSTRING(id,1,12) BETWEEN " + TimeStamp_pq(filter.t_from) + " AND " + TimeStamp_pq(filter.t_to);
        } else {
            entrywherestr += " WHERE SUBSTRING(id,1,12) >= " + TimeStamp_pq(filter.t_from);
        }
    } else {
        if (use_t_to) {
            entrywherestr += " WHERE SUBSTRING(id,1,12) <= " + TimeStamp_pq(filter.t_to);
        }
    }

    ERRHERE(".entries");
    if (!read_Entries_pq(apq,log,entrywherestr)) LOAD_LOG_PQ_RETURN(false);

    // *** Breakpoints are really only for backwards compatibility.
    //ERRHERE(".breakpoints");
    //if (!read_Breakpoints_pq(apq,log)) LOAD_LOG_PQ_RETURN(false);

    LOAD_LOG_PQ_RETURN(true);
}


} // namespace fz
