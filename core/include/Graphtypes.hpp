// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file declares Shared Memory Graph, Node and Edge types for use with the Formalizer.
 * These define the shared memory authoritative version of the data structure for use in C++ code.
 * 
 * This header file contains multple sections:
 * - essential types and memory management classes
 * - essential Graph data structure classes
 * - element-wise functions operating on Graphtypes (this could be moved to a separate header)
 * 
 * Functions operating on multi-element subsets are in Graphinfo.hpp.
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
#include <algorithm>

// Boost
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace bi = boost::interprocess;

// core
#include "error.hpp"
#include "general.hpp"
#include "TimeStamp.hpp"
#include "Graphbase.hpp"

namespace fz {

// Forward declaration so that Graph_modifications can call a protected Edge constructor.
#ifndef __GRAPHMODIFY_HPP
class Graph_modifications;
#endif

/// Formalizer specific base types for ease of modification (movable pointers)
typedef bi::offset_ptr<Graph> Graph_Graph_ptr;
typedef bi::offset_ptr<Topic> Graph_Topic_ptr;
typedef bi::offset_ptr<Node> Graph_Node_ptr;
typedef bi::offset_ptr<Edge> Graph_Edge_ptr;

/// Formalizer specific base types for ease of modification (container types)
typedef bi::managed_shared_memory segment_memory_t;
typedef bi::managed_shared_memory::segment_manager segment_manager_t; // the shared memory segment manager
typedef bi::allocator<void, segment_manager_t> void_allocator; // convertible to any other allocator<T> (we can use one allocator for all inner containers)

static constexpr const char* const shared_mem_exception_stub = "attempted to obtain shared memory manager or allocator while ";

/// Exception thrown manager or allocator are requested when none are active.
class Shared_Memory_exception {
    std::string sharedmemexceptioncase;
public:
    Shared_Memory_exception(std::string _sharedmemexceptioncase) : sharedmemexceptioncase(_sharedmemexceptioncase) {
        ADDERROR("graph_mem_managers",shared_mem_exception_stub+sharedmemexceptioncase);
    }
    std::string what() { return std::string(shared_mem_exception_stub) + sharedmemexceptioncase; }
};

/// Every shared memory segment (e.g. for a different copy of a Graph) can have its own instance of these references.
struct shared_memory_manager {
    segment_memory_t * segmem_ptr;
    const void_allocator * alloc_inst_ptr;
    bool remove_on_exit;
    shared_memory_manager(segment_memory_t & _segmem, void_allocator & allocinst): segmem_ptr(&_segmem), alloc_inst_ptr(&allocinst), remove_on_exit(true) {}
};

/**
 * Keep track of multiple shared memory segments, switch which one is
 * considered active for allocator use, and provide manager and allocator
 * references.
 * 
 * This class simplifies working with one or more shared memory segments,
 * for example, for separate copies of the Graph, while still keeping
 * memory allocation simple for container classes by having them work
 * with the active set. (No need to provide allocators as parameters in
 * functions within Graphtypes and beyond.)
 * 
 * Note that some of the methods in this class will throw an exception
 * if a segment manager or void allocator is requested when none are
 * active (nullptr). This is a safety measure to ensure that programs
 * using this class do in fact set up at least one manager and allocator.
 */
class graph_mem_managers {
protected:
    std::map<std::string, shared_memory_manager> managers;
    std::map<std::string, shared_memory_manager>::iterator active_it;
    shared_memory_manager * active;
    std::string active_name;
    //bool remove_on_exit;
    std::map<std::string, shared_memory_manager>::iterator cache_it; // used when temporarily switching
public:
    graph_mem_managers(): active_it(managers.end()), active(nullptr) {} //, remove_on_exit(true) {}
    ~graph_mem_managers();
    bool add_manager(std::string segname, segment_memory_t & segmem, void_allocator & allocinst);
    /**
     * Remove a manager from the set of managers and delete the
     * corresponding allocator and segment memory objects -
     * but do not destroy the shared memory.
     * 
     * Call this after you have finished working with the shared memory
     * that another process created and that you are not responsible for.
     * By removing the manager and associated objects you can prevent
     * accidentally working with stale shared memory pointes, and you can
     * receive new pointers with the same name in the future.
     * 
     * Note: Removing shared memory is a separate operation that can
     *       be called explicitly or by having set `remove_on_exit`. It
     *       is normally the responsibility of the process that created
     *       the shared memory to do so.
     * 
     * @param segname Name of the managed segment to forget.
     * @param return True if the named manager existed and was successfully removed.
     */
    bool forget_manager(std::string segname);
    bool set_active(std::string segname);
    bool set_active_it(std::map<std::string, shared_memory_manager>::iterator _active_it);
    void set_remove_on_exit(bool _removeonexit) { if (active) active->remove_on_exit = _removeonexit; }
    shared_memory_manager * get_active() const { return active; }
    auto get_active_it() const { return active_it; }
    const std::string & get_active_name() const { return active_name; }
    void cache() { cache_it = get_active_it(); }
    void uncache() { set_active_it(cache_it); }
    segment_memory_t * get_segmem() const;
    const segment_manager_t & get_segman() const;
    const void_allocator & get_allocator() const; ///< Get allocator for the active segment.
    const void_allocator & get_allocator(std::string segname) const; ///< Get allocator for named segment (does not change active)

