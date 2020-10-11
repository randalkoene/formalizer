// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Graph, Node and Edge types for use with the Formalizer.
 * These define the authoritative version of the data structure for use in C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPHTYPES_HPP.
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

#ifndef __GRAPHTYPES_HPP
#include "coreversion.hpp"
#define __GRAPHTYPES_HPP (__COREVERSION_HPP)

// Boost libraries need the following.
#pragma GCC diagnostic warning "-Wuninitialized"

// std
#include <ctime>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

// core
#include "error.hpp"
#include "TimeStamp.hpp"

#ifndef MAXTIME_T

#ifdef _TYPEBITS
#define HITIME_T ((time_t)(((unsigned long)1) << (_TYPEBITS(time_t) - 1)))
#else
#define HITIME_T ((time_t)(((unsigned long)1) << (BITS(time_t) - 1)))
#endif // _TYPEBITS

/*
   The following is a precaution for the localtime() function on 64 bit platforms,
   because localtime() often cannot produce time stamps with years greater than 9999.
*/
#ifdef __x86_64__
#define MAXTIME_T 253202544000
#else
#define MAXTIME_T ((time_t)(~HITIME_T))
#endif // __x86_64__

#endif // MAXTIME_T

namespace fz {

// Forward declarations for reference before further detailing.
struct Node_ID_key;
struct Edge_ID_key;
struct Topic_Keyword;
class Topic;
class Node;
class Edge;
class Graph;

/// Formalizer specific base types for ease of modification (fixed size)
typedef uint8_t GraphID8bit;
typedef uint16_t GraphIDyear;
typedef float Keyword_Relevance;    /// Type for real-valued Keyword relevance (to Topic), presently assumed to be in the interval [0.0,1.0]
typedef uint16_t Topic_ID;            /// Type for unique Topic IDs
typedef float Topic_Relevance;        /// Type for real-valued Topic relevance (of Node), presently assumed to be in the interval [0.0,1.0]
typedef float Graphdecimal;
typedef int Graphsigned;
typedef bool Graphflag;

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

#define NODE_NULLKEY_STR "{null-key}"

static constexpr const char* const node_exception_stub = "attempted Node_ID construction with invalid ";

/// Exception thrown when a Node ID is of invalid form.
class ID_exception {
    std::string idexceptioncase;
public:
    ID_exception(std::string _idexceptioncase) : idexceptioncase(_idexceptioncase) {
        ADDERROR("Node_ID::Node_ID",node_exception_stub+idexceptioncase);
    }
    std::string what() { return std::string(node_exception_stub) + idexceptioncase; }
};

/**
 * Timestamp IDs in the format required for Node IDs.
 * These include all time components from year to second, as well as an additional
 * minor_id (this is NOT a decimal, since .10 is higher than .9).
 * Note that the time formatting is not the same as in the C time structure 'tm'.
 * Days and months count from 1. The year is given as is, not relative to the
 * UNIX epoch. (By contrast, tm time subtracts 1900 years.)
 * 
 * These structures are mainly used as unique IDs, but conversion to UNIX time
 * is provided through member functions.
 * 
 * Note: A non-standard ID time stamp can be created and used. The quick
 *       isnullstamp() test can detect the special case where non-standard
 *       values are used to create a null-stamp, so that the get_local_time()
 *       and get_epoch_time() functions return well defined results for those.
 *       For greater assurance, the Node_ID_key and Edge_ID_key classes
 *       call specific thorough `valid_...` test functions.
 */
struct ID_TimeStamp {
    GraphID8bit minor_id;
    GraphID8bit second;
    GraphID8bit minute;
    GraphID8bit hour;
    GraphID8bit day;
    GraphID8bit month;
    GraphIDyear year;

    /// Initializes as NODE_NULL_IDSTAMP.
    ID_TimeStamp(): minor_id(0), second(0), minute(0), hour(0), day(0), month(0), year(0) {}

