// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <numeric>
#include <iomanip>
#include <sstream>
#include <algorithm>

// core
#include "error.hpp"
#include "general.hpp"
#include "Graphtypes.hpp"
#include "fzpostgres.hpp"
#include "Graphpostgres.hpp"


namespace fz {

/**
 * Notes about the Postgres Graph layout:
 * 1. The Node id is presently limited to 16 characters. That limit could become a
 *    problematic constraint if there are tools that automatically generate more than
 *    9 new Nodes per second. In that case, the data layout can be updated to a version
 *    that has more room (and conversion from one format to the other can be done).
 *    Alternatively, the id could be implemented as a decimal number or a user defined
 *    type.
 * 2. The Postgres layout is presently optimized for human readability directly in the
 *    database table. If it turns out that the conversion overhead (e.g. from target
 *    date time stamps to time_t seconds) is problematic then it's worth experimenting
 *    with reloading speed optimized formats.
 * 3. There are many options for how to store the Edges between Nodes. In dil2al, they
 *    were stored in a separate file (DIL-by-ID), but grouped by Dependency Node as a
 *    list of Superiors. They could be contained directly in the Nodes storage by
 *    using variable length arrays containing Edge information. The method being
 *    tested in this version is one where Edges are stored as a separate table,
 *    with the Superior target Node, concatenated with the Dependency source Node, as
 *    the primary key. The data structure of the in-memory Graph is a separate matter.
 *    Comments about the different possibilities are documented on the Formalizer
 *    Refactor Trello board.
 */

/// Table layout for Graph structure with Nodes and Edges

// This can be provided at compile-time with a -D option.
#ifndef DEFAULT_PQ_SCHEMANAME
    #define DEFAULT_PQ_SCHEMANAME "formalizeruser"
#endif

//std::string pq_schemaname(DEFAULT_PQ_SCHEMANAME); // This is now in standard.hpp

std::string pq_enum_td_property("('unspecified','inherit','variable','fixed','exact')");
std::string pq_enum_td_pattern("('patt_daily','patt_workdays','patt_weekly','patt_biweekly','patt_monthly','patt_endofmonthoffset','patt_yearly','OLD_patt_span','patt_nonperiodic')");

std::string pq_nodelayout(
    "id char(16) PRIMARY KEY,"
    "topics smallint[],"
    "topicrelevance real[],"
    "valuation real,"
    "completion real,"
    "required integer,"
    "text text,"
    "targetdate timestamp (0),"
    "tdproperty td_property,"
    "isperiodic boolean,"
    "tdperiodic td_pattern,"
    "tdevery integer,"
    "tdspan integer"
    );

std::string pq_edgelayout(
    "id char(33)," // e.g. 20200601093905.1>20140428115038.1
    "dependency real,"
    "significance real," // (also known as unbounded importance)
    "importance real,"   // (also known as bounded importance)
    "urgency real,"      // (also known as computed urgency)
    "priority real"     // (also known as computed priority)
    );

std::string pq_topiclayout(
    "id smallint,"
    "supid smallint,"
    "tag text,"
    "title text,"
    "keyword text[],"
    "relevance real[]");


/**
 * Create enumerated types in database for Node and Edge data.
 * 
 * @param conn active database connection.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @return true if types were successfully created.
 */
bool create_Enum_Types_pq(PGconn* conn, std::string schemaname) {
    ERRHERE(".1");
    std::string pq_maketype("CREATE TYPE "+schemaname+".td_property AS ENUM "+pq_enum_td_property);
    if (!simple_call_pq(conn,pq_maketype)) return false;
    ERRHERE(".2");
    pq_maketype = "CREATE TYPE "+schemaname+".td_pattern AS ENUM "+pq_enum_td_pattern;
    return simple_call_pq(conn,pq_maketype);
}

/**
 * Create a new database table for Topics.
 * 
 * @param conn active database connection.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @return true if table was successfully created.
 */
bool create_Topics_table_pq(PGconn *conn, std::string schemaname) {
    ERRHERE(".1");
    std::string pq_maketable("CREATE TABLE "+schemaname+".Topics ("+pq_topiclayout+')');
    return simple_call_pq(conn,pq_maketable);
}

/**
 * Create a new database table for Nodes.
 * 
 * @param conn active database connection.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @return true if table was successfully created.
 */
bool create_Nodes_table_pq(PGconn* conn, std::string schemaname) {
    ERRHERE(".1");
    // modify the node layout to include the schema name in the user defined type names
    auto pos1 = pq_nodelayout.find("td_property");
    std::string pqnlayout(pq_nodelayout.substr(0,pos1)+schemaname+'.');
    auto pos2 = pq_nodelayout.find("td_pattern",pos1+3);
    pqnlayout += pq_nodelayout.substr(pos1,pos2-pos1)+schemaname+'.';
    pqnlayout += pq_nodelayout.substr(pos2);
    std::string pq_maketable("CREATE TABLE "+schemaname+".Nodes ("+pqnlayout+')');
    return simple_call_pq(conn,pq_maketable);
}

/**
 * Create a new database table for Edges.
 * 
 * @param conn active database connection.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @return true if table was successfully created.
 */
bool create_Edges_table_pq(PGconn* conn, std::string schemaname) {
    ERRHERE(".1");
    std::string pq_maketable("CREATE TABLE "+schemaname+".Edges ("+pq_edgelayout+')');
    return simple_call_pq(conn,pq_maketable);
}

bool add_Topic_pq(PGconn *conn, std::string schemaname, const Topic *topic) {
    ERRHERE(".1");
    if (!topic)
        ERRRETURNFALSE(__func__, "unable to add a NULL Topic");

    Topic_pq tpq(topic);
    std::string tstr("INSERT INTO "+schemaname + ".Topics VALUES "+ tpq.All_Topic_Data_pqstr());
    return simple_call_pq(conn,tstr);
}

bool add_Node_pq(PGconn *conn, std::string schemaname, const Node *node) {
    ERRHERE(".1");
    if (!node)
        ERRRETURNFALSE(__func__, "unable to add a NULL Node");

    Node_pq npq(node);
    std::string nstr("INSERT INTO " + schemaname + ".Nodes VALUES " + npq.All_Node_Data_pqstr());
    return simple_call_pq(conn, nstr);
}

bool add_Edge_pq(PGconn *conn, std::string schemaname, const Edge *edge) {
    ERRHERE(".1");
    if (!edge)
        ERRRETURNFALSE(__func__, "unable to add a NULL Edge");

    Edge_pq epq(edge);
    std::string estr("INSERT INTO " + schemaname + ".Edges VALUES " + epq.All_Edge_Data_pqstr());
    return simple_call_pq(conn, estr);
}

/**
 * Store all the Nodes and Edges of the Graph in the PostgreSQL database.
 * 
 * @param graph a Graph containing all of the Nodes and Edges.
 * @param dbname database name.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @param progress_func points to an optional progress indicator function.
 * @returns true if the Graph was successfully stored in the database.
 */
bool store_Graph_pq(const Graph& graph, std::string dbname, std::string schemaname, void (*progress_func)(unsigned long, unsigned long)) {
    ERRHERE(".setup");
    PGconn* conn = connection_setup_pq(dbname);
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define STORE_GRAPH_PQ_RETURN(r) { PQfinish(conn); return r; }

    ERRHERE(".schema");
    if (!create_Formalizer_schema_pq(conn, schemaname)) STORE_GRAPH_PQ_RETURN(false);

    ERRHERE(".enum");
    if (!create_Enum_Types_pq(conn, schemaname)) STORE_GRAPH_PQ_RETURN(false);

    ERRHERE(".topicstable");
    if (!create_Topics_table_pq(conn, schemaname)) STORE_GRAPH_PQ_RETURN(false);

    ERRHERE(".topics");
    unsigned long n = graph.get_topics().get_topictags().size();
    unsigned long ncount = 0;
    for (auto topic = graph.get_topics().get_topictags().begin(); topic != graph.get_topics().get_topictags().end(); ++topic) {
        if (!add_Topic_pq(conn, schemaname,(*topic))) STORE_GRAPH_PQ_RETURN(false);
        ncount++;
        if (progress_func) (*progress_func)(n,ncount);
    }

    ERRHERE(".nodestable");
    if (!create_Nodes_table_pq(conn, schemaname)) STORE_GRAPH_PQ_RETURN(false);

    ERRHERE(".nodes");
    n = graph.num_Nodes();
    ncount = 0;
    for (auto node = graph.begin_Nodes(); node != graph.end_Nodes(); ++node) {
        if (!add_Node_pq(conn, schemaname,node->second)) STORE_GRAPH_PQ_RETURN(false);
        ncount++;
        if (progress_func) (*progress_func)(n,ncount);
    }

    ERRHERE(".edgestable");
    if (!create_Edges_table_pq(conn, schemaname)) STORE_GRAPH_PQ_RETURN(false);

    ERRHERE(".edges");
    n = graph.num_Edges();
    ncount = 0;
    for (auto edge = graph.begin_Edges(); edge != graph.end_Edges(); ++edge) {
        if (!add_Edge_pq(conn, schemaname,edge->second)) STORE_GRAPH_PQ_RETURN(false);
        ncount++;
        if (progress_func) (*progress_func)(n,ncount);
    }

    //*** Make an inventory of what other bits of information need a corresponding version in the
    //*** database format, e.g. possibly caches of up and down edges lists, etc.

    STORE_GRAPH_PQ_RETURN(true);
}

const std::string pq_topic_fieldnames[_pqt_NUM] = {"id",
                                                   "supid",
                                                   "tag",
                                                   "title",
                                                   "keyword",
                                                   "relevance"};
const std::string pq_node_fieldnames[_pqn_NUM] = {"id",
                                                  "topics",
                                                  "topicrelevance",
                                                  "valuation",
                                                  "completion",
                                                  "required",
                                                  "text",
                                                  "targetdate",
                                                  "tdproperty",
                                                  "isperiodic",
                                                  "tdperiodic",
                                                  "tdevery",
                                                  "tdspan"};
const std::string pq_edge_fieldnames[_pqe_NUM] = {"id",
                                                  "dependency",
                                                  "significance",
                                                  "importance",
                                                  "urgency",
                                                  "priority"};
unsigned int pq_topic_field[_pqt_NUM];
unsigned int pq_node_field[_pqn_NUM];
unsigned int pq_edge_field[_pqe_NUM];

/**
 * Retrieve field column numbers for topics query to make sure the
 * correct field numbers are used. This is an extra safety measure
 * in case formats are changed in the future and in case of potential
 * database version mismatch.
 * 
 * This function updates the field numbers in pq_topic_field.
 * The field names that this version assumes are in the variable
 * pq_topic_fieldnames. The fields are enumerated with pq_Tfields.
 * 
 * @param res a valid pointer obtained by PQgetResult().
 * @return true if all the field names were found.
 */
bool get_Topic_pq_field_numbers(PGresult *res) {
    if (!res) return false;

    for (auto i = 0; i<_pqt_NUM; i++) {
        if ((pq_topic_field[i] = PQfnumber(res,pq_topic_fieldnames[i].c_str())) < 0) {
            ERRRETURNFALSE(__func__,"field '"+pq_topic_fieldnames[i]+"' not found in database topics table");
        }
    }
    return true;
}

bool get_Node_pq_field_numbers(PGresult *res) {
    if (!res) return false;

    for (auto i = 0; i<_pqn_NUM; i++) {
        if ((pq_node_field[i] = PQfnumber(res,pq_node_fieldnames[i].c_str())) < 0) {
            ERRRETURNFALSE(__func__,"field '"+pq_node_fieldnames[i]+"' not found in database nodes table");
        }
    }
    return true;
}

bool get_Edge_pq_field_numbers(PGresult *res) {
    if (!res) return false;

    for (auto i = 0; i<_pqe_NUM; i++) {
        if ((pq_edge_field[i] = PQfnumber(res,pq_edge_fieldnames[i].c_str())) < 0) {
            ERRRETURNFALSE(__func__,"field '"+pq_edge_fieldnames[i]+"' not found in database edges table");
        }
    }
    return true;
}

/**
 * Convert textual arrays of keywords and keyword-relevance values
 * to a vector of Topic_Keyword pairs.
 * 
 * Keywords are allowed to contain more than one word, separated
 * by spaces. This function strips away the front and back
 * double-quotes that the PGgetvalue() function returns around
 * such composite keywords.
 * 
 * @param keywordstr string containing the textual array of keywords.
 * @param relevancestr string containing the textual array of relevance values.
 * @return vector of Topic_Keyword pairs of (keyword,relevance).
 */
std::vector<Topic_Keyword> keyrel_from_pq(std::string keywordstr, std::string relevancestr) {
    std::vector<Topic_Keyword> krels;
    auto kvec = array_from_pq(keywordstr);
    auto rvec = array_from_pq(relevancestr);
    if (kvec.size()!=rvec.size()) {
        ADDERROR(__func__,"number of keywords does not match number of relevance values");
        return krels;
    }
    for (long unsigned int i = 0; i < kvec.size(); ++i) {
        if ((kvec[i].front()=='"') && (kvec[i].back()=='"')) {
            kvec[i].erase(0,1);
            kvec[i].pop_back();
        }
        krels.emplace_back(kvec[i],std::stof(rvec[i]));
    }
    return krels;
}

bool read_Topics_pq(PGconn* conn, std::string schemaname, Topic_Tags & topictags) {
    if (!query_call_pq(conn,"SELECT * FROM "+schemaname+".topics ORDER BY "+pq_topic_fieldnames[pqt_id],false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);

    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<_pqt_NUM) ERRRETURNFALSE(__func__,"not enough fields in topic table");

        if (!get_Topic_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            std::string tag = PQgetvalue(res, r, pq_topic_field[pqt_tag]);
            std::string title = PQgetvalue(res, r, pq_topic_field[pqt_title]);
            int id = std::atoi(PQgetvalue(res,r, pq_topic_field[pqt_id]));
            int new_id = topictags.find_or_add_Topic(tag,title);
            if (id!=new_id) ERRRETURNFALSE(__func__,"stored topic id does not match newly generated id");

            Topic * topic = topictags.get_topictags()[id];
            int supid = std::atoi(PQgetvalue(res,r, pq_topic_field[pqt_supid]));
            if (id!=supid) topic->set_supid(supid);

            std::vector<Topic_Keyword> * tkr = const_cast<std::vector<Topic_Keyword> *>(&topic->get_keyrel()); // explicitly making this modifiable
            *tkr = keyrel_from_pq(PQgetvalue(res, r, pq_topic_field[pqt_keyword]),PQgetvalue(res, r, pq_topic_field[pqt_relevance]));

        }

        PQclear(res);
    }

