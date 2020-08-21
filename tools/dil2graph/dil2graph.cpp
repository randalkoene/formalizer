// Copyright 2020 Randal A. Koene
// License TBD

/**
 * dil2graph is a simple conversion tool from the HTML-style DIL Files and Detailed-Items-by-ID
 * format of DIL_entry content to a Graph composed of Node and Edge data structures.
 * 
 * Please note that one of the most important differences between the DIL hierarchy format
 * used in dil2al and the v2.x Graph format is where target dates and target date properties
 * are specified. In the DIL hierarchy format this used to be done on the Edges, which could
 * lead to some confusing or near-arbitrary combined results for a node. In the Graph format,
 * those specifications are done at the Nodes. For more, see the description in the
 * Formalizer doc at: https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.tarhfe395l5v
 */

#include <array>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <vector>

// dil2al compatibility
#include "dil2al.hh"
#include "dil2al_minimal.hpp"

// Formalizer core
#include "Graphpostgres.hpp"
#include "Graphtypes.hpp"
#include "dilaccess.hpp"
#include "error.hpp"
#include "general.hpp"

// Tool specific
#include "dil2graph.hpp"
#include "tl2log.hpp"

std::string server_long_id;

using namespace fz;

std::string dbname; /// This is initialized to $USER.

//----------------------------------------------------
// Definitions of functions declared in dil2graph.hpp:
//----------------------------------------------------

/**
 * Uses the topical DIL file name to create a topic tag.
 * 
 * @param diltopic reference to DIL_Topical_List object.
 * @return tag string.
 */
std::string convert_DIL_Topics_file_to_tag(DIL_Topical_List &diltopic) {
    std::string tstr(diltopic.dil.file.chars());
    std::size_t pathend = tstr.rfind('/');
    std::string tag;
    if (pathend == std::string::npos)
        tag = tstr;
    else
        tag = tstr.substr(pathend + 1);
    std::size_t htmlstart = tag.rfind('.');
    if (htmlstart != std::string::npos)
        tag = tag.substr(0, htmlstart);
    return tag;
}

/**
 * Compares the local target date specifications on the connections to all
 * superiors of a DIL_entry in order to determine whether a target date is
 * specified at that node and, if so, what that target date is.
 * 
 * \attention Note 1: It is not safe to use a converted target date without
 * also converting target date properties (with get_Node_tdproperty()),
 * because a target date value of -1 is insufficient to designate an
 * unspecified target date in Graph 2.0+ format.
 * 
 * \attention Note 2: In Graph 2.0+ format, it is possible to cache a hint
 * for a possible specified target date in the targetdate parameter while
 * designating the target date 'unspecified' or 'inherit' in the tdproperty
 * parameter. Such hints are lost if converted back to DIL Hierarchy v1.x
 * format.
 * 
 * @param e valid DIL_entry.
 * @return the logical local target date specificiation or -1 if unspecified.
 */
time_t get_Node_Target_Date(DIL_entry &e) {
    time_t tdate = -1;
    time_t tdate_candidate = -1;
    for (int i = 0; (tdate_candidate = e.Local_Target_Date(i)) > -2; i++) {
        if (tdate_candidate > -1) {
            if (tdate < 0)
                tdate = tdate_candidate;
            else if (tdate_candidate < tdate)
                tdate = tdate_candidate;
        }
    }
    return tdate;
}

/**
 * Collect topics from topical DIL files, match with topic tags, and create more
 * as needed while also collecting topic titles and keywords.
 * 
 * New topic tags, associated titles and keywords are stored in a separate data
 * structure.
 * 
 * @param graph reference to valid Graph object.
 * @param node reference to valid Node object.
 * @param entry valid DIL entry.
 * @return the number of topics converted.
 */
int convert_DIL_Topics_to_topics(Topic_Tags &topics, Node &node, DIL_entry &entry) {
    if (!entry.parameters)
        return 0;

    int topicsconverted = 0;
    PLL_LOOP_FORWARD(DIL_Topical_List, entry.parameters->topics.head(), 1) {
        std::string tag(convert_DIL_Topics_file_to_tag(*e));
        if (!tag.empty()) {
            // get a topic tag index for the tag string
            unsigned int topicid = topics.find_or_add_Topic(tag, e->dil.title.chars());
            // store the index and relevance value in the Node's topics map
            if (node.add_topic(topics, topicid, e->relevance))
                topicsconverted++;
        }
    }
    return topicsconverted;
}

