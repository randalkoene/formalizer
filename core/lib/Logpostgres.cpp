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
    for (const auto & [chunk_key, chunk_ptr] : const_cast<Log *>(&log)->get_Chunks()) { // had to remove const to use get_Chunks() here
        if (!add_Logchunk_pq(apq, *chunk_ptr)) STORE_LOG_PQ_RETURN(false);
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

/**
 * Append Entry to existing table in schema of PostgreSQL database.
 * 
 * @param entry A valid Log entry object.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log entry was successfully stored in the database.
 */
bool append_Log_entry_pq(const Log_entry & entry, Postgres_access & pa) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_LOG_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".append");
    if (!add_Logentry_pq(apq, entry)) STORE_LOG_PQ_RETURN(false);

    STORE_LOG_PQ_RETURN(true);
}

/**
 * Close the Chunk specified, which must already exist within a table in
 * schema of PostgreSQL database.
 * 
 * Note that this same function can be used to reopen a Log chunk by
 * specifying a `t_close` time equal to `FZ_TCHUNK_OPEN`. See how this
 * is done in `fzlog.cpp:reopen_chunk()`.
 * 
 * @param chunk A valid Log chunk object with valid t_close time.
 * @param pa Access object with database name and Formalizer schema name.
 * @returns True if the Log chunk was successfully updated to closed status.
 */
bool close_Log_chunk_pq(const Log_chunk & chunk, Postgres_access & pa) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define CLOSE_LOG_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".close");
    std::string close_cmd_pq("UPDATE "+apq.pq_schemaname+".Logchunks SET tclose = "+TimeStamp_pq(chunk.get_close_time())+" WHERE id = "+TimeStamp_pq(chunk.get_open_time()));
    if (!simple_call_pq(apq.conn, close_cmd_pq))
        CLOSE_LOG_PQ_RETURN(false);

    CLOSE_LOG_PQ_RETURN(true);
}

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
bool append_Log_chunk_pq(const Log_chunk & chunk, Postgres_access & pa) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_LOG_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".append");
    if (!add_Logchunk_pq(apq, chunk)) STORE_LOG_PQ_RETURN(false);

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

/**
 * This function converts the Log entry minor ID to a three digit zero-padded
 * integer for storage in Postgres. This does not affect how the minor ID is
 * retrieved and converted back to its constituent part of a Log entry ID.
 * This is done to ensure that order comparisons and sorting in Postgres are
 * produce the same results as with the lexical comparison operators defined
 * for Log_TimeStamp.
 * 
 * @param minor_id A Log entry minor-ID number.
 * @return A zero-padded three digit numerical string.
 */
std::string entry_minor_id_pq(unsigned int minor_id) {
    char minor_id_digits[4] = "000";
    minor_id_digits[2] += (minor_id % 10);
    minor_id_digits[1] += ((minor_id/10) % 10);
    minor_id_digits[0] += ((minor_id/100) % 10);
    return minor_id_digits;
}