    /// standardization functions and operators
    bool isnullstamp() const { return (month == 0) || (year<1900); }
    bool operator< (const ID_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,second,minor_id)
             < std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.second,rhs.minor_id);
    }
    bool operator== (const ID_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,second,minor_id)
             == std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.second,rhs.minor_id);
    }
    std::tm get_local_time();
    time_t get_epoch_time(); // inlined below
};

/* +----- begin: Historical development comments: -----+
    Historical development comments:

    About the previous union approach:
    Before switching to proper use of std::tie() aimed at ensuring the correct
    order no matter if a system uses little or big endian types, Node_ID_key
    was a union of ID_TimeStamp and ID_Compare. It included the following lines:

    struct ID_Compare {
        uint32_t id_major;
        uint32_t id_minor;
    };

    ID_Compare idC;
    Node_ID_key(): idC( { .id_major = 0, .id_minor = 0 } ) {}
    Node_ID_key(const ID_Compare& _idC): idC(_idC) {}
    return std::tie(idC.id_major, idC.id_minor) < std::tie(rhs.idC.id_major, rhs.idC.id_minor);

    And before that, there was this attempt:
    bool operator< (const Node_ID_key& rhs) const {
        if (idC.id_major < rhs.idC.id_major) return true;
        if (idC.id_minor < rhs.idC.id_minor) return true;
        return false;
    }

    And before that, this one:
    struct Node_ID_Compare {
        bool operator() (const Node_ID& lhs, const Node_ID& rhs) {
            return lhs < rhs;
        }
    };
   +----- end  : Historical development comments: -----+
*/

/**
 * Standardized Formalizer Node ID key.
 * 
 * The principal constructors (all but the default constructor) each
 * call a validity test for the key format and can throw an
 * ID_exception.
 * 
 * In various containers, the ordering of Node ID keys is determined
 * by the provided operator<(). It calls its equivalent in ID_TimeStamp,
 * where std::tie() enables lexicographical comparison, from the largest
 * temporal component to the smallest.
 * 
 * The default constructor is provided for various special use cases such
 * as initialization of containers. The `isnullkey()` function can test
 * for this special state.
 */
struct Node_ID_key { // used to be a union with `ID Compare idC;` (see comments above struct)
    ID_TimeStamp idT;

    Node_ID_key(): idT() {} /// Try to use this one only for container element initialization and such.

    Node_ID_key(const ID_TimeStamp& _idT);
    Node_ID_key(std::string _idS);

    /// standardization functions and operators
    bool isnullkey() const { return idT.month == 0; }
    bool operator< (const Node_ID_key& rhs) const { return (idT < rhs.idT); }
    bool operator== (const Node_ID_key& rhs) const { return (idT == rhs.idT); }
    std::string str() const; // inlined below

    // friend (utility) functions
    friend bool identical_Node_ID_key(const Node_ID_key & key1, const Node_ID_key & key2, std::string & trace);
};

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
 * Standardized Formalizer Edge ID key.
 * 
 * The principal constructors (all but the default constructor) each
 * call a validity test for the key format of each key (dep and sup),
 * and can throw an ID_exception.
 * 
 * In various containers, the ordering of Edge ID keys is determined
 * by the provided operator<(). It uses std::tie() to sequentially
 * call its equivalent in ID_TimeStamp for sup and then for dep, where
 * std::tie() is used again to enable lexicographical comparison, from
 * the largest temporal component to the smallest.
 * 
 * The default constructor is provided for various special use cases such
 * as initialization of containers. The `isnullkey()` function can test
 * for this special state.
 */
struct Edge_ID_key {
    Node_ID_key dep;
    Node_ID_key sup;

    Edge_ID_key() {} /// Try to use this one only for container element initialization and such.

    Edge_ID_key(Node_ID_key _depkey, Node_ID_key _supkey): dep(_depkey), sup(_supkey) {}
    Edge_ID_key(std::string _idS);

