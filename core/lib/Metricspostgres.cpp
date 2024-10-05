// Copyright 2024 Randal A. Koene
// License TBD

/**
 * This database access library needs to be able to:
 * - Create metrics tables used in DayWiz.
 * - Read data for a specific day from a metrics table.
 * - Write data for a specific day to a metrics table.
 */

// std
//#include <>

// core
#include "error.hpp"
#include "general.hpp"
#include "fzpostgres.hpp"
#include "Metricspostgres.hpp"


namespace fz {

/**
 * Create the table for Metrics data if it does not already exist.
 * 
 * @param conn is an open database connection.
 * @param schemaname is the Formalizer schema name (usually Graph_access::pq_schemaname).
 * @param metricstable is the Metrics table name (e.g. "wiztable").
 */
bool create_Metrics_table(const active_pq& apq, const std::string& metricstable, const std::string& metricstablelayout) {
    if (!apq.conn)
        return false;

    std::string pq_cmdstr = "CREATE TABLE IF NOT EXISTS "+apq.pq_schemaname+"."+metricstable+'('+metricstablelayout+')';
    return simple_call_pq(apq.conn,pq_cmdstr);
}

/**
 * Store a new Metrics data in the PostgreSQL database. Creates the table if necessary.
 * 
 * This version of the store function will update an entry if it exists.
 * 
 * @param data metrics data.
 * @param pa a standard database access stucture with database name and schema name.
 * @returns true if the data was successfully stored in the database.
 */
bool store_Metrics_data_pq(const Metrics_data & data, Postgres_access & pa) {
    ERRHERE(".setup");
    if (data.empty()) return false;

    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_DATA_PQ_RETURN(r) { PQfinish(conn); return r; }

    active_pq apq(conn,pa.pq_schemaname());
    if (!create_Metrics_table(apq, data.tablename, data.layout())) {
        STORE_DATA_PQ_RETURN(false);
    }

    std::string insertdatastr("INSERT INTO "+pa.pq_schemaname() + "." + data.tablename + " VALUES (" + data.all_values_pqstr() + ") ON CONFLICT (id) DO UPDATE SET data = "+data.datastr());
    if (!simple_call_pq(conn, insertdatastr)) {
        STORE_DATA_PQ_RETURN(false);
    }

    STORE_DATA_PQ_RETURN(true);
}

/**
 * Read a single data entry from a Metrics table in the database.
 * 
 * The `data` should specify `data.tablename` and a working `idstr()` method.
 * 
 * @param data Data structure that clearly identifies the data and receives the resulting text.
 * @param pa access data with database name and schema name.
 */
bool read_Metrics_data_pq(Metrics_data & data, Postgres_access & pa) {
    ERRHERE(".setup");
    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_DATA_PQ_RETURN(r) { PQfinish(conn); return r; }

    std::string pqcmdstr = "SELECT data FROM "+pa.pq_schemaname()+ "." + data.tablename+" WHERE id="+data.idstr();
    if (!query_call_pq(conn, pqcmdstr, false)) LOAD_DATA_PQ_RETURN(false);
  
    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res) < 1) {
            ADDERROR(__func__,"not enough fields in data result");
            LOAD_DATA_PQ_RETURN(false);
        }

        data.data.clear();
        for (int r = 0; r < rows; ++r) {

            data.data.append(PQgetvalue(res, r, 0));

        }

        PQclear(res);
    }

    LOAD_DATA_PQ_RETURN(true);
}

/**
 * Read all IDs from Metrics table in the database.
 * 
 * The `data` should specify `data.tablename`.
 * 
 * @param[in] data Data structure that clearly identifies the table in `data.tablename`.
 * @param[in] pa Access data with database name and schema name.
 * @param[out] ids Vector of ID strings, each of which can be parsed for its components.
 * @return True if successful.
 */
bool read_Metrics_IDs_pq(Metrics_data & data, Postgres_access & pa, std::vector<std::string> & ids) {
    ERRTRACE;
    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_DATA_PQ_RETURN(r) { PQfinish(conn); return r; }

    std::string pqcmdstr = "SELECT id FROM "+pa.pq_schemaname()+ "." + data.tablename;
    if (!query_call_pq(conn, pqcmdstr, false)) LOAD_DATA_PQ_RETURN(false);
  
    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res) < 1) {
            ADDERROR(__func__,"not enough fields in data result");
            LOAD_DATA_PQ_RETURN(false);
        }

        for (int r = 0; r < rows; ++r) {

            ids.emplace_back(PQgetvalue(res, r, 0));
            rtrim(ids.back()); // as the returned values seems to be space padded

        }

        PQclear(res);
    }

    LOAD_DATA_PQ_RETURN(true);
}

bool delete_Metricsdata_pq(const active_pq & apq, const Metrics_data & data) {
    ERRTRACE;
    if (!apq.conn)
        return false;

    std::string tstr("DELETE FROM "+apq.pq_schemaname+"." + data.tablename + " WHERE id = "+data.idstr());
    return simple_call_pq(apq.conn, tstr);
}

bool delete_Metrics_data_pq(const Metrics_data & data, Postgres_access & pa) {
    ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define DELETE_DATA_PQ_RETURN(r) { PQfinish(apq.conn); return r; }
    apq.pq_schemaname = pa.pq_schemaname();

    ERRHERE(".delete");
    if (!delete_Metricsdata_pq(apq, data)) DELETE_DATA_PQ_RETURN(false);

    DELETE_DATA_PQ_RETURN(true);
}

bool delete_Metrics_table_pq(const std::string& tablename, Postgres_access & pa) {
	ERRTRACE;
    active_pq apq;
    apq.conn = connection_setup_pq(pa.dbname());
    if (!apq.conn) return false;

    apq.pq_schemaname = pa.pq_schemaname();

    std::string tstr("DROP TABLE IF EXISTS "+apq.pq_schemaname+"." + tablename);
    bool r = simple_call_pq(apq.conn, tstr);

    PQfinish(apq.conn);
    return r;
}

bool count_Metrics_table_pq(Metrics_data & data, Postgres_access & pa) {
	ERRTRACE;
    pa.access_initialize();
    PGconn* conn = connection_setup_pq(pa.dbname());
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define COUNT_DATA_PQ_RETURN(r) { PQfinish(conn); return r; }

    std::string pqcmdstr = "SELECT count(*) AS exact_count FROM "+pa.pq_schemaname()+ "." + data.tablename;
    if (!query_call_pq(conn, pqcmdstr, false)) COUNT_DATA_PQ_RETURN(false);
  
    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res) < 1) {
            ADDERROR(__func__,"not enough fields in data result");
            COUNT_DATA_PQ_RETURN(false);
        }

        data.data.clear();
        for (int r = 0; r < rows; ++r) {

            data.data.append(PQgetvalue(res, r, 0));

        }

        PQclear(res);
    }

    COUNT_DATA_PQ_RETURN(true);
}

} // namespace fz
