// Copyright 2020 Randal A. Koene
// License TBD

// std
#include <cmath>

// core
#include "error.hpp"
#include "general.hpp"
#include "Graphinfo.hpp"
#include "Graphmodify.hpp"


namespace fz {

/** 
 * Copies a complete set of Node data from a buffer on heap to shared memory Node object.
 * 
 * @param graph Valid Graph object.
 * @param node Valid Node object (this may be in a different shared memory buffer, not part of graph).
 */
void Node_data::copy(Graph & graph, Node & node) {
    node.set_text(utf8_text);
    node.set_completion(completion);
    node.set_required((unsigned int) round(hours*3600.0));
    node.set_valuation(valuation);
    node.set_targetdate(targetdate);
    node.set_tdproperty(tdproperty);
    node.set_tdpattern(tdpattern);
    node.set_tdevery(tdevery);
    node.set_tdspan(tdspan);
    node.set_repeats((tdpattern != patt_nonperiodic) && (tdproperty != variable) && (tdproperty != unspecified));

    for (const auto & tag_str : topics) {
        VERYVERBOSEOUT("\n  adding Topic tag: "+tag_str+'\n');
        Topic * topic_ptr = graph.find_Topic_by_tag(tag_str);
        if (!topic_ptr) {
            standard_exit_error(exit_general_error, "Unknown Topic: "+tag_str, __func__);
        }
        Topic_Tags & topictags = *(const_cast<Topic_Tags *>(&graph.get_topics())); // We need the list of Topics from the memory-resident Graph.
        node.add_topic(topictags, topic_ptr->get_id(), 1.0);
    }
}

/** 
 * Copies a complete set of Edge data from a buffer on heap to shared memory Edge object.
 * 
 * @param edge Valid Edge object (in a shared memory buffer).
 */
void Edge_data::copy(Edge & edge) {
    edge.set_dependency(dependency);
    edge.set_significance(significance);
    edge.set_importance(importance);
    edge.set_urgency(urgency);
    edge.set_priority(priority);
}

Graphmod_error::Graphmod_error(exit_status_code ecode, std::string msg) : exit_code(ecode) {
    safecpy(msg, message, 256);
}

bool Graphmod_results::add(Graph_modification_request _request, const Node_ID_key & _nkey) {
    graphmemman.cache();
    if (!graphmemman.set_active(segment_name)) {
        ADDERROR(__func__, "Unable to activate segment "+segment_name+" for results data");
        graphmemman.uncache();
        return false;
    }
    results.emplace_back(_request, _nkey);
    graphmemman.uncache();
    return true;
}

bool Graphmod_results::add(Graph_modification_request _request, const Edge_ID_key & _ekey) {
    graphmemman.cache();
    if (!graphmemman.set_active(segment_name)) {
        ADDERROR(__func__, "Unable to activate segment "+segment_name+" for results data");
        graphmemman.uncache();
        return false;
    }
    results.emplace_back(_request, _ekey);
    graphmemman.uncache();
    return true;
}

bool Graphmod_results::add(Graph_modification_request _request, const std::string _name, const Node_ID_key & _nkey) {
    graphmemman.cache();
    if (!graphmemman.set_active(segment_name)) {
        ADDERROR(__func__, "Unable to activate segment "+segment_name+" for results data");
        graphmemman.uncache();
        return false;
    }
    results.emplace_back(_request, _name, _nkey);
    graphmemman.uncache();
    return true;
}

bool Graphmod_results::add(Graph_modification_request _request, const std::string _name) {
    graphmemman.cache();
    if (!graphmemman.set_active(segment_name)) {
        ADDERROR(__func__, "Unable to activate segment "+segment_name+" for results data");
        graphmemman.uncache();
        return false;
    }
    results.emplace_back(_request, _name);
    graphmemman.uncache();
    return true;
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
            case namedlist_add: {
                infostr += "\n\tadded Node with ID "+(modres.node_key.str()+" to NNL ")+modres.resstr.c_str();
                break;
            }
            case namedlist_remove: {
                infostr += "\n\tremoved Node with ID "+(modres.node_key.str()+" from NNL ")+modres.resstr.c_str();
                break;
            }
            case namedlist_delete: {
                infostr += "\n\tdelete Named Node List ";
                infostr += modres.resstr.c_str();
            }
            case graphmod_edit_node: {
                infostr += "\n\tedited Node with ID "+modres.node_key.str();
                break;
            }
            case graphmod_edit_edge: {
                infostr += "\n\tedited Edge with ID "+modres.edge_key.str();
                break;
            }
            case batchmod_targetdates: {
                infostr += "\n\tupdated target dates of movable Nodes in Named Node List "+std::string(modres.resstr.c_str());
                break;
            }
            case batchmod_tpassrepeating: {
                infostr += "\n\tupdated target dates of repeating Nodes in Named Node List "+std::string(modres.resstr.c_str());
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
    if (!gmoddata.node_ptr) {
        return nullptr;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Node construction");
        return nullptr;
    }

    Node & requested_node = *gmoddata.node_ptr;
    Node_ptr node_ptr = graph.create_and_add_Node(requested_node.get_id_str());
    if (!node_ptr) {
        return nullptr;
    }
   
    node_ptr->copy_content(requested_node);
    return node_ptr;
}

/// Create an Edge in the Graph's shared segment and add it to the Graph.
Edge_ptr Graph_modify_add_edge(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.edge_ptr) {
        return nullptr;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Edge construction");
        return nullptr;
    }

    Edge & requested_edge = *gmoddata.edge_ptr;
    Edge_ptr edge_ptr = graph.create_and_add_Edge(requested_edge.get_id_str());
    if (!edge_ptr) {
        return nullptr;
    }
    
    edge_ptr->copy_content(requested_edge);
    return edge_ptr;
}

/// Edit a Node in the Graph.
Node_ptr Graph_modify_edit_node(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.node_ptr) {
        return nullptr;
    }