/**
 * Obtains the node tdproperty from tdfixed(), tdexact() and whether the
 * target date is locally specified.
 * 
 * This function also pays attention to the special case where tdfixed()
 * is true but the local target date is unspecified. In that case, the
 * Graph format v2.0+ target date property 'inherit' is returned. For
 * more information about this see the <a href="https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.nu3mb52d1k6n">target date parameters</a>
 * section in the Formalizer documentation.
 * 
 * @param e valid DIL entry.
 * @return the td_property for the node.
 */
td_property get_Node_tdproperty(DIL_entry &e) {
    if (e.tdexact())
        return td_property::exact;

    bool isfromlocal;
    int haslocal, numpropagating;
    //time_t td =
    e.Target_Date_Info(isfromlocal, haslocal, numpropagating);

    if (e.tdfixed()) {
        if (haslocal)
            return td_property::fixed;
        else
            return td_property::inherit;
    }

    if (haslocal)
        return td_property::variable;

    return td_property::unspecified;
}

/**
 * Converts periodictask_t to td_pattern.
 * 
 * This is done carefully, rather than simply casting, because there may be
 * different organizations and other changes ahead.
 * 
 * @param e valid DIL entry.
 * @return the td_pattern for the node.
 */
td_pattern get_Node_tdpattern(DIL_entry &e) {
    switch (e.tdperiod()) {
    case periodictask_t::pt_daily:
        return td_pattern::patt_daily;
    case periodictask_t::pt_workdays:
        return td_pattern::patt_workdays;
    case periodictask_t::pt_weekly:
        return td_pattern::patt_weekly;
    case periodictask_t::pt_biweekly:
        return td_pattern::patt_biweekly;
    case periodictask_t::pt_monthly:
        return td_pattern::patt_monthly;
    case periodictask_t::pt_endofmonthoffset:
        return td_pattern::patt_endofmonthoffset;
    case periodictask_t::pt_yearly:
        return td_pattern::patt_yearly;
    case periodictask_t::OLD_pt_span:
        return td_pattern::OLD_patt_span;
    case periodictask_t::pt_nonperiodic:
        return td_pattern::patt_nonperiodic;
    default:
        return td_pattern::patt_nonperiodic;
    }
}

/**
 * Convert all data in a DIL_entry formatted node to a Node object.
 * 
 * Note that there are some differences between the DIL Entry data format and
 * the Node data format, especially these:
 * 1. Target date and target date properties are Node specific instead of
 *    Edge specific (as they were in the DIL entry format).
 * 2. Target date properties have been updated to include a specific 'inherit'
 *    option (which is no longer inferred from the target date value) and an
 *    explicit 'unspecified' option.
 * 3. The Node.text data is by default explicitly considered to be encoded as
 *    UTF8 HTML5. (This was not explicit in the DIL Hierarchy and care needs to
 *    be taken during conversion.)
 * 
 * @param e pointer to a valid DIL_entry.
 * @param graph reference to a valid Graph object.
 * @param convmet a structure that stores metrics about the conversion process.
 * @return Node pointer if the conversion was successful, NULL otherwise.
 */
