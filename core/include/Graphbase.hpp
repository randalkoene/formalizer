// Copyright 2020 Randal A. Koene
// License TBD

/** @file Graphbase.hpp
 * This header file declares Shared Memory Graph, Node and Edge types for use with the Formalizer.
 * These define the shared memory authoritative version of the data structure for use in C++ code.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __GRAPHBASE_HPP.
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
 *    ***Note 20240914: This is changing now that we are intending to use UTD Nodes
 *       differently.
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

#ifndef __GRAPHBASE_HPP
#include "coreversion.hpp"
#define __GRAPHBASE_HPP (__COREVERSION_HPP)

// std
//#include <ctime>
//#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <deque>

// core
#include "error.hpp"
#include "ReferenceTime.hpp"
#include "TimeStamp.hpp"


namespace fz {

// Forward declarations for reference before further detailing.
struct Node_ID_key;
struct Edge_ID_key;
struct Topic_Keyword;
class Topic;
class Node;
class Edge;
class Graph;

typedef Node* Node_ptr;
typedef Edge* Edge_ptr;
typedef Graph* Graph_ptr;

/// Formalizer specific base types for ease of modification (fixed size)
typedef uint8_t GraphID8bit;
typedef uint16_t GraphIDyear;
typedef float Keyword_Relevance; /// Type for real-valued Keyword relevance (to Topic), presently assumed to be in the interval [0.0,1.0]
typedef uint16_t Topic_ID;       /// Type for unique Topic IDs
typedef float Topic_Relevance;   /// Type for real-valued Topic relevance (of Node), presently assumed to be in the interval [0.0,1.0]
typedef float Graphdecimal;
typedef int Graphsigned;
typedef bool Graphflag;

typedef std::vector<Node_ptr> Node_Index;
typedef std::multimap<time_t, Node_ptr> targetdate_sorted_Nodes; ///< Sorting is done by the time_t key comparison function.
typedef std::map<const Node_ID_key, Node_ptr> key_sorted_Nodes;

typedef std::map<std::string, std::string> Graph_info_label_value_pairs;

typedef std::pair<std::string, float> Tag_Label_Real_Value; ///< Such as used with Topics and various possible defined tags.
typedef std::vector<Tag_Label_Real_Value> Tag_Label_Real_Value_Vector;

#define NODE_ID_STR_NUMCHARS 16
#define NODE_NULLKEY_STR "{null-key}"
#define HIGH_TOPIC_INDEX_WARNING 1000 /// at this index number report a warning just in case it is in error
#define UNKNOWN_TOPIC_ID UINT16_MAX   /// this value is returned if unknown, e.g see Node::main_topic()
// The Unix epoch-time equivalent of the earliest valid ID time stamp in 1999.
#define NODE_ID_FIRST_VALID_TEPOCH 915177600

/**
 * For more information about td_property values, as well as future expansions, please
 * see the Formalizer documentation section <a href="https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.nu3mb52d1k6n">Target date parameters in Graph v2.0+</a>.
 * Also consider Note 2 of the documentation of dil2graph.cc:get_Node_Target_Date()
 * about target date hints in the Graph 2.0+ format parameters.
 */
enum td_property { unspecified, inherit, variable, fixed, exact, _tdprop_num };
extern const std::string td_property_str[_tdprop_num];
extern const std::map<std::string, td_property> td_property_map;

enum td_pattern { patt_daily, patt_workdays, patt_weekly, patt_biweekly, patt_monthly, patt_endofmonthoffset, patt_yearly, OLD_patt_span, patt_nonperiodic, _patt_num };
extern const std::string td_pattern_str[_patt_num];
extern const std::map<std::string, td_pattern> td_pattern_map;

static constexpr const char* const graph_exception_stub = "attempted Graph access without valid reference to resident Graph ";

