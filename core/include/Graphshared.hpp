// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header contains classes that are content equivalents of Graphtypes classes that
 * are usable in interprocess shared memory.
 * 
 * Note that some of the classes in Graphtypes can be used directly, because they do not
 * rely on their own dynamic memory allocation, but instead have a fixed data structure.
 * For example, ID_TimeStamp is directly usable in shared memory.
 * 
 * The constructors of these types focus on constructing from their Graphtypes equivalents.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHSHARED_HPP.
 */

#ifndef __GRAPHSHARED_HPP
#include "coreversion.hpp"
#define __GRAPHSHARED_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <cstring>

// core
#include "Graphtypes.hpp"


namespace fz {

/**
 * Reusable directly from Graphtypes:
 * 
 * ID_TimeStamp
 * Node_ID_key
 * Edge_ID_key
 * Keyword_Relevance
 * Topic_ID
 * td_property
 * td_property_str (this array of strings does not change)
 * td_pattern
 * td_pattern_str (this array of strings does not change)
 * Edge
 * 
 * Needs replacement:
 * 
 * Node_ID (idS_cache)
 * Edge_ID (idS_cache)
 * Topic_Keyword (keyword)
 * Topic (tag, title, keyrel)
 * Topic_Tags_Vector
 * Topic_Tags (topictags, topicbytag)
 * Topics_Set
 * Edges_Set
 * Node (id, topics, text, supedges, depedges)
 * Node_Map
 * Edge_Map
 * Node_Index
 * Graph (nodes, edges, topics, )
 * 
 */

constexpr size_t NODE_ID_STRSZ = 16+1; // add room to null-terminate
constexpr size_t EDGE_ID_STRSZ = 16+1+16+1;
constexpr size_t TOPIC_KEYWORD_STRSZ = 40+1;
constexpr size_t TOPIC_TAG_STRSZ = 60+1;
constexpr size_t TOPIC_TITLE_STRSZ = 60+1;
constexpr size_t TOPIC_KEYREL_ARRSZ = 16;
constexpr size_t TOPIC_TAGS_ARRSZ = 1000;

/**
 * Sharable version of Node_ID.
 */
class Node_ID_shr {
protected:
    Node_ID_key idkey;
    char idS_cache[NODE_ID_STRSZ];
public:
    Node_ID_shr(const Node_ID & nid);
    //Node_ID_shr(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    //Node_ID_shr(const ID_TimeStamp _idT);

    Node_ID_shr() = delete; // explicitly forbid the default constructor, just in case

    Node_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
};

/**
 * Sharable version of Edge_ID.
 */
class Edge_ID_shr {
protected:
    Edge_ID_key idkey;
    char idS_cache[EDGE_ID_STRSZ];
public:
    Edge_ID_shr(const Edge_ID & eid);

    Edge_ID_shr() = delete; // explicitly forbid the default constructor, just in case

    Edge_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
};

struct Topic_Keyword_shr {
    char keyword[TOPIC_KEYWORD_STRSZ];
    Keyword_Relevance relevance;

    //Topic_Keyword_shr(std::string k, Keyword_Relevance r): keyword(k), relevance(r) {}
    Topic_Keyword_shr(const Topic_Keyword & tkey);
    Topic_Keyword_shr(): keyword(""), relevance(0.0) {} // used when preparing arrays
};

class Topic_shr {
protected:
    Topic_ID id;
    Topic_ID supid;
    char tag[TOPIC_TAG_STRSZ];
    char title[TOPIC_TITLE_STRSZ];
    Topic_Keyword_shr keyrel[TOPIC_KEYREL_ARRSZ];
    size_t keyrelnum;

public:
    Topic_shr(const Topic & topic);

    /// safely inspect parameters
    Topic_ID get_id() const { return id; }
    Topic_ID get_supid() const { return supid; }
    std::string get_tag() const { return tag; }
    std::string get_title() const { return title; }

