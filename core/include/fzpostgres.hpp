// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header provides basic Postgres access within the Formalizer environment context.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __FZPOSTGRES_HPP.
 */

#ifndef __FZPOSTGRES_HPP
#include "coreversion.hpp"
#define __FZPOSTGRES_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <cctype>
#include <vector>

// core
//#include "error.hpp"
#include "standard.hpp"
#include "config.hpp"
#include "TimeStamp.hpp"

/**
 * On Ubuntu, to install the libpq libraries, including the libpq-fe.h header file,
 * do: sudo apt-get install libpq-dev
 * You may also have to add /usr/include or /usr/include/postgresql to the CPATH or
 * to the includes in the Makefile, e.g. -I/usr/include/postgresql.
 */
#include <libpq-fe.h>

/// Default Formalizer Postgres database name. See https://trello.com/c/Lww33Lym.
#ifndef DEFAULT_DBNAME
    #define DEFAULT_DBNAME "formalizer"
#endif
/// Default Formalizer Postgres schema name. See https://trello.com/c/Lww33Lym.
#ifndef DEFAULT_PQ_SCHEMANAME
    #define DEFAULT_PQ_SCHEMANAME "formalizeruser"
#endif

namespace fz {

//extern std::string pq_schemaname; // *** This is now being supplied through standard.hpp:Graph_access::pq_schemaname

PGconn* connection_setup_pq(std::string dbname);

bool simple_call_pq(PGconn* conn, std::string astr);

bool query_call_pq(PGconn* conn, std::string qstr, bool request_single_row_mode);

int sample_query_data(PGconn *conn, unsigned int rstart, unsigned int rend, unsigned int cstart, unsigned int cend, std::string &databufstr);

bool create_Formalizer_schema_pq(PGconn* conn, std::string schemaname);

std::vector<std::string> array_from_pq(std::string pq_array_str);

/// Uses the UTC / Local Time convention built into TimeStamp().
inline std::string TimeStamp_pq(time_t t) {
    if (t<0) return "'infinity'";
    return TimeStamp("'%Y%m%d %H:%M'",t);
}

time_t epochtime_from_timestamp_pq(std::string pqtimestamp);

/**
 * A simulation class that enables Postgres call testing.
 * 
 * Call the SimulateChanges() member function to activate simulation.
 * Call the ActualChanges() member function to enable Postgres changes.
 * The default state is to make actual Postgres changes.
 * 
 * Use the global instantiation SimPQ.
 */
class Simulate_PQ_Changes {
protected:
    bool simulate_pq_changes = false; /// Set this to test Postgres calls.
    std::string simulated_pq_calls; /// Postgres calls are appended here when simulated.

public:
    void SimulateChanges() { simulate_pq_changes = true; }
    void ActualChanges() { simulate_pq_changes = false; }
    bool SimulatingPQChanges() const { return simulate_pq_changes; }
    void AddToSimLog(std::string & pqcall) { simulated_pq_calls += pqcall + '\n'; }
    bool SimPQChangesAndLog(std::string & pqcall) {
        if (simulate_pq_changes) AddToSimLog(pqcall);
        return simulate_pq_changes;
    }
    std::string & GetLog() { return simulated_pq_calls; }
};

extern Simulate_PQ_Changes SimPQ;

/**
 * Communication info for active database connection and specified schema.
 */
struct active_pq {
    PGconn* conn;
    std::string pq_schemaname;

    active_pq(): conn(nullptr) {}
    active_pq(PGconn * _conn, std::string _schemaname): conn(_conn), pq_schemaname(_schemaname) {}
};

class fzpq_configurable: public configurable {
public:
    fzpq_configurable(formalizer_standard_program & fsp);

    bool set_parameter(const std::string & parlabel, const std::string & parvalue);

    std::string dbname;
    std::string pq_schemaname;

};

/**
 * A standardized way to access the Formalizer database.
 * 
 */
struct Postgres_access {
    fzpq_configurable config;

    bool is_server; /// authoritative server programs should set this flag

    /**
     * Carry out initializations needed to enable access to the Postgres database.
     * 
     * @param add_option_args_here receiving string where "d:s:" are appended to extend
     *                             command line options recognized.
     * @param add_usage_top_here receiving string where the option format is appended
     *                           to extend the top line of usage output.
     */
    Postgres_access(formalizer_standard_program & fsp, std::string & add_option_args_here, std::string & add_usage_top_here, bool _isserver = false): config(fsp), is_server(_isserver) {
        //COMPILEDPING(std::cout, "PING-Graph_access().1\n");
        add_option_args_here += "d:s:";
        add_usage_top_here += " [-d <dbname>] [-s <schemaname>]";
    }

    void access_initialize(); ///< Call this before attempting to open a database connection.

    void usage_hook();

    bool options_hook(char c, std::string cargs);

    // Providing these to shield users of fzpostrges from further complexities about
    // where the actual dbname and pq_schemaname variables reside.
    const std::string & dbname() { return config.dbname; }
    const std::string & pq_schemaname() { return config.pq_schemaname; }

protected:
    void dbname_error();
    void schemaname_error();
//public:

};


} // namespace fz

#endif // __FZPOSTGRES_HPP