Node *convert_DIL_entry_to_Node(DIL_entry &e, Graph &graph, ConversionMetrics &convmet) {
    try {
        Node *node = new Node(std::string(e.str()));
        if (node) {
            if (convert_DIL_Topics_to_topics(const_cast<Topic_Tags &>(graph.get_topics()), *node, e) < 1) {
                EOUT << "Converted no topics for [DIL#" << e.str() << "]\n";
                convmet.notopics++;
            }
            node->set_valuation(e.Valuation());
            node->set_completion(e.Completion_State());
            node->set_required(e.Time_Required());
            if (e.Entry_Text())
                node->set_text(e.Entry_Text()->chars()); /// This automatically replaces any UTF8 invalid codes.
            node->set_targetdate(get_Node_Target_Date(e));
            node->set_tdproperty(get_Node_tdproperty(e));
            node->set_repeats(e.tdperiod() != pt_nonperiodic);
            node->set_tdpattern(get_Node_tdpattern(e));
            node->set_tdevery(e.tdevery());
            node->set_tdspan(e.tdspan());
        }
        return node;

    } catch (ID_exception idexception) {
        EOUT << "Skipping [DIL#" << e.str() << "], " << idexception.what() << endl;
        convmet.skippedentries++;
    }
    return NULL;
}

/**
 * Convert all data in a DIL_Superior formatted edge to an Edge object.
 * 
 * @param depentry reference to a valid dependency (source) DIL_entry.
 * @param supentry reference to a valid superior (target) DIL_entry.
 * @param dilsup reference to a valid DIL_Superior object.
 * @param graph reference to a valid Graph object.
 * @param convmet a structure that stores metrics about the conversion process.
 * @return Edge pointer if the conversion was successful, NULL otherwise.
 */
Edge *convert_DIL_Superior_to_Edge(DIL_entry &depentry, DIL_entry &supentry, DIL_Superiors &dilsup, Graph &graph, ConversionMetrics &convmet) {
    Node *dep = graph.Node_by_id(Node_ID_key(depentry.chars()));
    if (!dep)
        convmet.missingdepnode++;
    Node *sup = graph.Node_by_id(Node_ID_key(supentry.chars()));
    if (!sup)
        convmet.missingsupnode++;
    if ((!dep) || (!sup))
        return NULL;

    Edge *edge = new Edge(*dep, *sup);

    if (edge) {
        edge->set_dependency(dilsup.relevance);
        edge->set_significance(dilsup.unbounded);
        edge->set_importance(dilsup.bounded);
        edge->set_urgency(dilsup.urgency);
        edge->set_priority(dilsup.priority);
    }

    return edge;
}

/**
 * Find the keyword,relevance pairs in a DIL Topical File.
 * 
 * This function requires that `basedir` is valid, otherwise the shellcmd2str()
 * call will throw a runtime_error. This function does not distinguish between
 * actual files and symlinks (see detect_DIL_Topics_Symlinks()).
 * 
 * Note: Perhaps this function belongs in the utilities.cc library of dil2al.
 * 
 * @return a vector containing strings of the form Keyword (Relevance).
 */
std::vector<Topic_Keyword> get_DIL_Topics_File_KeyRels(std::string dilfilepath) {
    std::string diltopicfiles = shellcmd2str("sed -n 's/^[<]B[>]Topic Keywords, k_{top} (and relevance in [[]0,1[]]):[<].B[>]\\(.*\\)$/\\1/p' " + dilfilepath);
    std::vector<std::string> krelstrvec = split(diltopicfiles, ',');
    std::vector<Topic_Keyword> krels;
    for (auto it = krelstrvec.begin(); it != krelstrvec.end(); ++it) {
        std::string keyword;
        auto relpos = it->find_first_of("(");
        if (relpos == std::string::npos)
            keyword = *it;
        else
            keyword = it->substr(0, relpos);
        trim(keyword);
        float relevance = 1.0;
        if (relpos == std::string::npos) {
            ADDWARNING(__func__, "in " + dilfilepath + " the keyword " + keyword + " defaults to 1.0 relevance");
        } else {
            relevance = strtof(it->substr(relpos + 1).c_str(), NULL);
            if (relevance == 0.0F) {
                ADDWARNING(__func__, "in " + dilfilepath + " the keyword " + keyword + " has zero or invalid relevance, defaulting to 1.0");
                relevance = 1.0;
            }
        }
        if (keyword.empty()) {
            ADDWARNING(__func__, "skipping empty keyword in " + dilfilepath);
        } else {
            krels.emplace_back(keyword, relevance);
        }
    }
    return krels;
}

