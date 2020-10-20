// Copyright 2020 Randal A. Koene
// License TBD

// std
//#include <>

// core
#include "error.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"


namespace fz {

std::string unique_name_Graphmod() {
    return TimeStamp("%Y%m%d%H%M%S",ActualTime());
}

/**
 * A client program uses this function to allocate new shared memory and to
 * construct an empty `Graph_modifications` object there. That objet is used
 * to communicate Graph modification requests to `fzserverpq`.
 */
Graph_modifications * allocate_Graph_modifications_in_shared_memory(std::string segname, unsigned long segsize) {
    segment_memory_t * segment = graphmemman.allocate_and_activate_shared_memory(segname, segsize);
    if (!segment)
        return nullptr;

    graphmemman.set_remove_on_exit(true); // the client cleans up the request it "served" up to the fzserverpq
    return segment->construct<Graph_modifications>("graphmod")();
}

Graph_modifications::Graph_modifications() : data(graphmemman.get_allocator()) {
    graph_ptr = nullptr;
    if (!graphmemman.get_Graph(graph_ptr)) {
        throw(Shared_Memory_exception("none containing a Graph are active"));
    }
}

std::string Graph_modifications::generate_unique_Node_ID_str() {
    time_t t_now = ActualTime();
    // Check for other Nodes with that time stamp.
    key_sorted_Nodes nodes = Nodes_created_in_time_interval(*graph_ptr, t_now, t_now+1);

    // Is there still a minor_id available (single digit)?
    uint8_t minor_id = 1;
    if (!nodes.empty()) {
        minor_id = std::prev(nodes.end())->second->get_id().key().idT.minor_id + 1;
        if (minor_id > 9) {
            // *** Note that instead of returning empty, we could wait a second and try again.
            ADDERROR(__func__, "No minor_id digits remaining for Node ID with time stamp "+Node_ID_TimeStamp_from_epochtime(t_now));
            return "";
        }
    }

    // Generate Node ID time stamp with minor-ID.
    std::string nodeid_str = Node_ID_TimeStamp_from_epochtime(t_now, minor_id);

    return nodeid_str;
}

Node * Graph_modifications::request_add_Node() {
    // Create new Node object in the shared memory segment being used to share a modifications request stack.
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem)
        return nullptr;

    std::string nodeid_str = generate_unique_Node_ID_str();
    if (nodeid_str.empty())
        return nullptr;

    Node * node_ptr = smem->construct<Node>(bi::anonymous_instance)(nodeid_str); // this normal pointer is emplaced into an offset_ptr
    if (!node_ptr)
        return nullptr;

    data.emplace_back(graphmod_add_node, node_ptr);
    return node_ptr;
}

} // namespace fz