    segment_memory_t * allocate_and_activate_shared_memory(std::string segment_name, unsigned long segmentsize);
    //std::unique_ptr<Graph> allocate_Graph_in_shared_memory(); // *** gets tricky with Boost Interprocess
    Graph_ptr allocate_Graph_in_shared_memory(); ///< server, allocate a shared memory segment and construct an empty Graph
    Graph_ptr find_Graph_in_shared_memory(); ///< client, find a Graph in an existing shared memory segment
    /**
     * Get a Graph pointer from a pointer variable or set that variable
     * to the Graph address in shared memory if the variable was set to
     * nullptr.
     * 
     * For example, see how this is used in Graphmodify.cpp and in
     * fzgraphedit.cpp.
     * 
     * Note: If the pointer variable provided is not nullptr then it is
     *       assumed that it contains a valid pointer to a Graph in
     *       shared memory. Therefore, make sure to initialize the
     *       pointer variable to nullptr at the start of a program!
     * 
     * @param graph_ptr A reference to a pointer variable.
     * @return The address of a Graph in shared memory or nullptr if not found.
     */
    Graph_ptr get_Graph(Graph_ptr & graph_ptr);
    void info(Graph_info_label_value_pairs & meminfo);
    std::string info_str();
};

extern graph_mem_managers graphmemman; ///< Global access to shared memory managers for Graph data structures.

static constexpr size_t Graph_ID_STR_LEN = 2*20; // Must fit both Node and Edge ID strings.

typedef bi::allocator<char, segment_manager_t> char_allocator;
// There is no need for GraphIDcache to be a container class with allocator (see TL#202010150635.1).
//typedef bi::basic_string<char, std::char_traits<char>, char_allocator> GraphIDcache;
struct GraphIDcache {
    char s[Graph_ID_STR_LEN] = { 0 }; // initialize whole buffer to null
    GraphIDcache(std::string _s) { safecpy(_s, s, Graph_ID_STR_LEN); }
    GraphIDcache & operator= (std::string _s) { safecpy(_s, s, Graph_ID_STR_LEN); return *this; }
    const char * c_str() const { return s; }
};

static constexpr size_t Named_List_String_LEN = 80;
struct Named_List_String {
    char s[Named_List_String_LEN] = { 0 }; // initialize whole buffer to null
    Named_List_String() {}
    Named_List_String(std::string _s) { safecpy(_s, s, Named_List_String_LEN); }
    Named_List_String & operator= (std::string _s) { safecpy(_s, s, Named_List_String_LEN); return *this; }
    const char * c_str() const { return s; }
    bool operator< (const Named_List_String& rhs) const {
        return std::lexicographical_compare(s,s+Named_List_String_LEN,rhs.s,rhs.s+Named_List_String_LEN);
    }
    bool operator== (const Named_List_String& rhs) const {
        std::string s1(c_str());
        std::string s2(rhs.c_str());
        return s1 == s2;
    }
};

typedef bi::basic_string<char, std::char_traits<char>, char_allocator> Keyword_String;
typedef bi::basic_string<char, std::char_traits<char>, char_allocator> Topic_String;
typedef bi::basic_string<char, std::char_traits<char>, char_allocator> Node_utf8_text;
//typedef bi::basic_string<char, std::char_traits<char>, char_allocator> Named_List_String;
typedef bi::basic_string<char, std::char_traits<char>, char_allocator> Server_Addr_String;

typedef bi::allocator<Topic_Keyword, segment_manager_t> Topic_Keyword_allocator;
typedef bi::vector<Topic_Keyword, Topic_Keyword_allocator> Topic_KeyRel_Vector;

typedef bi::allocator<Graph_Topic_ptr, segment_manager_t> TopicPtr_allocator;
typedef bi::vector<Graph_Topic_ptr, TopicPtr_allocator> Topic_Tags_Vector;

//typedef bi::allocator<Graph_Node_ptr, segment_manager_t> NodePtr_allocator;
//typedef bi::vector<Graph_Node_ptr, NodePtr_allocator> Node_Index;

typedef bi::allocator<Graph_Edge_ptr, segment_manager_t> EdgePtr_allocator;
typedef bi::set<Graph_Edge_ptr, std::less<Graph_Edge_ptr>, EdgePtr_allocator> Edges_Set;

typedef std::pair<const Topic_String, Graph_Topic_ptr> Topic_Map_value_type;
//typedef std::pair<Topic_String, Graph_Topic_ptr> movable_to_Topic_Map_value_type;
typedef bi::allocator<Topic_Map_value_type, segment_manager_t> Topic_Map_value_type_allocator;
typedef bi::map<Topic_String, Graph_Topic_ptr, std::less<Topic_String>, Topic_Map_value_type_allocator> TopicbyTag_Map;

typedef std::pair<const Topic_ID, float> Topics_Set_value_type;
//typedef std::pair<Topic_ID, float> movable_to_Topics_Set_value_type;
typedef bi::allocator<Topics_Set_value_type, segment_manager_t> Topics_Set_value_type_allocator;
typedef bi::map<Topic_ID, float, std::less<Topic_ID>, Topics_Set_value_type_allocator> Topics_Set;

typedef std::pair<const Node_ID_key, Graph_Node_ptr> Node_Map_value_type;
//typedef std::pair<Node_ID_key, Graph_Node_ptr> movable_to_Node_Map_value_type;
typedef bi::allocator<Node_Map_value_type, segment_manager_t> Node_Map_value_type_allocator;
typedef bi::map<Node_ID_key, Graph_Node_ptr, std::less<Node_ID_key>, Node_Map_value_type_allocator> Node_Map;

typedef std::pair<const Edge_ID_key, Graph_Edge_ptr> Edge_Map_value_type;
//typedef std::pair<Edge_ID_key, Graph_Edge_ptr> movable_to_Edge_Map_value_type;
typedef bi::allocator<Edge_Map_value_type, segment_manager_t> Edge_Map_value_type_allocator;
typedef bi::map<Edge_ID_key, Graph_Edge_ptr, std::less<Edge_ID_key>, Edge_Map_value_type_allocator> Edge_Map;

typedef bi::allocator<Node_ID_key, segment_manager_t> Node_ID_key_allocator;
/**
 * A type used for named Lists (or ordered collections) of Nodes.
 * For detailed information see https://trello.com/c/zcUpEAXi, and for reasons
 * why these Lists store Node ID keys instead of shared-memory pointers to
 * Nodes see:
 * https://trello.com/c/zcUpEAXi/189-fzserverpq-named-lists#comment-5fac08dc178a257eb6f953ac
 */
typedef bi::deque<Node_ID_key, Node_ID_key_allocator> Node_List;
typedef bi::set<Node_ID_key, std::less<Node_ID_key>, Node_ID_key_allocator> Node_Set;
//typedef bi::vector<const Node_ID_key, Node_ID_key_allocator> Node_List;

/**
 * Node ID that caches its ID stamp for frequent use.
 */
class Node_ID {
protected:
    Node_ID_key idkey;
    GraphIDcache idS_cache; // cached string version of the ID to speed things up
public:
    Node_ID(std::string _idS): idkey(_idS), idS_cache(_idS.c_str()) {} // idS_cache(_idS.c_str(), graphmemman.get_allocator()) {}
    Node_ID(const ID_TimeStamp _idT);