/**
 * Search the DIL Topic File that corresponds with the topic for defined
 * keyword,relevance pairs and add those to the keyrel vector.
 * 
 * Note that this replaces any previous content in keyrel.
 * 
 * @param topic pointer to a Topic object.
 * @return the number of keyword,relevance pairs found.
 */
unsigned int collect_topic_keyword_relevance_pairs(Topic *topic) {
    if (!topic)
        return 0;

    // identify the relevant DIL Topic File
    std::string dilfilepath = basedir.chars();
    dilfilepath += RELLISTSDIR;
    dilfilepath += topic->get_tag() + ".html";
    // open the file and find keyword,relevance pairs, and copy those to the keyrel vector
    std::vector<Topic_Keyword> *tkr = const_cast<std::vector<Topic_Keyword> *>(&topic->get_keyrel()); // explicitly making this modifiable
    *tkr = get_DIL_Topics_File_KeyRels(dilfilepath);

    return topic->get_keyrel().size();
}

/**
 * Detect if DIL Topic Files are actual or symlinks.
 * 
 * The integer result for each file is -1 if not a symlink. Otherwise, the
 * number is the index of the DIL Topic file that the symink points to.
 * If the symbolic link target could not be resolved then the special
 * problem value -2 is set.
 * 
 * @param a vector containing a list of absolute DIL Topic File names.
 * @param num integer reference that receives the number of symlinks found.
 * @return a vector of integer symlink references.
 */
std::vector<int> detect_DIL_Topics_Symlinks(const std::vector<std::string> &dilfilepaths, int &num) {
    std::vector<int> symlinks(dilfilepaths.size(), -1);
    std::vector<std::string> dfname(dilfilepaths.size());
    for (size_t i = 0; i < dilfilepaths.size(); i++) {
        std::filesystem::path p = dilfilepaths[i];
        dfname[i] = p.filename();
    }
    num = 0;
    for (size_t i = 0; i < dilfilepaths.size(); i++) {
        std::filesystem::path p = dilfilepaths[i];
        if (std::filesystem::exists(p) && std::filesystem::is_symlink(p)) {
            num++;
            std::string target = std::filesystem::read_symlink(p).filename();
            auto it = std::find(dfname.begin(), dfname.end(), target);
            if (it == dfname.end()) {
                ADDWARNING(__func__, "symbolic DIL File link from " + std::string(p.filename()) + " to " + target + " unresolved");
                symlinks[i] = -2; // special problem value
            } else {
                symlinks[i] = it - dfname.begin();
            }
        }
    }
    return symlinks;
}

/**
 * Convert a complete Detailed_Items_List to Graph format.
 * 
 * @param dil pointer to a valid Detailed_Items_List.
 * @param convmet a structure that stores metrics about the conversion process.
 * @return pointer to Graph, or NULL.
 */
Graph *convert_DIL_to_Graph(Detailed_Items_List *dil, ConversionMetrics &convmet) {
    ERRHERE(".1");
    if (!dil)
        ERRRETURNNULL(__func__, "unable to build Graph from NULL Detailed_Items_List");

    // Start an empty Graph
    Graph *graph = new Graph();
    if (!graph)
        ERRRETURNNULL(__func__, "unable to initialize Graph");

    ERRHERE(".2");
    // Add all the DIL_entry nodes to the Graph as Node objects
    PLL_LOOP_FORWARD(DIL_entry, dil->list.head(), 1) {
        if (!graph->add_Node(convert_DIL_entry_to_Node(*e, *graph, convmet))) {
            if (graph->error == Graph::g_addnullnode) {
                convmet.nullnodes++;
            } else if (graph->error == Graph::g_adddupnode) {
                convmet.duplicates++;
                EOUT << "Duplicate node [DIL#" << e->str() << "]" << endl;
            } else {
                convmet.unknownnodeerror++;
                EOUT << "UNKNOWN error while attempting to add Node\n";
            }
        }
    }

    ERRHERE(".3");
    // Add all the connections between DIL_entry nodes to the Graph as Edge objects
    PLL_LOOP_FORWARD(DIL_entry, dil->list.head(), 1) {
        DIL_Superiors *dilsup;
        for (int i = 0; (dilsup = e->Projects(i)); ++i) {
            DIL_entry *s = dilsup->Superiorbyid();
            if (e == s) {
                convmet.selfconnections++;
                ADDWARNING(__func__, "Self-connection at [DIL#" + std::string(e->chars()) + "] (this is permitted)");
            }
            if (!graph->add_Edge(convert_DIL_Superior_to_Edge(*e, *s, *dilsup, *graph, convmet))) {
                if (graph->error == Graph::g_addnulledge) {
                    convmet.nulledges++;
                } else if (graph->error == Graph::g_adddupedge) {
                    convmet.duplicateedges++;
                    EOUT << "Duplicate edge from [DIL#" << e->str() << "] to [DIL#" << s->str() << "]" << endl;
                } else {
                    convmet.unknownedgeerror++;
                    EOUT << "UNKNOWN error while attempting to add Edge\n";
                }
            }
        }
    }

    ERRHERE(".4");
    // Add any keyword,relevance pairs or identified Topics
    const Topic_Tags_Vector &t = graph->get_topics().get_topictags();
    for (auto it = t.begin(); it != t.end(); ++it) {
        if (collect_topic_keyword_relevance_pairs(*it) < 1) {
            convmet.topicsanskeyrel++;
            VOUT << "No keyword,relevance pairs found for topic " << (*it)->get_tag() << " (unusual but possible)\n";
        }
    }

    return graph;
}

