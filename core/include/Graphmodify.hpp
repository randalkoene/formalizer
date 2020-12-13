// Copyright 2020 Randal A. Koene
// License TBD

/**
 * Classes and functions used to request and carry out modifications of Graph data structures.
 * 
 * Versioning is based on https://semver.org/ and the C++ headers define __GRAPHMODIFY_HPP.
 */

#ifndef __GRAPHMODIFY_HPP
#include "coreversion.hpp"
#define __GRAPHMODIFY_HPP (__COREVERSION_HPP)

//#define USE_COMPILEDPING

// std
//#include <>

// core
#include "error.hpp"
#include "Graphtypes.hpp"


namespace fz {

/// On-heap data structure used when building an Add/Edit-Node request, initialized to compile-time default values.
struct Node_data {
    std::string utf8_text;
    Graphdecimal completion = 0.0;
    Graphdecimal hours = 0.0;
    Graphdecimal valuation = 0.0;
    std::vector<std::string> topics;
    time_t targetdate = RTt_unspecified;
    td_property tdproperty = variable;
    td_pattern tdpattern = patt_nonperiodic;
    Graphsigned tdevery = 0;
    Graphsigned tdspan = 1;

    /** 
     * Copies a complete set of Node data from a buffer on heap to shared memory Node object.
     * 
     * @param graph Valid Graph object.
     * @param node Valid Node object (this may be in a different shared memory buffer, not part of graph).
     */
    void copy(Graph & graph, Node & node);
};

/// On-heap data structure used when building an Add/Edit-Edge request, initialized to compile-time default values.
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


enum Graph_modification_request {
    graphmod_add_node,
    graphmod_add_edge,
    namedlist_add,
    namedlist_remove,
    namedlist_delete,
    graphmod_edit_node,
    graphmod_edit_edge,
    NUM_graphmod_requests
};

/**
 * This is the data structure used by the server to return information about
 * the error that caused a Graph modification request stack to be rejected.
 * The requested modifications are all rejected together as soon as the first
 * potential error is detected during verification. This way, there are no
 * partial modifications to fix.
 * 
 * The error codes used here are defined within the same enumeration space
 * as exit_ error codes defined in `error.hpp` (see there for the list of
 * codes). This way, the code can be interpreted and can also be used
 * immediately to terminate the client program call that made the request.
 */
struct Graphmod_error {
    exit_status_code exit_code;
    char message[256];

    Graphmod_error(exit_status_code ecode, std::string msg);
};

/**
 * This is the data structure used by the server to return information about
 * the successful results of a Graph modification request. A request stack is
 * carried only if the entire stack can be processed successfully. The
 * information returned is a vector of these elements, each of which delivers
 * useful information about Graph modifications made, such as the new ID of
 * a Node or Edge that was created.
 */
struct Graphmod_result {
    Graph_modification_request request_handled;
    Node_ID_key node_key;   ///< Return ID if an Add-Node request was handled successfully.
    Edge_ID_key edge_key;   ///< Return ID if an Add-Edge request was handled successfully.
    Named_List_String resstr; ///< Return string information about the result (e.g. name of Named Node List).

    Graphmod_result(Graph_modification_request _request, const Node_ID_key & _nkey) : request_handled(_request), node_key(_nkey) {}
    Graphmod_result(Graph_modification_request _request, const Edge_ID_key & _ekey) : request_handled(_request), edge_key(_ekey) {}
    Graphmod_result(Graph_modification_request _request, const std::string _name, const Node_ID_key & _nkey) : request_handled(_request), node_key(_nkey), resstr(_name) {}
};

typedef bi::allocator<Graphmod_result, segment_manager_t> Graphmod_result_allocator;
typedef bi::vector<Graphmod_result, Graphmod_result_allocator> Graphmod_result_Vector;

/**
 * This is the results object that manages the collection of results for all
 * requests on the stack.
 * 
 * Construct this object within the shared segment that will be returned to the
 * calling client. See, for example, how this is done in `fzserverpq`.
 * 
 * @param segname The name of the shared segment that will deliver the results.
 */
struct Graphmod_results {
    Graphmod_result_Vector results;

    Graphmod_results(std::string segname) : results(graphmemman.get_allocator(segname)) {}
    std::string info_str();
};

//typedef std::uint32_t Edit_flags;
/**
 * This is the data structure used for elements of the request stack for
 * Graph modification. Each part of the modification requested is represented
 * by one of these elements, including the data that is pointed to by one or
 * more of the shared memory pointers.
 * 
 * For more information about Edit_flags and Edit protocols, see:
 * https://trello.com/c/ooCyccJ0/95-fzedit-node-and-edge-editor#comment-5fc059eee4536d8a147db56f
 */
struct Graphmod_data: public Edit_flags {
    Graph_modification_request request;
    Graph_Node_ptr node_ptr;
    Graph_Edge_ptr edge_ptr;
    Named_Node_List_Element_ptr nodelist_ptr;