    Node_ID() = delete; // explicitly forbid the default constructor, just in case

    Node_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache.c_str(); }
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
    Edge_ID(std::string _idS): idkey(_idS), idS_cache(_idS.c_str()) {} // , graphmemman.get_allocator()) {} /// Try to use this one only for container element initialization and such.
    Edge_ID(Edge_ID_key _idkey);
    Edge_ID(Node &_dep, Node &_sup);

    Edge_ID() = delete; // explicitly forbid the default constructor, just in case

    Edge_ID_key key() const { return idkey; }
    std::string str() const { return idS_cache.c_str(); }
};

struct Topic_Keyword {
    Keyword_String keyword;
    Keyword_Relevance relevance;

    Topic_Keyword(std::string k, Keyword_Relevance r): keyword(k.c_str(), graphmemman.get_allocator()), relevance(r) {}
};

class Topic {
protected:
    Topic_ID id;                /// unique number that is equal to the index in the Topic_Tags vector
    Topic_ID supid;             /// optional id/index of superior topic (for grouping), none if supid==id
    Topic_String tag;           /// unique tag label (the original tags are derived directly from DIL file names)
    Topic_String title;         /// optional topic title (e.g. obtained from DIL file)
    Topic_KeyRel_Vector keyrel; /// optional list of keywords and relevance ratios

public:
    // To make sure this is only created via bi::construc or as part of Topic_Tags_Vector (there via [] operator).
    Topic(Topic_ID _id, std::string _tag, std::string _title) : id(_id), supid(_id), tag(_tag.c_str(), graphmemman.get_allocator()),
                                                                title(_title.c_str(), graphmemman.get_allocator()),
                                                                keyrel(graphmemman.get_allocator()) {}

public:
    /// safely inspect parameters
    Topic_ID get_id() const { return id; }
    Topic_ID get_supid() const { return supid; }
    const Topic_String & get_tag() const { return tag; }
    const Topic_String & get_title() const { return title; }