/// Return the Log entry ID between apostrophes.
std::string Logentry_pq::id_pqstr() {
    return "'"+entry->get_id_str().substr(0,13)+entry_minor_id_pq(entry->get_minor_id())+"'";
    //return "'"+entry->get_id_str()+"'";
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
            const Log_chunk * chunk = log.get_chunk(chunkkey);
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
 * With optional WHERE statement and optional LIMIT and direction statement.
 */
bool read_Chunks_pq(active_pq & apq, Log & log, std::string wherestr = "", std::string limitdirstr = "") {
    ERRTRACE;
    if (!query_call_pq(apq.conn,"SELECT * FROM "+apq.pq_schemaname+".Logchunks"+wherestr+" ORDER BY "+pq_chunk_fieldnames[pqlc_id]+limitdirstr,false)) return false;

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

                    log.add_Chunk(chunkstamp,nid,chunkclose_t);
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
                const Log_chunk * chunk = log.get_chunk(chunkkey);
                if (!chunk)
                    ERRRETURNFALSE(__func__,"stored Entry ("+entryid_str+") refers to Log chunk not found in Log");

                std::unique_ptr<Log_entry> entry;
                if (nodeid_str.empty() || (nodeid_str=="{null-key}")) { // make Log_entry object without Node specifier
                    entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, chunk);
                    const_cast<Log_chunk *>(chunk)->add_Entry(*entry); // add to chunk.entries
                    log.get_Entries().insert({entryid.key(),std::move(entry)}); // entry is now nullptr

                } else {
                    // attempt to build a Node_ID object
                    try {
                        const Node_ID_key nodeidkey(nodeid_str);
                        // const Node_ID nodeid(nodeid_str); // *** not sure why we were doing this

                        // make Log_entry object with Node specifier
                        entry = std::make_unique<Log_entry>(entryid.key().idT, entrytext, nodeidkey, chunk);
                        const_cast<Log_chunk *>(chunk)->add_Entry(*entry);
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

#define USE_NODE_HISTORY_CACHE_TABLE
#ifdef USE_NODE_HISTORY_CACHE_TABLE
const std::string pq_history_fieldnames[3] = {"nid",
                                              "chunkids",
                                              "entryids"};

unsigned int pq_history_field[3];

bool get_History_pq_field_numbers(PGresult *res) {
    if (!res) return false;

    for (auto i = 0; i<3; i++) {
        if ((pq_history_field[i] = PQfnumber(res,pq_history_fieldnames[i].c_str())) < 0) {
            ERRRETURNFALSE(__func__,"field '"+pq_history_fieldnames[i]+"' not found in database histories cache table");
        }
    }
    return true;
}

void replace_double_quotes(std::string & s) {
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i]=='"')
            s[i] = '\'';
}

/**
 * Unfortunately, Postgres output strings with array data cannot simply be used
 * unmodified as array input data. Here are some things this needs to do:
 * - If there are double quotes in the string then it's almost ready, just
 *   replace those with single quotes.
 * - Otherwise, you have to put single quotes around every of the comma
 *   delimited elements.
 */
void convert_array_output_to_input_pq(std::string & entryids_str) {
    if (entryids_str.empty())
        return;

    if (entryids_str[0]=='"') {
        replace_double_quotes(entryids_str);
        return;
    }

    char * buf = new char[2*entryids_str.size()];
    size_t bufpos = 0;
    buf[bufpos] = '\''; ++bufpos;
    for (size_t i = 0; i < entryids_str.size(); ++i) {
        if (entryids_str[i] == ',') {
            buf[bufpos] = '\''; ++bufpos;
            buf[bufpos] = ','; ++bufpos;
            buf[bufpos] = '\''; ++bufpos;
            continue;
        }
        buf[bufpos] = entryids_str[i]; ++bufpos;
    }
    buf[bufpos] = '\''; ++bufpos;
    buf[bufpos] = '\0';
    entryids_str = buf;
    delete[] buf;
}

bool parse_PQValues_to_Node_history(PGresult *res, int r, Node_history & nodehist, std::string & nodeid_str) {
    nodeid_str = PQgetvalue(res, r, pq_history_field[0]);
    std::string chunkids_str = PQgetvalue(res, r, pq_history_field[1]);
    std::string entryids_str = PQgetvalue(res, r, pq_history_field[2]);
    //rtrim(entryids_str);
    //rtrim(chunkids_str);

    bool haschunks = chunkids_str.size() >= 12;
    bool hasentries = entryids_str.size() >= 14; // can be false if a Node only has an empty chunk
    if (haschunks) {
        if (chunkids_str.front() == '{')
            chunkids_str.erase(0,1);
        if (chunkids_str.back() == '}')
            chunkids_str.pop_back();
        auto chunkidsvec = split(chunkids_str,',');
        // Add all of these chunks owned by the Node to the set of chunks.
        for (auto & chunkid_str : chunkidsvec) {
            try {
                nodehist.chunks.emplace(chunkid_str);
            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__,"Invalid Chunk ID ["+chunkid_str+"], "+idexception.what());
            }
        }
    }

    if (hasentries) {
        if (entryids_str.front() == '{')
            entryids_str.erase(0,1);
        if (entryids_str.back() == '}')
            entryids_str.pop_back();
        auto entryidsvec = split(entryids_str,',');
        // Make sure the chunks surrounding these entries are also included.
        for (auto & entryid_str : entryidsvec) {
            try {
                nodehist.chunks.emplace(entryid_str.substr(0,12)); // the set type discards duplicates
            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__,"Invalid Entry ID ["+entryid_str+"], "+idexception.what());
            }
        }
        haschunks = true; // even if only due to additions from entries
    }

    return true;
}