/**
 * Closing and clean-up actions when exiting the program.
 * 
 * Note that the exit status here needs to be an integer rather than
 * the enumerated exit_status_code type, because it is also linked
 * into dil2al object code.
 * 
 * @param status exit status to return to the program caller.
 */
void Exit_Now(int status) {
    exit_report();
    exit_postop();
    ERRWARN_SUMMARY(VOUT);
    Clean_Exit(status);
}

void key_pause() {
    VOUT << "...Presse ENTER to continue (or CTRL+C to exit).\n";
    std::string enterstr;
    std::getline(cin, enterstr);
}

//----------------------------------------------------
// Definitions of file-local scope functions:
//----------------------------------------------------

void print_version(std::string progname) {
    std::cout << progname << " " << server_long_id << '\n';
}

void print_usage(std::string progname) {
    std::cout << "Usage: " << progname << " [-d <dbname>] [-m] [-L|-D|-T]\n"
              << "       " << progname << " -v\n"
              << '\n'
              << "  Options:\n"
              << "    -d store resulting Graph in Postgres account <dbname>\n"
              << "       (default is $USER)\n"
              << "    -m manual decisions (no automatic fixes)\n"
              << "    -L load only (no conversion and storage)\n"
              << "    -D DIL hierarchy conversion only\n"
              << "    -T Task Log conversion only\n"
              << "    -v print version info\n"
              << '\n'
              << server_long_id << '\n'
              << '\n';
}

bool load_only = false; /// Alternative call, merely to test database loading.
bool dil_only = false;
bool tl_only = false;

void process_commandline(int argc, char *argv[]) {
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "d:LDTm")) != EOF) {

        switch (c) {
        case 'd':
            dbname = optarg;
            break;

        case 'm':
            manual_decisions = true;
            break;

        case 'L':
            load_only = true;
            break;

        case 'D':
            dil_only = true;
            break;

        case 'T':
            tl_only = true;
            break;

        case 'v':
            print_version(argv[0]);
            Clean_Exit(exit_ok);

        default:
            print_usage(argv[0]);
            Clean_Exit(exit_ok);
        }
    }
}

/**
 * Progress indicator that prints a '=' each time the counter reaches
 * another 10th of the total.
 */
void node_pq_progress_func(unsigned long n, unsigned long ncount) {
    if (ncount >= n) {
        VOUT << "==\n";
        VOUT.flush();
        return;
    }
    unsigned long tenth = n / 10;
    if ((n % 10) > 0)
        tenth++;
    if (ncount < tenth)
        return;
    if ((ncount % tenth) == 0) {
        VOUT << "==";
        VOUT.flush();
    }
}