    /// table references
    const Topic_Keyword_shr * get_keyrel() const { return keyrel; }
    const size_t keyrel_num() const { return keyrelnum; }

    /// change parameters
    //void set_supid(Topic_ID _supid) { supid = _supid; }

};

typedef Topic_shr * Topic_shr_ptr;
typedef Topic_shr_ptr * Topic_Tags_Vector_shr;
struct Topic_by_Tag {
    char tag[TOPIC_TAG_STRSZ];
    Topic_shr_ptr topicptr;
};

class Topic_Tags_shr {
protected:
    Topic_Tags_Vector_shr topictags[TOPIC_TAGS_ARRSZ];
    Topic_by_Tag topicbytag[TOPIC_TAGS_ARRSZ];
    size_t topictagsnum;

public:
    Topic_Tags_shr(): topictagsnum(0) {}
    Topic_Tags_shr(const Topic_Tags & ttags);
    //~Topic_Tags() { for (auto it = topictags.begin(); it!=topictags.end(); ++it) delete (*it); }

    /// tables references
    const Topic_Tags_Vector_shr * get_topictags() const { return topictags; }

    /// tables sizes
    size_t num_Topics() const { return topictagsnum; }

    //Topic_ID find_or_add_Topic(std::string tag, std::string title);

    Topic_shr_ptr find_by_id(Topic_ID _id);

    Topic_shr_ptr find_by_tag(std::string _tag);

};

// Note that Topic_ID is a kind of integer.
struct Topics_Set_Pair {
    Topic_ID id;
    float relevance;
};
typedef Topics_Set_Pair * Topics_Set_shr;

typedef Edge * Edge_ptr;
typedef Edge_ptr * Edges_Set_shr;

class Node_shr {
    friend class Graph_shr;
    friend class Edge;
protected:
    const Node_ID_shr id;
    Topics_Set_shr topics[];
    float valuation;
    float completion;
    time_t required;
    char * text;
    time_t targetdate;
    td_property tdproperty;
    bool repeats;
    td_pattern tdpattern;
    int tdevery;
    int tdspan;

    Graph_shr *graph;
    Edges_Set_shr supedges[];
    Edges_Set_shr depedges[];
 
    #define SEM_TRAVERSED 1
    mutable int semaphore;

    int get_semaphore() { return semaphore; }
    void set_semaphore(int sval) { semaphore = sval; }
    bool set_all_semaphores(int sval);
    time_t nested_inherit_targetdate();
    time_t inherit_targetdate();

public:
    //Node_shr(std::string id_str) : id(id_str) {}
    Node_shr(const Node & node) : id(node.get_id()) {}

    // safely inspect data
    const Node_ID_shr &get_id() const { return id; }
    std::string get_id_str() const { return id.str(); }
    const Topics_Set_shr * get_topics() const { return topics; }
    float get_valuation() const { return valuation; }
    float get_completion() const { return completion; }
    time_t get_required() const { return required; }
    const char * get_text() const { return text; }
    time_t get_targetdate() const { return targetdate; }
    std::string get_targetdate_str() const { return TimeStampYmdHM(targetdate); }
    td_property get_tdproperty() const { return tdproperty; }
    bool get_repeats() const { return repeats; }
    td_pattern get_tdpattern() const { return tdpattern; }
    int get_tdevery() const { return tdevery; }
    int get_tdspan() const { return tdspan; }

    /// change parameters: topics
    //bool add_topic(Topic_Tags &topictags, Topic_ID topicid, float topicrelevance);
    //bool add_topic(Topic_Tags &topictags, std::string tag, std::string title, float topicrelevance);
    //bool add_topic(Topic_ID topicid, float topicrelevance); /// Use this version if the Node is already in a Graph
    //bool add_topic(std::string tag, std::string title, float topicrelevance); /// Use this version if the Node is already in a Graph
    //bool remove_topic(uint16_t id);
    //bool remove_topic(std::string tag);