    /// table references
    const Topic_KeyRel_Vector & get_keyrel() const { return keyrel; }

    /// change parameters
    void set_supid(Topic_ID _supid) { supid = _supid; }

    /// helper functions
    std::string keyrel_str() const;

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
    Topic_Tags(): topictags(graphmemman.get_allocator()), topicbytag(graphmemman.get_allocator()) {}
    //~Topic_Tags() { for (auto it = topictags.begin(); it!=topictags.end(); ++it) delete (*it); }

    /// tables references
    const Topic_Tags_Vector &get_topictags() const { return topictags; }
    const TopicbyTag_Map &get_topicbytag() const { return topicbytag; }

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
    Topic * find_by_id(Topic_ID _id) const; // inlined below

    /**
     * Search the topicbytag map and return a pointer to the Topic if the tag
     * was found.
     * 
     * @param _tag a topic tag label
     * @return pointer to Topic object in topictags vector, or NULL if not found.
     */
    Topic * find_by_tag(const std::string _tag) const;

    std::map<std::string, Topic_ID> tag_by_index() const;

    std::vector<Topic_ID> tags_to_indices(std::vector<std::string> tagsvector) const;

    /// friend (utility) functions
    friend bool identical_Topic_Tags(Topic_Tags & ttags1, Topic_Tags & ttags2, std::string & trace);
};

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
    Graphsigned tdspan;      /// count of number of repetitions (0 means unlimited)

    Graph_Graph_ptr graph;   /// this is set when the Node is added to a Graph
    Edges_Set supedges; /// this set maintained for rapid Edge access to superior Nodes
    Edges_Set depedges; /// this set maintained for rapid Edge access to dependency Nodes
 
    Edit_flags editflags;   /// flags used to indicate Node data that has been modified (or should be modified, https://trello.com/c/eUjjF1yZ)

    #define SEM_TRAVERSED 1
    mutable int semaphore; /// used to detect graph traversal etc.

    int get_semaphore() { return semaphore; }
    void set_semaphore(int sval) { semaphore = sval; }
    bool set_all_semaphores(int sval);
    time_t nested_inherit_targetdate(Node_ptr & origin); // only called by the same or by inherit_targetdate()
    time_t inherit_targetdate(Node_ptr * origin = nullptr);        // may be called by effective_targetdate()

public:
    // Protected constructor to ensure Nodes are created in the correct type of memory.
    Node(std::string id_str) : id(id_str.c_str()), topics(graphmemman.get_allocator()),
                               text(graphmemman.get_allocator()), supedges(graphmemman.get_allocator()),
                               depedges(graphmemman.get_allocator()) {}
    //Node() : id(""), topics(graphmemman.get_allocator()),
    //                           text(graphmemman.get_allocator()), supedges(graphmemman.get_allocator()),
    //                           depedges(graphmemman.get_allocator()) {}