    Graphmod_data(Graph_modification_request _request, Node * _node_ptr) : request(_request), node_ptr(_node_ptr), edge_ptr(nullptr), nodelist_ptr(nullptr) {}
    Graphmod_data(Graph_modification_request _request, Edge * _edge_ptr) : request(_request), node_ptr(nullptr), edge_ptr(_edge_ptr), nodelist_ptr(nullptr) {}
    Graphmod_data(Graph_modification_request _request, Named_Node_List_Element * _nodelist_ptr) : request(_request), node_ptr(nullptr), edge_ptr(nullptr), nodelist_ptr(_nodelist_ptr) {}

};

typedef bi::allocator<Graphmod_data, segment_manager_t> Graphmod_data_allocator;
typedef bi::vector<Graphmod_data, Graphmod_data_allocator> Graphmod_data_Vector;

/**
 * This data structure is the most efficient method to request a stack of
 * Graph modifications. It is constructed in shared memory using the
 * `allocate_Graph_modifications_in_shared_memory()` below and given to
 * `fzserverpq`. See `fzaddnote` for an example of its use.
 * 
 * All requested Graph modifications are in relation to an existing Graph,
 * which is referenced in shared memory at the address in the `graph_ptr`
 * cached pointer. Note that this variable is not a offset_ptr, because
 * it is not pointing to an object in the same segment. The `fzserverpq`
 * does not rely on this value to apply the modification requests.
 * 
 * Creating this object will throw a Shared_Memory_Exception if no
 * shared memory containing a Graph is active.
 */
class Graph_modifications {
protected:
    Graph_ptr graph_ptr; ///< See details about this variable in the description above.
public:

    Graphmod_data_Vector data;

    Graph_modifications();

    Graph_ptr get_reference_Graph() const { return graph_ptr; }