    return true;
}

bool node_topics_from_pq(Node & node, std::string topicsstr, std::string topicrelevancestr) {
    auto tvec = array_from_pq(topicsstr);
    auto trvec = array_from_pq(topicrelevancestr);

    if (tvec.size()!=trvec.size()) {
        ERRRETURNFALSE(__func__,"number of topics does not match number of topic relevance values for Node ["+node.get_id().str()+']');
    }

    for (long unsigned int i = 0; i < tvec.size(); ++i) {
        node.add_topic(atoi(tvec[i].c_str()),std::stof(trvec[i])); // ** maybe try-catch std::stoi instead
    }

    return true;
}

const std::map<std::string, td_property> tdprop_by_pqtdprop = {{"unspecified", td_property::unspecified},
                                                               {"inherit", td_property::inherit},
                                                               {"variable", td_property::variable},
                                                               {"fixed", td_property::fixed},
                                                               {"exact", td_property::exact}};

const std::map<std::string, td_pattern> tdpatt_by_pqtdpatt = {{"patt_daily",td_pattern::patt_daily},
                                                              {"patt_workdays",td_pattern::patt_workdays},
                                                              {"patt_weekly",td_pattern::patt_weekly},
                                                              {"patt_biweekly",td_pattern::patt_biweekly},
                                                              {"patt_monthly",td_pattern::patt_monthly},
                                                              {"patt_endofmonthoffset",td_pattern::patt_endofmonthoffset},
                                                              {"patt_yearly",td_pattern::patt_yearly},
                                                              {"OLD_patt_span",td_pattern::OLD_patt_span},
                                                              {"patt_nonperiodic",td_pattern::patt_nonperiodic}};

