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

#include <ctime>
#include <cstdint>
#include "error.hpp"
#include "TimeStamp.hpp"

#ifndef MAXTIME_T // ---------- MAXTIME_T

#ifdef _TYPEBITS
#define HITIME_T ((time_t)(((unsigned long)1) << (_TYPEBITS(time_t) - 1)))
#else
#define HITIME_T ((time_t)(((unsigned long)1) << (BITS(time_t) - 1)))
#endif

/*
   The following is a precaution for the localtime() function on 64 bit platforms,
   because localtime() often cannot produce time stamps with years greater than 9999.
*/
#ifdef __x86_64__
#define MAXTIME_T 253202544000
#else
#define MAXTIME_T ((time_t)(~HITIME_T))
#endif

#endif // --------------------- MAXTIME_T

// Some experimental options mostly used only while thinking through the optimal solution
//#define EXP_GRAPH_IS_FIXEDARRAY
#define EXP_GRAPH_IS_MAP
//#define EXP_GRAPH_IS_UNORDERED_MAP
//#define EXP_GRAPH_IS_STACK
//#define EXP_GRAPH_IS_VECTOR
#define EXP_NODETOPICS_IS_SET
//#define EXP_NODETOPICS_IS_FIXEDARRAY // This is possibly useful as per https://trello.com/c/OUCT4oPa

#ifdef EXP_GRAPH_IS_MAP
#include <map>
#endif
#ifdef EXP_NODETOPICS_IS_SET
#include <set>
#endif
#include <vector>

namespace fz {

class Graph; /// Declared here for friend class declarations before further detailing.
class Node;  /// Declared here for reference before further detailing.
class Edge;  /// Declared here for reference before further detailing.

class ID_exception {
    std::string idexceptioncase;
    static constexpr const char* const stub = "attempted Node_ID construction with invalid ";
public:
    ID_exception(std::string _idexceptioncase) : idexceptioncase(_idexceptioncase) {
        ADDERROR("Node_ID::Node_ID",stub+idexceptioncase);
    }
    std::string what() { return std::string(stub + idexceptioncase); }
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
 * Time conversion to UNIX time is carried out by the get_time() functions.
 */
struct ID_TimeStamp {
    uint8_t minor_id;
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    int16_t year;
    ID_TimeStamp(): minor_id(0), second(0), minute(0), hour(0), day(0), month(0), year(0) {}
    bool operator< (const ID_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,second,minor_id)
             < std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.second,rhs.minor_id);
    }
    bool operator== (const ID_TimeStamp& rhs) const {
        return std::tie(year,month,day,hour,minute,second,minor_id)
             == std::tie(rhs.year,rhs.month,rhs.day,rhs.hour,rhs.minute,rhs.second,rhs.minor_id);
    }
    std::tm get_local_time();
    time_t get_epoch_time() {
        std::tm tm = get_local_time();
        return mktime(&tm);
    }
};

/*
struct ID_Compare {
    uint32_t id_major;
    uint32_t id_minor;
};
*/

union Node_ID_key {
    ID_TimeStamp idT;
    //ID_Compare idC;
    //Node_ID_key(): idC( { .id_major = 0, .id_minor = 0 } ) {}
    Node_ID_key(): idT() {}
    //Node_ID_key(const ID_Compare& _idC): idC(_idC) {}
    Node_ID_key(const ID_TimeStamp& _idT);
    Node_ID_key(std::string _idS);
    bool operator< (const Node_ID_key& rhs) const {
        // Note: Using tie() here to achieve lexicographical comparison is a life saver.
        // Much time was lost trying to unravel problems doing this manually!
        // Compares the first element in the tie() first, then the second, and so forth.
        return (idT < rhs.idT);
        //return std::tie(idT.year,idT.month,idT.day,idT.hour,idT.minute,idT.second,idT.minor_id)
        //     < std::tie(rhs.idT.year,rhs.idT.month,rhs.idT.day,rhs.idT.hour,rhs.idT.minute,rhs.idT.second,rhs.idT.minor_id);
        //return std::tie(idC.id_major, idC.id_minor) < std::tie(rhs.idC.id_major, rhs.idC.id_minor);
    }
    /*bool operator< (const Node_ID_key& rhs) const {
        if (idC.id_major < rhs.idC.id_major) return true;
        if (idC.id_minor < rhs.idC.id_minor) return true;
        return false;
    }*/

    friend bool identical_Node_ID_key(const Node_ID_key & key1, const Node_ID_key & key2, std::string & trace);
};