    if (!graphmemman.set_active(graph_segname)) { // activate in case topics or other container elements are added
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Node edits with possible allocations");
        return nullptr;
    }

    Node & modifications_node = *gmoddata.node_ptr;
    Node_ptr node_ptr = graph.Node_by_id(modifications_node.get_id().key());
    if (!node_ptr) {
        return nullptr;
    }
   
    // *** In the following, Edit_flags could instead be in Node and therefore also in modifications_node.
    node_ptr->edit_content(modifications_node,gmoddata);
    return node_ptr;
}

/// Edit and Edge in the Graph.
Edge_ptr Graph_modify_edit_edge(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    // *** Not yet implemented.
    return nullptr;
}

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
 * @return True when advancement modifications were made, false if an invalid circumstance was encountered.
 */
bool Node_advance_repeating(Node & node, int N_advance, Edit_flags & editflags, time_t t_ref) {
    ERRTRACE;

    if (N_advance==0)
        return true;

    if ((node.get_tdspan()==1) || (node.get_tdspan()<0)) { // correct invalid spans
        node.set_repeats(false);
        node.set_tdspan(0);
        editflags.set_Edit_repeats();
        editflags.set_Edit_tdspan();
    }

    if (!node.get_repeats())
        return false;

    time_t t_TD = node.effective_targetdate(); // *** is this necessary? can repeating Nodes inherit their targetdate? if so, should the t_pass tests in functions below check effective instead of get_targetdate()?
    if (t_TD <= RTt_unspecified) {
        editflags.set_Edit_error();
        ADDERROR(__func__, "Unspecified or invalid target date of repeating Node "+node.get_id_str()+" does not permit advancement.");
        return false;
    
    }

    if (t_ref < 0) { // *** oh-oh... this doesn't seem to be used here at all!
        t_ref = ActualTime();
    }

    auto span = node.get_tdspan();
    bool unlimited = span == 0;
    while ((N_advance>0) && ((span>1) || unlimited)) {
        t_TD = Add_to_Date(t_TD, node.get_tdpattern(), node.get_tdevery());
        if (!unlimited)
            --span;
        --N_advance;
    }
    if (!unlimited) {
        if (span<2) {
            span = 0;
            node.set_repeats(false);
            editflags.set_Edit_repeats();
        }
        node.set_tdspan(span);
        editflags.set_Edit_tdspan();

    }
    node.set_targetdate(t_TD);
    editflags.set_Edit_targetdate();

    return true;
}