td_property tdproperty_from_pq(std::string pqtdproperty) {
    try {
        td_property tdprop = tdprop_by_pqtdprop.at(pqtdproperty);
        return tdprop;
    } catch (const std::out_of_range& oor) {
        ADDERROR(__func__,"tdprop_by_pqtdprop key out of range: "+std::string(oor.what()));
        return td_property::unspecified;
    }
}

td_pattern tdpattern_from_pq(std::string pqtdpattern) {
    try {
        td_pattern tdpatt = tdpatt_by_pqtdpatt.at(pqtdpattern);
        return tdpatt;
    } catch (const std::out_of_range& oor) {
        ADDERROR(__func__,"tdpatt_by_pqtdpatt key out of range: "+std::string(oor.what()));
        return td_pattern::patt_nonperiodic;
    }
}

bool read_Nodes_pq(PGconn* conn, std::string schemaname, Graph & graph) {
    if (!query_call_pq(conn,"SELECT * FROM "+schemaname+".nodes ORDER BY "+pq_node_fieldnames[pqn_id],false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);
  
    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<_pqn_NUM) ERRRETURNFALSE(__func__,"not enough fields in nodes table");

        if (!get_Node_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            std::string id = PQgetvalue(res, r, pq_node_field[pqn_id]);
            try {
                Node * node = new Node(id);

                if (!graph.add_Node(node)) {
                    if (graph.error == Graph::g_adddupnode) {
                        ERRRETURNFALSE(__func__,"duplicate Node ["+id+']');
                    } else {
                        ERRRETURNFALSE(__func__,"unknown error while attempting to add Node");
                    }
                }

                if (!node_topics_from_pq(*node,PQgetvalue(res, r, pq_node_field[pqn_topics]),PQgetvalue(res, r, pq_node_field[pqn_topicrelevance]))) {
                    return false;
                }

                node->set_valuation(std::stof(PQgetvalue(res, r, pq_node_field[pqn_valuation])));
                node->set_completion(std::stof(PQgetvalue(res, r, pq_node_field[pqn_completion])));
                node->set_required(atoi(PQgetvalue(res, r, pq_node_field[pqn_required])));
                node->set_text_unchecked(PQgetvalue(res, r, pq_node_field[pqn_text]));
                node->set_targetdate(targetdate_from_timestamp_pq(PQgetvalue(res, r, pq_node_field[pqn_targetdate])));
                node->set_tdproperty(tdproperty_from_pq(PQgetvalue(res, r, pq_node_field[pqn_tdproperty])));
                node->set_repeats(PQgetvalue(res, r, pq_node_field[pqn_isperiodic])[0]=='t');
                node->set_tdpattern(tdpattern_from_pq(PQgetvalue(res, r, pq_node_field[pqn_tdperiodic])));
                node->set_tdevery(atoi(PQgetvalue(res, r, pq_node_field[pqn_tdevery])));
                node->set_tdspan(atoi(PQgetvalue(res, r, pq_node_field[pqn_tdspan])));

            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__,"Invalid Node ID ["+id+"], "+idexception.what());
            }

        }

        PQclear(res);
    }

    return true;
}

