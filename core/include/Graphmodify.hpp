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
#include "Graphtypes.hpp"


namespace fz {

enum Graph_modification_request {
    graphmod_add_node,
    NUM_graphmod_requests
};

struct Graphmod_data {
    Graph_modification_request request;
    Graph_Node_ptr node_ptr;

    Graphmod_data(Graph_modification_request _request, Node * _node_ptr) : request(_request), node_ptr(_node_ptr) {}
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

};

/**
 * A client program uses this function to allocate new shared memory and to
 * construct an empty `Graph_modifications` object there. That objet is used
 * to communicate Graph modification requests to `fzserverpq`.
 */
Graph_modifications * allocate_Graph_modifications_in_shared_memory(std::string segname, unsigned long segsize);

/// Generate a unique name for the shared memory segment for Graph modification requests.
std::string unique_name_Graphmod();

} // namespace fz

#endif // __GRAPHMODIFY_HPP