    /// change parameters: state 
    //void set_valuation(float v) { valuation = v; }
    //void set_completion(float c) { completion = c; }
    //void set_required(time_t Treq) { required = Treq; }

    /// change parameters: content
    //void set_text(const std::string utf8str);
    //void set_text_unchecked(const std::string utf8str) { text = utf8str; } /// Use only where guaranteed!

    /// change parameters: scheduling
    //void set_targetdate(time_t t) { targetdate = t; }
    //void set_tdproperty(td_property tprop) { tdproperty = tprop; }
    //void set_repeats(bool r) { repeats = r; }
    //void set_tdpattern(td_pattern tpat) { tdpattern = tpat; }
    //void set_tdevery(int multiplier) { tdevery = multiplier; }
    //void set_tdspan(int count) { tdspan = count; }

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

    const Edges_Set_shr * sup_Edges() const { return supedges; }
    const Edges_Set_shr * dep_Edges() const { return depedges; }

    /// friend (utility) functions
    friend Topic_shr * main_topic(Topic_Tags_shr & topictags, Node_shr & node); // friend function to ensure search with available Topic_Tags
    friend Topic_shr * main_topic(Graph_shr & _graph, Node_shr & node);
};

struct Node_Map_Pair {
    Node_ID_key key;
    Node_shr * node;
};
typedef Node_Map_Pair * Node_Map_shr;

struct Edge_Map_Pair {
    Edge_ID_key key;
    Edge * edge;
};
typedef Edge_Map_Pair * Edge_Map_shr;

typedef Node_shr * Node_shr_ptr;
typedef Node_shr_ptr * Node_Index_shr;

class Graph_shr {
    friend class Node_shr;
public:
    enum errcodes { g_noerrors, g_addnullnode, g_adddupnode, g_addnulledge, g_adddupedge, g_removenulledge, g_removeunknownedge };
    errcodes error = g_noerrors; /// Stores a code for the most recent error encountered.
protected:
    // keeping nodes protected here purely as a precaution against accidental map modification
    Node_Map_shr nodes[];
    size_t nodesnum = 0;
    Edge_Map_shr edges[];
    size_t edgesnum = 0;
    Topic_Tags_shr topics;

    bool warn_loops = true;

    void set_all_semaphores(int sval);

public:
    /// tables references
    const Topic_Tags_shr & get_topics() const { return topics; }

    /// tables sizes
    size_t num_Nodes() const { return nodesnum; }
    size_t num_Edges() const { return edgesnum; }
    size_t num_Topics() const { return topics.num_Topics(); }

    /// nodes table: extend
    //bool add_Node(Node &node);
    //bool add_Node(Node *node);

    /// edges table: extend
    //bool add_Edge(Edge &edge);
    //bool add_Edge(Edge *edge);

    /// edges table: reduce
    //bool remove_Edge(Edge &edge);
    //bool remove_Edge(Edge *edge);
 
    /// nodes table: get node
    size_t begin_Nodes() const { return 0; }
    size_t end_Nodes() const { return nodesnum+1; }
    Node_shr * Node_by_id(const Node_ID_key & id) const; // inlined below
    Node_shr * Node_by_idstr(std::string idstr) const; // inclined below
    Node_Index_shr get_Indexed_Nodes() const;

    /// edges table: get edge
    size_t begin_Edges() const { return 0; }
    size_t end_Edges() const { return edgesnum; }
    Edge * Edge_by_id(const Edge_ID_key & id) const; // inlined below

    /// topics table: get topic
    Topic_shr * find_Topic_by_id(Topic_ID _id) { return topics.find_by_id(_id); }

    /// crossref tables: topics x nodes

    Topic_shr * main_Topic_of_Node(Node_shr & node) { return main_topic(topics,node); }

    // *** is this still in use??
    Node_Map_shr * temptonodemap() { return nodes; }
};


} // namespace fz

#endif // __GRAPHSHARED_HPP
