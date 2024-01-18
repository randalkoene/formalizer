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


enum Graph_modification_request {
    graphmod_add_node,
    graphmod_add_edge,
    namedlist_add,
    namedlist_remove,
    namedlist_delete,
    graphmod_edit_node,
    graphmod_edit_edge,
    batchmod_targetdates,
    batchmod_tpassrepeating,
    graphmod_remove_edge,
    NUM_graphmod_requests
};

const std::map<Graph_modification_request, std::string> Graph_modification_request_str = {
    {graphmod_add_node, "add_node"},
    {graphmod_add_edge, "add_edge"},
    {namedlist_add, "add_to_NNL"},
    {namedlist_remove, "remove_from_NNL"},
    {namedlist_delete, "delete_NNL"},
    {graphmod_edit_node, "edit_node"},
    {graphmod_edit_edge, "edit_edge"},
    {batchmod_targetdates, "batch_targetdates"},
    {batchmod_tpassrepeating, "batch_tpassrepeating"},
    {graphmod_remove_edge, "remove_edge"}
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
 * 
 * Note: Upon successful update, a `batchmod_targetdates` request also returns
 *       this structure, with a reference in `resstr` to a Named Node List
 *       containing all of the Nodes for which targetdates were updated.
 */
struct Graphmod_result {
    Graph_modification_request request_handled;
    Node_ID_key node_key;   ///< Return ID if an Add-Node request was handled successfully.
    Edge_ID_key edge_key;   ///< Return ID if an Add-Edge request was handled successfully.
    Named_List_String resstr; ///< Return string information about the result (e.g. name of Named Node List).

    Graphmod_result(Graph_modification_request _request, const Node_ID_key & _nkey) : request_handled(_request), node_key(_nkey) {}
    Graphmod_result(Graph_modification_request _request, const Edge_ID_key & _ekey) : request_handled(_request), edge_key(_ekey) {}
    Graphmod_result(Graph_modification_request _request, const std::string _name, const Node_ID_key & _nkey) : request_handled(_request), node_key(_nkey), resstr(_name) {}
    Graphmod_result(Graph_modification_request _request, const std::string _name) : request_handled(_request), resstr(_name) {}
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
protected:
    std::string segment_name; ///< This is only used while creating the results structure.
public:
    Graphmod_result_Vector results;

    Graphmod_results(std::string segname) : segment_name(segname), results(graphmemman.get_allocator(segname)) {}

    bool add(Graph_modification_request _request, const Node_ID_key & _nkey);
    bool add(Graph_modification_request _request, const Edge_ID_key & _ekey);
    bool add(Graph_modification_request _request, const std::string _name, const Node_ID_key & _nkey);
    bool add(Graph_modification_request _request, const std::string _name);

    std::string info_str();
};

typedef std::vector<Graphmod_result> Graphmod_unshared_result_Vector;

struct Graphmod_unshared_results {
public:
    Graphmod_unshared_result_Vector results;

    Graphmod_unshared_results() {}

    bool add(Graph_modification_request _request, const Node_ID_key & _nkey);
    bool add(Graph_modification_request _request, const Edge_ID_key & _ekey);
    bool add(Graph_modification_request _request, const std::string _name, const Node_ID_key & _nkey);
    bool add(Graph_modification_request _request, const std::string _name);

    std::string info_str();
};

struct TD_Node_shm {
    time_t td;
    Node_ID_key nkey;
    void set(time_t t, const Node_ID_key & k) {
        td = t;
        nkey = k;
    }
};
typedef bi::offset_ptr<TD_Node_shm> TD_Node_shm_offsetptr;


// *** Trying this, even though I haven't actually concluded that the array was the cause of the segfault!
//typedef bi::allocator<TD_Node_shm, segment_manager_t> TD_Node_shm_allocator;
//typedef bi::vector<TD_Node_shm, TD_Node_shm_allocator> TD_Node_shm_Vector;

// Use this to build a constant size array of TD_Node_shm elements in shared memory.
struct Batchmod_targetdates {
    TD_Node_shm_offsetptr tdnkeys;
    //TD_Node_shm_Vector tdnkeys;
    size_t tdnkeys_num = 0;
    /// Used by fzupdate.cpp:update_variable().
    Batchmod_targetdates(const targetdate_sorted_Nodes & nodelist, segment_memory_t & graphmod_shm) {
    //Batchmod_targetdates(const targetdate_sorted_Nodes & nodelist) : tdnkeys(graphmemman.get_allocator()) {
        tdnkeys = graphmod_shm.construct<TD_Node_shm>(bi::anonymous_instance)[nodelist.size()](); // *** Watch out! Not testing for failure here.
        tdnkeys_num = nodelist.size();
        VERYVERBOSEOUT("Created shared memory array for "+std::to_string(tdnkeys_num)+" elements.\n");
        if (tdnkeys.get()) {
            size_t i = 0;
            for (const auto & [t, node_ptr] : nodelist) {
                if (!node_ptr) {
                    standard_exit_error(exit_bad_request_data, "Received null-node", __func__);
                }
                tdnkeys[i].set(t, node_ptr->get_id().key());
                ++i;
            }
        } else {
            VERYVERBOSEOUT("No usable shared memory array constructed.\n");
            ADDERROR(__func__, "No usable shared memory array constructed.\n");
        }
    }
};
typedef bi::offset_ptr<Batchmod_targetdates> Batchmod_targetdates_offsetptr;
typedef Batchmod_targetdates * Batchmod_targetdates_ptr;

struct Batchmod_tpass {
    time_t t_pass;
    Batchmod_tpass(time_t _tpass) : t_pass(_tpass) {}
};
typedef bi::offset_ptr<Batchmod_tpass> Batchmod_tpass_offsetptr;
typedef Batchmod_tpass * Batchmod_tpass_ptr;

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
    Graph_Node_ptr node_ptr = nullptr;
    Graph_Edge_ptr edge_ptr = nullptr;
    Named_Node_List_Element_ptr nodelist_ptr = nullptr;
    Batchmod_targetdates_offsetptr batchmodtd_ptr = nullptr;
    Batchmod_tpass_offsetptr batchmodtpass_ptr = nullptr;
    time_t t_pass = RTt_unspecified;

    Graphmod_data(Graph_modification_request _request, Node * _node_ptr) : request(_request), node_ptr(_node_ptr) {}
    Graphmod_data(Graph_modification_request _request, Edge * _edge_ptr) : request(_request), edge_ptr(_edge_ptr) {}
    Graphmod_data(Graph_modification_request _request, Named_Node_List_Element * _nodelist_ptr) : request(_request), nodelist_ptr(_nodelist_ptr) {}
    Graphmod_data(Batchmod_targetdates * _batchmodtd_ptr) : request(batchmod_targetdates), batchmodtd_ptr(_batchmodtd_ptr) {}
    Graphmod_data(Batchmod_tpass * _batchmodtpass_ptr) : request(batchmod_tpassrepeating), batchmodtpass_ptr(_batchmodtpass_ptr) {}

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
    std::string segment_name; ///< This is also used only by the requesting program.
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
    /// Build an REMOVE EDGE request. Returns a pointer to the Edge data carrier being created (in shared memory).
    Edge * request_remove_Edge(std::string ekeystr);
    /// Build an EDIT EDGE request. Returns a pointer to the Edge data being created (in shared memory).
    Edge * request_edit_Edge(std::string ekeystr); // alt: (const Edge_ID_key & ekey);
    /// Build a Named Node List request. Returns a pointer to Named_Node_List_Element data (in shared memory).
    Named_Node_List_Element * request_Named_Node_List_Element(Graph_modification_request request, const std::string _name, const Node_ID_key & nkey);
    /// Build a BATCH modification request for a list of Nodes and targetdates. Retruns a pointer to Batchmod_targetdates created (in shared memory).
    Batchmod_targetdates * request_Batch_Node_Targetdates(const targetdate_sorted_Nodes & nodelist);
    /// Build a BATCH modification request for a list of Nodes and t_pass. Retruns a pointer to Batchmod_targetdates created (in shared memory).
    Batchmod_tpass * request_Batch_Node_Tpass(time_t t_pass); //, const targetdate_sorted_Nodes & nodelist);

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
 * Update repeating Nodes past a specific time.
 * 
 * The `Edit_flags` of each updated Node are set in accordance with the modifications that take place on that
 * Node. This is used by the `Graphpostgres` process. Remember to clear the `Edit_flags` after sychronizing to
 * the database.
 * 
 * @param sortednodes[in] A list of target date sorted incomplete Node pointers. These could be all or just repeated.
 * @param t_pass[in] The time past which to update repeating Nodes.
 * @return A target date sorted list of Node pointers that were updated. Use this to synchronize to the database.
 */
targetdate_sorted_Nodes Update_repeating_Nodes(const targetdate_sorted_Nodes & sortednodes, time_t t_pass);

/// Add a Node to a Named Node List.
Named_Node_List_ptr Graph_modify_list_add(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Remove a Node from a Named Node List.
bool Graph_modify_list_remove(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Deleta a Named Node List.
bool Graph_modify_list_delete(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/// Modify the targetdates of a batch of Nodes.
bool Graph_modify_batch_node_targetdates(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

/**
 * Modify the targetdates of a batch of repeating Nodes past t_pass time.
 * Updated notes are put into an 'repeating_updated' Named Node List.
 * 
 * Note that the 'repeating_updated' NNL is modified even if no Nodes were
 * updated. If the number of updated Nodes is zero then the NNL is simply
 * deleted.
 * 
 * @param graph A memory-resident Graph.
 * @param graph_segname The shared memory segment name of the memory-resident Graph.
 * @param gmoddata A Graph modifications data structure.
 * @return The number of Nodes modified (and placed in 'repeating_updated'), or -1 for error.
 */
ssize_t Graph_modify_batch_node_tpassrepeating(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata);

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
 * 4. At present, `t_ref` IS NOT USED FOR ANYTHING (!!!). Consequently, advancing a
 *    repeating Node with this function does not pay attention to passing any particular
 *    time. This is probably by `Update_repeating_Nodes()` uses a while-loop to make
 *    single-step advances for each repeating Node. Perhaps this is the desired behavior,
 *    as it allows a completed repeating Node to move on to its next instance, even if
 *    that is beyond current (emulated) time. Otherwise, repeating Nodes completed
 *    before the target date of their current instance might put themselves back on the
 *    Schedule for the same day. Then again - does it need to be done this way? Or can
 *    using `t_ref` in this function apply a test such that a repeating Node can move
 *    on to an instance beyond t_ref, but will then stop advancing. If this were the
 *    protocol, then setting `t_ref=RTt_maxtime` would be the default to deactive such
 *    a constraint.
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
 * Skip N instances of a repeating Node.
 * 
 * Note A: This is using a while-loop, because of the issue in Note 4 of
 *         Node_advance_repeating().
 * Note B: This function does not take a `T_ref` parameter. It is specifically intended
 *         to enable skipping an arbitrary number of instances without regard to a
 *         time threshold.
 * 
 * @param node The repeating Node that should skip instances.
 * @param num_skip The number of instances to skip.
 * @param editflags Reference to Edit_flags object that returns modifications carried out.
 */
void Node_skip_num(Node & node, unsigned int num_skip, Edit_flags & editflags);

/**
 * Skip instances of a repeating Node past a specified time.
 * 
 * Note A: This is using a while-loop, because of the issue in Note 4 of
 *         Node_advance_repeating().
 * Note B: Unlike the Schedule updating `Update_repeating_Nodes()` function, this
 *         single-Node instance skipping function purposely does not make an
 *         exception for Nodes with an annual repeat pattern.
 * 
 * @param node The repeating Node that should skip instances.
 * @param t_pass The threshold time past which to skip instances.
 * @param editflags Reference to Edit_flags object that returns modifications carried out.
 */
void Node_skip_tpass(Node & node, time_t t_pass, Edit_flags & editflags);

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

/**
 * Copy Nodes from a batch to a Named Node List. This does not include synchronization to database.
 * See, for example, how this is used when processing a `batchmod_targetdates` request in `fzserverpq`.
 */
bool batch_to_NNL(Graph & graph, const Batchmod_targetdates & batchnodes, std::string list_name);

/**
 * Copy Nodes from a sorted list to a Named Node List. This does not include synchronization to database.
 * See, for example, how this is used when processing a `batchmod_tpassrepeating` request in `fzserverpq`.
 * 
 * @param graph A memory-resident Graph.
 * @param sortednodes The target date sorted list of Nodes.
 * @param list_name A Named Node List. If it already exists then it is deleted (even if no new one is created).
 * @return The number of Nodes copied from the sorted list to the NNL, or the error code -1.
 */
ssize_t sorted_to_NNL(Graph & graph, const targetdate_sorted_Nodes & sortednodes, std::string list_name);

} // namespace fz

#endif // __GRAPHMODIFY_HPP