public:
    // No public constructor to ensure that Nodes are created by Graphs that are allocator aware.

    // safely inspect data
    const Node_ID &get_id() const { return id; }
    std::string get_id_str() const { return id.str(); }

    float get_valuation() const { return valuation; }
    float get_completion() const { return completion; }
    time_t get_required() const { return required; }
    long get_required_minutes() const { return required/60; }
    float get_required_hours() const { return ((float)required)/3600.0; }
    time_t seconds_applied() const { return completion*(float)required; }
    long minutes_applied() const { return seconds_applied()/60; }
    float hours_applied() const { return ((float)minutes_applied())/60.0; }
    time_t seconds_to_complete() const; // inlined below
    long minutes_to_complete() const { return seconds_to_complete()/60; }
    float hours_to_complete() const { return ((float)seconds_to_complete())/3600.0; }

    const Node_utf8_text & get_text() const { return text; }

    time_t get_targetdate() const { return targetdate; }
    std::string get_targetdate_str() const { return TimeStampYmdHM(targetdate); }
    td_property get_tdproperty() const { return tdproperty; }
    bool td_unspecified() const { return tdproperty == td_property::unspecified; }
    bool td_inherit() const { return tdproperty == td_property::inherit; }
    bool td_variable() const { return tdproperty == td_property::variable; }
    bool td_fixed() const { return tdproperty == td_property::fixed; }
    bool td_exact() const { return tdproperty == td_property::exact; }

    bool get_repeats() const { return repeats; }
    td_pattern get_tdpattern() const { return tdpattern; }
    bool daily() const { return tdpattern == td_pattern::patt_daily; }
    bool workdays() const { return tdpattern == td_pattern::patt_workdays; }
    bool weekly() const { return tdpattern == td_pattern::patt_weekly; }
    bool biweekly() const { return tdpattern == td_pattern::patt_biweekly; }
    bool monthly() const { return tdpattern == td_pattern::patt_monthly; }
    bool endofmonthoffset() const { return tdpattern == td_pattern::patt_endofmonthoffset; }
    bool yearly() const { return tdpattern == td_pattern::patt_yearly; }
    int get_tdevery() const { return tdevery; }
    int get_tdspan() const { return tdspan; }

    const Topics_Set &get_topics() const { return topics; }

    /// edit flags specify which Node parameters have been modified from stored values
    const Edit_flags & get_editflags() { return editflags; }
    void clear_editflags() { editflags.clear(); }

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
    void set_text_unchecked(const std::string utf8str) { text = utf8str.c_str(); } /// Use only where guaranteed!

    /// change parameters: scheduling
    void set_targetdate(time_t t) { targetdate = t; }
    void set_tdproperty(td_property tprop) { tdproperty = tprop; }
    void set_repeats(bool r) { repeats = r; }
    void set_tdpattern(td_pattern tpat) { tdpattern = tpat; }
    void set_tdevery(int multiplier) { tdevery = multiplier; }
    void set_tdspan(int count) { tdspan = count; }

    void copy_content(Node & from_node);
    void edit_content(Node & from_node, const Edit_flags & edit_flags);
    void edit_content(const Node_data & nodedata, const Edit_flags & edit_flags);

    /// Graph relative operations
    time_t effective_targetdate(Node_ptr * origin = nullptr);

    /// helper functions

    /**
     * Returns a vector of target dates, including those determined by the Node's
     * repeat pattern and span, up to a specified maximum time.
     * 
     * @param t_max The maximum time to include in the vector.
     * @param N_max Maximum size of list to return (zero means no size limit).
     * @param t An optional start time, defaults to a Node's effective target date. See how this is used in Graphinfo.cpp:Nodes_with_repeats_by_targetdate().
     * @return A vector of UNIX epoch times.
     */
    std::vector<time_t> repeat_targetdates(time_t t_max, size_t N_max = 0, time_t t = RTt_unspecified);

    /**
     * Report the main Topic Index-ID of the Node, as indicated by the maximum
     * `Topic_Relevance` value.
     * 
     * @return Topic_ID of main Topic.
     */
    Topic_ID main_topic_id();

    /**
     * Reports if the Node is a member of a specific Topic and optionally returns
     * the associted relevance value.
     * 
     * @param topictag A Topic tag string.
     * @param topicrel Pointer to a variable that can receive the relevance value (if not nullptr).
     * @param topictags Optional pointer to the Topic Tags set to use (uses internal graph reference if nullptr).
     * @return True if the Node is a member of the Topic.
     */
    bool in_topic(const std::string topictag, float * topicrel = nullptr, Topic_Tags * topictags = nullptr);

    /**
     * Returns a Node's Topics and Topic relevance values as a map of Topic tags and floats.
     * 
     * @param topictags Optional pointer to the Topic Tags set to use (uses internal graph reference if nullptr).
     * @return A map of strings to floats representing Topic tags and their respective relevance values.
     */
    std::map<std::string, float> Topic_TagRels(Topic_Tags * topictags = nullptr);

    const Edges_Set & sup_Edges() const { return supedges; }
    const Edges_Set & dep_Edges() const { return depedges; }

    /// friend (utility) functions
    friend Topic * main_topic(const Topic_Tags & topictags, const Node & node); // friend function to ensure search with available Topic_Tags
    friend Topic * main_topic(Graph & _graph, Node & node);
    friend bool identical_Nodes(Node & node1, Node & node2, std::string & trace);
};