bool read_Edges_pq(PGconn* conn, std::string schemaname, Graph & graph) {
    if (!query_call_pq(conn,"SELECT * FROM "+schemaname+".edges ORDER BY "+pq_edge_fieldnames[pqe_id],false)) return false;

    //sample_query_data(conn,0,4,0,100,tmpout);

    PGresult *res;

    while ((res = PQgetResult(conn))) { // It's good to use a loop for single row mode cases.

        const int rows = PQntuples(res);
        if (PQnfields(res)<_pqe_NUM) ERRRETURNFALSE(__func__,"not enough fields in edges table");

        if (!get_Edge_pq_field_numbers(res)) return false;

        for (int r = 0; r < rows; ++r) {

            std::string id = PQgetvalue(res, r, pq_edge_field[pqe_id]);
            try {
                Edge * edge = new Edge(graph,id); // Adding to graph already happens in this call.

                edge->set_dependency(std::stof(PQgetvalue(res, r, pq_edge_field[pqe_dependency])));
                edge->set_importance(std::stof(PQgetvalue(res, r, pq_edge_field[pqe_importance])));
                edge->set_priority(std::stof(PQgetvalue(res, r, pq_edge_field[pqe_priority])));
                edge->set_significance(std::stof(PQgetvalue(res, r, pq_edge_field[pqe_significance])));
                edge->set_urgency(std::stof(PQgetvalue(res, r, pq_edge_field[pqe_urgency])));

            } catch (ID_exception idexception) {
                ERRRETURNFALSE(__func__,"Invalid Edge ID ["+id+"], "+idexception.what());
            }

        }

        PQclear(res);
    }


    return true;
}