/// This is the main() that should actually be in a separate program.
/// It is here only for testing purposes.
int alt_main() {
    ERRHERE(".1");

    VOUT << "Operating in Load-only mode. Testing database loading.\n";

    key_pause();

    Graph graph;
    if (!load_Graph_pq(graph, dbname)) {
        EOUT << "\nSomething went wrong! Unable to load Graph from Postgres database.\n";
        Exit_Now(exit_database_error);
    }
    VOUT << "Graph re-loading data test:\n";
    VOUT << "  Number of Topics = " << graph.get_topics().get_topictags().size() << endl;
    VOUT << "  Number of Nodes  = " << graph.num_Nodes() << endl;
    VOUT << "  Number of Edges  = " << graph.num_Edges() << endl
         << endl;

    key_pause();

    Exit_Now(exit_ok);

    return 0;
}

std::pair<Detailed_Items_List *, Graph *> interactive_conversion() {
    ERRHERE(".1");
    key_pause();

    VOUT << "Let's load the Detailed Items List:\n\n";
    ERRHERE(".2");
    Detailed_Items_List *dil;
    if (!(dil = get_DIL_Graph())) {
        EOUT << "\nSomethihg went wrong! Unable to load Detailed Items List.\n";
        Exit_Now(exit_DIL_error);
    }
    VOUT << "\nDetailed Items List loaded with " << dil->list.length() << " DIL_entry elements.\n\n";

    key_pause();

    VOUT << "Now, let's convert the Detailed Items List to Graph format:\n\n";
    ERRHERE(".3");
    ConversionMetrics convmet;
    Graph *graph;
    if ((graph = convert_DIL_to_Graph(dil, convmet)) == NULL) {
        EOUT << "\nSomething went wrong! Unable to convert to Graph.\n";
        Exit_Now(exit_conversion_error);
    }
    VOUT << "\nDetailed Items List converted to Graph with " << graph->num_Nodes() << " Node elements.\n\n";

    VOUT << "Conversion process notices:\n";
    VOUT << "Number of self-connections found = " << convmet.selfconnections << endl;
    VOUT << "Topics without keyrel pairs      = " << convmet.topicsanskeyrel << endl
         << endl;

    VOUT << "Number of NULL Nodes encountered = " << convmet.nullnodes << endl;
    VOUT << "Number of entries skipped        = " << convmet.skippedentries << endl;
    VOUT << "Number of duplicate IDs found    = " << convmet.duplicates << endl;
    VOUT << "Number of unknown node errors    = " << convmet.unknownnodeerror << endl;
    VOUT << "Number of nodes with zero topics = " << convmet.notopics << endl;
    VOUT << "Number of NULL Edges encountered = " << convmet.nulledges << endl;
    VOUT << "Number of duplicate connections  = " << convmet.duplicateedges << endl;
    VOUT << "Number of unknown edge errors    = " << convmet.unknownedgeerror << endl;
    VOUT << "Number of missing dependenies    = " << convmet.missingdepnode << endl;
    VOUT << "Number of missing superiors      = " << convmet.missingsupnode << endl
         << endl;

    VOUT << "Recap of converted hierarchy:\n";
    std::vector<std::string> diltopicfilenames = get_DIL_Topics_File_List();
    unsigned int numDILtopics = diltopicfilenames.size();
    int numsymlinks = 0;
    std::vector<int> diltopicsymlinks = detect_DIL_Topics_Symlinks(diltopicfilenames, numsymlinks);
    VOUT << "Number of topics by DIL Files    = " << numDILtopics << " (" << numsymlinks << " symlinks)" << endl;
    VOUT << "Number of Topics identified      = " << graph->get_topics().get_topictags().size() << endl;

    VOUT << "Number of DIL entries            = " << dil->list.length() << endl;
    VOUT << "Number of Nodes                  = " << graph->num_Nodes() << endl;

    VOUT << "Number of connections            = " << get_DIL_hierarchy_num_connections(dil) << endl;
    VOUT << "Number of Edges                  = " << graph->num_Edges() << endl
         << endl;

    int conversion_problems = convmet.conversion_problems_sum();
    if (((long)graph->num_Nodes()) != dil->list.length())
        conversion_problems++;
    if ((numDILtopics - numsymlinks) != graph->get_topics().get_topictags().size())
        conversion_problems++;
    if (conversion_problems > 0) {
        VOUT << "A total of " << conversion_problems << " likely conversion problems was encountered.\nIt is strongly recommended NOT to proceed with writing to Postgres.\n\nPlease type 'proceed' if you wish to proceed anyway: ";
        std::string proceedchoice;
        std::getline(cin, proceedchoice);
        if (proceedchoice != "proceed") {
            VOUT << "\nWise choice.\n";
            Exit_Now(exit_cancel);
        } else
            VOUT << "Proceeding despite noted reservations.\n";
    } else
        key_pause();

//#define TEST_STOP_AFTER_CONVERT
#ifdef TEST_STOP_AFTER_CONVERT
    Exit_Now(exit_cancel);
#endif // TEST_STOP_AFTER_CONVERT

//#define TEST_SIMULATE_PQ_CHANGES
#ifdef TEST_SIMULATE_PQ_CHANGES
    SimPQ.SimulateChanges();
#endif // TEST_SIMULATE_PQ_CHANGES

    VOUT << "Finally, let's store the Graph as Postgres data.\n\n";
    ERRHERE(".4");
    VOUT << "+----+----+----+---+\n";
    VOUT << "|    :    :    :   |\n";
    VOUT.flush();
    if (!store_Graph_pq(*graph, dbname, node_pq_progress_func)) {
        EOUT << "\nSomething went wrong! Unable to store (correctly) in Postgres database.\n";
        Exit_Now(exit_database_error);
    }
    VOUT << "\nGraph stored in Postgres database.\n\n";

    if (SimPQ.SimulatingPQChanges()) {
        VOUT << "Postgres database changes were SIMULATED. Writing Postgres call log to file at:\n/tmp/dil2graph-Postgres-calls.log\n\n";
        std::ofstream pqcallsfile;
        pqcallsfile.open("/tmp/dil2graph-Postgres-calls.log");
        pqcallsfile << SimPQ.GetLog() << '\n';
        pqcallsfile.close();
    }

    key_pause();

    return std::make_pair(dil, graph);
}

