// Copyright 2020 Randal A. Koene
// License TBD

/**
 * NOTE: This library component has been taken out of active maintenance to save on
 *       unnecessary compilation time, since it is not presently being used by any
 *       core or tools programs.
 */

// std
#include <iomanip>

// core
#include "general.hpp"
#include "utf8.hpp"
#include "Graphunshared.hpp"

namespace fz {

/**
 * Returns the id (index) of a topic tag and adds it to the list of topic
 * tags if it was not already there.
 * 
 * If added:
 * 1) A new Topic object is created at the end of the topictags vector with
 * nextid, tag and title as constructor parameters.
 * 2) The new vector element is then called at index nextid, and a pointer to it is used
 * as the value at key=tag to add to the topicbytag map.
 * 
 * Note that the found or newly generated topic index is compared with the
 * compile-time constant HIGH_TOPIC_INDEX_WARNING to report very large indices
 * considered to be probably mistaken.
 * 
 * @param tag a topic tag label.
 * @param title a title string.
 * @return id (index) of the topic tag in the topictags vector.
 */
uint16_t Topic_Tags::find_or_add_Topic(std::string tag, std::string title) {
    Topic * topic = find_by_tag(tag);
    if (topic) {
        unsigned int foundid = topic->get_id();
        //+"/"+topic->get_tag()
        if (foundid>HIGH_TOPIC_INDEX_WARNING) ADDWARNING(__func__,"suspiciously large index topic->get_id()="+std::to_string(foundid)+" for topic "+tag);
        return foundid;
    }
    unsigned int nextid = topictags.size();
    if (nextid>UINT16_MAX) throw(std::overflow_error("Topics_Tags exceeds uint16_t tags capacity"));
    if (nextid>HIGH_TOPIC_INDEX_WARNING) ADDWARNING(__func__,"suspiciously large index topictags.size()="+std::to_string(nextid)+" for topic "+tag);
    Topic * newtopic = new Topic(nextid, tag, title); /// keyword,relevance pairs are added by a separate call
    topictags.emplace_back(newtopic);
    if (topictags[nextid]->get_id()!=nextid) ADDWARNING(__func__,"wrong index at topictags["+std::to_string(nextid)+"].get_id()="+std::to_string(topictags[nextid]->get_id()));
    topicbytag[tag] = newtopic;
    //if (tag=="components") ADDWARNING(__func__,"components Topic object at "+std::to_string((long) topicbytag[tag])+" from "+std::to_string((long) &(topictags[nextid])));
    if (topicbytag.size()!=topictags.size())
        ADDWARNING(__func__,"topicbytag map and topictags vector sizes differ after adding topic "+tag);
    if (topicbytag[tag]->get_id()!=nextid)
        ADDWARNING(__func__,"topicbytag[\""+tag+"\"]->get_id()!="+std::to_string(nextid));
    return nextid;
}

/**
 * Search the topicbytag map and return a pointer to the Topic if the tag
 * was found.
 * 
 * @param _tag a topic tag label
 * @return pointer to Topic object in topictags vector, or NULL if not found.
 */
Topic * Topic_Tags::find_by_tag(std::string _tag) {
    if (_tag.empty()) return NULL;
    auto it = topicbytag.find(_tag);
    if (it==topicbytag.end()) return NULL;
    //if (it->second->get_id()>1000) ADDWARNING(__func__,"this seems wrong at iterator for "+it->first+ " at "+std::to_string((long) it->second));
    return it->second;
}

Node_ID::Node_ID(const ID_TimeStamp _idT): idkey(_idT) {
    idS_cache = Node_ID_TimeStamp_to_string(idkey.idT);
}

bool Node::add_topic(Topic_Tags &topictags, uint16_t topicid, float topicrelevance) {
    if (!topictags.find_by_id(topicid)) {
        ADDWARNING(__func__,"could not find topic with id="+std::to_string(topicid)+" in Node #"+id.str());
        return false; /// id needs to exist in Topic_Tags first
    }
    auto ret = topics.emplace(topicid,topicrelevance); // set, so unique ids only
    return ret.second; // was it actually added?
}

bool Node::add_topic(Topic_Tags &topictags, std::string tag, std::string title, float topicrelevance) {
    uint16_t id = topictags.find_or_add_Topic(tag, title);
    auto ret = topics.emplace(id,topicrelevance);
    return ret.second;
}

bool Node::add_topic(uint16_t topicid, float topicrelevance) {
    if (!graph) return false;
    return add_topic(graph->topics, topicid, topicrelevance); // NOTICE: Accessing friend class member variable directly to avoid const issues with get_topics().
}

bool Node::add_topic(std::string tag, std::string title, float topicrelevance) {
    if (!graph) return false;
    return add_topic(graph->topics, tag, title, topicrelevance); // NOTICE: Accessing friend class member variable directly to avoid const issues with get_topics().
}

bool Node::remove_topic(uint16_t id) {
    if (topics.size()<=1) return false; /// By convention, you must have at least one topic tag.
    return (topics.erase(id)>0);
}

bool Node::remove_topic(std::string tag) {
    if (!graph) return false;
    if (topics.size()<=1) return false; /// By convention, you must have at least one topic tag.
    Topic* topic = graph->topics.find_by_tag(tag);
    if (!topic) return false;
    return remove_topic(topic->get_id());
}

/**
 * Attempt to set the semaphore variables of all Nodes in the graph to a specified
 * value.
 * 
 * @return true if sucessful, false if the Node is not connected to a valid Graph.
 */
bool Node::set_all_semaphores(int sval) {
    if (!graph) return false;
    graph->set_all_semaphores(sval);
    return true;
}

/**
 * This is the nested recursive Graph traversal function used to search for
 * inherited target dates. Normally only call this function recursively or
 * from inherit_targetdate().
 * 
 * Like the effective_targetdate() function, this function attempts to correct
 * a situation where the local targetdate is unspecified while the local
 * tdproperty does not indicate that. For more information, see the comments
 * of the effective_targetdate() function.
 * 
 * @return the local targetdate parameter value if specified, or MAXTIME_T if
 * a loop conditin was encountered, or the earliest target date returned by
 * recursion to superior Nodes, or MAXTIME_T if there were none.
 */
time_t Node::nested_inherit_targetdate() {
    if (get_semaphore()==SEM_TRAVERSED) {
        if (graph) { // send an optional loop warning
            if (graph->warn_loops) ADDWARNING(__func__,"loop detected at Node DIL#"+get_id().str());
        }
        return (MAXTIME_T);
    }

    set_semaphore(SEM_TRAVERSED);

    if ((tdproperty != td_property::unspecified) && (tdproperty != td_property::inherit)) {
        if (targetdate >= 0)
            return targetdate; // send back the local parameter value

        // apply a fix
        if (tdproperty == td_property::fixed)
            tdproperty = td_property::inherit;
        else
            tdproperty = td_property::unspecified;       
    }

    // continue the recursive search
    time_t earliest = (MAXTIME_T);
    for (auto it = supedges.begin(); it != supedges.end(); ++it) {
        Node & supnode = *((*it)->sup);
        time_t sup_targetdate = supnode.nested_inherit_targetdate();
        if (sup_targetdate<earliest) earliest = sup_targetdate;
    }
    return earliest;
}

/**
 * This function attempt to determine an inherited target date from superior Nodes. It
 * does not use the local targetdate parameter of the Node.
 * 
 * Use this function with care. It is normally used by the safe function
 * effective_targetdate() to resolve unspecified targe dates.
 * 
 * This function return the special value -2 if the Node is not connected to a Graph.
 * 
 * @return the earliest date and time in time_t format that could be retrieved
 * from the tree of superior Nodes, or MAXTIME_T if none could be found.
 */
time_t Node::inherit_targetdate() {
    // ***It is technically possible to cache the result found here to speed up
    // ***future calls to this function. To do that safely, calls to set_targetdate()
    // ***would need to invalidate the cache for all dependency Nods of the Node
    // ***where the targetdate was changed. It is not clear if the time savings
    // ***is worth the added complexity.

    if (!set_all_semaphores(0)) return -2;
    set_semaphore(SEM_TRAVERSED); // not using targetdate of this Node
    time_t earliest = (MAXTIME_T);
    for (auto it = supedges.begin(); it != supedges.end(); ++it) {
        Node & supnode = *((*it)->sup);
        time_t sup_targetdate = supnode.nested_inherit_targetdate();
        if (sup_targetdate<earliest) earliest = sup_targetdate;
    }
    return earliest;
}

/**
 * Determine the effective target date of the Node using the inheritance protocol.
 * 
 * If the Node target date is specified at the Node then that target date is used.
 * If the Node target date is unspecifeid, then the target dates of Superiors are
 * recursively checked to find and inherit the earliest one.
 * 
 * \attention In Graph v2.x format, the tdproperty values 'unspecified' and 'inherit' always
 * take precedence over the value of the local targetvalue parameter. Both of
 * those properties demand that the effective target date of the Node is obtained
 * by inheritance from Superiors, and if none is inherited then -1 is returned to
 * indicate that no target date is to be taken into consideration when scheduling
 * this Node.
 * 
 * \attention If the tdproperty value is not 'unspecified' or 'inherit' then the local
 * targetdate value of the Node is used, unless that value is smaller than 0. If
 * it is smaller than 0 then this function attempts to correct the specification
 * as follows:
 * a) If tdproperty was 'fixed' then tdproperty is corrected to 'inherit'.
 * b) If tdproperty was any other value then tdproperty is corrected to 'unspecified'.
 * 
 * \attention Where this function is concerned, any value in the local targetdate parameter
 * is ignored if tdproperty is 'unspecified' or 'inherit'. It is therefore
 * technically possible to use the targetdate parameter as a cache for a potential
 * target date hint (in case tdproperty is changed), but keep in mind that such
 * hints are not guaranteed to be preserved to earlier or later versions of the
 * data structure format.
 * 
 * Historical: This is the Graph v2.x equivalent of the dil2al v1.x DIL_entry::Target_Date()
 * function that obtains a target date, propagated as needed from Superiors, while also
 * taking note of and protecting against possible loops in the graph structure.
 * 
 * @return the date and time in time_t seconds format, or -1 if there is no target date
 * to take into account when scheduling this Node.
 */
time_t Node::effective_targetdate() {
    if ((tdproperty == td_property::unspecified) || (tdproperty == td_property::inherit))
        return inherit_targetdate();

    if (targetdate >= 0)
        return targetdate;

    if (tdproperty == td_property::fixed)
        tdproperty = td_property::inherit;
    else
        tdproperty = td_property::unspecified;

    return inherit_targetdate();
}

/**
 * Set the Node.text parameter content and ensure that it contains
 * valid UTF8 encoded content.
 * 
 * Attempts to assign content to the text parameter as provided, and
 * replaces any invalid UTF8 code points with a replacement
 * character. The default replacement character is the UTF8
 * 'REPLACEMENT CHARACTER' 0xfffd.
 * 
 * If invalid UTF8 code points were encountered and replaced then it
 * is reported through ADDWARNING.
 * 
 * Note 1: ASCII text is valid UTF8 text and will be assigned
 *         unaltered.
 * Note 2: This does not test for valid HTML5 at this time.
 * 
 * @param utf8str a string that should contain UTF8 encoded text.
 */
void Node::set_text(const std::string utf8str) {
    text = utf8_safe(utf8str);
}

/**
 * Report the main Topic Index-ID of the Node, as indicated by the maximum
 * `Topic_Relevance` value.
 * 
 * @return Topic_ID of main Topic.
 */
Topic_ID Node::main_topic_id() {
    Topic_ID main_id = 0;
    Topic_Relevance max_rel = 0.0;
    for (const auto& [t_id, t_rel] : topics) {
        if (t_rel>max_rel)
            main_id = t_id;
    }
    return main_id;
}

Edge_ID::Edge_ID(Edge_ID_key _idkey): idkey(_idkey) {
    std::string formerror;
    if (!valid_Node_ID(idkey.dep.idT,formerror)) throw(ID_exception(formerror));
    if (!valid_Node_ID(idkey.sup.idT,formerror)) throw(ID_exception(formerror));

    idS_cache = Node_ID_TimeStamp_to_string(idkey.dep.idT)+'>'+Node_ID_TimeStamp_to_string(idkey.sup.idT);
}

Edge_ID::Edge_ID(Node &_dep, Node &_sup): idkey(_dep.get_id().key(),_sup.get_id().key()) {
    idS_cache = Node_ID_TimeStamp_to_string(idkey.dep.idT)+'>'+Node_ID_TimeStamp_to_string(idkey.sup.idT);
}

/**
 * An Edge can only exist between two Nodes. Therefore, if the actual Node
 * objects are not provided a Graph must be provided in which the Nodes
 * with specified IDs must exist.
 * 
 * This constructor also adds the Edge to the Graph. No separate call is
 * needed.
 * 
 * Validity of the ID string is tested in the Edge_ID_key via Edge_ID.
 * 
 * This constructor throws exceptions if the ID is invalid or if the
 * Node objects indicated by the ID are not found.
 * 
 * @param graph a valid Graph containing Node objects.
 * @param id_str the Edge ID string, validity will be tested.
 */
Edge::Edge(Graph & graph, std::string id_str): id(id_str) {
    std::string formerror;
    if (!(dep = graph.Node_by_id(id.key().dep))) {
        formerror = "dependency not found";
        throw(ID_exception(formerror));
    }
    if (!(sup = graph.Node_by_id(id.key().sup))) {
        formerror = "superior not found";
        throw(ID_exception(formerror));
    }
    if (!graph.add_Edge(this)) {
        formerror = "unable to add to Graph";
        throw(ID_exception(formerror));
    }
}

bool Graph::add_Node(Node &node) {
    std::pair<Node_Map::iterator, bool> ret;
    ret = nodes.insert(std::pair<Node_ID_key, Node *>(node.get_id().key(), &node));
    if (!ret.second)
        error = g_adddupnode;
    else
        node.graph = this;
    return ret.second;
}

bool Graph::add_Node(Node *node) {
    if (!node) {
        error = g_addnullnode;
        return false;
    }
    return add_Node(*node);
}

bool Graph::add_Edge(Edge &edge) {
    std::pair<Edge_Map::iterator, bool> ret;
    ret = edges.insert(std::pair<Edge_ID_key, Edge *>(edge.get_id().key(), &edge));
    if (!ret.second) {
        error = g_adddupedge;
        return false;
    }
    
    edge.get_dep()->supedges.emplace(&edge); // update rapid access set
    edge.get_sup()->depedges.emplace(&edge); // update rapid access set
    return true;
}

bool Graph::add_Edge(Edge *edge) {
    if (!edge) {
        error = g_addnulledge;
        return false;
    }
    return add_Edge(*edge);
}

bool Graph::remove_Edge(Edge &edge) {
    Node & dep = *(edge.get_dep());
    Node & sup = *(edge.get_sup());
    Edge * e = &edge; // by using these this will work even if we store Edge in the map instead of Edge*

    if (edges.erase(edge.get_id().key())<1) {
        error = g_removeunknownedge;
        return false;
    }

    dep.supedges.erase(e); // update rapid access set
    sup.depedges.erase(e); // update rapid access set
    return true;
}

bool Graph::remove_Edge(Edge *edge) {
    if (!edge) {
        error = g_removenulledge;
        return false;
    }
    return remove_Edge(*edge);
}

/**
 * Set the semaphore variables of all Nodes in the Graph to a specified value.
 * 
 * An example where this is used are Graph traversing functions, such as the
 * Node::inherit_targetdate() function.
 */
void Graph::set_all_semaphores(int sval) {
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
        it->second->set_semaphore(sval);
}

Node_Index Graph::get_Indexed_Nodes() const {
    Node_Index nodeindex;
    for (const auto& nodekp: nodes) {
        nodeindex.push_back(nodekp.second);
    }
    return nodeindex;
}

// +----- begin: friend functions -----+

/**
 * Find the main Topic of a Node, as indicated by the maximum
 * Topic_Relevance value.
 * 
 * Note: This is a friend function in order to ensure that the search for
 *       the Topic object is called only when a valid Topic_Tags list
 *       can provid pointers to them.
 * 
 * @param Topic_Tags a valid Topic_Tags list.
 * @param node a Node for which the main Topic is requested.
 * @return a pointer to the Topic object (or nullptr if not found).
 */
Topic * main_topic(Topic_Tags & topictags, Node & node) {
    Topic * maintopic = nullptr;
    Topic_Relevance max_rel = 0.0;
    for (const auto& [t_id, t_rel] : node.topics) {
        if (t_rel>max_rel)
            maintopic = topictags.find_by_id(t_id);
    }
    return maintopic;
}

/**
 * Find the main Topic of a Node, as indicated by the maximum
 * Topic_Relevance value.
 * 
 * Note: This is a friend function in order to ensure that the search for
 *       the Topic object is called only when a valid Topic_Tags list that
 *       is known to a valid Graph can provid pointers to them.
 * 
 * @param _graph a valid Graph that has a Topic_Tags list.
 * @param node a Node for which the main Topic is requested.
 * @return a pointer to the Topic object (or nullptr if not found).
 */
Topic * main_topic(Graph & _graph, Node & node) {
    return _graph.main_Topic_of_Node(node);
}

// +----- end  : friend functions -----+

} // namespace fz
