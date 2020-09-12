#!/usr/bin/python3
#
# Copyright 2020 Randal A. Koene
# License TBD

"""
This is the Python module for the Formalizer authoritative Graph data structures
as copies directly from Graphtypes.hpp/cpp. For more information about the layout of
the data structure, please see there.

The primary utility of these Python classes is to work in Python with Graph, Node,
Edge and Topics data that is received from or will be sent to Formalizer Data
Servers.

"""

class Node:
    """Node object."""
    id = Node_ID()          ##< unique Node identifier
    topics = Topics_Set()   ##< a map of pairs of unique topic tag index and relevance value
    valuation = 0.0         ##< presently only using values 0.0 and greater (typically [1.0,3.0])
    completion = 0.0        ##< 1.0 = done, -1.0 = obsolete, -2.0 = replaced, -3.0 = done differently, -4.0 = no longer possible / did not come to pass
    required = 0            ##< seconds
    text = ''               ##< by default assumed to contain UTF8 HTML5
    targetdate = -1         ##< when tdproperty=unspecified then targetdate should be set to -1
    tdproperty = 'unspecified' ##<unspecified, inherit, variable, fixed, exact
    repeats = False         ##< must be false if tdproperty is unspecified or variable
    tdpattern = None        ##< can be used to remember an optional periodicity even if isperiodic=false
    tdevery = 1             ##< multiplier for pattern interval
    tdspan = 1              ##< count of number of repetitions
    graph = None            ##< this is set when the Node is added to a Graph
    supedges = Edges_Set()  ##< this set maintained for rapid Edge access to superior Nodes
    depedges = Edges_Set()  ##< this set maintained for rapid Edge access to dependency Nodes

    def __init__(self, _graph):
        if (_graph):
            self.graph = _graph
        