/**
 * DEFINITELY PUT PLENTY OF DESCRIPTION HERE!
 */
class Node_ID {
protected:
    Node_ID_key idkey;
    std::string idS_cache; // cached string version of the ID to speed things up
public:
    Node_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Node_ID(const ID_TimeStamp _idT);
    Node_ID() = delete; // explicitly forbid the default constructor, just in case
    Node_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
};

struct Edge_ID_key {
    Node_ID_key dep;
    Node_ID_key sup;
    Edge_ID_key() {}
    Edge_ID_key(Node_ID_key _depkey, Node_ID_key _supkey): dep(_depkey), sup(_supkey) {}
    Edge_ID_key(std::string _idS);
    bool operator<(const Edge_ID_key &rhs) const {
        return std::tie(sup,dep) < std::tie(rhs.sup,rhs.dep);
        //return std::tie(sup.idC.id_major, sup.idC.id_minor, dep.idC.id_major, dep.idC.id_minor) < std::tie(rhs.sup.idC.id_major, rhs.sup.idC.id_minor, rhs.dep.idC.id_major, rhs.dep.idC.id_minor);
        /*if (sup.idC.id_major < rhs.sup.idC.id_major) return true;
        if (sup.idC.id_minor < rhs.sup.idC.id_minor) return true;
        if (dep.idC.id_major < rhs.dep.idC.id_major) return true;
        if (dep.idC.id_minor < rhs.dep.idC.id_minor) return true;
        return false;*/
    }
    friend bool identical_Edge_ID_key(const Edge_ID_key & key1, const Edge_ID_key & key2, std::string & trace);
};

class Edge_ID {
protected:
    Edge_ID_key idkey;
    std::string idS_cache; // cached string version of the ID to speed things up
public:
    //Edge_ID(std::string dep_idS, std::string sup_idS); // Not clear that this one is every needed
    Edge_ID(std::string _idS): idkey(_idS), idS_cache(_idS) {}
    Edge_ID(Edge_ID_key _idkey);
    Edge_ID(Node &_dep, Node &_sup);
    Edge_ID() = delete; // explicitly forbid the default constructor, just in case
    Edge_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache; }
};

struct Topic_Keyword {
    std::string keyword;
    float relevance;
    Topic_Keyword(std::string k, float r): keyword(k), relevance(r) {}
};

#define HIGH_TOPIC_INDEX_WARNING 1000 /// at this index number report a warning just in case it is in error

class Topic {
protected:
    uint16_t id;                       /// unique number that is equal to the index in the Topic_Tags vector
    uint16_t supid;                    /// optional id/index of superior topic (for grouping), none if supid==id
    std::string tag;                   /// unique tag label (the original tags are derived directly from DIL file names)
    std::string title;                 /// optional topic title (e.g. obtained from DIL file)
    std::vector<Topic_Keyword> keyrel; /// optional list of keywords and relevance ratios
public:
    Topic(uint16_t _id, std::string _tag, std::string _title): id(_id), supid(_id), tag(_tag), title(_title) {}
    uint16_t get_id() const { return id; }
    uint16_t get_supid() const { return supid; }
    std::string get_tag() const { return tag; }
    std::string get_title() const { return title; }
    const std::vector<Topic_Keyword> & get_keyrel() const { return keyrel; }

    void set_supid(uint16_t _supid) { supid = _supid; }

    friend bool identical_Topics(const Topic & topic1, const Topic & topic2, std::string & trace);
};

// Only pointers, not the objects themselves. (See Dangerous code card in Software Engineering Update Trello board.)
typedef std::vector<Topic*> Topic_Tags_Vector;

/** Topic tag data, indexed by Topic id.
 * 
 * Topic objects are created during calls to find_or_add_Topic() in this class.
 * Those objects are destroyed when the instance of this class is deleted.
 */
class Topic_Tags {
protected:
    Topic_Tags_Vector topictags;
    std::map<std::string,Topic*> topicbytag;
public:
    ~Topic_Tags() { for (auto it = topictags.begin(); it!=topictags.end(); ++it) delete (*it); }
    const Topic_Tags_Vector &get_topictags() const { return topictags; }
    uint16_t find_or_add_Topic(std::string tag, std::string title);
    Topic * find_by_id(uint16_t _id) {
        if (_id>=topictags.size()) return NULL;
        return topictags.at(_id);
    }
    Topic * find_by_tag(std::string _tag);

