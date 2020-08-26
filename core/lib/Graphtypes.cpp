// Copyright 2020 Randal A. Koene
// License TBD

#include <iomanip>
#include "utfcpp/source/utf8.h"

#include "general.hpp"
#include "Graphtypes.hpp"

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
    Topic * newtopic = new Topic(nextid,tag,title); /// keyword,relevance pairs are added by a separate call
    topictags.emplace_back(newtopic);
    if (topictags[nextid]->get_id()!=nextid) ADDWARNING(__func__,"wrong index at topictags["+std::to_string(nextid)+"].get_id()="+std::to_string(topictags[nextid]->get_id()));
    topicbytag[tag] = newtopic;
    //if (tag=="components") ADDWARNING(__func__,"components Topic object at "+std::to_string((long) topicbytag[tag])+" from "+std::to_string((long) &(topictags[nextid])));
    if (topicbytag.size()!=topictags.size()) ADDWARNING(__func__,"topicbytag map and topictags vector sizes differ after adding topic "+tag);
    if (topicbytag[tag]->get_id()!=nextid) ADDWARNING(__func__,"topicbytag[\""+tag+"\"]->get_id()!="+std::to_string(nextid));
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

const std::string td_property_str[_tdprop_num] = {"unspecified",
                                                  "inherit",
                                                  "variable",
                                                  "fixed",
                                                  "exact"};

const std::string td_pattern_str[_patt_num] =  {"patt_daily",
                                                "patt_workdays",
                                                "patt_weekly",
                                                "patt_biweekly",
                                                "patt_monthly",
                                                "patt_endofmonthoffset",
                                                "patt_yearly",
                                                "OLD_patt_span",
                                                "patt_nonperiodic"};

#define VALID_NODE_ID_FAIL(f) \
    {                         \
        formerror = f;        \
        return false;         \
    }

/**
 * Test if a ID_TimeStamp can be used as a valid Node_ID.
 * 
 * Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param idT reference to an ID_TimeStamp object.
 * @param formerror a string that collects specific error information if there is any.
 * @return true if valid.
 */
bool valid_Node_ID(const ID_TimeStamp &idT, std::string &formerror) {
    if (idT.year < 1999)
        VALID_NODE_ID_FAIL("year");
    if ((idT.month < 1) || (idT.month > 12))
        VALID_NODE_ID_FAIL("month");
    if ((idT.day < 1) || (idT.day > 31))
        VALID_NODE_ID_FAIL("day");
    if (idT.hour > 23)
        VALID_NODE_ID_FAIL("hour");
    if (idT.minute > 59)
        VALID_NODE_ID_FAIL("minute");
    if (idT.second > 59)
        VALID_NODE_ID_FAIL("second");
    if (idT.minor_id < 1)
        VALID_NODE_ID_FAIL("minor_id");
    return true;
}

/**
 * Test if a string can be used to form a valid Node_ID.
 * 
 * Checks string length, period separating time stamp from minor ID,
 * all digits in time stamp and minor ID, and time stamp components
 * within valid ranges. Note that years before 1999 are disqualified,
 * since the Formalizer did not exist before then.
 * 
 * @param id_str a string of the format YYYYmmddHHMMSS.num.
 * @param formerror a string that collects specific error information if there is any.
 * @param id_timestamp if not NULL, receives valid components.
 * @return true if valid.
 */
bool valid_Node_ID(std::string id_str, std::string &formerror, ID_TimeStamp *id_timestamp) {

    if (id_str.length() < 16)
        VALID_NODE_ID_FAIL("string size");
    if (id_str[14] != '.')
        VALID_NODE_ID_FAIL("format");
    for (int i = 0; i < 14; i++)
        if (!isdigit(id_str[i]))
            VALID_NODE_ID_FAIL("digits");

    ID_TimeStamp idT;
    idT.year = stoi(id_str.substr(0, 4));
    idT.month = stoi(id_str.substr(4, 2));
    idT.day = stoi(id_str.substr(6, 2));
    idT.hour = stoi(id_str.substr(8, 2));
    idT.minute = stoi(id_str.substr(10, 2));
    idT.second = stoi(id_str.substr(12, 2));
    idT.minor_id = stoi(id_str.substr(15));
    if (!valid_Node_ID(idT, formerror))
        return false;

    if (id_timestamp)
        *id_timestamp = idT;
    return true;
}

