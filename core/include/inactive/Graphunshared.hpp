// Copyright 2020 Randal A. Koene
// License TBD

/**
 * NOTE: This library component has been taken out of active maintenance to save on
 *       unnecessary compilation time, since it is not presently being used by any
 *       core or tools programs.
 */

/**
 * This header file declares Graph, Node and Edge types for use with the Formalizer.
 * These define the authoritative version of the data structure for use in C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPHUNSHARED_HPP.
 */

/**
 * Notes about the C++ Node object structure:
 * 1. Node effort is still expressed as the combination of completion ratio and estimated
 *    time required. An alternative is suggested in https://trello.com/c/Rnm84Hld.
 * 2. The text in a Node should not contain `@<labelinfo>@` tags, as they will have been
 *    filtered out and explicitly added to the Node as part of its tags mapping. When
 *    converting back to the DIL_entry type used in dil2al those tags will need to be
 *    translated back into `@<labelinfo>@` tags that are inserted into the text data.
 * 3. Although an 'unspecified' target date is now one of the four basic types of
 *    target dates enumerated (unspecified, variable, fixed, exact), the convention
 *    is still that the targetdate should be set to an invalid value if unspecified.
 * 4. The targetdate does not yet include a time zone specification.
 * 5. There is one targetdate per Node. The multiple target dates possibility in DIL_entry
 *    was never sucessfully used and caused more extra work than it was worth.
 * 6. The isperiodic flag has been taken out of tdproperty, since the other tdproperty
 *    options are mutually exclusive and therefore easily enumerated, while isperiodic is
 *    a possible complement to either fixed or exact.
 * 7. The Node.text content is now (by default) assumed to be encoded in UTF8 HTML5. Care
 *    needs to be taken during value assignment.
 * 
 * Notes about the C++ Edge object structure:
 * 1. Edges are identified by a specific pair of dependency and superior Nodes.
 * 2. Where a unique ID is required, the combination of dep->id and sup->id provides that.
 * 3. Sorting of Edges for iteration could be done either dep->id first, then sup->id or
 *    vice versa. A typical tree of tasks that complete a Milestone is one where a superior
 *    has many dependencies, not one where a dependency has many superiors. So, even though
 *    this may not be true for every case, the Graph format assumes it to be more common.
 *    The sorting method (implemented through the .less() comparator) therefore applies to
 *    sup->id first, then to dep->id, effectively grouping dependencies together.
 * 3. *** There are still many questions to be resolved about optimal in-memory structures
 *    *** and fast search and retrieval under various circumstances. Some cached pointer
 *    *** arrays/vectors may be used.
 */

#ifndef __GRAPHUNSHARED_HPP
#include "coreversion.hpp"
#define __GRAPHUNSHARED_HPP (__COREVERSION_HPP)

// std
#include <ctime>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

// core
#include "error.hpp"
#include "TimeStamp.hpp"
#include "Graphbase.hpp"

namespace fz {

/// Formalizer specific base types for ease of modification (container types)
typedef std::string GraphIDcache;
typedef std::string Keyword_String;
typedef std::string Topic_String;
typedef std::vector<Topic_Keyword> Topic_KeyRel_Vector;
typedef std::vector<Topic*> Topic_Tags_Vector; ///< Only pointers, not the objects themselves. (See Dangerous code card in Software Engineering Update Trello board.)
typedef std::map<std::string, Topic *> TopicbyTag_Map;
typedef std::map<Topic_ID,float> Topics_Set; // to keep relevance as well (otherwise we could use a set)
typedef std::set<Edge*> Edges_Set;
typedef std::string Node_utf8_text;
typedef std::map<Node_ID_key,Node*> Node_Map;
typedef std::map<Edge_ID_key,Edge*> Edge_Map;
typedef std::vector<Node*> Node_Index;

/**
 * Node ID that caches its ID stamp for frequent use.
 */
class Node_ID {
protected:
    Node_ID_key idkey;
    GraphIDcache idS_cache; // cached string version of the ID to speed things up
public:
    Node_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Node_ID(const ID_TimeStamp _idT);

