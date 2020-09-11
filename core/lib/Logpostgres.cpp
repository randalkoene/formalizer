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
    "id char(14),"  // pqle_id
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
    ERRHERE(".1");
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
    ERRHERE(".1");
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
    ERRHERE(".1");
    if (!apq.conn)
        return false;

    std::string pq_maketable("CREATE TABLE "+apq.pq_schemaname+".Logentries ("+pq_LElayout+')');
    return simple_call_pq(apq.conn,pq_maketable);
}

bool add_Breakpoint_pq(const active_pq & apq, const Log_chunk_ID_key & bptopid) {
    ERRHERE(".1");
    if (!apq.conn)
        return false;

    Breakpoint_pq bpq(&bptopid);
    std::string tstr("INSERT INTO "+apq.pq_schemaname + ".Breakpoints VALUES "+ bpq.All_Breakpoint_Data_pqstr());
    return simple_call_pq(apq.conn,tstr);
}

bool add_Logchunk_pq(const active_pq & apq, const Log_chunk & chunk) {
    ERRHERE(".1");
    if (!apq.conn)
        return false;

    Logchunk_pq lpq(&chunk);
    std::string tstr("INSERT INTO "+apq.pq_schemaname + ".Logchunks VALUES "+ lpq.All_Logchunk_Data_pqstr());
    return simple_call_pq(apq.conn,tstr);
}

bool add_Logentry_pq(const active_pq & apq, const Log_entry & entry) {
    ERRHERE(".1");
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
    ERRHERE(".setup");
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname);
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_LOG_PQ_RETURN(r) { PQfinish(apq.conn); return r; }

    ERRHERE(".schema");
    if (!create_Formalizer_schema_pq(apq.conn, apq.pq_schemaname)) STORE_LOG_PQ_RETURN(false);

    //ERRHERE(".enum");
    //if (!create_Enum_Types_pq(conn, schemaname)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".breakpointstable");
    if (!create_Breakpoints_table_pq(apq)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".breakpoints");
    unsigned long n = log.num_Breakpoints();
    unsigned long ncount = 0;
    for (Log_chunk_ID_key_deque::size_type bpidx = 0; bpidx<n; ++bpidx) {
        Log_chunk_ID_key & bptopidkey = const_cast<Log *>(&log)->get_Breakpoint_first_chunk_id_key(bpidx);
        if (!add_Breakpoint_pq(apq, bptopidkey)) STORE_LOG_PQ_RETURN(false);
        ncount++;
        if (progressfunc) (*progressfunc)(n,ncount);
    }

    ERRHERE(".logchunkstable");
    if (!create_Logchunks_table_pq(apq)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".logchunks");
    n = log.num_Chunks();
    ncount = 0;
    for (Log_chunk_ptr_deque::size_type cidx = 0; cidx<n; ++cidx) {
        Log_chunk * chunkptr = const_cast<Log *>(&log)->get_chunk(cidx); // *** slightly risky, untested pointer
        if (!add_Logchunk_pq(apq, *chunkptr)) STORE_LOG_PQ_RETURN(false);
        ncount++;
        if (progressfunc) (*progressfunc)(n,ncount);
    }

    ERRHERE(".logentriestable");
    if (!create_Logentries_table_pq(apq)) STORE_LOG_PQ_RETURN(false);

    ERRHERE(".logentries");
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

} // namespace fz
