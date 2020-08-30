// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Graph, Node aned Edge Postgres types for use with the Formalizer.
 * These define the authoritative version of the data structure for storage in PostgreSQL.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHPOSTGRES_HPP.
 */

/**
 * The functions and classes in this header are used to prepare Node and Edge data of a
 * Graph for storage in a Postgres database, and they are used to retrieve Node and
 * Edge data of a graph from the Postgres database.
 * 
 * The data structure in C++ is defined in Graphtypes.hpp. The data structure in
 * Postgres is defined here.
 * 
 * Functions are also provided to initialze Graph data in Postgres by converting a
 * complete Graph with all Nodes and Edges from another storage format to Postgres
 * format. At present, the only other format supported is the original HTML storage
 * format used in dil2al.
 * 
 * Even though this is C++, we use libpq here (not libpqxx) to communicate with the Postgres
 * database. The lipbq is the only standard library for Postgres and is perfectly usable
 * from C++.
 * 
 * This version is focused on simplicity and human readibility, not maximum speed. A
 * complete Graph is intialized relatively rarely. Unless automated tools generate new
 * Nodes and Edges very quickly, writing speed should not be a major issue.
 * 
 * Reading speed may require a different approach even now, because reading the whole
 * Graph into memory is a more frequent occurrence.
 * 
 * I don't actually need to make a particularly fast table builder. After all, you typically
 * only rarely need to create a new table. Mostly, you just add single rows to the table
 * (be it the Graph table, the Task Log table, etc).
 * 
 * 
 * You could also use COPY (https://www.postgresql.org/docs/9.6/sql-copy.html) to copy
 * data between database and file.
 * 
 */

#ifndef __GRAPHPOSTGRES_HPP
#include "coreversion.hpp"
#define __GRAPHPOSTGRES_HPP (__COREVERSION_HPP)

#include "Graphtypes.hpp"

/**
 * On Ubuntu, to install the libpq libraries, including the libpq-fe.h header file,
 * do: sudo apt-get install libpq-dev
 * You may also have to add /usr/include or /usr/include/postgresql to the CPATH or
 * to the includes in the Makefile, e.g. -I/usr/include/postgresql.
 */
#include <libpq-fe.h>

namespace fz {

enum pq_Tfields { pqt_id, pqt_supid, pqt_tag, pqt_title, pqt_keyword, pqt_relevance, _pqt_NUM };
enum pq_Nfields { pqn_id, pqn_topics, pqn_topicrelevance, pqn_valuation, pqn_completion, pqn_required, pqn_text, pqn_targetdate, pqn_tdproperty, pqn_isperiodic, pqn_tdperiodic, pqn_tdevery, pqn_tdspan, _pqn_NUM };
enum pq_Efields { pqe_id, pqe_dependency, pqe_significance, pqe_importance, pqe_urgency, pqe_priority, _pqe_NUM };

#define DEFAULT_PQ_SCHEMANAME "formalizer"
extern std::string pq_schemaname;

PGconn* connection_setup_pq(std::string dbname);

bool simple_call_pq(PGconn* conn, std::string astr);

bool query_call_pq(PGconn* conn, std::string qstr, bool request_single_row_mode);

int sample_query_data(PGconn *conn, unsigned int rstart, unsigned int rend, unsigned int cstart, unsigned int cend, std::string &databufstr);

bool create_Formalizer_schema_pq(PGconn* conn);

bool create_Enum_Types_pq(PGconn* conn);

bool create_Nodes_table_pq(PGconn* conn);

bool create_Edges_table_pq(PGconn* conn);

bool add_Topic_pq(PGconn *conn, const Topic *topic);

bool add_Node_pq(PGconn *conn, const Node *node);

bool add_Edge_pq(PGconn *conn, const Edge *edge);

bool store_Graph_pq(const Graph& graph, std::string dbname, void (*progressfunc)(unsigned long, unsigned long) = NULL);

bool load_Graph_pq(Graph& graph, std::string dbname);

std::vector<std::string> load_Node_parameter_interval(std::string dbname, pq_Nfields param, unsigned long from_row, unsigned long num_rows);

std::vector<std::string> load_Edge_parameter_interval(std::string dbname, pq_Efields param, unsigned long from_row, unsigned long num_rows);

inline std::string TimeStamp_pq(time_t t) {
    if (t<0) return "'infinity'";
    return TimeStamp("'%Y%m%d %H:%M'",t);
}

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
 * A data types conversion helper class that can deliver the Postgres Topics table
 * equivalent INSERT value expression for all data content in a Topic.
 * 
 * This helper class does not modify the Topic in any way.
 */
class Topic_pq {
protected:
    const Topic* topic; // pointer to a (const Topic), i.e. *topic is treated as constant
public:
    Topic_pq(const Topic* _topic): topic(_topic) {} // See Trello card about (const type)* pointers.

    std::string id_pqstr();
    std::string supid_pqstr();
    std::string tag_pqstr();
    std::string title_pqstr();
    std::string All_Topic_keyword_pqstr();
    std::string All_Topic_relevance_pqstr();
    std::string All_Topic_Data_pqstr();
};

/**
 * A data types conversion helper class that can deliver the Postgres Node table
 * equivalent INSERT value expression for all data content in a Node.
 * 
 * This helper class does not modify the Node in any way.
 */
class Node_pq {
protected:
    const Node* node; // pointer to a (const Node), i.e. *node is treated as constant
public:
    Node_pq(const Node* _node): node(_node) {} // See Trello card about (const type)* pointers.

    std::string id_pqstr();
    std::string topics_pqstr();
    std::string topicrelevance_pqstr();
    std::string valuation_pqstr();
    std::string completion_pqstr();
    std::string required_pqstr();
    std::string text_pqstr();
    std::string targetdate_pqstr();
    std::string tdproperty_pqstr();
    std::string isperiodic_pqstr();
    std::string tdperiodic_pqstr();
    std::string tdevery_pqstr();
    std::string tdspan_pqstr();
    std::string All_Node_Data_pqstr();
};

/**
 * A data types conversion helper class that can deliver the Postgres Edge table
 * equivalent INSERT value expression for all data content in an Edge.
 * 
 * This helper class does not modify the Edge in any way.
 */
class Edge_pq {
protected:
    const Edge* edge; // pointer to a (const Edge), i.e. *edge is treated as constant
public:
    Edge_pq(const Edge* _edge): edge(_edge) {} // See Trello card about (const type)* pointers.

    std::string id_pqstr();
    std::string dependency_pqstr();
    std::string significance_pqstr();
    std::string importance_pqstr();
    std::string urgency_pqstr();
    std::string priority_pqstr();
    std::string All_Edge_Data_pqstr();
};

} // namespace fz

#endif // __GRAPHPOSTGRES_HPP
