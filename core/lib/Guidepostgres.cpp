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
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_SNIPPET_PQ_RETURN(r) { PQfinish(conn); return r; }

    active_pq apq(conn,pa.pq_schemaname());
    if (!create_Guide_table(apq,snippet.tablename,snippet.layout())) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    std::string insertsnippetstr("INSERT INTO "+pa.pq_schemaname() + "." + snippet.tablename + " VALUES (" + snippet.all_values_pqstr() + ')');
    if (!simple_call_pq(conn,insertsnippetstr)) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    STORE_SNIPPET_PQ_RETURN(true);
}

/**
 * Store multiple Guide snippets in the PostgreSQL database. Creates the table if necessary.
 * 
 * At least the first element (index 0) of the vector must contain a valid `tablename` and
 * must produce valid `layout()` information.
 * 
 * @param snippets a vector guide snippets.
 * @param pa a standard database access stucture with database name and schema name.
 * @returns true if the snippet was successfully stored in the database.
 */
bool store_Guide_multi_snippet_pq(const std::vector<Guide_snippet_ptr> & snippets, Postgres_access & pa) {
    ERRTRACE;
    if (snippets.empty()) return false;

    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_SNIPPET_PQ_RETURN(r) { PQfinish(conn); return r; }

    active_pq apq(conn,pa.pq_schemaname());
    if (!create_Guide_table(apq,snippets[0]->tablename,snippets[0]->layout())) {
        STORE_SNIPPET_PQ_RETURN(false);
    }

    std::string insertcmdtargetstr("INSERT INTO "+pa.pq_schemaname() + "." + snippets[0]->tablename + " VALUES (");

    for (size_t i = 0; i < snippets.size(); ++i) {

        if (snippets[i]->snippet.empty()) {
            ADDWARNING(__func__, "Empty guide snippet skipped (index="+std::to_string(i)+')');
            continue;
        }

        std::string insertsnippetstr(insertcmdtargetstr + snippets[i]->all_values_pqstr() + ')');
        if (!simple_call_pq(conn,insertsnippetstr)) {
            STORE_SNIPPET_PQ_RETURN(false);
        }
    }

    STORE_SNIPPET_PQ_RETURN(true);
}

/**
 * Read a single snippet from a Guide table in the database.
 * 
 * The `snippet` should specify `snippet.tablename` and a working `idstr()` method.
 * 
 * @param snippet data structure that clearly identifies the snippet and receives the resulting text.
 * @param pa access data with database name and schema name.
 */
bool read_Guide_snippet_pq(Guide_snippet & snippet, Postgres_access & pa) {
    ERRHERE(".setup");
    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_SNIPPET_PQ_RETURN(r) { PQfinish(conn); return r; }

    if (!query_call_pq(conn,"SELECT snippet FROM "+pa.pq_schemaname()+ "." + snippet.tablename+" WHERE id="+snippet.idstr(),false)) LOAD_SNIPPET_PQ_RETURN(false);

    //sample_query_data(conn,0,4,0,100,tmpout);
  
    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)< 1) {
            ADDERROR(__func__,"not enough fields in snippet result");
            LOAD_SNIPPET_PQ_RETURN(false);
        }

        //if (!get_Node_pq_field_numbers(res)) LOAD_SNIPPET_PQ_RETURN(false); // *** we only asked for one field

        snippet.snippet.clear();
        for (int r = 0; r < rows; ++r) {

            snippet.snippet.append(PQgetvalue(res, r, 0));

        }

        PQclear(res);
    }

    LOAD_SNIPPET_PQ_RETURN(true);
}


} // namespace fz
