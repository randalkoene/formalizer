// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This source file was separated out of Graphtypes.cpp in order to produce a separate compiled .obj
 * file for Graph comparison (friend) helper utility functions. Those functions will not be needed by
 * every program that needs the Graph data type header.
 */


//#include <iomanip>
//#include "utfcpp/source/utf8.h" // be careful, looks like it can lead to multiple defines

#include "general.hpp"
#include "Graphtypes.hpp"

namespace fz {

// +----- begin: friend functions -----+
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
        Node * n1 = nm->second.get();
        Node * n2 = graph2.Node_by_id(nm->first);
        trace = traceroot + nm->second->get_id().str();
        if ((!n1) || (!n2)) return false;
        trace = traceroot;
        if (!identical_Nodes(*n1,*n2,trace)) return false;
    }

    traceroot = "G.edges:";
    for (auto em = graph1.edges.begin(); em != graph1.edges.end(); ++em) {
        Edge * e1 = em->second.get();
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