void interactive_validation(Detailed_Items_List *dil, Graph *graph) {
    ERRHERE(".1");
    if ((!dil) || (!graph)) {
        EOUT << "Unable to validate due to dil==NULL or graph==NULL\n";
        Exit_Now(exit_general_error);
    }
    VOUT << "Now, let's validate the database by reloading the Graph and comparing it with the one that was stored.\n";

    Graph reloaded;
    if (!load_Graph_pq(reloaded, dbname)) {
        EOUT << "\nSomething went wrong! Unable to load back from Postgres database.\n";
        Exit_Now(exit_database_error);
    }
    VOUT << "Graph re-loading data test:\n";
    VOUT << "  Number of Topics = " << reloaded.get_topics().get_topictags().size() << endl;
    VOUT << "  Number of Nodes  = " << reloaded.num_Nodes() << endl;
    VOUT << "  Number of Edges  = " << reloaded.num_Edges() << endl
         << endl;

    ERRHERE(".2");
    std::string trace;
    if (identical_Graphs(*graph, reloaded, trace)) {
        VOUT << "The converted Graph that was stored and the reloaded Graph from the database are identical!\n";
        VOUT << "Validation passed.\n\n";
    } else {
        VOUT << "The converted and reloaded Graphs have differences...\n";
        VOUT << "Validation failed.\n\n";
        VOUT << "Trace: " << trace << endl
             << endl;
    }

    ERRHERE(".3");
    key_pause();

    VOUT << "Comparative order of Nodes (first 10):\n\n";
    std::string rowstr("DIL ID\t\t\tNode ID\t\t\tPostgres id\t\tReloaded ID\n----------------\t----------------\t----------------\t----------------\n");
    DIL_entry *e = dil->list.head();
    auto n_it = graph->begin_Nodes();
    auto r_it = reloaded.begin_Nodes();
    auto v = load_Node_parameter_interval(dbname, pqn_id, 0, 10);
    for (unsigned int i = 0; i < 10; i++) {
        if (e) {
            rowstr += e->str() + '\t';
            e = e->Next();
        } else {
            rowstr += "\t\t\t";
        }
        if (n_it != graph->end_Nodes()) {
            rowstr += n_it->second->get_id().str() + '\t';
            ++n_it;
        } else {
            rowstr += "\t\t\t";
        }
        if (i < v.size()) {
            rowstr += v[i] + '\t';
        } else {
            rowstr += "\t\t\t";
        }
        if (r_it != reloaded.end_Nodes()) {
            rowstr += r_it->second->get_id().str() + '\n';
            ++r_it;
        } else {
            rowstr += '\n';
        }
    }
    rowstr += '\n';
    VOUT << rowstr;

    VOUT << "Comparative order of Edges (first 10):\n\n";
    rowstr = "DEP>SUP ID\t\t\t\tReloaded DEP>SUP ID\n---------------->----------------\t---------------->----------------\n";
    auto e_it = graph->begin_Edges();
    auto re_it = reloaded.begin_Edges();
    //auto v = load_Edge_parameter_interval(dbname,pqe_id,0,10);
    for (unsigned int i = 0; i < 10; i++) {
        if (e_it != graph->end_Edges()) {
            rowstr += e_it->second->get_id().str() + '\t';
            ++e_it;
        } else {
            rowstr += "\t\t\t";
        }
        /*
    if (i<v.size()) {
      rowstr += v[i] + '\t';
    } else {
      rowstr += "\t\t\t";
    }
    */
        if (re_it != reloaded.end_Edges()) {
            rowstr += re_it->second->get_id().str() + '\n';
            ++re_it;
        } else {
            rowstr += '\n';
        }
    }
    rowstr += '\n';
    VOUT << rowstr;

    key_pause();
}

