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
bool create_Guide_table(PGconn* conn, const std::string schemaname, const std::string guidetable, const std::string guidetablelayout) {
    if (!conn)
        return false;

    std::string pq_cmdstr = "CREATE TABLE IF NOT EXISTS "+schemaname+"."+guidetable+'('+guidetablelayout+')';
    return true;
}

/**
 * Store a new Guide snippet in the PostgreSQL database.
 * 
 * @param snippet a guide snippet.
 * @param dbname database name.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @returns true if the snippet was successfully stored in the database.
 */
bool store_Guide_snippet_pq(const Guide_snippet & snippet, std::string dbname, std::string schemaname) {
    ERRHERE(".setup");
    PGconn* conn = connection_setup_pq(dbname);
    if (!conn) return false;

    if (snippet.empty()) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_SNIPPET_PQ_RETURN(r) { PQfinish(conn); return r; }

    if (!create_Guide_table(conn,schemaname,snippet.tablename,snippet.layout())) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    std::string insertsnippetstr("INSERT INTO "+schemaname + snippet.tablename + " VALUES "+ snippet.all_values_pqstr());
    if (!simple_call_pq(conn,insertsnippetstr)) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    STORE_SNIPPET_PQ_RETURN(true);
}

} // namespace fz