/**
 * Depending on the tdpattern type of a Node, potentially modify its targetdate
 * in accordance with that repetition pattern and reset its completion ratio.
 * 
 * This does essentially the same as `Node_advance_repeating()` one iteration.
 * 
 * This is a server function, e.g. called within `fzserverpq`.
 * 
 * @param node Reference to a valid Node object.
 * @param editflags Specifies the parameters that were modified. Use this to update the database.
 * @param T_ref Optional reference time (if negative then Actual time is used).
 * @return True if the repeating Node was modified for the completed iteration, false if an invalid circumstance was encountered.
 */
bool Node_completed_repeating(Node & node, Edit_flags & editflags, time_t t_ref = RTt_unspecified) {
    ERRTRACE;

    node.set_completion(0.0);
    editflags.set_Edit_completion();

    if (!Node_advance_repeating(node, 1, editflags, t_ref)) {
        return false;
    }

    return true;
}

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
 * 3. If an invalid circumstance is encountered then the special flag
 * Edit_flags::error is set. The calling function should still check for other
 * flags and synchronize modifications to the database, as some modifications can
 * take place before an error is encountered.
 * 
 * *** Future improvement notes:
 * A. While automatically correcting `required` when `completion > 1.0` is
 * useful, it is not the full measure of improvements planned, which are:
 * - [Something better than completion ratio + time required](https://trello.com/c/Rnm84Hld).
 * - [Additional completion conditions](https://trello.com/c/oa3zFBdd).
 * - [Tags that teach time required](https://trello.com/c/JqxApvhO).
 * B. Consider moving the `editflags` into the Node objects as a cache variable. Then,
 * have the `set_` functions automatically modify the `editflags`. And add function to
 * `clear_all_editflags()` that only certain programs can call (e.g. when loading
 * the Graph into shared memory). This way, all normal paths that lead to Node changes
 * also cause flag setting. Optionally, you could then move some of these more
 * sophisticated modification functions into the Node objects as well, as long as they
 * still set flags.
 * 
 * See, for example, how this is used in fzserverpq/tcp_server_handlers.cpp:node_add_logged_time().
 * 
 * @param node Reference to a valid Node object.
 * @param add_minutes The number of minutes that were logged.
 * @param T_ref An optional reference time (if negative then Actual time is used).
 * @return Edit_flags indicating the parameters of the Node that were modified. Use this to update the database.
 */
Edit_flags Node_apply_minutes(Node & node, unsigned int add_minutes, time_t T_ref) {
    Edit_flags editflags;
    auto seconds_applied = node.seconds_applied();
    seconds_applied += 60*add_minutes;
    auto required = node.get_required();
    if (seconds_applied > required) {
        if (node.get_repeats()) {
            Node_completed_repeating(node, editflags, T_ref);
        } else {
            node.set_required(seconds_applied);
            editflags.set_Edit_required();
            node.set_completion(1.0);
        }
    } else {
        node.set_completion(((float)seconds_applied)/((float)required));
    }
    editflags.set_Edit_completion();
    return editflags;
}

/**
 * Skip N instances of a repeating Node.
 * 
 * Note: This function does not take a `T_ref` parameter. It is specifically intended
 *       to enable skipping an arbitrary number of instances without regard to a
 *       time threshold.
 * 
 * @param node The repeating Node that should skip instances.
 * @param num_skip The number of instances to skip.
 * @param editflags Reference to Edit_flags object that returns modifications carried out.
 */
void Node_skip(Node & node, unsigned int num_skip, Edit_flags & editflags) {
    if (!node.get_repeats()) {
        return;
    }

    if (Node_advance_repeating(node, num_skip, editflags, RTt_maxtime)) { // *** using RTt_maxtime here in case T_ref is put to use
        // if an advance happened, then make sure completion is set to 0
        node.set_completion(0.0);
        editflags.set_Edit_completion();
    }
}

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
void Node_skip(Node & node, time_t t_pass, Edit_flags & editflags) {
    bool updated = false;
    while (node.get_repeats() && (node.get_targetdate()<=t_pass)) {
        if (!Node_completed_repeating(node, editflags)) {
            break;
        }
        updated = true;
    }
    if (updated) { // *** notice the inconsistency that this is used here but not everywhere
        const_cast<Edit_flags *>(&(node.get_editflags()))->set_Edit_flags(editflags.get_Edit_flags());
    }
}

/**
 * Update repeating Nodes past a specific time.
 * 
 * This is a server function, e.g. called within `fzserverpq`.
 * For the rationale, see the explanation at https://trello.com/c/eUjjF1yZ/222-how-graph-components-are-edited#comment-5fd8fed424188014cb31a937.
 * 
 * @param sortednodes[in] A list of target date sorted incomplete Node pointers. These could be all or just repeated.
 * @param t_pass[in] The time past which to update repeating Nodes.
 * @param editflags[out] Reference to Edit_flags object that returns modifications that apply to one or more Nodes.
 * @return A target date sorted list of Node pointers that were updated. Use this to synchronize to the database.
 */
targetdate_sorted_Nodes Update_repeating_Nodes(const targetdate_sorted_Nodes & sortednodes, time_t t_pass) {
    targetdate_sorted_Nodes updatedrepeating;
    for (const auto & [t, node_ptr] : sortednodes) {
        if (t > t_pass) {
            break;
        }
        Edit_flags editflags;
        bool updated = false;
        while (node_ptr->get_repeats() && (node_ptr->get_targetdate()<=t_pass) && (node_ptr->get_tdpattern() != patt_yearly)) {
            if (!Node_completed_repeating(*node_ptr, editflags)) {
                break;
            }
            updated = true;
        }
        if (updated) {
            const_cast<Edit_flags *>(&(node_ptr->get_editflags()))->set_Edit_flags(editflags.get_Edit_flags());
            updatedrepeating.emplace(node_ptr->get_targetdate(), node_ptr);
        }
    }
    return updatedrepeating;
}

/// Add a Node to a Named Node List.
Named_Node_List_ptr Graph_modify_list_add(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.nodelist_ptr) {
        return nullptr;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Node ID key addition to Named Node List");
        return nullptr;
    }

    Named_Node_List_Element & requested_element = *gmoddata.nodelist_ptr;
    Node * node_ptr = graph.Node_by_id(requested_element.nkey);
    if (!node_ptr) {
        return nullptr;
    }

    return graph.add_to_List(requested_element.name.c_str(), *node_ptr);
}

/// Remove a Node from a Named Node List.
bool Graph_modify_list_remove(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.nodelist_ptr) {
        return false;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for Node ID key removal from Named Node List");
        return false;
    }

    Named_Node_List_Element & requested_element = *gmoddata.nodelist_ptr;
    return graph.remove_from_List(requested_element.name.c_str(), requested_element.nkey);
}

/// Deleta a Named Node List.
bool Graph_modify_list_delete(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.nodelist_ptr) {
        return false;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for deletion of Named Node List");
        return false;
    }

    Named_Node_List_Element & requested_element = *gmoddata.nodelist_ptr;
    return graph.delete_List(requested_element.name.c_str());
}

