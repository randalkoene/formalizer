// Copyright 2020 Randal A. Koene
// License TBD

// std
//#include <>

// core
#include "error.hpp"
#include "fzpostgres.hpp"
#include "Guidepostgres.hpp"


namespace fz {

/**
 * Create the table for Guide snippets if it does not already exist.
 * 
 * @param conn is an open database connection.
 * @param schemaname is the Formalizer schema name (usually Graph_access::pq_schemaname).
 * @param guidetable is the Guide table name (e.g. "guide_system").
 */
bool create_Guide_table(const active_pq & apq, const std::string guidetable, const std::string guidetablelayout) {
    if (!apq.conn)
        return false;

    std::string pq_cmdstr = "CREATE TABLE IF NOT EXISTS "+apq.pq_schemaname+"."+guidetable+'('+guidetablelayout+')';
    return simple_call_pq(apq.conn,pq_cmdstr);
}

/**
 * Store a new Guide snippet in the PostgreSQL database. Creates the table if necessary.
 * 
 * @param snippet a guide snippet.
 * @param pa a standard database access stucture with database name and schema name.
 * @returns true if the snippet was successfully stored in the database.
 */
bool store_Guide_snippet_pq(const Guide_snippet & snippet, Postgres_access & pa) {
    ERRHERE(".setup");
    if (snippet.empty()) return false;

    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname);
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_SNIPPET_PQ_RETURN(r) { PQfinish(conn); return r; }

    active_pq apq(conn,pa.pq_schemaname);
    if (!create_Guide_table(apq,snippet.tablename,snippet.layout())) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    std::string insertsnippetstr("INSERT INTO "+pa.pq_schemaname + "." + snippet.tablename + " VALUES (" + snippet.all_values_pqstr() + ')');
    if (!simple_call_pq(conn,insertsnippetstr)) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    STORE_SNIPPET_PQ_RETURN(true);
}

} // namespace fz