/**
 * Load all the Nodes, Edges and Topics of the Graph from the PostgreSQL database.
 * 
 * @param graph a Graph for the Nodes and Edges, etc, typically empty.
 * @param dbname database name.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname) 
 * @returns true if the Graph was successfully loaded from the database.
 */
bool load_Graph_pq(Graph& graph, std::string dbname, std::string schemaname) {

    ERRHERE(".setup");
    PGconn* conn = connection_setup_pq(dbname);
    if (!conn) return false;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_GRAPH_PQ_RETURN(r) { PQfinish(conn); return r; }

    ERRHERE(".topics");
    Topic_Tags * Ttags = const_cast<Topic_Tags *>(&graph.get_topics()); // explicitly make this modifiable here
    if (!read_Topics_pq(conn,schemaname, *Ttags)) LOAD_GRAPH_PQ_RETURN(false);

    ERRHERE(".nodes");
    if (!read_Nodes_pq(conn,schemaname, graph)) LOAD_GRAPH_PQ_RETURN(false);

    ERRHERE(".edges");
    if (!read_Edges_pq(conn,schemaname, graph)) LOAD_GRAPH_PQ_RETURN(false);

    LOAD_GRAPH_PQ_RETURN(true);

}

/**
 * Load specific Node parameter column interval from PostgreSQL database.
 * 
 * This interface attempts to hide as much as possible about the Postgres specifics
 * of the operation, in order to preserve a Formalizer database access protocol
 * across different possible underlying database choices.
 * 
 * Note: For this reason, the lowest possible interval start is 0 (not 1, as per
 * SQL row numbering convention).
 * 
 * @param dbname the Postgres database.
 * @param schemaname Formalizer schema name (usually Graph_access::pq_schemaname)
 * @param param the enumerated parameter identifier.
 * @param from_row the first row in the interval, counting from 0.
 * @param num_rows the number of rows in the intervial.
 * @return a vector with string elements, one for each row.
 */