    Node_ID() = delete; // explicitly forbid the default constructor, just in case

    Node_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
};

/**
 * Edge ID that caches its ID stamp for frequent use.
 */
class Edge_ID {
protected:
    Edge_ID_key idkey;
    GraphIDcache idS_cache; // cached string version of the ID to speed things up
public:
    //Edge_ID(std::string dep_idS, std::string sup_idS); // Not clear that this one is ever needed
    Edge_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {} /// Try to use this one only for container element initialization and such.
    Edge_ID(Edge_ID_key _idkey);
    Edge_ID(Node &_dep, Node &_sup);

    Edge_ID() = delete; // explicitly forbid the default constructor, just in case

    Edge_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
};

struct Topic_Keyword {
    Keyword_String keyword;
    Keyword_Relevance relevance;

    Topic_Keyword(std::string k, Keyword_Relevance r): keyword(k), relevance(r) {}
};

class Topic {
protected:
    Topic_ID id;                /// unique number that is equal to the index in the Topic_Tags vector
    Topic_ID supid;             /// optional id/index of superior topic (for grouping), none if supid==id
    Topic_String tag;           /// unique tag label (the original tags are derived directly from DIL file names)
    Topic_String title;         /// optional topic title (e.g. obtained from DIL file)
    Topic_KeyRel_Vector keyrel; /// optional list of keywords and relevance ratios

public:
    Topic(Topic_ID _id, std::string _tag, std::string _title): id(_id), supid(_id), tag(_tag), title(_title) {}

    /// safely inspect parameters
    Topic_ID get_id() const { return id; }
    Topic_ID get_supid() const { return supid; }
    std::string get_tag() const { return tag; }
    std::string get_title() const { return title; }

    /// table references
    const Topic_KeyRel_Vector & get_keyrel() const { return keyrel; }

    /// change parameters
    void set_supid(Topic_ID _supid) { supid = _supid; }

    /// friend (utility) functions
    friend bool identical_Topics(const Topic & topic1, const Topic & topic2, std::string & trace);
};

/** Topic tag data, arranged by integer Index-ID.
 * 
 * This class relates each Topic to both an integer Index-ID and to a Tag-string.
 * 
 * - Topic objects can be found by using either of those two identifiers.
 * - Topic objects are created during calls to find_or_add_Topic() in this class.
 * - Those objects are destroyed when the instance of this class is deleted.
 */
class Topic_Tags {
protected:
    Topic_Tags_Vector topictags; ///< This provides Topic pointers by Topic Index-ID.
    TopicbyTag_Map topicbytag;   ///< This provides Topic pointers by Tag-string key.

public:
    ~Topic_Tags() { for (auto it = topictags.begin(); it!=topictags.end(); ++it) delete (*it); }

    /// tables references
    const Topic_Tags_Vector &get_topictags() const { return topictags; }

    /// tables sizes
    Topic_Tags_Vector::size_type num_Topics() const { return topictags.size(); }

    /**
     * Returns the id (index) of a topic tag and adds it to the list of topic
     * tags if it was not already there.
     * 
     * If added:
     * 
     * 1. A new Topic object is created at the end of the `topictags` vector with
     * `nextid`, `tag` and `title` as constructor parameters.
     * 2. The new vector element is then called at index `nextid`, and a pointer to it is used
     * as the value at key=`tag` to add to the `topicbytag` map.
     * 
     * Note that the found or newly generated topic index is compared with the
     * compile-time constant `HIGH_TOPIC_INDEX_WARNING` to report very large indices
     * considered to be probably mistaken.
     * 
     * @param tag a topic tag label.
     * @param title a title string.
     * @return id (index) of the topic tag in the `topictags` vector.
     */
    Topic_ID find_or_add_Topic(std::string tag, std::string title);