bool batch_to_NNL(Graph & graph, const Batchmod_targetdates & batchnodes, std::string list_name) {
    VERYVERBOSEOUT("Updating the '"+list_name+"' Named Node List\n");

    graph.delete_List(list_name);

    if (batchnodes.tdnkeys_num<1) {
        return true;
    }

    //size_t copied = 0;
    Node_ptr node_ptr = graph.Node_by_id(batchnodes.tdnkeys[0].nkey);
    if (!node_ptr) {
        ERRRETURNFALSE(__func__, "Node "+batchnodes.tdnkeys[0].nkey.str()+" not found in Graph");
    }
    Named_Node_List * nnl_ptr = graph.add_to_List(list_name, *node_ptr);
    if (!nnl_ptr) {
        ERRRETURNFALSE(__func__, "Unable to create the "+list_name+" Named Node List for updated Nodes to synchronize to database");
    }

    //++copied;
    for (size_t i = 1; i < batchnodes.tdnkeys_num; ++i) {
        node_ptr = graph.Node_by_id(batchnodes.tdnkeys[i].nkey);
        if (!node_ptr) {
            ERRRETURNFALSE(__func__, "Node "+batchnodes.tdnkeys[i].nkey.str()+" not found in Graph");
        }
        if (graph.add_to_List(*nnl_ptr, *node_ptr)) {
            //++copied;
        }
    }
    //return copied;

    return true;
}