    friend bool identical_Topic_Tags(Topic_Tags & ttags1, Topic_Tags & ttags2, std::string & trace);
};

/**
 * For more information about td_property values, as well as future expansions, please
 * see the Formalizer documentation section <a href="https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.nu3mb52d1k6n">Target date parameters in Graph v2.0+</a>.
 * Also consider Note 2 of the documentation of dil2graph.cc:get_Node_Target_Date()
 * about target date hints in the Graph 2.0+ format parameters.
 */
enum td_property { unspecified, inherit, variable, fixed, exact, _tdprop_num };

enum td_pattern { patt_daily, patt_workdays, patt_weekly, patt_biweekly, patt_monthly, patt_endofmonthoffset, patt_yearly, OLD_patt_span, patt_nonperiodic, _patt_num };

extern const std::string td_property_str[_tdprop_num];
extern const std::string td_pattern_str[_patt_num];

#ifdef EXP_NODETOPICS_IS_SET
//typedef std::set<uint16_t> Topics_Set;
typedef std::map<uint16_t,float> Topics_Set; // to keep relevance as well
#endif
#ifdef EXP_NODETOPICS_IS_FIXEDARRAY
typedef uint16_t[16] Topics_Set;
#endif

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

typedef std::set<Edge*> Edges_Set;

/**
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
    const Node_ID id;
    Topics_Set topics; /// a map of pairs of unique topic tag index and relevance value
    float valuation;  /// presently only using values 0.0 and greater (typically [1.0,3.0])
    float completion; /// 1.0 = done, -1.0 = obsolete, -2.0 = replaced, -3.0 = done differently, -4.0 = no longer possible / did not come to pass
    time_t required;  /// seconds
    std::string text; /// by default assumed to contain UTF8 HTML5
    time_t targetdate;      /// when tdproperty=unspecified then targetdate should be set to -1
    td_property tdproperty; /// unspecified, inherit, variable, fixed, exact
    bool repeats;           /// must be false if tdproperty is unspecified or variable
    td_pattern tdpattern;   // can be used to remember an optional periodicity even if isperiodic=false
    int tdevery;            /// multiplier for pattern interval
    int tdspan;             /// count of number of repetitions

    Graph *graph;          /// this is set when the Node is added to a Graph
    Edges_Set supedges;    /// this set maintained for rapid Edge access to superior Nodes
    Edges_Set depedges;    /// this set maintained for rapid Edge access to dependency Nodes
    #define SEM_TRAVERSED 1
    mutable int semaphore; /// used to detect graph traversal etc.

    int get_semaphore() { return semaphore; }
    void set_semaphore(int sval) { semaphore = sval; }
    bool set_all_semaphores(int sval);
    time_t nested_inherit_targetdate(); // only called by the same or by inherit_targetdate()
    time_t inherit_targetdate(); // may be called by effective_targetdate()

public:
    Node(std::string id_str) : id(id_str) {}

    // safely inspect immediate object data
    const Node_ID &get_id() const { return id; }
    const Topics_Set &get_topics() const { return topics; }
    float get_valuation() const { return valuation; }
    float get_completion() const { return completion; }
    time_t get_required() const { return required; }
    const std::string & get_text() const { return text; }
    time_t get_targetdate() const { return targetdate; }
    std::string get_targetdate_str() const { return TimeStampYmdHM(targetdate); }
    td_property get_tdproperty() const { return tdproperty; }
    bool get_repeats() const { return repeats; }
    td_pattern get_tdpattern() const { return tdpattern; }
    int get_tdevery() const { return tdevery; }
    int get_tdspan() const { return tdspan; }

    // change immediate object data
    bool add_topic(Topic_Tags &topictags, uint16_t topicid, float topicrelevance);
    bool add_topic(Topic_Tags &topictags, std::string tag, std::string title, float topicrelevance);
    bool add_topic(uint16_t topicid, float topicrelevance); /// Use this version if the Node is already in a Graph
    bool add_topic(std::string tag, std::string title, float topicrelevance); /// Use this version if the Node is already in a Graph
    bool remove_topic(uint16_t id);
    bool remove_topic(std::string tag);
    void set_valuation(float v) { valuation = v; }
    void set_completion(float c) { completion = c; }
    void set_required(time_t Treq) { required = Treq; }
    void set_text(const std::string utf8str);
    void set_text_unchecked(const std::string utf8str) { text = utf8str; } /// Use only where guaranteed!
    void set_targetdate(time_t t) { targetdate = t; }
    void set_tdproperty(td_property tprop) { tdproperty = tprop; }
    void set_repeats(bool r) { repeats = r; }
    void set_tdpattern(td_pattern tpat) { tdpattern = tpat; }
    void set_tdevery(int multiplier) { tdevery = multiplier; }
    void set_tdspan(int count) { tdspan = count; }

    // Graph relative operations
    time_t effective_targetdate();

    friend bool identical_Nodes(Node & node1, Node & node2, std::string & trace);

};

class Edge {
    friend class Node;
protected:
    const Edge_ID id;
    float dependency;
    float significance; // (also known as unbounded importance)
    float importance; // (also known as bounded importance)
    float urgency; // (also known as computed urgency)
    float priority; // (also known as computed priority)

    Node* dep; // rapid access
    Node* sup; // rapid access

public:
    Edge(Node &_dep, Node &_sup): id(_dep,_sup), dep(&_dep), sup(&_sup) {}
    Edge(Graph & graph, std::string id_str);
    // safely inspect edge data
    auto get_id() const { return id; }
    Node* get_dep() const { return dep; }
    Node* get_sup() const { return sup; }
    float get_dependency() const { return dependency; }
    float get_significance() const { return significance; }
    float get_importance() const { return importance; }
    float get_urgency() const { return urgency; }
    float get_priority() const { return priority; }

    // change edge data
    void set_dependency(float d) { dependency = d; }
    void set_significance(float s) { significance = s; }
    void set_importance(float i) { importance = i; }
    void set_urgency(float u) { urgency = u; }
    void set_priority(float p) { priority = p; }

    friend bool identical_Edges(Edge & edge1, Edge & edge2, std::string & trace);
};

#ifdef EXP_GRAPH_IS_FIXEDARRAY
class Graph {
protected:
    Node nodes[10000];
    Edge edges[200000];
};
#endif

#ifdef EXP_GRAPH_IS_MAP

// As per: https://stackoverflow.com/questions/1102392/how-can-i-use-stdmaps-with-user-defined-types-as-key
/* Now I've defined the operator< for Node_ID anyway, so I probably don't even need this.
struct Node_ID_Compare {
    bool operator() (const Node_ID& lhs, const Node_ID& rhs) {
        return lhs < rhs;
    }
};
*/