std::string Node_ID_TimeStamp_to_string(const ID_TimeStamp idT) {
    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (int) idT.year
       << std::setw(2) << (int) idT.month
       << std::setw(2) << (int) idT.day
       << std::setw(2) << (int) idT.hour
       << std::setw(2) << (int) idT.minute
       << std::setw(2) << (int) idT.second
       << '.' << std::setw(1) << (int) idT.minor_id;
    return ss.str();
}

/**
 * Convert standardized Formalizer Node ID time stamp into local time
 * data object.
 * 
 * Note: This function ignores the `minor_id` value.
 * 
 * @return local time data objet with converted Node ID time stamp.
 */
std::tm ID_TimeStamp::get_local_time() {
    std::tm tm = { 0 };
    tm.tm_year = year-1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    return tm;
}

Node_ID_key::Node_ID_key(const ID_TimeStamp& _idT) { //}: idC( { .id_major = 0, .id_minor = 0 } ) {
    idT=_idT;
    std::string formerror;
    if (!valid_Node_ID(_idT,formerror)) throw(ID_exception(formerror));
}

Node_ID_key::Node_ID_key(std::string _idS) { //}: idC( { .id_major = 0, .id_minor = 0 } ) {
    std::string formerror;
    if (!valid_Node_ID(_idS,formerror,&idT)) throw(ID_exception(formerror));
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
    uint16_t id = topictags.find_or_add_Topic(tag,title);
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
 * Set the Node.text parameter content an ensure that it contains
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
    utf8::reset_utf_fixes();
    text = utf8::replace_invalid(utf8str);
    if (utf8::check_utf_fixes()>0) ADDWARNING(__func__,"replaced "+std::to_string(utf8::check_utf_fixes())+" invalid UTF8 code points in Node description ("+utf8str.substr(0,20)+"...)");
}

/**
 * Report the main Topic (by ID) of the Node, as indicated by the maximum
 * Topic_Relevance value.
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
}

Edge_ID_key::Edge_ID_key(std::string _idS) {
    std::string formerror;
    size_t arrowpos = _idS.find('>');
    if (arrowpos==std::string::npos) {
        formerror = "arrow";
        throw(ID_exception(formerror));
    }
    if (!valid_Node_ID(_idS.substr(0,arrowpos),formerror,&dep.idT)) throw(ID_exception(formerror));
    if (!valid_Node_ID(_idS.substr(arrowpos+1),formerror,&sup.idT)) throw(ID_exception(formerror));
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

#define VALIDATIONFAIL(v1,v2) { \
    trace += ":DIFF(" + v1 + ',' + v2 + ')'; \
    return false; \
}

bool identical_Topics(const Topic & topic1, const Topic & topic2, std::string & trace) {
    std::string traceroot = trace;
    trace += "id";
    if (topic1.id!=topic2.id) VALIDATIONFAIL(std::to_string(topic1.id),std::to_string(topic2.id));
    trace = traceroot + "supid";
    if (topic1.supid!=topic2.supid) VALIDATIONFAIL(std::to_string(topic1.supid),std::to_string(topic2.supid));
    trace = traceroot + "tag";
    if (topic1.tag!=topic2.tag) VALIDATIONFAIL(topic1.tag,topic2.tag);
    trace = traceroot + "title";
    if (topic1.title!=topic2.title) VALIDATIONFAIL(topic1.title,topic2.title);

    trace = traceroot + "keyrel.size";
    if (topic1.keyrel.size()!=topic2.keyrel.size()) VALIDATIONFAIL(std::to_string(topic1.keyrel.size()),std::to_string(topic2.keyrel.size()));
    traceroot += "keyrel:";
    for (std::size_t kr = 0; kr < topic1.keyrel.size(); ++kr) {
        trace = traceroot + std::to_string(kr) + ":keyword";
        if (topic1.keyrel[kr].keyword!=topic2.keyrel[kr].keyword) VALIDATIONFAIL(topic1.keyrel[kr].keyword,topic2.keyrel[kr].keyword);
        trace = traceroot + std::to_string(kr) + ":relevance";
        if (topic1.keyrel[kr].relevance!=topic2.keyrel[kr].relevance) VALIDATIONFAIL(to_precision_string(topic1.keyrel[kr].relevance,3),to_precision_string(topic2.keyrel[kr].relevance,3));
    }

    return true;
}

bool identical_Topic_Tags(Topic_Tags & ttags1, Topic_Tags & ttags2, std::string & trace) {
    std::string traceroot = trace;
    trace += "topictags.size";
    if (ttags1.topictags.size()!=ttags2.topictags.size()) VALIDATIONFAIL(std::to_string(ttags1.topictags.size()),std::to_string(ttags2.topictags.size()));
    trace = traceroot + "topicbytag.size";
    if (ttags1.topicbytag.size()!=ttags2.topicbytag.size()) VALIDATIONFAIL(std::to_string(ttags1.topicbytag.size()),std::to_string(ttags2.topicbytag.size()));
    traceroot += "topictags:";
    for (std::size_t tid = 0; tid < ttags1.topictags.size(); ++tid) {
        trace = traceroot + std::to_string(tid) + ':';
        if (!identical_Topics(*ttags1.topictags[tid],*ttags2.topictags[tid],trace)) return false;
    }

    return true;
}

bool identical_Node_ID_key(const Node_ID_key & key1, const Node_ID_key & key2, std::string & trace) {
    // Let's do this in lexicographical manner.
    trace += "Node_ID_key";
    return key1.idT == key2.idT;
    //return std::tie(key1.idC.id_major,key1.idC.id_minor) == std::tie(key2.idC.id_major, key2.idC.id_minor);
}

/**
 * Determine if two Node objects contain the same data.
 * 
 * Note that this does not compare the rapid-access Edges_Set supedges and depedges,
 * since they are sets of pointers created as Graph Edges are added. They might
 * end up in a different order, but they ought to be the same ones as in the
 * Edge_Map.
 */