class Edge {
    friend class Graph;
    friend class Node;
    friend class Graph_modifications;
protected:
    const Edge_ID id;
    Graphdecimal dependency;
    Graphdecimal significance; // (also known as unbounded importance)
    Graphdecimal importance;   // (also known as bounded importance)
    Graphdecimal urgency;      // (also known as computed urgency)
    Graphdecimal priority;     // (also known as computed priority)

    Graph_Node_ptr dep; // rapid access
    Graph_Node_ptr sup; // rapid access

public:
    // Create only through graph with awareness of allocators.
    Edge(Node &_dep, Node &_sup): id(_dep,_sup), dep(&_dep), sup(&_sup) {}
    Edge(Graph & graph, std::string id_str);

    // These were designed for Graph_modifications to use in building a modifications request stack.
    Edge(const Node_ID_key & _dep, const Node_ID_key & _sup): id(Edge_ID_key(_dep, _sup)) {}
    Edge(const Edge_ID_key & ekey): id(ekey) {}

public:
    // safely inspect data
    Edge_ID get_id() const { return id; }
    std::string get_id_str() const { return id.str(); }
    Edge_ID_key get_key() const { return id.key(); }
    Node_ID_key get_dep_key() const { return id.key().dep; }
    Node_ID_key get_sup_key() const { return id.key().sup; }
    std::string get_dep_str() const { return id.key().dep.str(); }
    std::string get_sup_str() const { return id.key().sup.str(); }
    Node* get_dep() const { return dep.get(); }
    Node* get_sup() const { return sup.get(); }
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

    void copy_content(Edge & from_edge);

    /// friend (utility) functions
    friend bool identical_Edges(Edge & edge1, Edge & edge2, std::string & trace);
};

/**
 * A named List (or ordered collections) of Nodes.
 * For detailed information see https://trello.com/c/zcUpEAXi.
 */