std::vector<std::string> load_Node_parameter_interval(std::string dbname, std::string schemaname, pq_Nfields param, unsigned long from_row, unsigned long num_rows) {
    std::vector<std::string> v;
    if (param>=_pqn_NUM) {
        ADDERROR(__func__,"unknown pq_Nfields enumerated parameter ("+std::to_string(param)+')');
        return v;
    }
    if (num_rows==0) return v;

    PGconn *conn = connection_setup_pq(dbname);
    if (!conn) return v;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_NODE_PARAMETER_INTERVAL_RETURN(v) { PQfinish(conn); return v; }

    PGresult *res = NULL;
    int rows = 0;
    // Instead of "LIMIT n" this could use "FETCH FIRST n ROW ONLY".
    //if (query_call_pq(conn, "SELECT "+pq_node_fieldnames[param]+" FROM " + schemaname + ".nodes OFFSET "+std::to_string(from_row)+" LIMIT "+std::to_string(num_rows), false)) {
    if (query_call_pq(conn, "SELECT "+pq_node_fieldnames[param]+" FROM " + schemaname + ".nodes ORDER BY "+pq_node_fieldnames[pqn_id]+" OFFSET "+std::to_string(from_row)+" LIMIT "+std::to_string(num_rows), false)) {
        res = PQgetResult(conn);
        if (res) {
            rows = PQntuples(res);
        } else {
            LOAD_NODE_PARAMETER_INTERVAL_RETURN(v);
        }
    }

    for (int r = 0; r<rows; ++r) {
        v.emplace_back(PQgetvalue(res,r,0));
    }
    PQclear(res);

    LOAD_NODE_PARAMETER_INTERVAL_RETURN(v);
}

/**
 * Load specific Edge parameter column interval from PostgreSQL database.
 * 
 * This interface attempts to hide as much as possible about the Postgres specifics
 * of the operation, in order to preserve a Formalizer database access protocol
 * across different possible underlying database choices.
 * 
 * Note: For this reason, the lowest possible interval start is 0 (not 1, as per
 * SQL row numbering convention).
 * 
 * @param dbname the Postgres database.
 * @param schemaname the Formalizer schema name (usually provided by Graph_access::pq_schemaname)
 * @param param the enumerated parameter identifier.
 * @param from_row the first row in the interval, counting from 0.
 * @param num_rows the number of rows in the intervial.
 * @return a vector with string elements, one for each row.
 */
std::vector<std::string> load_Edge_parameter_interval(std::string dbname, std::string schemaname, pq_Efields param, unsigned long from_row, unsigned long num_rows) {
    std::vector<std::string> v;
    if (param>=_pqe_NUM) {
        ADDERROR(__func__,"unknown pq_Efields enumerated parameter ("+std::to_string(param)+')');
        return v;
    }
    if (num_rows==0) return v;

    PGconn *conn = connection_setup_pq(dbname);
    if (!conn) return v;

    // Define a clean return that closes the connection to the database and cleans up.
    #define LOAD_EDGE_PARAMETER_INTERVAL_RETURN(v) { PQfinish(conn); return v; }

    PGresult *res = NULL;
    int rows = 0;
    // Instead of "LIMIT n" this could use "FETCH FIRST n ROW ONLY".
    //if (query_call_pq(conn, "SELECT "+pq_node_fieldnames[param]+" FROM " + schemaname + ".nodes OFFSET "+std::to_string(from_row)+" LIMIT "+std::to_string(num_rows), false)) {
    if (query_call_pq(conn, "SELECT "+pq_edge_fieldnames[param]+" FROM " + schemaname + ".edges ORDER BY "+pq_edge_fieldnames[pqe_id]+" OFFSET "+std::to_string(from_row)+" LIMIT "+std::to_string(num_rows), false)) {
        res = PQgetResult(conn);
        if (res) {
            rows = PQntuples(res);
        } else {
            LOAD_EDGE_PARAMETER_INTERVAL_RETURN(v);
        }
    }

    for (int r = 0; r<rows; ++r) {
        v.emplace_back(PQgetvalue(res,r,0));
    }
    PQclear(res);

    LOAD_EDGE_PARAMETER_INTERVAL_RETURN(v);
}

// ======================================
// Definitions of class member functions:
// ======================================

/// Return the topic tag id number.
std::string Topic_pq::id_pqstr() {
    return std::to_string(topic->get_id());
}

/// Return the optional superior topic tag id number.
std::string Topic_pq::supid_pqstr() {
    return std::to_string(topic->get_supid());
}

