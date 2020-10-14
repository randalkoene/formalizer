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
//#include <set>
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

typedef std::vector<Node*> Node_Index;

typedef std::map<std::string, std::string> Graph_info_label_value_pairs;

#define NODE_NULLKEY_STR "{null-key}"
#define HIGH_TOPIC_INDEX_WARNING 1000 /// at this index number report a warning just in case it is in error

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

// +----- end  : inline member functions -----+

} // namespace fz

#endif // __GRAPHBASE_HPP