/**
 * Load the cached Log history of a specific Node.
 * 
 * @param[in] apq Access object with database name and schema name.
 * @param[in] nkey A Node ID key.
 * @param[out] nodehist A Node_history object that receives the resulting data.
 */
bool load_Node_history_cache_entry_pq(active_pq & apq, const Node_ID_key & nkey, Node_history & nodehist) {
    ERRTRACE;

    std::string loadstr("SELECT * FROM "+apq.pq_schemaname+".histories WHERE nid = '"+nkey.str()+"'");
    if (!query_call_pq(apq.conn, loadstr, false)) {
        std::string errstr("Unable to load Node history references from cache table. Perhaps run `fzquerypq -R histories`.");
        ADDERROR(__func__, errstr);
        VERBOSEERR(errstr+'\n');
        return false; // *** you might need to explicitly allow for nodes without histories
    }

    std::string nodeid_str;

    PGresult *res;

    while ((res = PQgetResult(apq.conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<3) ERRRETURNFALSE(__func__,"not enough fields in histories cache table");

        if (!get_History_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            if (!parse_PQValues_to_Node_history(res, r, nodehist, nodeid_str)) {
                return false;
            }

        }

        PQclear(res);
    }

    return true;
}

/**
 * Load a Node_histories object from a complete cache table in the database.
 * 
 * @param[in] pa Access object with valid database and schema identifiers.
 * @param[out] nodehistories A Node_histories object.
 * @return True if cache table loading was successful.
 */
bool load_Node_history_cache_table_pq(Postgres_access & pa, Node_histories & nodehistories) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_NHCT_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".load");

    std::string loadstr("SELECT * FROM "+apq.pq_schemaname+".histories");
    if (!query_call_pq(apq.conn, loadstr, false)) {
        std::string errstr("Unable to load Node history references from cache table. Perhaps run `fzquerypq -R histories`.");
        ADDERROR(__func__, errstr);
        VERBOSEERR(errstr+'\n');
        LOAD_NHCT_PQ_RETURN(false); // *** you might need to explicitly allow for nodes without histories
    }

    PGresult *res;

    while ((res = PQgetResult(apq.conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<3) {
            ADDERROR(__func__,"not enough fields in histories cache table");
            LOAD_NHCT_PQ_RETURN(false);
        }

        if (!get_History_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            std::string nodeid_str;
            history_ptr nodehist_ptr = std::make_unique<Node_history>();
            if (!nodehist_ptr) {
                ADDERROR(__func__, "Unable to create Node_history object");
                LOAD_NHCT_PQ_RETURN(false);
            }
            if (!parse_PQValues_to_Node_history(res, r, *(nodehist_ptr.get()), nodeid_str)) {
                ADDERROR(__func__, "Parsing query response for Node_history failed");
                LOAD_NHCT_PQ_RETURN(false);
            }
            try {
                Node_ID_key nkey(nodeid_str);
                nodehistories.emplace(nkey, std::move(nodehist_ptr));
            } catch (ID_exception idexception) {
                ADDERROR(__func__,"Invalid Node ID ["+nodeid_str+"], "+idexception.what());
                LOAD_NHCT_PQ_RETURN(false);
            }

        }

        PQclear(res);
    }

    LOAD_NHCT_PQ_RETURN(true);
}

