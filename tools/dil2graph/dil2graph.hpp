// Copyright 2020 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the dil2graph tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __DIL2GRAPH_HPP.
 */

#ifndef __DIL2GRAPH_HPP
#include "version.hpp"
#define __DIL2GRAPH_HPP (__VERSION_HPP)

#include "dilaccess.hpp"

using namespace fz;

/// Postgres database name
extern std::string dbname; // provided in dil2graph.cpp, initialized to $USER

struct ConversionMetrics {
    int nullnodes;
    int skippedentries;
    int duplicates;
    int unknownnodeerror;
    int notopics;
    int nulledges;
    int duplicateedges;
    int selfconnections;
    int missingdepnode;
    int missingsupnode;
    int unknownedgeerror;
    int topicsanskeyrel;
    ConversionMetrics() : nullnodes(0), skippedentries(0), duplicates(0), unknownnodeerror(0), notopics(0), nulledges(0), duplicateedges(0), selfconnections(0), missingdepnode(0), missingsupnode(0), unknownedgeerror(0), topicsanskeyrel(0) {}
    int conversion_problems_sum() { // These can be problematic.
      return nullnodes + skippedentries + duplicates + unknownnodeerror + notopics + nulledges + duplicateedges + missingdepnode + missingsupnode + unknownedgeerror;
    }
    int conversion_metrics_sum() { // This is the simple sum of counts, problematic or not.
      return conversion_problems_sum() + selfconnections + topicsanskeyrel;
    }
};

void Exit_Now(int status);

std::string convert_DIL_Topics_file_to_tag(DIL_Topical_List &diltopic);

int convert_DIL_Topics_to_topics(Topic_Tags & topics, Node &node, DIL_entry &entry);

time_t get_Node_Target_Date(DIL_entry &e);

td_property get_Node_tdproperty(DIL_entry &e);

td_pattern get_Node_tdpattern(DIL_entry &e);

Node *convert_DIL_entry_to_Node(DIL_entry &e, Graph &graph, ConversionMetrics &convmet);

Edge * convert_DIL_Superior_to_Edge(DIL_entry &depentry, DIL_entry &supentry, DIL_Superiors &dilsup, Graph & graph, ConversionMetrics &convmet);

std::vector<Topic_Keyword> get_DIL_Topics_File_KeyRels(std::string dilfilepath);

unsigned int collect_topic_keyword_relevance_pairs(Topic * topic);

std::vector<int> detect_DIL_Topics_Symlinks(const std::vector<std::string> &dilfilepaths, int & num);

std::vector<int> detect_DIL_Topics_Symlinks(const std::vector<std::string> &dilfilepaths, int & num);

Graph *convert_DIL_to_Graph(Detailed_Items_List *dil, ConversionMetrics &convmet);

#endif // __DIL2GRAPH_HPP