    /// standardization functions and operators
    bool isnullkey() const { return dep.idT.month == 0; }
    bool operator<(const Edge_ID_key &rhs) const { return std::tie(sup,dep) < std::tie(rhs.sup,rhs.dep); }
    std::string str() const; // inlined below

    friend bool identical_Edge_ID_key(const Edge_ID_key & key1, const Edge_ID_key & key2, std::string & trace);
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

#define HIGH_TOPIC_INDEX_WARNING 1000 /// at this index number report a warning just in case it is in error

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

/**
 * For more information about td_property values, as well as future expansions, please
 * see the Formalizer documentation section <a href="https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.nu3mb52d1k6n">Target date parameters in Graph v2.0+</a>.
 * Also consider Note 2 of the documentation of dil2graph.cc:get_Node_Target_Date()
 * about target date hints in the Graph 2.0+ format parameters.
 */
enum td_property { unspecified, inherit, variable, fixed, exact, _tdprop_num };
extern const std::string td_property_str[_tdprop_num];

enum td_pattern { patt_daily, patt_workdays, patt_weekly, patt_biweekly, patt_monthly, patt_endofmonthoffset, patt_yearly, OLD_patt_span, patt_nonperiodic, _patt_num };
extern const std::string td_pattern_str[_patt_num];


// *** WE REALLY NEED SOME CLASS COMMENTS (at least some of what was at DIL_entry or link to docs)
/**
 * 
 * targetdate: For details, see the description at https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.nu3mb52d1k6n.
 * topics: Topic tags specify logical categories to which a Node belongs. A Node can belong
 *         to many categories, and new topic categories can be defined freely.
 *         By convention, every Node must have at least 1 topic tag at all times. Since Node
 *         storage can now be independent of Topic tagging there is no strict implementation
 *         reason for this. It has backward compatibility value. See the historical node for
 *         more.
 *         (historical) In the dil2al implementation of a Node (DIL_entry), most of the Node
 *         data is stored in a 'DIL File', also known as a Topic or Topical File. Every Node
 *         had to be stored in at least one such file (but could be copied in multiple such
 *         files), so that Node data was stored. A Node could not belong to zero DIL Files.
 */

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

bool valid_Node_ID(std::string id_str, std::string &formerror, ID_TimeStamp *id_timestamp = NULL);

bool valid_Node_ID(const ID_TimeStamp &idT, std::string &formerror);

std::string Node_ID_TimeStamp_to_string(const ID_TimeStamp idT);

// +----- end  : standardization functions -----+

// +----- begin: inline member functions -----+

/**
 * Convert standardized Formalizer Node ID time stamp into UNIX epoch time.
 * 
 * Note: This function ignores the `minor_id` value. The resulting time
 * value is normally indicative of the time when the Node was created. It
 * is theoretically possible for multiple Nodes to generate the same
 * get_epoch_time() output if their IDs differed only in `minor_id`.
 * 
 * @return UNIX epoch time equivalent of Node ID time stamp.
 */
inline time_t ID_TimeStamp::get_epoch_time() {
    std::tm tm = get_local_time();
    return mktime(&tm);
}

/**
 * Convert Node ID key into standardized Node ID stamp.
 * 
 * @return a string with the Node ID stamp (or NODE_NULLKEY_STR).
 */
inline std::string Node_ID_key::str() const {
    if (isnullkey())
        return NODE_NULLKEY_STR;
    else
        return Node_ID_TimeStamp_to_string(idT);
}

/**
 * Convert Edge ID key into standardized Edge ID stamp.
 * 
 * @return a string with the Edge ID stamp (or NODE_NULLKEY_STR).
 */
inline std::string Edge_ID_key::str() const {
    if (isnullkey())
        return NODE_NULLKEY_STR;
    else
        return Node_ID_TimeStamp_to_string(dep.idT)+'>'+Node_ID_TimeStamp_to_string(sup.idT);
}

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

#endif // __GRAPHTYPES_HPP