/**
 * Get lists of Log chunks and Log entries from the Node history cache table and
 * load all of the chunks and entries listed there into a Log object.
 * 
 * This function is called by `load_partial_Log_pq()` if the filter specifies a Node.
 * 
 * Note that the history cache reflects a Node's actual chain of chunks and
 * entries. This does NOT include chunks that belong to other Nodes, even if those
 * chunks contain entries that belong to this node.
 * Consequently, IF the presentation of a Node history should include only
 * complete chunks then chunks and extra entries need to be added to the history.
 * 
 * The entries to load are derived by SUBSTRING from chunk IDs. That takes care of
 * loading all entries in the chunks within a Node's history. Separately, and in
 * advance, entries that are not within Node-owned chunks need to be identified and
 * the corresponding chunks added to the set-to-load.
 * 
 * @param apq Access object with database name and schema name.
 * @param log Log object where Chunks and Entries are added.
 * @param filter A Log_filter structure where at least the nkey is specified.
 * @param chunkwherestr A prepared PQ WHERE string specifying Node and possibly time interval.
 * @param entrywherestr A prepared PQ WHERE string specifying Node.
 * @return True if successfully loaded into Log.
 */
bool load_Node_history_pq(active_pq & apq, Log & log, const Log_filter & filter, std::string & chunkwherestr, std::string & entrywherestr) {
    ERRTRACE;

    // 1. We need to make the complete set of chunks here, probably by converting the
    //    strings back to actual Node_history set for the Node.
    // 2. Then we can quickly and easily run through all of the entries, adding
    //    corresponding Log chunk ID key's to the chunk set (without duplication).
    // 3. Then we can use the list of chunks to make the WHERE id IN string for
    //    chunk loading. And we can use the same string and SUBSTRING(id) to
    //    load all of the entries that belong in those chunks.

    Node_history nodehist;
    if (!load_Node_history_cache_entry_pq(apq, filter.nkey, nodehist)) {
        return false;
    }

    // set up the WHERE IN part first, then possibly add WHERE >= and WHERE <= parts

    // Temporal constraints are imposed by comparators in the WHERE string, but
    // a number of chunks limit needs to be established on the collected set
    // first if there is one.
    if ((filter.limit>0) && (nodehist.chunks.size()>filter.limit)) {
        // *** I haven't yet found documentation about whether std::prev(begin()) is safe,
        //     so, I'm doing this the long way to avoid possible debugging for now.
        auto keepfrom_it = nodehist.chunks.end();
        for (unsigned long i = filter.limit; i>0; --i) {
            if (keepfrom_it != nodehist.chunks.begin()) {
                --keepfrom_it;
            }
        }
        nodehist.chunks.erase(nodehist.chunks.begin(),keepfrom_it);
    }

    nodehist.add_surrounding_chunks();

    if (!nodehist.chunks.empty()) { //if (haschunks) {
        // Now the full WHERE string for the chunks (and the entries).
        chunkwherestr = " WHERE id IN (";
        entrywherestr = " WHERE SUBSTRING(id,1,12) IN (";
        chunkwherestr.reserve(30+nodehist.chunks.size()*24);
        entrywherestr.reserve(60+nodehist.chunks.size()*24);
        for (const auto & chunkidkey : nodehist.chunks) {
            time_t t = chunkidkey.get_epoch_time();
            chunkwherestr += TimeStamp_pq(t) + ',';
            //chunkwherestr += TimeStamp_to_TimeStamp_pq(chunkidstr) + ',';
            entrywherestr += '\''+TimeStampYmdHM(t) + "',";
        }
        chunkwherestr.back() = ')';
        entrywherestr.back() = ')';

        if (filter.t_from != RTt_unspecified) {
            chunkwherestr += " AND id >= " + TimeStamp_pq(filter.t_from);
            entrywherestr += " AND SUBSTRING(id,1,12) >= '" + TimeStampYmdHM(filter.t_from) + '\'';
        }
        if (filter.t_to != RTt_unspecified) {
            chunkwherestr += " AND id <= " + TimeStamp_pq(filter.t_to);
            entrywherestr += " AND SUBSTRING(id,1,12) <= '" + TimeStampYmdHM(filter.t_to) + '\'';
        }

        if (!read_Chunks_pq(apq,log,chunkwherestr)) return false;

        if (!read_Entries_pq(apq,log,entrywherestr)) return false;
    }

    return true;
}    
#else
/**
 * Find all of the Log chunks and Log entries (within the interval) that belong to
 * a specified Node, in two phases.
 * 
 * This function is called by `load_partial_Log_pq()` if the filter specifies a Node.
 * 
 * @param apq Access object with database name and schema name.
 * @param log Log object where Chunks and Entries are added.
 * @param filter A Log_filter structure where at least the nkey is specified.
 * @param chunkwherestr A prepared PQ WHERE string specifying Node and possibly time interval.
 * @param entrywherestr A prepared PQ WHERE string specifying Node.
 * @return True if successfully loaded into Log.
 */
