// Copyright 2024 Randal A. Koene
// License TBD

/**
 * This header file is used for declarations specific to the nbgrid part of the
 * nodeboard tool.
 * 
 * Versioning is based on https://semver.org/ and the C++ header defines __NBRENDER_HPP.
 */

#ifndef __NBGRID_HPP
#include "version.hpp"
#define __NBGRID_HPP (__VERSION_HPP)

struct td_error_pair {
    Node * sup = nullptr;
    Node * dep = nullptr;
    td_error_pair() {}
    td_error_pair(Node* _sup, Node* _dep): sup(_sup), dep(_dep) {}
    bool specifies_error() const { return (sup != nullptr) && (dep != nullptr); }
};

/**
 * Minimal information needed to construct the visualization tree.
 */
struct Node_Tree_Vertex {
    const Node_Tree_Vertex * above;
    const Node * node_ptr;  // Node at this vertex.
    targetdate_sorted_Nodes below; // Target date sorted list of vertices below.
    unsigned int below_level;

    // *** Used in td order solving test:
    time_t td; // If different than effective target date then it proposes a solution to a td order error.
    Node * tderror_node; // If a td order error solution is proposed then this indicates the Node with the problem for which it is proposed.

    Node_Tree_Vertex(const Node& node, const Node_Tree_Vertex * _above, unsigned int _blevel): above(_above), node_ptr(&node), below_level(_blevel) {}

    void update_tdate_of_node_below(Node* below_ptr, time_t new_tdate);
};

/**
 * Prepares a tree of Nodes that can subsequently be placed
 * in a Node_Grid.
 */
struct Node_Tree {
    const nodeboard & nb;
    bool is_superiors_tree = false;

    std::deque<Node_Tree_Vertex> vertices;
    std::map<Node_ID_key, Node_Tree_Vertex*> processed_nodes;

    std::vector<std::string> errors;

    Node_Tree(const nodeboard & _nb, bool superiors = false);

    bool is_processed_node(const Node& node) const;

    bool is_filtered_out(const Node& node) const;

    // Beware: Call this only if you have already checked processed_node()
    //         to prevent replacing previously placed pointers.
    void mark_processed(const Node& node, Node_Tree_Vertex * is_vertex);

    Node_Tree_Vertex & add_to_sorted_vertices(Node_Tree_Vertex * from_vertex, const Node& node);

    // NOTE: Sometimes, nodes are not placed (e.g. filtered out), in which case the last
    //       placed node above and the node for which dependencies are parsed on the
    //       next deeper dive into add_dependencies() are not the same.
    void add_dependency_vertices(const Node& from_node, Node_Tree_Vertex & from_vertex);

    void add_superior_vertices(const Node& from_node, Node_Tree_Vertex & from_vertex);

    Node_Tree_Vertex * get_vertex_by_nodekey(const Node_ID_key& nodekey) const;

    Node_Tree_Vertex * get_vertex_by_node(const Node& node) const;

    unsigned int num_levels() const;

	void propagate_up_earliest_td(Node_Tree_Vertex & vertex);

	void branch_sort_by_earliest_td_in_subtree();

    td_error_pair td_order_error(const Node_Tree_Vertex& vertex);

    td_error_pair find_next_td_order_error();

    void propose_dependencies_td_change(const td_error_pair& errorpair);

    void propose_superior_td_change(const td_error_pair& errorpair);

    bool propose_td_solutions();

    size_t number_of_proposed_td_changes() const;

    std::string list_of_proposed_td_changes_html() const;

    std::string get_td_changes_apply_url() const;

};

struct Grid_Occupation {
    unsigned int rows;
    unsigned int columns;
    std::unique_ptr<std::vector<bool>> occupied_ptr;
    unsigned int max_col_used = 0;
    unsigned int max_row_used = 0;
    Grid_Occupation(unsigned int r, unsigned int c) {
        init(r, c);
    }
    void init(unsigned int r, unsigned int c);
    void reset();
    bool get(unsigned int r, unsigned int c) const;
    bool set(unsigned int r, unsigned int c, bool val, bool allow_expand = true);
    void copy(const std::unique_ptr<std::vector<bool>> & source, unsigned int r, unsigned int c);
    void expand(unsigned int r, unsigned int c);
    unsigned int columns_used() const;
    unsigned int rows_used() const;
};

struct Node_Grid_Element {
    unsigned int col_pos = 0;
    unsigned int row_pos = 0;
    unsigned int span = 1;
    const Node * node_ptr;
    const Node * above;

    Node_Grid_Element(const Node & node, const Node * _above, unsigned int _colpos, unsigned int _rowpos): col_pos(_colpos), row_pos(_rowpos), node_ptr(&node), above(_above) {}
};

struct Node_Grid_Row {
    unsigned int legit_row;
    unsigned int row_number;
    // The following list is used to collect references to elements that
    // are in this row, so that we can generate the HTML output with grid
    // elements appearing in sensible order (which makes discovering the
    // cause of rendering errors much easier.
    std::vector<Node_Grid_Element*> elements;

    Node_Grid_Row(unsigned int _rownumber): legit_row(12345), row_number(_rownumber) {}
    bool is_legit_row() const { return legit_row == 12345; }
};


/**
 * Builds a grid of Node elements that can be displayed as a grid
 * on a nodeboard.
 * 
 * Elements are preprocessed into a tree using the Node_Tree class.
 */
class Node_Grid {
protected:
    const nodeboard & nb;

    Node_Tree tree;

public:
    std::map<Node_ID_key, Node_Grid_Element> nodes;
    std::vector<Node_Grid_Row> rows;
    Grid_Occupation occupied;

public:
    unsigned int max_columns = 0;
    unsigned int max_rows = 0;
    bool columns_cropped = false;
    bool rows_cropped = false;

    std::vector<std::string> errors;

public:
    Node_Grid(const nodeboard & _nb, bool superiors = false);

protected:
    Node_Grid_Row & extend();

    // Placing in the 'nodes' map ensures each Node appears only once in the grid.
    Node_Grid_Element & add(const Node & node, const Node * above, unsigned int _colpos, unsigned int _rowpos);

    // Placing in the 'rows' vector of vectors ensures properly sorted HTML grid output.
    Node_Grid_Element * add_to_row(unsigned int row_idx, unsigned int col_idx, const Node & node, const Node * above);

    unsigned int add_below(const Node & node_above, unsigned int node_row, unsigned int node_col);

    void find_node_spans();

public:
    std::string errors_str() const;

    size_t number_of_proposed_td_changes() const { return tree.number_of_proposed_td_changes(); }

    std::string list_of_proposed_td_changes_html() const { return tree.list_of_proposed_td_changes_html(); }

    std::string get_td_changes_apply_url() const { return tree.get_td_changes_apply_url(); }

};


#endif // __NBGRID_HPP