/// Exception thrown when a resident Graph is missing and
//  the error is not dealt with some other way.
class NoGraph_exception {
    std::string nographexceptioncase;
public:
    NoGraph_exception(std::string _nographexceptioncase) : nographexceptioncase(_nographexceptioncase) {
        ADDERROR("Graph::Graph",graph_exception_stub+nographexceptioncase);
    }
    std::string what() { return std::string(graph_exception_stub) + nographexceptioncase; }
};

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
    //ID_TimeStamp(const ID_TimeStamp & _idT): minor_id(_idT.minor_id), second(_idT.second), hour(_idT.hour), day(_idT.day), month(_idT.month), year(_idT.year) {}

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
    Node_ID_key(const Node_ID_key & _nodeidkey): idT(_nodeidkey.idT) {} // And this one for emplace.
    //Node_ID_key(const Node_ID_key & _nodeidkey): idT(*(const_cast<ID_TimeStamp *>(&_nodeidkey.idT))) {} // And this one for emplace.

    Node_ID_key(const ID_TimeStamp& _idT);
    Node_ID_key(std::string _idS);
    Node_ID_key(time_t t, uint8_t minor_id);

    /// standardization functions and operators
    bool isnullkey() const { return idT.month == 0; }
    bool operator< (const Node_ID_key& rhs) const { return (idT < rhs.idT); }
    bool operator== (const Node_ID_key& rhs) const { return (idT == rhs.idT); }
    std::string str() const; // inlined below

    // friend (utility) functions
    friend bool identical_Node_ID_key(const Node_ID_key & key1, const Node_ID_key & key2, std::string & trace);
};

typedef std::deque<Node_ID_key> base_Node_List; // Unshared alternative to Graphtypes:Node_List.
typedef std::set<Node_ID_key, std::less<Node_ID_key>> base_Node_Set; // Unshared alternative to Graphtypes:Node_Set.

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
    Edge_ID_key(const Edge_ID_key & _edgeidkey): dep(_edgeidkey.dep), sup(_edgeidkey.sup) {} // And this one for emplace.

    Edge_ID_key(Node_ID_key _depkey, Node_ID_key _supkey): dep(_depkey), sup(_supkey) {}
    Edge_ID_key(std::string _idS);

    /// standardization functions and operators
    bool isnullkey() const { return dep.idT.month == 0; }
    bool operator<(const Edge_ID_key &rhs) const { return std::tie(sup,dep) < std::tie(rhs.sup,rhs.dep); }
    std::string str() const; // inlined below

    friend bool identical_Edge_ID_key(const Edge_ID_key & key1, const Edge_ID_key & key2, std::string & trace);
};

typedef std::uint32_t Edit_flags_type;

extern const std::map<std::string, Edit_flags_type> flagbylabel;

/**
 * A bitmask of flags for the data components of a Node.
 * 
 * Note A: This is not only used to specify modifications (see Graphmodify), but
 *         also to build filters (see Graphinfo).
 * Note B: Graphmodify objects that are instantiated in shared memory inherit this,
 *         so this class needs to remain free of container class variables.
 */