/// Modify the targetdates of a batch of Nodes.
bool Graph_modify_batch_node_targetdates(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    if (!gmoddata.batchmodtd_ptr) {
        return false;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for batch modification of Nodes targetdates");
        return false;
    }

    //Batchmod_targetdates & batchmodtd = *(gmoddata.batchmodtd_ptr.get());
    //VERYVERBOSEOUT("Batch with "+std::to_string(batchmodtd.tdnkeys_num)+" Nodes and target dates received.\n");
    VERYVERBOSEOUT("Batch with "+std::to_string(gmoddata.batchmodtd_ptr->tdnkeys_num)+" Nodes and target dates received.\n");
    for (size_t i = 0; i < gmoddata.batchmodtd_ptr->tdnkeys_num; ++i) {
        Node_ptr node_ptr = graph.Node_by_id(gmoddata.batchmodtd_ptr->tdnkeys[i].nkey);
        if (!node_ptr) {
            ERRRETURNFALSE(__func__, "Node "+gmoddata.batchmodtd_ptr->tdnkeys[i].nkey.str()+" not found in Graph");
        }
        node_ptr->set_targetdate(gmoddata.batchmodtd_ptr->tdnkeys[i].td);
    }
    VERYVERBOSEOUT("Batch target dates updated.\n");

    return batch_to_NNL(graph, *(gmoddata.batchmodtd_ptr.get()), "batch_updated");
}

/**
 * Copy Nodes from a sorted list to a Named Node List. This does not include synchronization to database.
 * See, for example, how this is used when processing a `batchmod_tpassrepeating` request in `fzserverpq`.
 * 
 * @param graph A memory-resident Graph.
 * @param sortednodes The target date sorted list of Nodes.
 * @param list_name A Named Node List. If it already exists then it is deleted (even if no new one is created).
 * @return The number of Nodes copied from the sorted list to the NNL, or the error code -1.
 */
ssize_t sorted_to_NNL(Graph & graph, const targetdate_sorted_Nodes & sortednodes, std::string list_name) {
    ERRTRACE;
    VERYVERBOSEOUT("Updating the '"+list_name+"' Named Node List\n");

    graph.delete_List(list_name);

    if (sortednodes.empty()) {
        return 0;
    }

    auto source_it = sortednodes.begin();
    ssize_t copied = 0;
    Named_Node_List * nnl_ptr = graph.add_to_List(list_name, *(source_it->second));
    if (!nnl_ptr) {
        ERRRETURNFALSE(__func__, "Unable to create the "+list_name+" Named Node List for updated Nodes to synchronize to database");
    }

    ++source_it;
    ++copied;
    for ( ; source_it != sortednodes.end(); ++source_it) {
        if (graph.add_to_List(*nnl_ptr, *(source_it->second))) {
            ++copied;
        }
    }
    return copied;
}

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
ssize_t Graph_modify_batch_node_tpassrepeating(Graph & graph, const std::string & graph_segname, const Graphmod_data & gmoddata) {
    ERRTRACE;
    if (!gmoddata.batchmodtpass_ptr) {
        return -1;
    }

    if (!graphmemman.set_active(graph_segname)) {
        ADDERROR(__func__, "Unable to activate segment "+graph_segname+" for batch modification of Nodes past t_pass");
        return -1;
    }

    Batchmod_tpass & batchmodtpass = *(gmoddata.batchmodtpass_ptr.get());
    time_t t_pass = batchmodtpass.t_pass;
    targetdate_sorted_Nodes incomplete_repeating = Nodes_incomplete_and_repeating_by_targetdate(graph);

    targetdate_sorted_Nodes updated_repeating = Update_repeating_Nodes(incomplete_repeating, t_pass);

    return sorted_to_NNL(graph, updated_repeating, "repeating_updated");
}

