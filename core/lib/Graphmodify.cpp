// Copyright 2020 Randal A. Koene
// License TBD

// std
//#include <>

// core
#include "error.hpp"
#include "general.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"


namespace fz {

Graphmod_error::Graphmod_error(exit_status_code ecode, std::string msg) : exit_code(ecode) {
    safecpy(msg, message, 256);
}

std::string Graphmod_results::info_str() {
    std::string infostr("Graph modifications:");
    if (results.empty()) {
        infostr += "\n\tnone\n";
        return infostr;
    }

    for (const auto & modres : results) {
        switch (modres.request_handled) {
            case graphmod_add_node: {
                infostr += "\n\tadded Node with ID "+modres.node_key.str();
                break;
            }
            case graphmod_add_edge: {
                infostr += "\n\tadded Edge with ID "+modres.edge_key.str();
                break;
            }
            default: {
                // this should never happen
                infostr += "\n\tunrecognized modification request!";
            }
        }
    }
    infostr += '\n';
    return infostr;
}

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

Graph_modifications * find_Graph_modifications_in_shared_memory(std::string segment_name) {
    try {
        segment_memory_t * segment = new segment_memory_t(bi::open_only, segment_name.c_str()); // was bi::open_read_only

        void_allocator * alloc_inst = new void_allocator(segment->get_segment_manager());

        graphmemman.add_manager(segment_name, *segment, *alloc_inst);
        graphmemman.set_active(segment_name);
        graphmemman.set_remove_on_exit(false); // looks like you're a client and not a server here

        VERYVERBOSEOUT(graphmemman.info_str());

        return segment->find<Graph_modifications>("graphmod").first;

    } catch (const bi::interprocess_exception & ipexception) {
        std::string errstr("Unable to access shared memory '"+segment_name+"', "+std::string(ipexception.what()));
        VERBOSEERR(errstr+'\n');
        ERRRETURNNULL(__func__,errstr);
    }
    return nullptr;
}

/// See for example how this is used in fzserverpq.
Graphmod_error * prepare_error_response(std::string segname, exit_status_code ecode, std::string errmsg) {
    if (!graphmemman.set_active(segname)) {
        ADDERROR(__func__, "Unable to activate segment "+segname+" for error message "+errmsg);
        return nullptr;
    }

    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Graphmod_error * graphmoderror_ptr = smem->construct<Graphmod_error>("error")(ecode, errmsg);
    if (!graphmoderror_ptr) {
        ADDERROR(__func__, "Unable to construct Graphmod_error object for error message "+errmsg);
        return nullptr;
    }
    
    return graphmoderror_ptr;
}

/// See for example how this is used in fzgraph.
Graphmod_error * find_error_response_in_shared_memory(std::string segment_name) {
    if (!graphmemman.set_active(segment_name)) {
        ADDERROR(__func__, "Shared segment "+segment_name+" not found");
        return nullptr;
    }

    return graphmemman.get_segmem()->find<Graphmod_error>("error").first;
}

/// See for example how this is used in fzserverpq.
Graphmod_results * initialized_results_response(std::string segname) {
    if (!graphmemman.set_active(segname)) {
        ADDERROR(__func__, "Unable to activate segment "+segname+" for results data");
        return nullptr;
    }

    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Graphmod_results * graphmodresults_ptr = smem->construct<Graphmod_results>("results")(segname);
    if (!graphmodresults_ptr) {
        ADDERROR(__func__, "Unable to construct Graphmod_results object for results data");
        return nullptr;
    }
    
    return graphmodresults_ptr;
}

/// See for example how this is used in fzgraph.
Graphmod_results * find_results_response_in_shared_memory(std::string segment_name) {
    if (!graphmemman.set_active(segment_name)) {
        ADDERROR(__func__, "Unable to activate segment "+segment_name+" for results data");
        return nullptr;
    }

    return graphmemman.get_segmem()->find<Graphmod_results>("results").first;
}

/// Create a Node in the Graph's shared segment and add it to the Graph.
Node_ptr Graph_modify_add_node(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.node_ptr)
        return nullptr;

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Node construction");
        return nullptr;
    }

    Node & requested_node = *gmoddata.node_ptr;
    Node_ptr node_ptr = graph.create_and_add_Node(requested_node.get_id_str());
    if (!node_ptr)
        return nullptr;
    
    node_ptr->copy_content(requested_node);
    return node_ptr;
}

/// Create an Edge in the Graph's shared segment and add it to the Graph.
Edge_ptr Graph_modify_add_edge(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.edge_ptr)    void copy_content(Node & from_node);
        return nullptr;

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Edge construction");
        return nullptr;
    }

    Edge & requested_edge = *gmoddata.edge_ptr;
    Edge_ptr edge_ptr = graph.create_and_add_Edge(requested_edge.get_id_str());
    if (!edge_ptr)
        return nullptr;
    
    edge_ptr->copy_content(requested_edge);
    return edge_ptr;
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
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    std::string nodeid_str = generate_unique_Node_ID_str();
    if (nodeid_str.empty())
        return nullptr;

    Node * node_ptr = smem->construct<Node>(bi::anonymous_instance)(nodeid_str); // this normal pointer is emplaced into an offset_ptr
    if (!node_ptr) {
        ADDERROR(__func__, "Unable to construct Node in shared memory");
        return nullptr;
    }

    data.emplace_back(graphmod_add_node, node_ptr);
    return node_ptr;
}

/// Note that testing if depkey and supkey exists happens in the server (see https://trello.com/c/FQximby2/174-fzgraphedit-adding-new-nodes-to-the-graph-with-initial-edges#comment-5f8faf243d74b8364fac7739).
Edge * Graph_modifications::request_add_Edge(const Node_ID_key & depkey, const Node_ID_key & supkey) {
    // Create new Edge object in the shared memory segment being used to share a modifications request stack.
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Edge * edge_ptr = smem->construct<Edge>(bi::anonymous_instance)(depkey, supkey); // this normal pointer is emplaced into an offset_ptr
    if (!edge_ptr) {
        ADDERROR(__func__, "Unable to construct Edge in shared memory");
        return nullptr;
    }
    
    data.emplace_back(graphmod_add_edge, edge_ptr);
    return edge_ptr;
}

} // namespace fz