bool load_Node_history_pq(active_pq & apq, Log & log, const Log_filter & filter, std::string & chunkwherestr, std::string & entrywherestr) {
    ERRTRACE;
    std::string nidstr = "nid = '"+filter.nkey.str()+"'";
    chunkwherestr += nidstr;
    entrywherestr += " WHERE "+nidstr;
    
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
#endif

/**
 * Load the Log chunks and Log entries that satisfy a specific filter.
 * 
 * The filter may specify a time interval and may specify a Node to which the
 * Log chunks and Log entries must belong. The filter can also specify a
 * limit on the number of Log chunks to load, and it can specify loading
 * back-to-front (i.e. from latest to earliest).
 * 
 * Note: Presently, using a limit or back-to-front does not (yet) work
 *       with a specified Node.
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

    unsigned long limit = filter.limit;
    if (use_t_from && use_t_to) {
        limit = 0; // Having both t_from and t_to overrides limit.
    }
    bool back_to_front = filter.back_to_front;
    if (use_t_from || use_t_to) {
        back_to_front = false; // Loading direction is meaningless if t_from or t_to are specified.
    }

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
        bool res = load_Node_history_pq(apq, log, filter, chunkwherestr, entrywherestr);
        LOAD_LOG_PQ_RETURN(res);
    }

    // Create Postgres LIMIT and direction statement.
    std::string limitdirstr;
    if (back_to_front) {
        limitdirstr += " DESC";
    }
    if (limit>0) {

        limitdirstr += " LIMIT "+std::to_string(limit);
    }

    ERRHERE(".chunks");
    if (!read_Chunks_pq(apq, log, chunkwherestr, limitdirstr)) LOAD_LOG_PQ_RETURN(false);

    // If loading was constrained by a filter then always read the entries that belong
    // to the chunks that were read.
    if (use_t_from || use_t_to || (limit>0)) {
        // *** Not sure if this works properly if log was not empty, i.e. if the entries of
        //     some chunks may already have been loaded.
        // For the corrections applied here, see TL#202010161950, but note that those were not yet full corrections.
        entrywherestr += " WHERE SUBSTRING(id,1,12) BETWEEN '" + TimeStampYmdHM(log.oldest_chunk_t()) + "' AND '" + TimeStampYmdHM(log.newest_chunk_t()) + '\'';

        //entrywherestr += " WHERE SUBSTRING(id,1,12) BETWEEN " + TimeStamp_pq(log.oldest_chunk_t()) + " AND " + TimeStamp_pq(log.newest_chunk_t()+3600);
    }

    /* Replaced with the clause above where entries to load is determined by chunks loaded.
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
    */

    ERRHERE(".entries");
    if (!read_Entries_pq(apq,log,entrywherestr)) LOAD_LOG_PQ_RETURN(false);

    // *** Breakpoints are really only for backwards compatibility.
    //ERRHERE(".breakpoints");
    //if (!read_Breakpoints_pq(apq,log)) LOAD_LOG_PQ_RETURN(false);

    LOAD_LOG_PQ_RETURN(true);
}

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
bool load_last_chunk_and_entry_pq(Log & log, Postgres_access & pa) {
    ERRTRACE;

    Log_filter filter;
    filter.limit = 1;
    filter.back_to_front = true;

    return load_partial_Log_pq(log, pa, filter);
}