    /**
     * Find a Topic in the Topic_Tags table by Topic ID.
     * 
     * @param _id a Topic ID.
     * @return pointer to the Topic (or nullptr if not found).
     */
    Topic * find_by_id(Topic_ID _id); // inlined below

    /**
     * Search the topicbytag map and return a pointer to the Topic if the tag
     * was found.
     * 
     * @param _tag a topic tag label
     * @return pointer to Topic object in topictags vector, or NULL if not found.
     */
    Topic * find_by_tag(std::string _tag);

    /// friend (utility) functions
    friend bool identical_Topic_Tags(Topic_Tags & ttags1, Topic_Tags & ttags2, std::string & trace);
};

// *** WE REALLY NEED SOME CLASS COMMENTS (at least some of what was at DIL_entry or link to docs)

/**
 * The Node class is the principal object type within a Formalizer Graph.
 * 
 * (A woefully incomplete class documentation.)
 * 
 * Note that the set_text_unchecked() member function provides a fast way
 * to set the Node.text parameter, but could lead to invalid UTF8 content.
 * Use it only when the source is guaranteed to be UTF8 encoded, such as
 * when loading data from a database with built-in UTF8 type checking.
 */
class Node {
    friend class Graph;
    friend class Edge;
protected:
    const Node_ID id;        /// unique Node identifier
    Topics_Set topics;       /// a map of pairs of unique topic tag index and relevance value
    Graphdecimal valuation;  /// presently only using values 0.0 and greater (typically [1.0,3.0])
    Graphdecimal completion; /// 1.0 = done, -1.0 = obsolete, -2.0 = replaced, -3.0 = done differently, -4.0 = no longer possible / did not come to pass
    time_t required;         /// seconds
    Node_utf8_text text;     /// by default assumed to contain UTF8 HTML5
    time_t targetdate;       /// when tdproperty=unspecified then targetdate should be set to -1
    td_property tdproperty;  /// unspecified, inherit, variable, fixed, exact
    Graphflag repeats;       /// must be false if tdproperty is unspecified or variable
    td_pattern tdpattern;    /// can be used to remember an optional periodicity even if isperiodic=false
    Graphsigned tdevery;     /// multiplier for pattern interval
    Graphsigned tdspan;      /// count of number of repetitions

    Graph *graph;       /// this is set when the Node is added to a Graph
    Edges_Set supedges; /// this set maintained for rapid Edge access to superior Nodes
    Edges_Set depedges; /// this set maintained for rapid Edge access to dependency Nodes
 
    #define SEM_TRAVERSED 1
    mutable int semaphore; /// used to detect graph traversal etc.

    int get_semaphore() { return semaphore; }
    void set_semaphore(int sval) { semaphore = sval; }
    bool set_all_semaphores(int sval);
    time_t nested_inherit_targetdate(); // only called by the same or by inherit_targetdate()
    time_t inherit_targetdate();        // may be called by effective_targetdate()

public:
    Node(std::string id_str) : id(id_str) {}

    // safely inspect data
    const Node_ID &get_id() const { return id; }
    std::string get_id_str() const { return id.str(); }
    const Topics_Set &get_topics() const { return topics; }
    float get_valuation() const { return valuation; }
    float get_completion() const { return completion; }
    time_t get_required() const { return required; }
    const Node_utf8_text & get_text() const { return text; }
    time_t get_targetdate() const { return targetdate; }
    std::string get_targetdate_str() const { return TimeStampYmdHM(targetdate); }
    td_property get_tdproperty() const { return tdproperty; }
    bool get_repeats() const { return repeats; }
    td_pattern get_tdpattern() const { return tdpattern; }
    int get_tdevery() const { return tdevery; }
    int get_tdspan() const { return tdspan; }

    /// change parameters: topics
    bool add_topic(Topic_Tags &topictags, Topic_ID topicid, float topicrelevance);
    bool add_topic(Topic_Tags &topictags, std::string tag, std::string title, float topicrelevance);
    bool add_topic(Topic_ID topicid, float topicrelevance); /// Use this version if the Node is already in a Graph
    bool add_topic(std::string tag, std::string title, float topicrelevance); /// Use this version if the Node is already in a Graph
    bool remove_topic(uint16_t id);
    bool remove_topic(std::string tag);

