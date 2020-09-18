// Copyright 2020 Randal A. Koene
// License TBD

// std
//#include <>

// core
#include "error.hpp"
#include "standard.hpp"
#include "general.hpp"
#include "fzpostgres.hpp"

namespace fz {

Simulate_PQ_Changes SimPQ;

/**
 * Set up a connection with an existing Postgres database.
 * 
 * This also prepares a safe search search path. The database needs to exist.
 * If necessary, create it with the command `createdb [databasename]` (which
 * defaults to the user name).
 * 
 * @param: dbname the identifier of a database in a local Postgres setup.
 * @return a pointer to the connection if successfully created, otherwise NULL.
 */
PGconn* connection_setup_pq(std::string dbname) {

    PGconn     *conn;
    PGresult   *res;

    if (dbname.empty()) ERRRETURNNULL(__func__, "missing database identifier");
    dbname.insert(0, "dbname = ");

    // Make a connection to the database
    conn = PQconnectdb(dbname.c_str());

    //Check to see that the backend connection was successfully made
    if (PQstatus(conn) != CONNECTION_OK) ERRRETURNNULL(__func__, std::string("connection to database failed: ")+PQerrorMessage(conn));

    // Set always-secure search path, so malicious users can't take control
    res = PQexec(conn,"SELECT pg_catalog.set_config('search_path', '', false)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        ADDERROR(__func__, std::string("SET failed: ")+PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    /*
     * Should PQclear PGresult whenever it is no longer needed to avoid memory
     * leaks
     */
    PQclear(res);

    return conn;
}

/**
 * Send a simple action call to a Postgres database.
 * 
 * Note: If the global flag simulate_pq_changes==true then this function does not execute
 * Postgres calls. Instead, the call string will be added to simulated_pq_calls.
 * 
 * @param conn active database connection.
 * @param astr action call string.
 * @return true if action call was successful.
 */
bool simple_call_pq(PGconn* conn, std::string astr) {
    if (!conn) ERRRETURNFALSE(__func__,"unable to call database action without active database connection");

    if (SimPQ.SimPQChangesAndLog(astr))
        return true;

    // See the example at http://zetcode.com/db/postgresqlc/
    PGresult* res;

    res = PQexec(conn, astr.c_str());
        
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        ADDERROR(__func__, std::string(astr.substr(0,14)+" failed: ")+PQerrorMessage(conn)+"\nPQ COMMAND = "+astr); 
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

/**
 * Dispatch a Postgres query for asynchronous processing in batch or single row mode.
 * Uses PQsendQuery() and PQsetSingleRowMode().
 * 
 * To receive results, use PQgetResult(), calling that function repeatedly if in
 * single row mode.
 * 
 * @param conn a valid Postgres connection object.
 * @param qstr a Postgres query string.
 * @param request_single_row_mode switches only this query to single row mode if true.
 * @return true if the query was successfully dispatched.
 */
bool query_call_pq(PGconn* conn, std::string qstr, bool request_single_row_mode) {
    if (!conn) ERRRETURNFALSE(__func__,"unable to call database action without active database connection");

    if (SimPQ.SimPQChangesAndLog(qstr))
        return true;

    if (!PQsendQuery(conn, qstr.c_str())) {
        ERRRETURNFALSE(__func__,std::string("query dispatch failed: ")+PQerrorMessage(conn)+"\nPQ COMMAND = "+qstr);
    }

    if (request_single_row_mode) PQsetSingleRowMode(conn);

    return true;

}

/**
 * Helper function that dumps several rows of data to a string buffer for
 * easy inspection. A query_call_pq() or equivalent should precede this.
 * 
 * To sample all columns simple set cend > number of columns.
 * 
 * @param conn an active Postgres connection.
 * @param rstart first row to sample.
 * @param rend one after last row to sample.
 * @param cstart first column to sample.
 * @param cend one after last column to sample.
 * @param databufstr a string buffer for sampled data.
 * @return number of rows sampled.
 */
int sample_query_data(PGconn *conn, unsigned int rstart, unsigned int rend, unsigned int cstart, unsigned int cend, std::string &databufstr) {
    PGresult *res;
    int tot_rows_sampled = 0;
    std::string datastr;
    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.
        const int rows = PQntuples(res);
        const int columns = PQnfields(res);
        databufstr += "rows = " + std::to_string(rows) + ", columns = " + std::to_string(columns) + '\n';
        for (int r = rstart; (r < rows) && (r < (int) rend); ++r) {
            for (int c = cstart; (c < columns) && (c < (int) cend); ++c) {
                datastr = PQgetvalue(res, r, c);
                databufstr += '[' + datastr + "] ";
            }
            databufstr += '\n';
            tot_rows_sampled++;
        }

        PQclear(res);
    }
    return tot_rows_sampled;
}

/**
 * Create a new database schema for Formalizer data.
 * 
 * @param conn active database connection.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @return true if schema was successfully created.
 */
bool create_Formalizer_schema_pq(PGconn* conn, std::string schemaname) {
    ERRHERE(".1");
    std::string pq_makeschema("CREATE SCHEMA IF NOT EXISTS "+schemaname);
    return simple_call_pq(conn,pq_makeschema);
}

/**
 * Convert a single string containing Postgres query output that
 * represents an array into a vector of strings.
 * 
 * The Postgres array should be enclosed in curly brackets.
 * 
 * WARNING: This function assumes that every comma indicates a
 * next element of the array. It does not consider the case where
 * array elements may be quoted text that contains commas.
 * 
 * @param pq_array_str the Postgres query output.
 * @return a vector of strings, one for each array element.
 */
std::vector<std::string> array_from_pq(std::string pq_array_str) {
    std::vector<std::string> elems;
    if (pq_array_str.empty()) return elems;
    if (pq_array_str.front()=='{') pq_array_str.erase(0,1);
    if (pq_array_str.empty()) return elems;
    if (pq_array_str.back()=='}') pq_array_str.pop_back();
    if (pq_array_str.empty()) return elems;
    split(pq_array_str, ',', std::back_inserter(elems));
    return elems;
}

/**
 * Convert Postgres query result time stamp strings to Unix time,
 * for example, for Node target date parameter.
 * 
 * The format returned by PQgetvalue() is '2015-01-03 16:00:00'.
 * As long as DATESTYLE has not been altered (e.g. with 'set datestyle
 * to DMY'), the standard ISO format is YMD. It might be best to
 * confirm that.
 * Empty or 'infinity' (or other non-numerical time stamps) are
 * interpreted as unspecified, for which -1 is returned.
 * 
 * This uses the UTC / Local Time convention built into time_stamp_time().
 * 
 * @param pqtimestamp a time stamp string obtained via PQgetvalue().
 * @return Unix time stamp as seconds since 00:00:00 UTC, January 1,
 *         1970 or -1 when unspecified or invalid format.
 */
time_t epochtime_from_timestamp_pq(std::string pqtimestamp) {
    if (pqtimestamp.empty()) return -1;
    if ((pqtimestamp.front()<'0') || (pqtimestamp.front()>'9')) return -1;

    // remove non-digits and seconds to ensure Formalizer time stamp format
    pqtimestamp.erase(remove_if(pqtimestamp.begin(),pqtimestamp.end(),is_not_digit),pqtimestamp.end());
    while (pqtimestamp.size()>12) pqtimestamp.pop_back();

    return time_stamp_time(pqtimestamp);
}

/// This includes the `load()` call directly in the constructor.
fzpq_configurable::fzpq_configurable(): configurable("fzpostgres") {
    if (!load()) {
        const std::string configerrstr("Errors during configuration file processing");
        ADDERROR(__func__,configerrstr);
        VERBOSEERR(configerrstr);
    }
}

/// Configure configurable parameters.
bool fzpq_configurable::set_parameter(const std::string & parlabel, const std::string & parvalue) {
    CONFIG_TEST_AND_SET_PAR(dbname, "dbname", parlabel, parvalue);
    CONFIG_TEST_AND_SET_PAR(pq_schemaname, "pq_schemaname", parlabel, parvalue);
    CONFIG_PAR_NOT_FOUND(parlabel);
}

void Postgres_access::usage_hook() {
    FZOUT("    -d use Postgres database <dbname>\n");
    FZOUT("       (default is " DEFAULT_DBNAME  ")\n"); // used to be $USER, but this was clarified in https://trello.com/c/Lww33Lym
    FZOUT("    -s use Postgres schema <schemaname>\n");
    FZOUT("       (default is $USER with fallback to: formalizeruser)\n");
}

bool Postgres_access::options_hook(char c, std::string cargs) {
    switch (c) {

    case 'd':
        config.dbname = cargs;
        return true;

    case 's':
        config.pq_schemaname = cargs;
        return true;
    }

    return false;
}

void Postgres_access::dbname_error() {
    std::string errstr("Need a database to proceed. Defaults to formalizer."); // used to be $USER
    ADDERROR(__func__,errstr);
    FZERR('\n'+errstr+'\n');
    standard.exit(exit_database_error);
}

void Postgres_access::schemaname_error() {
    std::string errstr("Need a schema to proceed. Defaults to $USER or formalizeruser."); // used to be $USER
    ADDERROR(__func__,errstr);
    FZERR('\n'+errstr+'\n');
    standard.exit(exit_database_error);
}

void Postgres_access::access_initialize() {
    COMPILEDPING(std::cout,"PING-access_initialize()\n");
    if (dbname().empty()) { // attempt to get a default
        config.dbname = DEFAULT_DBNAME;
        /* See how this was clarified and changed in https://trello.com/c/Lww33Lym.
        char *username = std::getenv("USER");
        if (username)
            dbname = username;
        */
    }
    if (pq_schemaname().empty()) { // attempt to get a default
        char *username = std::getenv("USER");
        if (username)
            config.pq_schemaname = username;
    }

    if (dbname().empty())
        dbname_error();
    if (pq_schemaname().empty())
        schemaname_error();

    if (!standard.quiet) {
        FZOUT("Postgres database selected: "+dbname()+'\n');
        FZOUT("Postgres schema selected  : "+pq_schemaname()+'\n');
    }
}


} // namespace fz