typedef std::map<Node_ID_key,Node*> Node_Map;

typedef std::map<Edge_ID_key,Edge*> Edge_Map;

class Graph {
    friend class Node;
public:
    enum errcodes { g_noerrors, g_addnullnode, g_adddupnode, g_addnulledge, g_adddupedge, g_removenulledge, g_removeunknownedge };
    errcodes error = g_noerrors; /// Stores a code for the most recent error encountered.
protected:
    Node_Map nodes;
    Edge_Map edges;
    Topic_Tags topics;

    bool warn_loops = true;

    void set_all_semaphores(int sval);

public:
    bool add_Node(Node &node);
    bool add_Node(Node *node);
    bool add_Edge(Edge &edge);
    bool add_Edge(Edge *edge);
    bool remove_Edge(Edge &edge);
    bool remove_Edge(Edge *edge);
    // keeping nodes protected here purely as a precaution against accidental map modification
    unsigned long num_Nodes() const { return nodes.size(); }
    auto begin_Nodes() const { return nodes.begin(); }
    auto end_Nodes() const { return nodes.end(); }
    Node * Node_by_id(const Node_ID_key & id) const {
        auto it = nodes.find(id);
        if (it==nodes.end()) return NULL;
        return it->second;
    }
    unsigned long num_Edges() const { return edges.size(); }
    auto begin_Edges() const { return edges.begin(); }
    auto end_Edges() const { return edges.end(); }
    Edge * Edge_by_id(const Edge_ID_key & id) const {
        auto it = edges.find(id);
        if (it==edges.end()) return NULL;
        return it->second;
    }
    const Topic_Tags & get_topics() const { return topics; }
    Node_Map & temptonodemap() { return nodes; }

    friend bool identical_Graphs(Graph & graph1, Graph & graph2, std::string & trace);

};
#endif

bool valid_Node_ID(std::string id_str, std::string &formerror, ID_TimeStamp *id_timestamp = NULL);

bool valid_Node_ID(const ID_TimeStamp &idT, std::string &formerror);

std::string Node_ID_TimeStamp_to_string(const ID_TimeStamp idT);

} // namespace fz

#endif // __GRAPHTYPES_HPP