    /// change parameters: state 
    void set_valuation(float v) { valuation = v; }
    void set_completion(float c) { completion = c; }
    void set_required(time_t Treq) { required = Treq; }

    /// change parameters: content
    void set_text(const std::string utf8str);
    void set_text_unchecked(const std::string utf8str) { text = utf8str; } /// Use only where guaranteed!

    /// change parameters: scheduling
    void set_targetdate(time_t t) { targetdate = t; }
    void set_tdproperty(td_property tprop) { tdproperty = tprop; }
    void set_repeats(bool r) { repeats = r; }
    void set_tdpattern(td_pattern tpat) { tdpattern = tpat; }
    void set_tdevery(int multiplier) { tdevery = multiplier; }
    void set_tdspan(int count) { tdspan = count; }

    /// Graph relative operations
    time_t effective_targetdate();

    /// helper functions

    /**
     * Report the main Topic Index-ID of the Node, as indicated by the maximum
     * `Topic_Relevance` value.
     * 
     * @return Topic_ID of main Topic.
     */
    Topic_ID main_topic_id();

    const Edges_Set & sup_Edges() const { return supedges; }
    const Edges_Set & dep_Edges() const { return depedges; }

    /// friend (utility) functions
    friend Topic * main_topic(Topic_Tags & topictags, Node & node); // friend function to ensure search with available Topic_Tags
    friend Topic * main_topic(Graph & _graph, Node & node);
    friend bool identical_Nodes(Node & node1, Node & node2, std::string & trace);
};

class Edge {
    friend class Node;
protected:
    const Edge_ID id;
    Graphdecimal dependency;
    Graphdecimal significance; // (also known as unbounded importance)
    Graphdecimal importance;   // (also known as bounded importance)
    Graphdecimal urgency;      // (also known as computed urgency)
    Graphdecimal priority;     // (also known as computed priority)

    Node *dep; // rapid access
    Node *sup; // rapid access

public:
    Edge(Node &_dep, Node &_sup): id(_dep,_sup), dep(&_dep), sup(&_sup) {}
    Edge(Graph & graph, std::string id_str);

    // safely inspect data
    Edge_ID get_id() const { return id; }
    std::string get_id_str() const { return id.str(); }
    Edge_ID_key get_key() const { return id.key(); }
    Node_ID_key get_dep_key() const { return id.key().dep; }
    Node_ID_key get_sup_key() const { return id.key().sup; }
    std::string get_dep_str() const { return id.key().dep.str(); }
    std::string get_sup_str() const { return id.key().sup.str(); }
    Node* get_dep() const { return dep; }
    Node* get_sup() const { return sup; }
    float get_dependency() const { return dependency; }
    float get_significance() const { return significance; }
    float get_importance() const { return importance; }
    float get_urgency() const { return urgency; }
    float get_priority() const { return priority; }

    // change parameters
    void set_dependency(float d) { dependency = d; }
    void set_significance(float s) { significance = s; }
    void set_importance(float i) { importance = i; }
    void set_urgency(float u) { urgency = u; }
    void set_priority(float p) { priority = p; }

    /// friend (utility) functions
    friend bool identical_Edges(Edge & edge1, Edge & edge2, std::string & trace);
};

class Graph {
    friend class Node;
public:
    enum errcodes { g_noerrors, g_addnullnode, g_adddupnode, g_addnulledge, g_adddupedge, g_removenulledge, g_removeunknownedge };
    errcodes error = g_noerrors; /// Stores a code for the most recent error encountered.
protected:
    // keeping nodes protected here purely as a precaution against accidental map modification
    Node_Map nodes;
    Edge_Map edges;
    Topic_Tags topics;

    bool warn_loops = true;