class Edit_flags {
public:
    enum editmask : Edit_flags_type {
        topics       = 0b0000'0000'0000'0000'0000'0000'0000'0001, // A Node edit.
        valuation    = 0b0000'0000'0000'0000'0000'0000'0000'0010, // A Node edit.
        completion   = 0b0000'0000'0000'0000'0000'0000'0000'0100, // A Node edit.
        required     = 0b0000'0000'0000'0000'0000'0000'0000'1000, // A Node edit.
        text         = 0b0000'0000'0000'0000'0000'0000'0001'0000, // A Node edit.
        targetdate   = 0b0000'0000'0000'0000'0000'0000'0010'0000, // A Node edit.
        tdproperty   = 0b0000'0000'0000'0000'0000'0000'0100'0000, // A Node edit.
        repeats      = 0b0000'0000'0000'0000'0000'0000'1000'0000, // A Node edit.
        tdpattern    = 0b0000'0000'0000'0000'0000'0001'0000'0000, // A Node edit.
        tdevery      = 0b0000'0000'0000'0000'0000'0010'0000'0000, // A Node edit.
        tdspan       = 0b0000'0000'0000'0000'0000'0100'0000'0000, // A Node edit.
        topicrels    = 0b0000'0000'0000'0000'0000'1000'0000'0000, // A Node edit.
        tcreated     = 0b0000'0000'0000'0000'0001'0000'0000'0000, // A Node edit.
        supdep       = 0b0000'0000'0000'0000'0010'0000'0000'0000, // This one is not a Node edit, instead it refers to all Edge edits.
        nnl          = 0b0000'0000'0000'0000'0100'0000'0000'0000, // This one is not a Node edit, instead it refers to all Named Nodes List edits.
        dependency   = 0b0000'0000'0000'0000'1000'0000'0000'0000, // An Edge edit.
        significance = 0b0000'0000'0000'0001'0000'0000'0000'0000, // An Edge edit.
        importance   = 0b0000'0000'0000'0010'0000'0000'0000'0000, // An Edge edit.
        urgency      = 0b0000'0000'0000'0100'0000'0000'0000'0000, // An Edge edit.
        priority     = 0b0000'0000'0000'1000'0000'0000'0000'0000, // An Edge edit.
        tdpropbinpat = 0b0000'0000'0001'0000'0000'0000'0000'0000, // A Node edit (or search).
        supspecmatch = 0b0000'0000'0010'0000'0000'0000'0000'0000, // A Node search with specified superiors match.
        subtreematch = 0b0000'0000'0100'0000'0000'0000'0000'0000, // A Node search that requires belonging to a subtree.
        nnltreematch = 0b0000'0000'1000'0000'0000'0000'0000'0000, // A Node search that requires belonging to a map of subtrees with top Nodes in a NNL.
        error        = 0b0100'0000'0000'0000'0000'0000'0000'0000  // see how this is used in Node_advance_repeating()
    };
protected:
    Edit_flags_type editflags;
public:
    Edit_flags() : editflags(0) {}
    Edit_flags_type get_Edit_flags() const { return editflags; }
    void clear() { editflags = 0; }
    void set_Edit_flags(Edit_flags_type _editflags) { editflags = _editflags; }
    bool set_Edit_flag_by_label(const std::string flaglabel);
    void set_Edit_topics() { editflags |= Edit_flags::topics; }
    void set_Edit_topicrels() { editflags |= Edit_flags::topicrels; }
    void set_Edit_valuation() { editflags |= Edit_flags::valuation; }
    void set_Edit_completion() { editflags |= Edit_flags::completion; }
    void set_Edit_required() { editflags |= Edit_flags::required; }
    void set_Edit_text() { editflags |= Edit_flags::text; }
    void set_Edit_targetdate() { editflags |= Edit_flags::targetdate; }
    void set_Edit_tdproperty() { editflags |= Edit_flags::tdproperty; }
    void set_Edit_tdpropbinpat() { editflags |= Edit_flags::tdpropbinpat; }
    void set_Edit_repeats() { editflags |= Edit_flags::repeats; }
    void set_Edit_tdpattern() { editflags |= Edit_flags::tdpattern; }
    void set_Edit_tdevery() { editflags |= Edit_flags::tdevery; }
    void set_Edit_tdspan() { editflags |= Edit_flags::tdspan; }
    void set_Edit_tcreated() { editflags |= Edit_flags::tcreated; }
    void set_Edit_dependency() { editflags |= Edit_flags::dependency; }
    void set_Edit_significance() { editflags |= Edit_flags::significance; }
    void set_Edit_importance() { editflags |= Edit_flags::importance; }
    void set_Edit_urgency() { editflags |= Edit_flags::urgency; }
    void set_Edit_priority() { editflags |= Edit_flags::priority; }
    void set_Edit_supspecmatch() { editflags |= Edit_flags::supspecmatch; }
    void set_Edit_subtreematch() { editflags |= Edit_flags::subtreematch; }
    void set_Edit_nnltreematch() { editflags |= Edit_flags::nnltreematch; }
    void set_Edit_error() { editflags |= Edit_flags::error; }
    bool Edit_topics() const { return editflags & Edit_flags::topics; }
    bool Edit_topicrels() const { return editflags & Edit_flags::topicrels; }
    bool Edit_valuation() const { return editflags & Edit_flags::valuation; }
    bool Edit_completion() const { return editflags & Edit_flags::completion; }
    bool Edit_required() const { return editflags & Edit_flags::required; }
    bool Edit_text() const { return editflags & Edit_flags::text; }
    bool Edit_targetdate() const { return editflags & Edit_flags::targetdate; }
    bool Edit_tdproperty() const { return editflags & Edit_flags::tdproperty; }
    bool Edit_tdpropbinpat() const { return editflags & Edit_flags::tdpropbinpat; }
    bool Edit_repeats() const { return editflags & Edit_flags::repeats; }
    bool Edit_tdpattern() const { return editflags & Edit_flags::tdpattern; }
    bool Edit_tdevery() const { return editflags & Edit_flags::tdevery; }
    bool Edit_tdspan() const { return editflags & Edit_flags::tdspan; }
    bool Edit_tcreated() const { return editflags & Edit_flags::tcreated; }
    bool Edit_dependency() const { return editflags & Edit_flags::dependency; }
    bool Edit_significance() const { return editflags & Edit_flags::significance; }
    bool Edit_importance() const { return editflags & Edit_flags::importance; }
    bool Edit_urgency() const { return editflags & Edit_flags::urgency; }
    bool Edit_priority() const { return editflags & Edit_flags::priority; }
    bool Edit_supspecmatch() const { return editflags & Edit_flags::supspecmatch; }
    bool Edit_subtreematch() const { return editflags & Edit_flags::subtreematch; }
    bool Edit_nnltreematch() const { return editflags & Edit_flags::nnltreematch; }
    bool Edit_error() const { return editflags & Edit_flags::error; }
    bool None() const { return editflags == 0; }
};