std::string chunk_key_list_pq(const Log_chunk_ID_key_set & chunks) {
    if (chunks.empty())
        return "'{}'"; // this version doesn't need an explicit type case for empty array

    std::string arraystr("ARRAY[");
    arraystr.reserve(10+chunks.size()*16);
    for (const auto & chunkkey : chunks) {
        arraystr += '\'' + chunkkey.str() + "',";
    }
    arraystr.back() = ']';
    return arraystr;
}

std::string entry_key_list_pq(const Log_entry_ID_key_set & entries) {
    if (entries.empty())
        return "'{}'"; // this version doesn't need an explicit type case for empty array

    std::string arraystr("ARRAY[");
    arraystr.reserve(10+entries.size()*20);
    for (const auto & entrykey : entries) {
        arraystr += '\'' + entrykey.str().substr(0,13)+entry_minor_id_pq(entrykey.idT.minor_id) + "',"; // Updated to use the new minor-ID format for Log entries!
    }
    arraystr.back() = ']';
    return arraystr;
}

/**
 * Store a Node_history object to a cache table in the database to
 * speed up generation of a Node-specific Log history.
 * 
 * @param nodehist A Node_history object.
 * @param pa Access object with valid database and schema identifiers.
 * @return True if cache table storage was successful.
 */
bool store_Node_history_pq(const Node_histories & nodehist, Postgres_access & pa) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_LOG_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".clear");
    std::string tablename(apq.pq_schemaname+".histories");
    const std::string clearstr("DROP TABLE IF EXISTS "+tablename+" CASCADE");
    if (!simple_call_pq(apq.conn, clearstr)) {
        ADDERROR(__func__, "Unable to drop previous histories cache table");
        STORE_LOG_PQ_RETURN(false);
    }
 
    ERRHERE(".create");
    const std::string createstr("CREATE TABLE "+tablename+" (nid char(16) PRIMARY KEY, chunkids char(12)[], entryids char(16)[])");
    if (!simple_call_pq(apq.conn, createstr)) {
        ADDERROR(__func__, "Unable to create histories cache table");
        STORE_LOG_PQ_RETURN(false);
    }

    ERRHERE(".cache");
    for (const auto & [nidkey, historyptr] : nodehist) {
        std::string insertstr = "INSERT INTO "+tablename+" VALUES ('"+nidkey.str()+"',"+chunk_key_list_pq(historyptr->chunks)+','+entry_key_list_pq(historyptr->entries)+')';
        if (!simple_call_pq(apq.conn, insertstr)) {
            ADDERROR(__func__, "Unable to insert values into histories cache table. Postgres command: "+insertstr.substr(0,1024));
            STORE_LOG_PQ_RETURN(false);
        }
    }

    STORE_LOG_PQ_RETURN(true);
}

/**
 * Refresh the Node history cache table.
 * 
 * @param pa Access object with valid database and schema identifiers.
 * @return True if the refresh was successful.
 */
bool refresh_Node_history_cache_pq(Postgres_access & pa) {
    std::unique_ptr<Log> logptr = std::make_unique<Log>();

    // Load the complete Log.
    if (!load_Log_pq(*logptr, pa)) {
        FZERR("\nSomething went wrong! Unable to load Log from Postgres database.\n");
        standard.exit(exit_database_error);
    }

    logptr->setup_Chain_nodeprevnext();

    Node_histories nodehist(*(logptr.get()));

    if (!store_Node_history_pq(nodehist, pa)) {
        FZERR("\nSomething went wrong! Unable to store Node history cache in Postgres database.\n");
        standard.exit(exit_database_error);
    }

    return true;
}


} // namespace fz