Graph_modifications::Graph_modifications() : data(graphmemman.get_allocator()) {
    segment_name = graphmemman.get_active_name();
    graph_ptr = nullptr;
    if (!graphmemman.get_Graph(graph_ptr)) {
        throw(Shared_Memory_exception("none containing a Graph are active"));
    }
    graphmemman.set_active(segment_name);
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
    graphmemman.set_active(segment_name);
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

Node * Graph_modifications::request_edit_Node(std::string nkeystr) {
    // The Node must already exist in the memory-resident graph provided through graph_ptr.
    if (!graph_ptr) {
        return nullptr;
    }
    if (!(graph_ptr->Node_by_idstr(nkeystr))) {
        ADDERROR(__func__, "Node with ID "+nkeystr+" not found in Graph.");
        return nullptr;
    }

    // Create Node object with existing Node ID in the shared memory segment being used to share a modifications request stack.
    graphmemman.set_active(segment_name);
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Node * node_ptr = smem->construct<Node>(bi::anonymous_instance)(nkeystr); // this normal pointer is emplaced into an offset_ptr
    if (!node_ptr) {
        ADDERROR(__func__, "Unable to construct Node in shared memory");
        return nullptr;
    }

    data.emplace_back(graphmod_edit_node, node_ptr);
    return node_ptr;
}

/// Note that testing if depkey and supkey exists happens in the server (see https://trello.com/c/FQximby2/174-fzgraphedit-adding-new-nodes-to-the-graph-with-initial-edges#comment-5f8faf243d74b8364fac7739).
Edge * Graph_modifications::request_add_Edge(const Node_ID_key & depkey, const Node_ID_key & supkey) {
    // Create new Edge object in the shared memory segment being used to share a modifications request stack.
    graphmemman.set_active(segment_name);
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

Edge * Graph_modifications::request_edit_Edge(std::string ekeystr) {
    // The Edge must already exist in the memory-resident graph provided through graph_ptr.
    if (!graph_ptr) {
        return nullptr;
    }
    if (!(graph_ptr->Edge_by_idstr(ekeystr))) {
        ADDERROR(__func__, "Edge with ID "+ekeystr+" not found in Graph.");
        return nullptr;
    }

    // Create Edge object with existing Edge ID in the shared memory segment being used to share a modifications request stack.
    graphmemman.set_active(segment_name);
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Edge * edge_ptr = smem->construct<Edge>(bi::anonymous_instance)(ekeystr); // this normal pointer is emplaced into an offset_ptr
    if (!edge_ptr) {
        ADDERROR(__func__, "Unable to construct Edge in shared memory");
        return nullptr;
    }
    
    data.emplace_back(graphmod_edit_edge, edge_ptr);
    return edge_ptr;
}

Named_Node_List_Element * Graph_modifications::request_Named_Node_List_Element(Graph_modification_request request, const std::string _name, const Node_ID_key & nkey) {
    // Create new Named_Node_List_Element object in the shared memory segment being used to share a modifications request stack.
    graphmemman.set_active(segment_name);
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Named_Node_List_Element * listelement_ptr = smem->construct<Named_Node_List_Element>(bi::anonymous_instance)(_name, nkey); // this normal pointer is emplaced into an offset_ptr
    if (!listelement_ptr) {
        ADDERROR(__func__, "Unable to construct Named Node List Element in shared memory");
        return nullptr;
    }
    
    data.emplace_back(request, listelement_ptr);
    return listelement_ptr;
}

Batchmod_targetdates * Graph_modifications::request_Batch_Node_Targetdates(const targetdate_sorted_Nodes & nodelist) {
    // Create new Batchmod_targetdates object in the shared memory segment being used to share a modification request stack.
    graphmemman.set_active(segment_name);
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Batchmod_targetdates * batchmodtd_ptr = smem->construct<Batchmod_targetdates>(bi::anonymous_instance)(nodelist, *smem); // this normal pointer is emplaced into an offset_ptr
    if (!batchmodtd_ptr) {
        ADDERROR(__func__, "Unable to construct Batch Node targetdates structure in shared memory");
        return nullptr;
    }
    
    data.emplace_back(batchmodtd_ptr);
    return batchmodtd_ptr;
}

Batchmod_tpass * Graph_modifications::request_Batch_Node_Tpass(time_t t_pass) { //, const targetdate_sorted_Nodes & nodelist) {
    // Create new Batchmod_targetdates object in the shared memory segment being used to share a modification request stack.
    graphmemman.set_active(segment_name);
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem) {
        ADDERROR(__func__, "Shared segment pointer was null pointer");
        return nullptr;
    }

    Batchmod_tpass * batchmodtpass_ptr = smem->construct<Batchmod_tpass>(bi::anonymous_instance)(t_pass); // this normal pointer is emplaced into an offset_ptr
    //Batchmod_targetdates * batchmodtd_ptr = smem->construct<Batchmod_targetdates>(bi::anonymous_instance)(t_pass, nodelist, *smem); // this normal pointer is emplaced into an offset_ptr
    if (!batchmodtpass_ptr) {
        ADDERROR(__func__, "Unable to construct Batch Node t_pass structure in shared memory");
        return nullptr;
    }
    
    data.emplace_back(batchmodtpass_ptr);
    return batchmodtpass_ptr;
}

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
size_t copy_Incomplete_to_List(Graph & graph, const std::string to_name, size_t from_max, size_t to_max, int16_t _features, int32_t _maxsize) {
    targetdate_sorted_Nodes source_nodes = Nodes_incomplete_by_targetdate(graph);
    if (source_nodes.empty()) {
        return 0;
    }
    if (to_name.empty()) {
        return 0;
    }

    if (from_max == 0) { // make from_max the actual max we might copy
        from_max = source_nodes.size();
    }
    Named_Node_List_ptr nnl_ptr = graph.get_List(to_name);
    if (to_max > 0) { // this may add a constraint
        if (nnl_ptr) { // list exists
            if (nnl_ptr->list.size()>=to_max) { // unable to add any
                return 0;
            }
            to_max -= nnl_ptr->list.size();
        }
        if (to_max < from_max) { // copy only as many as may be added
            from_max = to_max; 
        }
    }

    auto source_it = source_nodes.begin();
    size_t copied = 0;
    if (!nnl_ptr) { // brand new list
        // initialize the new list and get a pointer to it, adding with that will be faster than many name lookups
        nnl_ptr = graph.add_to_List(to_name, *(source_it->second), _features, _maxsize);
        if (!nnl_ptr) {
            return 0; // something went wrong
        }
        ++source_it;
        --from_max;
        ++copied;
    }
    for ( ; source_it != source_nodes.end(); ++source_it) {
        if (graph.add_to_List(*nnl_ptr, *(source_it->second))) {
            ++copied;
            --from_max;
            if (from_max==0) {
                return copied;
            }
        }
    }

    return copied;
}

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
size_t update_shortlist_List(Graph & graph) {
    graph.delete_List("shortlist");
    size_t copied = graph.copy_List_to_List("recent", "shortlist", 5, 10, Named_Node_List::unique_mask, 10); // I have to specify maxsize=10 here or else it will copy maxsize=5 from recent
    copied += copy_Incomplete_to_List(graph, "shortlist", 0, 10);
    return copied;
}

} // namespace fz