class tdproperty_binary_pattern {
public:
    enum tdpropertymask : Edit_flags_type {
        unspecified  = 0b0000'0000'0000'0000'0000'0000'0000'0001,
        inherit      = 0b0000'0000'0000'0000'0000'0000'0000'0010,
        variable     = 0b0000'0000'0000'0000'0000'0000'0000'0100,
        fixed        = 0b0000'0000'0000'0000'0000'0000'0000'1000,
        exact        = 0b0000'0000'0000'0000'0000'0000'0001'0000
    };
protected:
    Edit_flags_type tdpropbinpattern;
public:
    tdproperty_binary_pattern() : tdpropbinpattern(0) {}
    Edit_flags_type get_tdpropbinpat_flags() const { return tdpropbinpattern; }
    void clear() { tdpropbinpattern = 0; }
    void set_tdpropbinpat_flags(Edit_flags_type _tdpropbinpattflags) { tdpropbinpattern = _tdpropbinpattflags; }
    bool set_tdpropbinpat_flag_by_label(const std::string flaglabel);
    void set_unspecified() { tdpropbinpattern |= tdproperty_binary_pattern::unspecified; }
    void set_inherit() { tdpropbinpattern |= tdproperty_binary_pattern::inherit; }
    void set_variable() { tdpropbinpattern |= tdproperty_binary_pattern::variable; }
    void set_fixed() { tdpropbinpattern |= tdproperty_binary_pattern::fixed; }
    void set_exact() { tdpropbinpattern |= tdproperty_binary_pattern::exact; }
    bool unspecified_is_set() const { return tdpropbinpattern & tdproperty_binary_pattern::unspecified; }
    bool inherit_is_set() const { return tdpropbinpattern & tdproperty_binary_pattern::inherit; }
    bool variable_is_set() const { return tdpropbinpattern & tdproperty_binary_pattern::variable; }
    bool fixed_is_set() const { return tdpropbinpattern & tdproperty_binary_pattern::fixed; }
    bool exact_is_set() const { return tdpropbinpattern & tdproperty_binary_pattern::exact; }
    bool None() const { return tdpropbinpattern == 0; }
    bool in_pattern(td_property _tdprop) const;
};