    /// Used in `request_add_Node()`. Returns empty string is no usable ID was available for current time.
    std::string generate_unique_Node_ID_str();
    /// Build an ADD_NODE request. Returns a pointer to the Node data being created (in shared memory).
    Node * request_add_Node();
    /// Build an EDIT NODE request. Returns a pointer to the Node data being created (in shared memory).
    Node * request_edit_Node(std::string nkeystr); // alt: (const Node_ID_key & nkey);
    /// Build an ADD EDGE request. Returns a pointer to the Edge data being created (in shared memory).
    Edge * request_add_Edge(const Node_ID_key & depkey, const Node_ID_key & supkey);
    /// Build an EDIT EDGE request. Returns a pointer to the Edge data being created (in shared memory).
    Edge * request_edit_Edge(std::string ekeystr); // alt: (const Edge_ID_key & ekey);
    /// Build a Named Node List request. Returns a pointer to Named_Node_List_Element data (in shared memory).
    Named_Node_List_Element * request_Named_Node_List_Element(Graph_modification_request request, const std::string _name, const Node_ID_key & nkey);

};

/**
 * A client program uses this function to allocate new shared memory and to
 * construct an empty `Graph_modifications` object there. That objet is used
 * to communicate Graph modification requests to `fzserverpq`.
 */
Graph_modifications * allocate_Graph_modifications_in_shared_memory(std::string segname, unsigned long segsize);

/**
 * A Graph server program uses this function to find shared memory that
 * contains a `Graph_modification` object. That object is parsed to respond
 * to modification requests.
 */
Graph_modifications * find_Graph_modifications_in_shared_memory(std::string segment_name);

/// Generate a unique name for the shared memory segment for Graph modification requests.
std::string unique_name_Graphmod();

/// See for example how this is used in fzserverpq.
Graphmod_error * prepare_error_response(std::string segname, exit_status_code ecode, std::string errmsg);

/// See for example how this is used in fzgraph.
Graphmod_error * find_error_response_in_shared_memory(std::string segment_name);

/// See for example how this is used in fzserverpq.
Graphmod_results * initialized_results_response(std::string segname);

/// See for example how this is used in fzgraph.
Graphmod_results * find_results_response_in_shared_memory(std::string segment_name);

/// Create a Node in the Graph's shared segment and add it to the Graph.
Node_ptr Graph_modify_add_node(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Create an Edge in the Graph's shared segment and add it to the Graph.
Edge_ptr Graph_modify_add_edge(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Edit a Node in the Graph.
Node_ptr Graph_modify_edit_node(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Edit and Edge in the Graph.
Edge_ptr Graph_modify_edit_edge(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/**
 * Update repeating Nodes past a specific timee.
 * 
 * @param sortednodes[in] A list of target date sorted incomplete Node pointers. These could be all or just repeated.
 * @param t_pass[in] The time past which to update repeating Nodes.
 * @param editflags[out] Reference to Edit_flags object that returns modifications that apply to one or more Nodes.
 * @return A target date sorted list of Node pointers that were updated. Use this to synchronize to the database.
 */
targetdate_sorted_Nodes Update_repeating_Nodes(const targetdate_sorted_Nodes & sortednodes, time_t t_pass, Edit_flags & editflags);

/// Add a Node to a Named Node List.
Named_Node_List_ptr Graph_modify_list_add(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Remove a Node from a Named Node List.
bool Graph_modify_list_remove(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Deleta a Named Node List.
bool Graph_modify_list_delete(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/**
 * Modify the targetdate of a repeating Node by carrying out one or more iterations
 * of advances in accordance with its `tdpattern` periodicity.
 * 
 * Notes:
 * 1. For repeating Nodes with limited `tdspan` at least one instance will remain.
 *    Refusing to complete and advance past the final instance of such a Node is
 *    a safety precaution, so that incomplete Nodes are not mysteriously eliminated from
 *    the schedule merely by updating the schedule.
 * 2. When a repeating Node with limited `tdspan` is modified to `tdspan < 2` then
 *    the `tdspan` is set to 0 and the Node no longer `repeats`, but the `tdpattern`
 *    is left unchanged. It can be used as a cached reminder of a previous repetition
 *    pattern setting (e.g. in case you with add more iterations).
 * 3. If a `tdspan==1` is found then it is set to 0 and `repeats` is turned off, just to
 *    to ensure a valid Node setting, in case the unexpected setting was a result of
 *    manual modification.
 * 
 * @param node Reference to a valid Node object.
 * @param N_advance Number of iterations to advance.
 * @param editflags Specifies the parameters that were modified. Use this to update the database.
 * @param t_ref Optional reference time (if negative then Actual time is used).
 * @return True when advanvement modifications were made, false if an invalid circumstance was encountered.
 */
bool Node_advance_repeating(Node & node, int N_advance, Edit_flags & editflags, time_t t_ref = RTt_unspecified);

/**
 * Updates a Node's completion ratio (and potentially updates required if
 * completion exceeds 1.0) in response to having logged a number of minutes
 * dedicated to the Node. For repeating Nodes, updates the targetdate if
 * specific conditions are met.
 * 
 * Notes:
 * 1. `add_minutes` is necessarily >= 0, which is different than directly
 * modifying parameters in ways that can increase or reduce. It only makes sense
 * to log positive time.
 * 2. Automatic correction of `required` when `completion > 1.0` is a point
 * where Formalizer 2.x behavior differs from that of Formalizer 1.x behavior.
 * 
 * *** Future improvement notes:
 * While automatically correcting `required` when `completion > 1.0` is
 * useful, it is not the full measure of improvements planned, which are:
 * - [Something better than completion ratio + time required](https://trello.com/c/Rnm84Hld).
 * - [Additional completion conditions](https://trello.com/c/oa3zFBdd).
 * - [Tags that teach time required](https://trello.com/c/JqxApvhO).
 * 
 * See, for example, how this is used in fzserverpq/tcp_server_handlers.cpp:node_add_logged_time().
 * 
 * @param node Reference to a valid Node object.
 * @param add_minutes The number of minutes that were logged.
 * @param T_ref An optional reference time (if negative then Actual time is used).
 * @return Edit_flags indicating the parameters of the Node that were modified. Use this to update the database.
 */
Edit_flags Node_apply_minutes(Node & node, unsigned int add_minutes, time_t T_ref = RTt_unspecified);

/**
 * Copy a number of Node IDs from a list of incomplete Nodes sorted by
 * effective target date to a Named Node List.
 * 
 * @param graph A valid Graph data structure.
 * @param to_name The name of the target Named Node List.
 * @param from_max Copy at most this many Node IDs (0 means no limit).
 * @param to_max Copy until the Named Node List contains this many Node IDs or more (0 means no limit).
 * @param _features Optional features to set if the target Named Node List is new.
 * @param _maxsize Optional maximum size to set if the target Named Node List is new.
 * @return The number of Node IDs copied.
 */
size_t copy_Incomplete_to_List(Graph & graph, const std::string to_name, size_t from_max = 0, size_t to_max = 0, int16_t _features = 0, int32_t _maxsize = 0);

/**
 * Updates the 'shortlist" Named Node List.
 * 
 * The 'shortlist' Named Node List is frequently used by Formalizer tools
 * that request a Node selection. To simplify that, this function exists
 * at the server level.
 * 
 * @param graph A valid Graph data structure.
 * @return The number of Nodes copied into the updated 'shortlist' Named Node List.
 */
size_t update_shortlist_List(Graph & graph);

} // namespace fz

#endif // __GRAPHMODIFY_HPP