bool identical_Nodes(Node & node1, Node & node2, std::string & trace) {
    trace += node1.id.str() + ':';
    std::string traceroot = trace;
    if (!identical_Node_ID_key(node1.id.key(),node2.id.key(),trace)) VALIDATIONFAIL(node1.id.str(),node2.id.str());

    trace = traceroot + "valuation";
    if (node1.valuation != node2.valuation ) VALIDATIONFAIL(to_precision_string(node1.valuation,3),to_precision_string(node2.valuation,3));
    trace = traceroot + "completion";
    if (node1.completion != node2.completion ) VALIDATIONFAIL(to_precision_string(node1.completion,3),to_precision_string(node2.completion,3));
    trace = traceroot + "required";
    if (node1.required != node2.required ) VALIDATIONFAIL(std::to_string(node1.required),std::to_string(node2.required));
    trace = traceroot + "text";
    if (node1.text != node2.text ) VALIDATIONFAIL(node1.text,node2.text);
    trace = traceroot + "targetdate";
    if (node1.targetdate != node2.targetdate ) VALIDATIONFAIL(node1.get_targetdate_str(),node2.get_targetdate_str());
    trace = traceroot + "tdproperty";
    if (node1.tdproperty != node2.tdproperty ) VALIDATIONFAIL(std::to_string(node1.tdproperty),std::to_string(node2.tdproperty));
    trace = traceroot + "repeats";
    if (node1.repeats != node2.repeats ) VALIDATIONFAIL(std::to_string(node1.repeats),std::to_string(node2.repeats));
    trace = traceroot + "tdpattern";
    if (node1.tdpattern != node2.tdpattern ) VALIDATIONFAIL(std::to_string(node1.tdpattern),std::to_string(node2.tdpattern));
    trace = traceroot + "tdevery";
    if (node1.tdevery != node2.tdevery ) VALIDATIONFAIL(std::to_string(node1.tdevery),std::to_string(node2.tdevery));
    trace = traceroot + "tdspan";
    if (node1.tdspan != node2.tdspan ) VALIDATIONFAIL(std::to_string(node1.tdspan),std::to_string(node2.tdspan));

    trace = traceroot + "topics.size";
    if (node1.topics.size() != node2.topics.size()) VALIDATIONFAIL(std::to_string(node1.topics.size()),std::to_string(node2.topics.size()));
    traceroot += "topics:";
    for (auto nt1 = node1.topics.begin(); nt1 != node1.topics.end(); ++nt1) {
        auto nt2 = node2.topics.find(nt1->first);
        trace = traceroot + std::to_string(nt1->first);
        if (nt2==node2.topics.end()) return false;
        trace += ":rel";
        if (nt1->second!=nt2->second) VALIDATIONFAIL(to_precision_string(nt1->second,3),to_precision_string(nt2->second,3));
    }

    return true;
}