/// Return the tag string between dollar-quoted tags that prevent any issues with characters in the tag.
std::string Topic_pq::tag_pqstr() {
    // Using the Postgres dollar-quoted tag method means no need to escape any characters within the text!
    return "$txt$"+topic->get_tag()+"$txt$";
}

/// Return the title string between dollar-quoted tags that prevent any issues with characters in the title.
std::string Topic_pq::title_pqstr() {
    // Using the Postgres dollar-quoted tag method means no need to escape any characters within the text!
    return "$txt$"+topic->get_title()+"$txt$";
}

/// Return the comma delimited string of keywords inside the ARRAY[] constructor.
std::string Topic_pq::All_Topic_keyword_pqstr() {
    const std::vector<Topic_Keyword> &k = topic->get_keyrel();
    if (k.size()<1) {
        // ADDWARNING(__func__,"Topic "+topic->get_tag()+" has zero keywords, which breaks backward compatible convention.");
        return "ARRAY[]";
    }

    auto comma_fold = [&k](auto it) {
        auto comma_fold_impl = [&k](auto it, auto& comma_fold_ref) mutable {
            if (it == k.end())
                return std::string("]");
            return ",'" + it->keyword + "'" + comma_fold_ref(std::next(it),comma_fold_ref);
        };
        return comma_fold_impl(it,comma_fold_impl);
    };

    return "ARRAY['"+k.begin()->keyword + "'" + comma_fold(std::next(k.begin()));
}

/// Return the coma delimeted 3-digit precision representation of keyword relevances inside the ARRAY[] constructor.
std::string Topic_pq::All_Topic_relevance_pqstr() {
    const std::vector<Topic_Keyword> &k = topic->get_keyrel();
    if (k.size()<1) {
        // ADDWARNING(__func__,"Topic "+topic->get_tag()+" has zero keywords, which breaks backward compatible convention.");
        return "ARRAY[]";
    }

    auto comma_fold = [&k](auto it) {
        auto comma_fold_impl = [&k](auto it, auto& comma_fold_ref) mutable {
            if (it == k.end())
                return std::string("]");

            return ',' + to_precision_string(it->relevance,3) + comma_fold_ref(std::next(it),comma_fold_ref);
        };
        return comma_fold_impl(it,comma_fold_impl);
    };

    return "ARRAY[" + to_precision_string(k.begin()->relevance,3) + comma_fold(std::next(k.begin()));
}

/* Code snippet:

As we now have te to_precision_string() function, we're replacing the std::stringstream approach
with a std::string approach. Keeping the previous implementation here for future reference.

    auto comma_fold = [&k](auto it) {
        auto comma_fold_impl = [&k](auto it, auto& comma_fold_ref) mutable {
            if (it == k.end())
                return std::string("]");

            std::stringstream stream_impl;
            stream_impl << ",'" << std::fixed << std::setprecision(3) << it->keyword << "'" << comma_fold_ref(std::next(it),comma_fold_ref);
            return stream_impl.str();
        };
        return comma_fold_impl(it,comma_fold_impl);
    };

    std::stringstream stream;
    stream << "ARRAY[" << std::fixed << std::setprecision(3) << k.begin()->keyword << comma_fold(std::next(k.begin()));
    return stream.str();

*/

/// Return the Postgres VALUES set for all Topic data between brackets.
std::string Topic_pq::All_Topic_Data_pqstr() {
    return "(" + id_pqstr() + ',' +
           supid_pqstr() + ',' +
           tag_pqstr() + ',' +
           title_pqstr() + ',' +
           All_Topic_keyword_pqstr() + ',' +
           All_Topic_relevance_pqstr() + ')';
}

/// Return the ID between apostrophes.
std::string Node_pq::id_pqstr() {
    return "'"+node->get_id().str()+"'";
}

/// Return the comma delimited string of topic tag numbers between curly brackets and apostrophes.
std::string Node_pq::topics_pqstr() {
    // Implementaton of set, vector, etc., to string borrowed from https://en.cppreference.com/w/cpp/algorithm/accumulate
    const Topics_Set &t = node->get_topics();
    if (t.size()<1) {
        ADDWARNING(__func__,"Node "+node->get_id().str()+" has zero topic tags, which breaks backward compatible convention.");
        return "'{}'";
    }

    /// Lambda function comma_fold
    auto comma_fold = [](std::string a, auto b) {
        return std::move(a) + ',' + std::to_string(b.first);
    };

    // Initialize the accumulate with the first map member
    //*** Does this need to go into a std::string s = before being returned?
    return "'{"+std::accumulate(std::next(t.begin()), t.end(), std::to_string(t.begin()->first), comma_fold)+"}'";
}