int main(int argc, char *argv[]) {
    ERRHERE(".1");
    server_long_id = "Formalizer:Conversion:DIL2Graph v" + version() + " (core v" + coreversion() + ")";
    din = &cin;
    eout = &cerr;
    vout = &cout;
    runnablename = argv[0];
    if (runnablename.contains('/'))
        runnablename = runnablename.after('/', -1);
    curdate = date_string();
    curtime = time_stamp("%Y%m%d%H%M");
    Output_Log_Append(curtime + '\n');
    char *username = std::getenv("USER");
    if (username)
        dbname = username;
    initialize();
    process_commandline(argc, argv);

    VOUT << server_long_id << " starting.\n";
    if (dbname.empty()) {
        EOUT << "\nNeed a database account to proceed. Defaults to $USER.\n";
        Exit_Now(exit_general_error);
    }
    VOUT << "Postgres database account selected: " << dbname << '\n';

    ERRHERE(".2");

    if (load_only) {
        alt_main(); // This does not return
    }

    if (!tl_only) {
        std::pair<Detailed_Items_List *, Graph *> dg = interactive_conversion();
        interactive_validation(dg.first, dg.second);
    }
    if (!dil_only) {
        interactive_TL2Log_conversion();
    }

    ERRHERE(".3");

    Exit_Now(exit_ok);
}

/*
  // ID unit tests
  ID_TimeStamp idt_test = { .year = 2020, .month=8, .day=17, .hour=13, .minute = 4, .second = 15, .minor_id = 1 };
  ID_Compare idc_test;
  VOUT << "Some ID unit tests:\n";
  VOUT << "ID_TimeStamp size = " << sizeof(idt_test) << " bytes\n";
  VOUT << "ID_Compare size   = " << sizeof(idc_test) << " bytes\n";
  unsigned char const * ptr = (unsigned char const *) (&idt_test);
  for (size_t i = 0; i<sizeof(idt_test); ++i) {
    VOUT << (int) *ptr << '\t';
    ++ptr;
  }
  VOUT << endl;
  ptr = (unsigned char const *) (&idc_test);
  for (size_t i = 0; i<sizeof(idc_test); ++i) {
    VOUT << (int) *ptr << '\t';
    ++ptr;
  }
  VOUT << endl;
*/
