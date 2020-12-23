// Copyright 2020 Randal A. Koene
// License TBD

//#define USE_COMPILEDPING

// std
#include <iomanip>

// Boost
#include <boost/interprocess/exceptions.hpp>

// core
#include "general.hpp"
#include "utf8.hpp"
#include "Graphtypes.hpp"

// Boost libraries need the following.
#pragma GCC diagnostic warning "-Wuninitialized"

namespace fz {

graph_mem_managers graphmemman; ///< Global access to shared memory managers for Graph data structures.

graph_mem_managers::~graph_mem_managers() {
    //if (remove_on_exit) {
    for (const auto & [name, manager] : managers) {
        if (manager.remove_on_exit)
            bi::shared_memory_object::remove(name.c_str());
    }
}

bool graph_mem_managers::add_manager(std::string segname, segment_memory_t & segmem, void_allocator & allocinst) {
    if (segname.empty())
        return false;

    shared_memory_manager smm(segmem, allocinst);
    return managers.emplace(segname, smm).second;
}

/**
 * Remove a manager from the set of managers and delete the
 * corresponding allocator and segment memory objects -
 * but do not destroy the shared memory.
 * 
 * Call this after you have finished working with the shared memory
 * that another process created and that you are not responsible for.
 * By removing the manager and associated objects you can prevent
 * accidentally working with stale shared memory pointes, and you can
 * receive new pointers with the same name in the future.
 * 
 * Note: Removing shared memory is a separate operation that can
 *       be called explicitly or by having set `remove_on_exit`. It
 *       is normally the responsibility of the process that created
 *       the shared memory to do so.
 * 
 * @param segname Name of the managed segment to forget.
 * @param return True if the named manager existed and was successfully removed.
 */
bool graph_mem_managers::forget_manager(std::string segname) {
    auto it = managers.find(segname);
    if (it == managers.end())
        return false;
    
    delete it->second.segmem_ptr;
    delete it->second.alloc_inst_ptr;
    managers.erase(it);

    if (active_name == segname) {
        active = nullptr;
        active_name = "";
    }
    return true;
}

bool graph_mem_managers::set_active(std::string segname) {
    auto it = managers.find(segname);
    if (it == managers.end()) {
        active = nullptr; // to prevent accidentally continuing with references to the wrong segment
        active_name = "";
        return false;
    }

    active = &(it->second);
    active_name = segname;
    return true;
}

segment_memory_t * graph_mem_managers::get_segmem() const {
    if (!active)
        throw(Shared_Memory_exception("none are active"));

    return active->segmem_ptr;
}

const segment_manager_t & graph_mem_managers::get_segman() const {
    if (!active)
        throw(Shared_Memory_exception("none are active"));

    return *(active->segmem_ptr->get_segment_manager());
}

const void_allocator & graph_mem_managers::get_allocator() const {
    if (!active)
        throw(Shared_Memory_exception("none are active"));

    return *(active->alloc_inst_ptr);
}

/// Get allocator for the active segment.
const void_allocator & graph_mem_managers::get_allocator(std::string segname) const {
    auto it = managers.find(segname);
    if (it == managers.end()) {
        throw(Shared_Memory_exception("shared segment "+segname+" not found in "+std::string(__func__)));
    }

    return *(it->second.alloc_inst_ptr);
}

/// Get allocator for named segment. The active segment is not changed.
segment_memory_t * graph_mem_managers::allocate_and_activate_shared_memory(std::string segment_name, unsigned long segmentsize) {
    if (segment_name.empty())
        return nullptr;

    if (segmentsize<1)
        return nullptr;

    bi::shared_memory_object::remove(segment_name.c_str()); // erase any previous shared memory with same name
    //bi::remove_shared_memory_on_destroy remove_on_destroy(segment_name.c_str()); // this calls remove in its destructor

    bi::permissions per;
    per.set_unrestricted(); // *** we might want to tighten this later, for now this is needed for web based access
    segment_memory_t * segment = new segment_memory_t(bi::create_only, segment_name.c_str(), segmentsize, 0, per);
    if (!segment) {
        ERRRETURNNULL(__func__, "Unable to create shared memory segment ("+segment_name+") with size "+std::to_string(segmentsize)+" bytes.");
    }

    void_allocator * alloc_inst = new void_allocator(segment->get_segment_manager());
    if (!alloc_inst) {
        ERRRETURNNULL(__func__, "Unable to create allocator for shared memory segment ("+segment_name+").");
    }

    if (!add_manager(segment_name, *segment, *alloc_inst)) {
        ERRRETURNNULL(__func__, "Unable to add segment manager for shared memory segment ("+segment_name+").");
    }
    if (!set_active(segment_name)) {
        ERRRETURNNULL(__func__, "Unable to activate shared memory segment ("+segment_name+").");
    }

    return segment;
}

//std::unique_ptr<Graph> graph_mem_managers::allocate_Graph_in_shared_memory() {
Graph_ptr graph_mem_managers::allocate_Graph_in_shared_memory() {

    segment_memory_t * segment = allocate_and_activate_shared_memory("fzgraph", 20*1024*1024); // *** improve this wild guess
    if (!segment)
        return nullptr;

    return segment->construct<Graph>("graph")(); // *** no parameters // std::less<char_string>(), alloc_inst);
}
/*
    Graph * gptr = segment->construct<Graph>("graph")(); // *** no parameters // std::less<char_string>(), alloc_inst);
    std::unique_ptr<Graph> graphptr(gptr);
    return graphptr;
}
*/

Graph_ptr graph_mem_managers::find_Graph_in_shared_memory() {
    std::string segment_name("fzgraph");
    try {
        segment_memory_t * segment = new segment_memory_t(bi::open_only, segment_name.c_str()); // was bi::open_read_only

        void_allocator * alloc_inst = new void_allocator(segment->get_segment_manager());

        add_manager(segment_name, *segment, *alloc_inst);
        set_active(segment_name);
        set_remove_on_exit(false); // looks like you're a client and not a server here

        VERYVERBOSEOUT(info_str());

        return segment->find<Graph>("graph").first;

    } catch (const bi::interprocess_exception & ipexception) {
        VERBOSEERR("Unable to access shared memory 'fzgraph', "+std::string(ipexception.what())+'\n');
        ERRRETURNNULL(__func__,"Unable to access shared memory 'fzgraph', "+std::string(ipexception.what()));
    }
    return nullptr;
}

/**
 * Get a Graph pointer from a pointer variable or set that variable
 * to the Graph address in shared memory if the variable was set to
 * nullptr.
 * 
 * For example, see how this is used in Graphmodify.cpp and in
 * fzaddnode.cpp.
 * 
 * Note: If the pointer variable provided is not nullptr then it is
 *       assumed that it contains a valid pointer to a Graph in
 *       shared memory. Therefore, make sure to initialize the
 *       pointer variable to nullptr at the start of a program!
 * 
 * @param graph_ptr A reference to a pointer variable.
 * @return The address of a Graph in shared memory or nullptr if not found.
 */
Graph_ptr graph_mem_managers::get_Graph(Graph_ptr & graph_ptr) {
    if (!graph_ptr) {

        graph_ptr = find_Graph_in_shared_memory(); // returns nullptr if not found

    }
    return graph_ptr;
}

void graph_mem_managers::info(Graph_info_label_value_pairs & meminfo) { //bi::managed_shared_memory & segment) {
    if (!active)
        return;
    
    auto segmem_ptr = active->segmem_ptr;
    if (!segmem_ptr)
        return;

    unsigned long the_result = segmem_ptr->get_size() - segmem_ptr->get_free_memory();
    meminfo["active_name"] = active_name;
    meminfo["num_named"] = std::to_string(segmem_ptr->get_num_named_objects());
    meminfo["num_unique"] = std::to_string(segmem_ptr->get_num_unique_objects());
    meminfo["size"] = std::to_string(segmem_ptr->get_size());
    meminfo["free"] = std::to_string(segmem_ptr->get_free_memory());
    meminfo["used"] = std::to_string(the_result);
}

std::string graph_mem_managers::info_str() { //bi::managed_shared_memory & segment) {
    if (!active)
        return "";
    
    auto segmem_ptr = active->segmem_ptr;
    if (!segmem_ptr)
        return "";

    unsigned long the_result = segmem_ptr->get_size() - segmem_ptr->get_free_memory();
    std::string info_str("Shared memory information:");
    info_str += "\n  selected shared segment  = " + active_name;
    info_str += "\n  number of named objects  = " + std::to_string(segmem_ptr->get_num_named_objects());
    info_str += "\n  number of unique objects = " + std::to_string(segmem_ptr->get_num_unique_objects());
    info_str += "\n  size                     = " + std::to_string(segmem_ptr->get_size());
    info_str += "\n  free memory              = " + std::to_string(segmem_ptr->get_free_memory());
    info_str += "\n  memory used              = " + std::to_string(the_result) + '\n';
    return info_str;
}

/**
 * Generate a string that shows Topic keywords and relevance values.
 * 
 * @return A string that concatenates the keywords with relevance values in brackets.
 */
std::string Topic::keyrel_str() const {
    std::string res;
    for (const auto & kr : keyrel) {
        res += kr.keyword.c_str() + (" (" + to_precision_string(kr.relevance, 1) + "), ");
    }
    res.pop_back();
    res.pop_back();
    return res;
}

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
        if (foundid>HIGH_TOPIC_INDEX_WARNING) ADDWARNING(__func__,"suspiciously large index topic->get_id()="+std::to_string(foundid)+(" for topic "+tag).c_str());
        return foundid;
    }
    unsigned int nextid = topictags.size();
    if (nextid>UINT16_MAX) throw(std::overflow_error("Topics_Tags exceeds uint16_t tags capacity"));
    if (nextid>HIGH_TOPIC_INDEX_WARNING) ADDWARNING(__func__,"suspiciously large index topictags.size()="+std::to_string(nextid)+(" for topic "+tag).c_str());
    //Topic * newtopic = new Topic(nextid, tag, title); /// keyword,relevance pairs are added by a separate call

    segment_memory_t * smem = graphmemman.get_segmem();
    if (smem) {

        Topic * newtopic = smem->construct<Topic>(bi::anonymous_instance)(nextid, tag, title); /// keyword,relevance pairs are added by a separate call

        if (newtopic) {
            topictags.emplace_back(newtopic);

            if (topictags[nextid]->get_id()!=nextid)
                ADDWARNING(__func__,"wrong index at topictags["+std::to_string(nextid)+"].get_id()="+std::to_string(topictags[nextid]->get_id()));

            Topic_String tstr(graphmemman.get_allocator());
            tstr = tag.c_str();
            topicbytag[tstr] = newtopic;
            //if (tag=="components") ADDWARNING(__func__,"components Topic object at "+std::to_string((long) topicbytag[tag])+" from "+std::to_string((long) &(topictags[nextid])));
            if (topicbytag.size()!=topictags.size())
                ADDWARNING(__func__,("topicbytag map and topictags vector sizes differ after adding topic "+tag).c_str());

            //Topic_String tstr(void_alloc);
            //tstr = tag.c_str();
            if (topicbytag[tstr]->get_id()!=nextid)
                ADDWARNING(__func__,("topicbytag[\""+tag+"\"]->get_id()!=").c_str()+std::to_string(nextid));
        } else {
            ADDERROR(__func__, "unable to construct new Topic in shared memory segment");
        }
    } else {
        ADDERROR(__func__, "unable to access shared memory segment");
    }

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
    Topic_String tstr(graphmemman.get_allocator());
    tstr = _tag.c_str();
    auto it = topicbytag.find(tstr);
    if (it==topicbytag.end()) return NULL;
    //if (it->second->get_id()>1000) ADDWARNING(__func__,"this seems wrong at iterator for "+it->first+ " at "+std::to_string((long) it->second));
    return it->second.get();
}