/// Return the comma delimited string of 3-digit precision topic relevance values between curly brackets and apostrophes.
std::string Node_pq::topicrelevance_pqstr() {
    // Again, using the lambda approach as demonstrated in topic_pqstr(), but combined with the stringstream
    const Topics_Set &t = node->get_topics();
    if (t.size()<1) {
        ADDWARNING(__func__,"Node "+node->get_id().str()+" has zero topic tags, which breaks backward compatible convention.");
        return "'{}'";
    }

    // digit-precision printing, as demonstrated in valuation_pqstr().
    auto comma_fold = [](std::string a, auto b) {
        return std::move(a) + ',' + to_precision_string(b.second,3);
    };

    // Initialize the accumulate with the first map member
    return "'{"+std::accumulate(std::next(t.begin()), t.end(), to_precision_string(t.begin()->second,3), comma_fold)+"}'";
}

/// Return the 3-digit precision string representation of the valuation.
std::string Node_pq::valuation_pqstr() {
    return to_precision_string(node->get_valuation(),3);
}

/// Return the 3-digit precision string representation of the completion.
std::string Node_pq::completion_pqstr() {
    return to_precision_string(node->get_completion(),3);
}

/// Return the number of seconds time required.
std::string Node_pq::required_pqstr() {
    return std::to_string(node->get_required());
}

/// Return the text between dollar-quoted tags that prevent any issues with characters in the text.
std::string Node_pq::text_pqstr() {
    // Using the Postgres dollar-quoted tag method means no need to escape any characters within the text!
    return "$txt$"+node->get_text()+"$txt$";
}

/// Return a target date time stamp that is recognized by Postgres.
std::string Node_pq::targetdate_pqstr() {
    return TimeStamp_pq(node->get_targetdate());
}

/// Return the target date property enumerated type string (e.g. variable, fixed, exact).
std::string Node_pq::tdproperty_pqstr() {
    return "'"+td_property_str[node->get_tdproperty()]+"'";
}

/// Return TRUE if periodic or FALSE if not periodic.
std::string Node_pq::isperiodic_pqstr() {
    if (node->get_repeats()) return "TRUE";
    else return "FALSE";
}

/// Return the pattern enumerated type string (e.g. patt_daily, patt_monthly) between apostrophes.
std::string Node_pq::tdperiodic_pqstr() {
    return "'"+td_pattern_str[node->get_tdpattern()]+"'";
}

/// Return the tdevery period multiplier.
std::string Node_pq::tdevery_pqstr() {
    return std::to_string(node->get_tdevery());
}

/// Return the tdspan count of instances.
std::string Node_pq::tdspan_pqstr() {
    return std::to_string(node->get_tdspan());
}

/// Return the Postgres VALUES set for all Node data between brackets.
std::string Node_pq::All_Node_Data_pqstr() {
    return "(" + id_pqstr() + ',' +
           topics_pqstr() + ',' +
           topicrelevance_pqstr() + ',' +
           valuation_pqstr() + ',' +
           completion_pqstr() + ',' +
           required_pqstr() + ',' +
           text_pqstr() + ',' +
           targetdate_pqstr() + ',' +
           tdproperty_pqstr() + ',' +
           isperiodic_pqstr() + ',' +
           tdperiodic_pqstr() + ',' +
           tdevery_pqstr() + ',' +
           tdspan_pqstr() + ')';
}

/// Return the unique depID>supID pair between apostrophes.
std::string Edge_pq::id_pqstr() {
    return "'"+edge->get_id().str()+"'";
}

/// Return the 3-digit precision string representation of the dependency.
std::string Edge_pq::dependency_pqstr() {
    return to_precision_string(edge->get_dependency(),3);
}

/// Return the 3-digit precision string representation of the significance.
std::string Edge_pq::significance_pqstr() {
    return to_precision_string(edge->get_significance(),3);
}

/// Return the 3-digit precision string representation of the importance.
std::string Edge_pq::importance_pqstr() {
    return to_precision_string(edge->get_importance(),3);
}

/// Return the 3-digit precision string representation of the urgency.
std::string Edge_pq::urgency_pqstr() {
    return to_precision_string(edge->get_urgency(),3);
}

/// Return the 3-digit precision string representation of the priority.
std::string Edge_pq::priority_pqstr() {
    return to_precision_string(edge->get_priority(),3);
}

/// Return the Postgres VALUES set for all Edge data between brackets.
std::string Edge_pq::All_Edge_Data_pqstr() {
    return "(" + id_pqstr() + ',' +
           dependency_pqstr() + ',' +
           significance_pqstr() + ',' +
           importance_pqstr() + ',' +
           urgency_pqstr() + ',' +
           priority_pqstr() + ')';
}

} // namespace fz