struct Named_Node_List {
    constexpr static std::int_fast16_t prepend_mask{ 0b0000'0000'0000'0001 }; // prepend instead of append
	constexpr static std::int_fast16_t unique_mask{ 0b0000'0000'0000'0010 };  // no duplicates (a set)
	constexpr static std::int_fast16_t fifo_mask{ 0b0000'0000'0000'0100 };    // FIFO replacement if maxsize is reached 
    //Named_List_String name; // *** the name is the map key
    Node_List list; // a deque that keeps elements in order received
    Node_Set set; // an additional key set that is only used if unique
protected:
    int16_t features;
    int32_t maxsize; ///< 0 means no limit and is the default
public:
    Named_Node_List(): list(graphmemman.get_allocator()), set(graphmemman.get_allocator()), features(0), maxsize(0) {} // name("", graphmemman.get_allocator()),
    Named_Node_List(const Node_ID_key & nkey, int16_t _features = 0, int32_t _maxsize = 0): list(graphmemman.get_allocator()), set(graphmemman.get_allocator()), features(_features), maxsize(_maxsize) { add(nkey); }
    //Named_Node_List(const Node_ID_key & nkey): list(graphmemman.get_allocator()), features(0) { list.emplace_back(nkey); }
    bool prepend() { return (features & prepend_mask) != 0; }
    bool unique() { return (features & unique_mask) != 0; }
    bool fifo() { return (features & fifo_mask) != 0; }
    bool add(const Node_ID_key & nkey);
    bool remove(const Node_ID_key & nkey);
    int16_t get_features() { return features; }
    int32_t get_maxsize() { return maxsize; }
    size_t size() { return list.size(); }
    bool contains(const Node_ID_key & nkey) {
        return (std::find(list.begin(), list.end(), nkey) != list.end());
    }
};
typedef Named_Node_List * Named_Node_List_ptr; // use this pointer only within the context of one program (not to be stored in shared memory)
typedef std::pair<const Named_List_String, Named_Node_List> Named_Node_List_Map_value_type;
typedef bi::allocator<Named_Node_List_Map_value_type, segment_manager_t> Named_Node_List_Map_value_type_allocator;
typedef bi::map<Named_List_String, Named_Node_List, std::less<Named_List_String>, Named_Node_List_Map_value_type_allocator> Named_Node_List_Map;
/**
 * This shared memory data structure is used in Graphmodify and fzserverpq.
 * (Perhaps it should be defined in Graphmodify.hpp.)
 */
struct Named_Node_List_Element {
    Named_List_String name;
    const Node_ID_key nkey;
    Named_Node_List_Element() {}
    Named_Node_List_Element(const std::string _name, const Node_ID_key & _nkey) : name(_name), nkey(_nkey) {}
};
typedef bi::offset_ptr<Named_Node_List_Element> Named_Node_List_Element_ptr; // this pointer can be used in shared memory (e.g. see Graphmod_data)

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
    Named_Node_List_Map namedlists;

    bool persistent_NNL = true; ///< Default is to synchronize Named Node Lists between in-memory and database state.

    uint16_t port_number = 8090; ///< Default Graph server port number (the server must update this cache).
    Server_Addr_String server_IP_str;   ///< Shared memory cache of active server IP address.

    bool warn_loops = true;

    void set_all_semaphores(int sval);

public:
    Graph(): nodes(graphmemman.get_allocator()), edges(graphmemman.get_allocator()), namedlists(graphmemman.get_allocator()), server_IP_str(graphmemman.get_allocator()) {}

    /// tables references
    const Topic_Tags & get_topics() const { return topics; }
    const Node_Map & get_nodes() const { return nodes; }

    /// tables sizes
    Node_Map::size_type num_Nodes() const { return nodes.size(); }
    Edge_Map::size_type num_Edges() const { return edges.size(); }
    Topic_Tags_Vector::size_type num_Topics() const { return topics.num_Topics(); }

    /// nodes table: extend
    bool add_Node(Node &node); // only allow Nodes allocated in the active shared segment
    bool add_Node(Node *node); // only allow Nodes allocated in the active shared segment
    Node * create_Node(std::string id_str); // create Node in the active shared segment
    Node * create_and_add_Node(std::string id_str); // create and immediately insert

    /// edges table: extend
    bool add_Edge(Edge &edge); // only allow Edges allocated in the active shared segment
    bool add_Edge(Edge *edge);
    Edge * create_Edge(Node &_dep, Node &_sup); // creates (without adding to Graph)
    Edge * create_and_add_Edge(std::string id_str); // create and immediately insert

    /// edges table: reduce
    bool remove_Edge(Edge &edge);
    bool remove_Edge(Edge *edge);
 
    /// nodes table: get node
    auto begin_Nodes() const { return nodes.begin(); }
    auto end_Nodes() const { return nodes.end(); }
    Node * Node_by_id(const Node_ID_key & id) const; // inlined below
    Node * Node_by_idstr(std::string idstr) const; // inlined below
    Node_Index get_Indexed_Nodes() const;

    /// edges table: get edge
    auto begin_Edges() const { return edges.begin(); }
    auto end_Edges() const { return edges.end(); }
    Edge * Edge_by_id(const Edge_ID_key & id) const; // inlined below
    Edge * Edge_by_idstr(std::string idstr) const; // inlined below

    /// topics table: get topic
    Topic * find_Topic_by_id(Topic_ID _id) { return topics.find_by_id(_id); }
    Topic * find_Topic_by_tag(const std::string _tag) { return topics.find_by_tag(_tag); }
    std::string find_Topic_Tag_by_id(Topic_ID _id);
    bool topics_exist(const Topics_Set & topicsset); // See how fzserverpq uses this.

    /// namedlists
    std::vector<std::string> get_List_names() const;
    Named_Node_List_ptr get_List(const std::string _name);
    Named_Node_List_ptr add_to_List(const std::string _name, const Node & node, int16_t _features = 0, int32_t _maxsize = 0);
    bool add_to_List(Named_Node_List & nnl, const Node & node) { return nnl.add(node.get_id().key()); }
    bool remove_from_List(const std::string _name, const Node_ID_key & nkey);
    bool delete_List(const std::string _name);
    void reset_Lists() { namedlists.clear(); }
    bool persistent_Lists() const { return persistent_NNL; }
    void set_Lists_persistence(bool _persistent) { persistent_NNL = _persistent; }
    /// See detailed function description in the `Graphtypes.cpp` file.
    size_t copy_List_to_List(const std::string from_name, const std::string to_name, size_t from_max = 0, size_t to_max = 0, int16_t _features = -1, int32_t _maxsize = -1);
    ssize_t edit_all_in_List(const std::string _name, const Edit_flags & editflags, const Node_data & nodedata);

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
    Topic * main_Topic_of_Node(const Node & node) const { return main_topic(topics,node); }

    std::string get_server_IPaddr() { return server_IP_str.c_str(); }
    void set_server_IPaddr(std::string _ipaddrstr) { server_IP_str = _ipaddrstr.c_str(); }
    uint16_t get_server_port() { return port_number; }
    void set_server_port(uint16_t _portnumber) { port_number = _portnumber; }
    std::string get_server_port_str() { return std::to_string(port_number); }
    std::string get_server_full_address() { return get_server_IPaddr() + ':' + get_server_port_str(); }

    /// friend (utility) functions
    friend bool identical_Graphs(Graph & graph1, Graph & graph2, std::string & trace);
};

// +----- begin: standardization functions -----+

/**
 * Returns a vector of label string and relevance value pairs for a
 * specified Node within a specified Graph.
 * 
 * Note: Also see the `Graphinfo:Topic_IDs_to_Tags()` function.
 */
Tag_Label_Real_Value_Vector Topic_tags_of_Node(Graph & graph, Node & node);

// +----- end  : standardization functions -----+

// +----- begin: inline member functions -----+

/**
 * Find a Topic in the Topic_Tags table by Topic ID.
 * 
 * @param _id a Topic ID.
 * @return pointer to the Topic (or nullptr if not found).
 */
inline Topic * Topic_Tags::find_by_id(Topic_ID _id) const {
    if (_id>=topictags.size()) return nullptr;
    return topictags.at(_id).get();
}

inline time_t Node::seconds_to_complete() const {
    if ((completion<0.0) || (completion>=1.0)) { // special code or (seemingly) complete
        return 0;
    }
    return required - seconds_applied();
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
    return it->second.get();
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
    return it->second.get();
}

/**
 * Find an Edge in the Graph by its ID key from a string.
 * 
 * @param idstr a string specifying an Edge ID key.
 * @return pointer to Edge (or nullptr if not found).
 */
inline Edge * Graph::Edge_by_idstr(std::string idstr) const {
    try {
        const Edge_ID_key edgeidkey(idstr);
        return Edge_by_id(edgeidkey);

    } catch (ID_exception idexception) {
        ADDERROR(__func__, "invalid Edge ID (" + idstr + ")\n" + idexception.what());
        return nullptr;
    }
    // never gets here
}

// +----- end  : inline member functions -----+

// +----- begin: element-wise functions operating on Graphtypes -----+

typedef std::pair<Node*, Graph *> Node_Graph_ptr_pair;

/**
 * Find a Node in a Graph by its ID key from a string.
 * 
 * The Graph is obtained from shared memory (if available) if a valid pointer
 * is not already provided.
 * 
 * Note: If the Graph is found (or already known), but the Node is not found
 *       then the pair returned contains the valid Graph pointer and a nullptr
 *       for the Node.
 * 
 * @param node_idstr A string specifying a Node ID key.
 * @param graph_ptr A pointer to the Graph, if previously identified in shared memory.
 * @return a pair of valid pointers to a Node and to the Graph, or nullptr for Node or Graph not found.
 */
Node_Graph_ptr_pair find_Node_by_idstr(const std::string & node_idstr, Graph * graph_ptr);

// +----- end  : element-wise functions operating on Graphtypes -----+

} // namespace fz

#endif // __GRAPHTYPES_HPP