Node_ID::Node_ID(const ID_TimeStamp _idT): idkey(_idT), idS_cache("") { // , graphmemman.get_allocator()) {
    idS_cache = Node_ID_TimeStamp_to_string(idkey.idT).c_str();
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
    uint16_t id = topictags.find_or_add_Topic(tag.c_str(), title.c_str());
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
 * @param origin A pointer buffer to report the Node from which the targetdate is inherited.
 * @return the local targetdate parameter value if specified, or MAXTIME_T if
 * a loop conditin was encountered, or the earliest target date returned by
 * recursion to superior Nodes, or MAXTIME_T if there were none.
 */
time_t Node::nested_inherit_targetdate(Node_ptr & origin) {
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
        Node_ptr nested_origin = nullptr;
        time_t sup_targetdate = supnode.nested_inherit_targetdate(nested_origin);
        if (sup_targetdate<earliest) {
            earliest = sup_targetdate;
            origin = nested_origin;
        }
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
 * @param origin Optional storage for pointer to the origin Node that provides the effective
 *               target date. See for example how this is used in `fzupdate`.
 * @return the earliest date and time in time_t format that could be retrieved
 * from the tree of superior Nodes, or MAXTIME_T if none could be found.
 */
time_t Node::inherit_targetdate(Node_ptr * origin) {
    // ***It is technically possible to cache the result found here to speed up
    // ***future calls to this function. To do that safely, calls to set_targetdate()
    // ***would need to invalidate the cache for all dependency Nodes of the Node
    // ***where the targetdate was changed. It is not clear if the time savings
    // ***is worth the added complexity.

    if (!set_all_semaphores(0)) return -2;
    set_semaphore(SEM_TRAVERSED); // not using targetdate of this Node
    time_t earliest = (MAXTIME_T);
    for (auto it = supedges.begin(); it != supedges.end(); ++it) {
        Node & supnode = *((*it)->sup);
        Node_ptr nested_origin = nullptr;
        time_t sup_targetdate = supnode.nested_inherit_targetdate(nested_origin);
        if (sup_targetdate<earliest) {
            earliest = sup_targetdate;
            if (origin) {
                *origin = nested_origin;
            }
        }
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
 * @param origin Optional storage for pointer to the origin Node that provides the effective
 *               target date. See for example how this is used in `fzupdate`. If no specified
 *               target date is found via inheritance then origin (if provided) is set to
 *               this Node by default (effectively local).
 * @return the date and time in time_t seconds format, or -1 if there is no target date
 * to take into account when scheduling this Node.
 */
time_t Node::effective_targetdate(Node_ptr * origin) {
    if (origin) {
        *origin = this; // the default
    }
    if ((tdproperty == td_property::unspecified) || (tdproperty == td_property::inherit))
        return inherit_targetdate(origin);

    if (targetdate >= 0) {
        return targetdate;
    }

    if (tdproperty == td_property::fixed)
        tdproperty = td_property::inherit;
    else
        tdproperty = td_property::unspecified;

    return inherit_targetdate(origin);
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
    text = utf8_safe(utf8str).c_str();
}

/**
 * Copy all the content from one Node object to another.
 * 
 * See for example how this is used in `Graph_modify_add_node()`.
 * 
 * Note: This does NOT include edges, because an Edge is an object at Graph
 *       level, not fully within a Node's scope.
 * 
 * @param from_node A Node object from which to copy all data to this Node.
 */
void Node::copy_content(Node & from_node) {
    set_valuation(from_node.get_valuation());
    set_completion(from_node.get_completion());
    set_required(from_node.get_required());
    set_text_unchecked(from_node.get_text().c_str()); //*(const_cast<std::string *>(&from_node.get_text())));
    set_targetdate(from_node.get_targetdate());
    set_tdproperty(from_node.get_tdproperty());
    set_repeats(from_node.get_repeats());
    set_tdpattern(from_node.get_tdpattern());
    set_tdevery(from_node.get_tdevery());
    set_tdspan(from_node.get_tdspan());
    for (const auto & [topic_id, topic_rel] : from_node.get_topics()) {
        add_topic(topic_id, topic_rel);
    }
}

/**
 * Edit Node data in accordance with reference data in the `from_node`, and
 * edit only the data indicated by the `edit_flags`.
 */
void Node::edit_content(Node & from_node, const Edit_flags & edit_flags) {
    if (edit_flags.Edit_valuation()) {
        set_valuation(from_node.get_valuation());
    }
    if (edit_flags.Edit_completion()) {
        set_completion(from_node.get_completion());
    }
    if (edit_flags.Edit_required()) {
        set_required(from_node.get_required());
    }
    if (edit_flags.Edit_text()) {
        set_text(from_node.get_text().c_str());
    }
    if (edit_flags.Edit_targetdate()) {
        set_targetdate(from_node.get_targetdate());
    }
    if (edit_flags.Edit_tdproperty()) {
        set_tdproperty(from_node.get_tdproperty());
    }
    if (edit_flags.Edit_repeats()) {
        set_repeats(from_node.get_repeats());
    }
    if (edit_flags.Edit_tdpattern()) {
        set_tdpattern(from_node.get_tdpattern());
    }
    if (edit_flags.Edit_tdevery()) {
        set_tdevery(from_node.get_tdevery());
    }
    if (edit_flags.Edit_tdspan()) {
        set_tdspan(from_node.get_tdspan());
    }
    if (edit_flags.Edit_topics()) {
        topics.clear();
        for (const auto & [topic_id, topic_rel] : from_node.get_topics()) {
            add_topic(topic_id, topic_rel);
        }
    }
    // Set the Edit_flags of the modified Node
    editflags.set_Edit_flags(edit_flags.get_Edit_flags());
}

/**
 * Returns a vector of target dates, including those determined by the Node's
 * repeat pattern and span, up to a specified maximum time.
 * 
 * @param t_max The maximum time to include in the vector.
 * @param N_max Maximum size of list to return (zero means no size limit).
 * @param t An optional start time, defaults to a Node's effective target date. See how this is used in Graphinfo.cpp:Nodes_with_repeats_by_targetdate().
 * @return A vector of UNIX epoch times.
 */
std::vector<time_t> Node::repeat_targetdates(time_t t_max, size_t N_max, time_t t) {
    std::vector<time_t> tdwithrepeats;
    if (t == RTt_unspecified) {
        t = effective_targetdate();
    } else if (t > t_max) {
        return tdwithrepeats;
    }

    if (!get_repeats()) {
        tdwithrepeats.emplace_back(t);
        return tdwithrepeats;
    }

    int span = get_tdspan();
    bool unlimited = false;
    if ((span==1) || (span<0)) {
        ADDWARNING(__func__, "Node "+get_id_str()+" has invalid tdspan, treating as non-repeating.");
        span = 1; // yes, we use this value locally
    } else {
        unlimited = span == 0;
    }

    auto pattern = get_tdpattern();
    auto every = get_tdevery();
    do {
        tdwithrepeats.emplace_back(t);
        --span;
    } while (((t = Add_to_Date(t, pattern, every)) <= t_max) && ((span > 0) || unlimited) && ((N_max == 0) || (tdwithrepeats.size()<N_max)));
    return tdwithrepeats;
}

/**
 * Report the main Topic Index-ID of the Node, as indicated by the maximum
 * `Topic_Relevance` value.
 * 
 * If there are no topics then UNKNOWN_TOPIC_ID is returned.
 * 
 * @return Topic_ID of main Topic.
 */
Topic_ID Node::main_topic_id() {
    if (topics.empty()) {
        return UNKNOWN_TOPIC_ID;
    }

    Topic_ID main_id = 0;
    Topic_Relevance max_rel = 0.0;
    for (const auto& [t_id, t_rel] : topics) {
        if (t_rel>max_rel)
            main_id = t_id;
    }
    return main_id;
}

Edge_ID::Edge_ID(Edge_ID_key _idkey): idkey(_idkey), idS_cache("") { //, graphmemman.get_allocator()) {
    std::string formerror;
    if (!valid_Node_ID(idkey.dep.idT,formerror)) throw(ID_exception(formerror));
    if (!valid_Node_ID(idkey.sup.idT,formerror)) throw(ID_exception(formerror));

    idS_cache = (Node_ID_TimeStamp_to_string(idkey.dep.idT)+'>'+Node_ID_TimeStamp_to_string(idkey.sup.idT)).c_str();
}

Edge_ID::Edge_ID(Node &_dep, Node &_sup): idkey(_dep.get_id().key(),_sup.get_id().key()), idS_cache("") { //, graphmemman.get_allocator()) {
    idS_cache = (Node_ID_TimeStamp_to_string(idkey.dep.idT)+'>'+Node_ID_TimeStamp_to_string(idkey.sup.idT)).c_str();
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
Edge::Edge(Graph & graph, std::string id_str): id(id_str.c_str()) {
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

/**
 * Copy all the content from one Edge object to another.
 * 
 * See for example how this is used in `Graph_modify_add_edge()`.
 * 
 * @param from_edge An Edge object from which to copy all data to this Edge.
 */
void Edge::copy_content(Edge & from_edge) {
    set_dependency(from_edge.get_dependency());
    set_significance(from_edge.get_significance());
    set_importance(from_edge.get_importance());
    set_urgency(from_edge.get_urgency());
    set_priority(from_edge.get_priority());
}

/**
 * Add a Node ID key to a Named Node List.
 * 
 * This function takes into account the `features` and `maxsize` specifications.
 * 
 * @param nkey A valid Node ID key.
 * @return True if the Node ID key was added, false if not (e.g. due to unique or maxsize settings).
 */
bool Named_Node_List::add(const Node_ID_key & nkey) {
    while ((maxsize > 0) && (((long) list.size()) >= maxsize)) {
        if (!fifo()) {
            return false; // full, and we can't push anything out
        } else {
            if (prepend()) { // pop the back
                if (unique()) { // remove the Node ID key that will be popped
                    set.erase(list.back());
                }
                list.pop_back();
            } else { // pop the front
                if (unique()) { // remove the Node ID key that will be popped
                    set.erase(list.front());
                }
                list.pop_front();
            }
        }
    }

    bool placed = true;
    if (unique()) {
        std::tie (std::ignore, placed) = set.emplace(nkey);
    }
    if (placed) {
        if (prepend()) {
            list.emplace_front(nkey);
        } else {
            list.emplace_back(nkey);
        }
    }
    return placed;
}

/**
 * Remove a Node ID key to a Named Node List.
 * 
 * This function takes into account the `features` specifications. If the
 * List can contain duplicates then the oldest is removed.
 * 
 * @param nkey A valid Node ID key.
 * @return True if the Node ID key was removed, false if it was not found.
 */
bool Named_Node_List::remove(const Node_ID_key & nkey) {
    if (unique()) {
        if (set.erase(nkey)<1) {
            return false; // nkey was not in the set
        }
    }

    if (prepend()) { // oldest is furthest toward the back
        for (auto n_it = list.rbegin(); n_it != list.rend(); ++n_it) {
            if (*n_it == nkey) {
                list.erase((n_it+1).base()); // see https://www.geeksforgeeks.org/how-to-erase-an-element-from-a-vector-using-erase-and-reverse_iterator/ and https://en.cppreference.com/w/cpp/iterator/reverse_iterator
                return true;
            }
        }
    } else { // oldest is furtherst toward the front
        for (auto n_it = list.begin(); n_it != list.end(); ++n_it) {
            if (*n_it == nkey) {
                list.erase(n_it);
                return true;
            }
        }
    }

    return false;
}

std::vector<std::string> Graph::get_List_names() const {
    std::vector<std::string> names_vec;
    for (const auto & [nls, nnl] : namedlists) {
        names_vec.emplace_back(nls.c_str());
    }
    return names_vec;
}

Named_Node_List_ptr Graph::get_List(const std::string _name) {
    if (_name.empty()) {
        return nullptr;
    }
    Named_List_String namekey(_name);
    auto it = namedlists.find(namekey);
    if (it == namedlists.end()) {
        return nullptr;
    }
    return &(it->second);
}

/**
 * Add the Node ID key of a Node to a Named Node List.
 * 
 * If the Named Node List does not exist then it is created. Only when a Named Node List
 * if first created can features and maxsize be set. Otherwise, the `_features` and
 * `_maxsize` arguments are ignored.
 * 
 * @param _name The name of the Named Node List.
 * @param node An existing Node object.
 * @param _features Optional features flags (default 0).
 * @param _maxsize Optional maximum List size (0 means no limit).
 * @return If successful this returns a pointer to the Named List where the Node was appended.
 */
Named_Node_List_ptr Graph::add_to_List(const std::string _name, const Node & node, int16_t _features, int32_t _maxsize) {
    if (_name.empty()) {
        return nullptr;
    }
    Named_List_String namekey(_name.c_str());
    auto it = namedlists.find(namekey);
    if (it == namedlists.end()) { // new named List
        //Node_ID_key nkey(node.get_id().key());
        auto [n_it, listadded] = namedlists.emplace(_name.c_str(), Named_Node_List{ node.get_id().key(), _features, _maxsize });
        if (!listadded) {
            return nullptr;
        } else {
            return &(n_it->second);
        }
    } else {
        it->second.add(node.get_id().key()); // == add_to_List(it->second, node);
        return &(it->second);
    }
}

/**
 * Copy Node ID keys from one Named Node List to another.
 * 
 * If the target List already exists then adding Nodes from the source List is
 * subject to established `features` and `maxsize` limit.
 * 
 * If the target List is new, then there are 2 possible situations:
 *   1. If the `_features` argument < 0 then copy features and maxsize from
 *      the source List (become a type of List just like the source List).
 *   2. Otherwise, apply the function arguments `_features` and `_maxsize`
 *      to the new target List.
 * 
 * Note: A similar copy function is also available from a target date sorted list
 *       of incomplete Nodes to a List, in the `Graphmodify` library.
 * 
 * @param from_name Name of the source Named Node List.
 * @param to_name Name of the target Named Node List.
 * @param from_max If > 0 then copy at most this many from the source List.
 * @param to_max If > 0 then copy at most until the target List has this size.
 * @param _features Optional features specification for new target List (by default copy from source).
 * @param _maxsize Optional maximum size limitation fro new target List (be default copy from source).
 * @return The number of Node ID keys copied.
 */
size_t Graph::copy_List_to_List(const std::string from_name, const std::string to_name, size_t from_max, size_t to_max, int16_t _features, int32_t _maxsize) {
    Named_Node_List_ptr from_ptr = get_List(from_name);
    if (!from_ptr) {
        return 0;
    }
    if (to_name.empty()) {
        return 0;
    }

    if (from_max == 0) { // make from_max the actual max we might copy
        from_max = from_ptr->list.size();
    }
    Named_Node_List_ptr nnl_ptr = get_List(to_name);
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

    auto source_it = from_ptr->list.begin();
    size_t copied = 0;
    if (!nnl_ptr) { // brand new list
        // initialize the new list and get a pointer to it, adding with that will be faster than many name lookups
        Node * node = Node_by_id(*source_it);
        if (!node) {
            return 0; // oddly, that Node ID was not found
        }
        if (_features>=0) {
            nnl_ptr = add_to_List(to_name, *node, _features, _maxsize);
        } else {
            nnl_ptr = add_to_List(to_name, *node, from_ptr->get_features(), from_ptr->get_maxsize());
        }
        if (!nnl_ptr) {
            return 0; // something went wrong
        }
        ++source_it;
        --from_max;
        ++copied;
    }
    for ( ; source_it != from_ptr->list.end(); ++source_it) {
        Node * node = Node_by_id(*source_it);
        if (!node) {
            return copied; // oddly, that Node ID was not found
        }
        if (add_to_List(*nnl_ptr, *node)) {
            ++copied;
            --from_max;
            if (from_max == 0) {
                return copied;
            }
        }
    }
    return copied;
}

bool Graph::remove_from_List(const std::string _name, const Node_ID_key & nkey) {
    if (_name.empty()) {
        return false;
    }
    Named_List_String namekey(_name.c_str());
    auto it = namedlists.find(namekey);
    if (it == namedlists.end()) {
        return false;
    }
    if (!it->second.remove(nkey)) {
        return false;
    }
    if (it->second.list.empty()) { // delete empty Named Node List
        return (namedlists.erase(namekey)>0);
    }
    return true;
}

bool Graph::delete_List(const std::string _name) {
    if (_name.empty()) {
        return false;
    }
    Named_List_String namekey(_name.c_str());
    return (namedlists.erase(namekey)>0);
}

/**
 * Note that this function will only allow insertion of a Node that was created in
 * the same shared memory segment.
 */
bool Graph::add_Node(Node &node) {
    auto insttype = bi::managed_shared_memory::get_instance_type(&node);
    if ((insttype != bi::named_type) && (insttype != bi::anonymous_type))
        return false; // node is unknown to shared memory management // *** not necessarily same segment!!!

    std::pair<Node_Map::iterator, bool> ret;
    ret = nodes.insert(std::pair<Node_ID_key, Graph_Node_ptr>(node.get_id().key(), &node));
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


Node * Graph::create_Node(std::string id_str) {
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem)
        return nullptr;

    return smem->construct<Node>(bi::anonymous_instance)(id_str);
}

Node * Graph::create_and_add_Node(std::string id_str) {
    Node * nptr = create_Node(id_str);
    if (!nptr)
        return nullptr;
    
    if (!add_Node(nptr)) {
        graphmemman.get_segmem()->destroy_ptr(nptr);
        return nullptr;
    }
    return nptr;
}

bool Graph::add_Edge(Edge &edge) {
    auto insttype = bi::managed_shared_memory::get_instance_type(&edge);
    if ((insttype != bi::named_type) && (insttype != bi::anonymous_type))
        return false; // node is unknown to shared memory management // *** not necessarily same segment!!!

    std::pair<Edge_Map::iterator, bool> ret;
    ret = edges.insert(std::pair<Edge_ID_key, Graph_Edge_ptr>(edge.get_id().key(), &edge));
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

Edge * Graph::create_Edge(Node &_dep, Node &_sup) {
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem)
        return nullptr;
    
    return smem->construct<Edge>(bi::anonymous_instance)(_dep, _sup); // not yet added to Graph
}

Edge * Graph::create_and_add_Edge(std::string id_str) {
    segment_memory_t * smem = graphmemman.get_segmem();
    if (!smem)
        return nullptr;

    return smem->construct<Edge>(bi::anonymous_instance)(*this,id_str); // this does adding to Graph as well
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
        nodeindex.push_back(nodekp.second.get());
    }
    return nodeindex;
}

std::string Graph::find_Topic_Tag_by_id(Topic_ID _id) {
    Topic * topic_ptr = topics.find_by_id(_id);
    if (topic_ptr) {
        return topic_ptr->get_tag().c_str();
    } else {
        return "UNKNOWN";
    }
}

/**
 * Confirm the existence of all Topic IDs in a set.
 * 
 * See for example how fzserverpq uses this to validate all of the Topic-IDs
 * provided for a new Node to be added to the Graph.
 * 
 * @param topicsset A set of Topic-IDs (and relevance values).
 * @return True if they all exist.
 */
bool Graph::topics_exist(const Topics_Set & topicsset) {
    for (const auto & [topicid, topicrel] : topicsset) {
        if (!find_Topic_by_id(topicid))
            return false;
    }
    return true;
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
Topic * main_topic(const Topic_Tags & topictags, const Node & node) {
    Topic * maintopic = nullptr;
    Topic_Relevance max_rel = -99999.0;
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

// +----- begin: element-wise functions operating on Graphtypes -----+

/**
 * Find a Node in a Graph by its ID key from a string.
 * 
 * The Graph is obtained from shared memory (if available) if a valid pointer
 * is not already provided.
 * 
 * Note: If the Graph is found (or already known), but the Node is not found
 *       then the pair returned contains the valid Graph pointer and a nullptr
 *       for the Node.
 * 
 * @param node_idstr A string specifying a Node ID key.
 * @param graph_ptr A pointer to the Graph, if previously identified in shared memory.
 * @return a pair of valid pointers to a Node and to the Graph, or nullptr for Node or Graph not found.
 */
Node_Graph_ptr_pair find_Node_by_idstr(const std::string & node_idstr, Graph * graph_ptr) {
    if (!graph_ptr) {

        graph_ptr = graphmemman.find_Graph_in_shared_memory();
        if (!graph_ptr) {
            std::string errstr("Memory resident Graph not found in shared segment ("+graphmemman.get_active_name()+')');
            ADDERROR(__func__, errstr);
            VERBOSEERR(errstr+'\n');
            return std::make_pair(nullptr, nullptr);
        }

    }

    Node * node_ptr = graph_ptr->Node_by_idstr(node_idstr);
    if (!node_ptr) {
        std::string errstr("Node ["+node_idstr+"] not found in Graph");
        ADDERROR(__func__, errstr);
        VERBOSEERR(errstr+'\n');
        return std::make_pair(nullptr, graph_ptr);
    }

    return std::make_pair(node_ptr, graph_ptr);
}

// +----- end  : element-wise functions operating on Graphtypes -----+

Tag_Label_Real_Value_Vector Topic_tags_of_Node(Graph & graph, Node & node) {
    Tag_Label_Real_Value_Vector res;
    const Topics_Set & tset = node.get_topics();
    for (const auto & [t_id, t_rel] : tset) {
        Topic * topic_ptr = graph.find_Topic_by_id(t_id);
        if (!topic_ptr) {
            ADDERROR(__func__, "Node "+node.get_id_str()+" has unknown Topic with ID "+std::to_string(t_id));
        } else {
            res.emplace_back(std::make_pair(topic_ptr->get_tag().c_str(), t_rel));
        }
    }
    return res;
}

} // namespace fz