bool identical_Edge_ID_key(const Edge_ID_key & key1, const Edge_ID_key & key2, std::string & trace) {
    // Let's do this in lexicographical manner.
    trace += "Edge_ID_key";
    return std::tie(key1.sup.idT,key1.dep.idT) == std::tie(key2.sup.idT,key2.dep.idT);
    //return std::tie(key1.sup.idC.id_major, key1.sup.idC.id_minor, key1.dep.idC.id_major, key1.dep.idC.id_minor) == std::tie(key2.sup.idC.id_major, key2.sup.idC.id_minor, key2.dep.idC.id_major, key2.dep.idC.id_minor);
}

bool identical_Edges(Edge & edge1, Edge & edge2, std::string & trace) {
    trace += edge1.id.str() + ':';
    std::string traceroot = trace;
    if (!identical_Edge_ID_key(edge1.id.key(),edge2.id.key(),trace)) VALIDATIONFAIL(edge1.id.str(),edge2.id.str());

    trace = traceroot + "dependency";
    if (edge1.dependency!=edge2.dependency) VALIDATIONFAIL(to_precision_string(edge1.dependency,3),to_precision_string(edge2.dependency,3));
    trace = traceroot + "significance";
    if (edge1.significance != edge2.significance ) VALIDATIONFAIL(to_precision_string(edge1.significance,3),to_precision_string(edge2.significance,3));
    trace = traceroot + "importance";
    if (edge1.importance != edge2.importance ) VALIDATIONFAIL(to_precision_string(edge1.importance,3),to_precision_string(edge2.importance,3));
    trace = traceroot + "urgency";
    if (edge1.urgency != edge2.urgency ) VALIDATIONFAIL(to_precision_string(edge1.urgency,3),to_precision_string(edge2.urgency,3));
    trace = traceroot + "priority";
    if (edge1.priority != edge2.priority ) VALIDATIONFAIL(to_precision_string(edge1.priority,3),to_precision_string(edge2.priority,3));

    return true;
}

/**
 * Compare two Graphs to report if they are data-identical.
 * 
 * @param graph1 the first Graph.
 * @param graph2 the second Graph.
 * @param trace if a difference is found then this contains a trace.
 * @return true if the two Graphs are equivalent.
 */
bool identical_Graphs(Graph & graph1, Graph & graph2, std::string & trace) {
    trace = "G.Topic_Tags:";
    if (!identical_Topic_Tags(graph1.topics,graph2.topics,trace)) return false;

    trace = "G.nodes.size:";
    if (graph1.nodes.size()!=graph2.nodes.size()) VALIDATIONFAIL(std::to_string(graph1.nodes.size()),std::to_string(graph2.nodes.size()));
    trace = "G.edges.size:";
    if (graph1.edges.size()!=graph2.edges.size()) VALIDATIONFAIL(std::to_string(graph1.edges.size()),std::to_string(graph2.edges.size()));

    std::string traceroot = "G.nodes:";
    for (auto nm = graph1.nodes.begin(); nm != graph1.nodes.end(); ++nm) {
        Node * n1 = nm->second;
        Node * n2 = graph2.Node_by_id(nm->first);
        trace = traceroot + nm->second->get_id().str();
        if ((!n1) || (!n2)) return false;
        trace = traceroot;
        if (!identical_Nodes(*n1,*n2,trace)) return false;
    }

    traceroot = "G.edges:";
    for (auto em = graph1.edges.begin(); em != graph1.edges.end(); ++em) {
        Edge * e1 = em->second;
        Edge * e2 = graph2.Edge_by_id(em->first);
        trace = traceroot + em->second->get_id().str();
        if ((!e1) || (!e2)) return false;
        trace = traceroot;
        if (!identical_Edges(*e1,*e2,trace)) return false;
    }

    return true;
}

// +----- end  : friend functions -----+

} // namespace fz