    void set_all_semaphores(int sval);

public:
    /// tables references
    const Topic_Tags & get_topics() const { return topics; }

    /// tables sizes
    Node_Map::size_type num_Nodes() const { return nodes.size(); }
    Edge_Map::size_type num_Edges() const { return edges.size(); }
    Topic_Tags_Vector::size_type num_Topics() const { return topics.num_Topics(); }

    /// nodes table: extend
    bool add_Node(Node &node);
    bool add_Node(Node *node);

    /// edges table: extend
    bool add_Edge(Edge &edge);
    bool add_Edge(Edge *edge);

    /// edges table: reduce
    bool remove_Edge(Edge &edge);
    bool remove_Edge(Edge *edge);
 
    /// nodes table: get node
    auto begin_Nodes() const { return nodes.begin(); }
    auto end_Nodes() const { return nodes.end(); }
    Node * Node_by_id(const Node_ID_key & id) const; // inlined below
    Node * Node_by_idstr(std::string idstr) const; // inclined below
    Node_Index get_Indexed_Nodes() const;

    /// edges table: get edge
    auto begin_Edges() const { return edges.begin(); }
    auto end_Edges() const { return edges.end(); }
    Edge * Edge_by_id(const Edge_ID_key & id) const; // inlined below

    /// topics table: get topic
    Topic * find_Topic_by_id(Topic_ID _id) { return topics.find_by_id(_id); }

    /// crossref tables: topics x nodes

    /**
     * Find a pointer to the main Topic of a Node as indicated by the maximum
     * `Topic_Relevance` value.
     * 
     * Note: To find the main Topic Index-ID instead use the function
     *       `Node::main_topic_id()`.
     * 
     * @param Topic_Tags a valid Topic_Tags list.
     * @param node a Node for which the main Topic is requested.
     * @return a pointer to the Topic object (or nullptr if not found).
     */
    Topic * main_Topic_of_Node(Node & node) { return main_topic(topics,node); }

    // *** is this still in use??
    Node_Map & temptonodemap() { return nodes; }

    /// friend (utility) functions
    friend bool identical_Graphs(Graph & graph1, Graph & graph2, std::string & trace);
};

// +----- begin: standardization functions -----+

// +----- end  : standardization functions -----+

// +----- begin: inline member functions -----+

/**
 * Find a Topic in the Topic_Tags table by Topic ID.
 * 
 * @param _id a Topic ID.
 * @return pointer to the Topic (or nullptr if not found).
 */
inline Topic * Topic_Tags::find_by_id(Topic_ID _id) {
    if (_id>=topictags.size()) return nullptr;
    return topictags.at(_id);
}

/**
 * Find a Node in the Graph by its ID key.
 * 
 * @param id a Node ID key.
 * @return pointer to Node (or nullptr if not found).
 */
inline Node * Graph::Node_by_id(const Node_ID_key & id) const {
    auto it = nodes.find(id);
    if (it==nodes.end()) return nullptr;
    return it->second;
}

/**
 * Find a Node in the Graph by its ID key from a string.
 * 
 * @param idstr a string specifying a Node ID key.
 * @return pointer to Node (or nullptr if not found).
 */
inline Node * Graph::Node_by_idstr(std::string idstr) const {
    try {
        const Node_ID_key nodeidkey(idstr);
        return Node_by_id(nodeidkey);

    } catch (ID_exception idexception) {
        ADDERROR(__func__, "invalid Node ID (" + idstr + ")\n" + idexception.what());
        return nullptr;
    }
    // never gets here
}

/**
 * Find an Edge in the Graph by its ID key.
 * 
 * @param id an Edge ID key.
 * @return pointer to Node (or nullptr if not found).
 */
inline Edge * Graph::Edge_by_id(const Edge_ID_key & id) const {
    auto it = edges.find(id);
    if (it==edges.end()) return nullptr;
    return it->second;
}

// +----- end  : inline member functions -----+

} // namespace fz

#endif // __GRAPHUNSHARED_HPP