/**
 * On-heap data structure used to specify values for data components of a Node.
 * Note that this is not only used when building an Add/Edit-Node request, initialized
 * to compile-time default values (see Graphmodify).
 * It is also used when building a filter (see Graphinfo).
 */
struct Node_data {
    std::string utf8_text;
    Graphdecimal completion = 0.0;
    Graphdecimal hours = 0.0;
    Graphdecimal valuation = 0.0;
    std::vector<std::string> topics;
    time_t targetdate = RTt_unspecified;
    bool repeats = false; // currently only used when building a filter
    td_property tdproperty = unspecified; // was before 20241126: variable;
    td_pattern tdpattern = patt_nonperiodic;
    Graphsigned tdevery = 1;
    Graphsigned tdspan = 0;

    /** 
     * Copies a complete set of Node data from a buffer on heap to shared memory Node object.
     * 
     * @param graph Valid Graph object.
     * @param node Valid Node object (this may be in a different shared memory buffer, not part of graph).
     */
    void copy(Graph & graph, Node & node);

    /**
     * Set parameter value from string by Edit_flag.
     * 
     * @param param_id An enumerated Edit_flags::editmask parameter identifier.
     * @param valstr A string containing the parameter value.
     * @return True if successfully interpreted and set.
     */
    bool parse_value(Edit_flags_type param_id, const std::string valstr);
};

/**
 * On-heap data structure used to specify values for data components of an Edge.
 * Note that this is not only used when building an Add/Edit-Edge request, initialized
 * to compile-time default values (see Graphmodify).
 * It is also used when building a filter (see Graphinfo).
 */
struct Edge_data {
    Graphdecimal dependency = 0.0;
    Graphdecimal significance = 0.0;
    Graphdecimal importance = 0.0;
    Graphdecimal urgency = 0.0;
    Graphdecimal priority = 0.0;

    /** 
     * Copies a complete set of Edge data from a buffer on heap to shared memory Edge object.
     * 
     * @param edge Valid Edge object (in a shared memory buffer).
     */
    void copy(Edge & edge);
};

// +----- begin: standardization functions -----+

/**
 * Convert a target date sorted multimap of Node pointers into a string of
 * Node ID strings as comma separated values.
 * 
 * @param[in] sorted_nodes The multimap of Node pointers sorted by a target date key.
 * @param[out] csv_str The string object reference for the resulting comma separated values.
 */
void tdsorted_Nodes_to_csv(const targetdate_sorted_Nodes & sorted_nodes, std::string & csv_str);

bool valid_Node_ID(std::string id_str, std::string &formerror, ID_TimeStamp *id_timestamp = NULL);

bool valid_Node_ID(const ID_TimeStamp &idT, std::string &formerror);

std::string Node_ID_TimeStamp_to_string(const ID_TimeStamp idT);

/**
 * Create a usable Node ID TimeStamp from an epoch time and a
 * minor-ID.
 * 
 * This can also be used to produce valid Node ID TimeStamps without
 * the node-ID extension by specifying `minor_id = 0`. (See for
 * example how that is used in `fzaddnode` to generate a base for
 * a Node ID before choosing the minor-ID.)
 * 
 * Valid epoch times must be greater than 1999-01-01 00:00:00.
 * If `throw_if_invalid` is true then an invalid epoch time
 * causes ID_exception to be thrown.
 * 
 * @param t A valid epoch time for a Node ID.
 * @param minor_id A single-digit minor-ID (0 means time stamp only).
 * @param throw_if_invalid If true then throw ID_exception for invalid specifications.
 * @return A string with a valid Node ID TimeStamp, including minor-ID if given (or empty if invalid).
 */
std::string Node_ID_TimeStamp_from_epochtime(time_t t, uint8_t minor_id = 0, bool throw_if_invalid = false);

/// Add to a date-time in accordance with a repeating pattern.
time_t Add_to_Date(time_t t, td_pattern pattern, int every);

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

// +----- end  : inline member functions -----+

} // namespace fz

#endif // __GRAPHBASE_HPP
