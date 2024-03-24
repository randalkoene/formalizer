// Copyright 2024 Randal A. Koene
// License TBD

/**
 * Node Board grid building.
 * 
 */

#include <memory>
#include <vector>
#include <map>

//#include "Graphinfo.hpp"
#include "nbrender.hpp"

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

struct Grid_Occupation {
    unsigned int rows;
    unsigned int columns;
    std::unique_ptr<std::vector<bool>> occupied_ptr;
    unsigned int max_col_used = 0;
    unsigned int max_row_used = 0;
    Grid_Occupation(unsigned int r, unsigned int c) {
        init(r, c);
    }
    void init(unsigned int r, unsigned int c) {
        occupied_ptr = std::make_unique<std::vector<bool>>(r*c, false);
        rows = r;
        columns = c;
    }
    void reset() {
        init(rows, columns);
    }
    bool get(unsigned int r, unsigned int c) const {
        if (r >= rows) return false;
        if (c >= columns) return false;
        return false;
        return occupied_ptr->at((r*columns) + c);
    }
    bool set(unsigned int r, unsigned int c, bool val, bool allow_expand = true) {
        if (r >= rows) {
            if (!allow_expand) return false;
            if (c >= columns) { // Expand both rows and columns
                expand(rows*2, columns*2);
                return set(r, c, val, allow_expand);
            } else { // Expand rows
                expand(rows*2, columns);
                return set(r, c, val, allow_expand);
            }
        } else if (c >= columns) { // Possibly expand columns
            if (!allow_expand) return false;
            expand(rows, columns*2);
            return set(r, c, val, allow_expand);
        }
        occupied_ptr->at((r*columns) + c) = val;
        if (r > max_row_used) max_row_used = r;
        if (c > max_col_used) max_col_used = c;
        return true;
    }
    void copy(const std::unique_ptr<std::vector<bool>> & source, unsigned int r, unsigned int c) {
        for (unsigned int j = 0; j < r; j++) {
            for (unsigned int i = 0; i < c; i++) {
                set(j, i, source->at((j*c) + i));
            }
        }
    }
    void expand(unsigned int r, unsigned int c) {
        std::unique_ptr<std::vector<bool>> old_ptr(occupied_ptr.release());
        unsigned int old_rows = rows;
        unsigned int old_columns = columns;
        init(r, c);
        copy(old_ptr, old_rows, old_columns);
    }
    unsigned int columns_used() const {
        return max_col_used+1;
    }
    unsigned int rows_used() const {
        return max_row_used+1;
    }
};

struct Node_Grid {
    const nodeboard & nb;

    std::map<Node_ID_key, Node_Grid_Element> nodes;
    std::vector<Node_Grid_Row> rows;
    Grid_Occupation occupied;

    unsigned int max_columns = 0;
    unsigned int max_rows = 0;
    bool columns_cropped = false;
    bool rows_cropped = false;

    std::vector<std::string> errors;

    Node_Grid(const nodeboard & _nb, bool superiors = false): nb(_nb), occupied(16, 256), max_columns(nb.max_columns), max_rows(nb.max_rows) {
        if (!nb.node_ptr) return;

        extend();
        add_to_row(0, 0, *nb.node_ptr, nullptr);

        if (superiors) {
            add_superiors(*nb.node_ptr, *nb.node_ptr, 0, 0);
        } else {
            add_dependencies(*nb.node_ptr, *nb.node_ptr, 0, 0);
        }
        find_node_spans(rows.size()-1);
    }

    //unsigned int num_elements() { return nodes.size(); }

    bool is_in_grid_or_seen_before(const Node & node) const {
        return nodes.find(node.get_id().key()) != nodes.end();
    }

    bool is_filtered_out(const Node & node) const {
        if (!nb.minimize_grid) return false;

        std::string node_text;
        return nb.filtered_out(&node, node_text);
    }

    // Placing in the 'nodes' map ensures each Node appears only once in the grid.
    Node_Grid_Element & add(const Node & node, const Node * above, unsigned int _colpos, unsigned int _rowpos) {
        Node_Grid_Element element(node, above, _colpos, _rowpos);
        auto it = nodes.emplace(node.get_id().key(), element).first;
        Node_Grid_Element & placed_element = (*it).second;
        return placed_element;
    }

    // Placing in the 'rows' vector of vectors ensures properly sorted HTML grid output.
    Node_Grid_Element * add_to_row(unsigned int row_idx, unsigned int col_idx, const Node & node, const Node * above) {
        if (occupied.get(row_idx, col_idx)) return nullptr; // Safety check!

        if (row_idx >= rows.size()) return nullptr;

        Node_Grid_Element & element = add(node, above, col_idx, row_idx);
        rows[row_idx].elements.emplace_back(&element);
        occupied.set(row_idx, col_idx, true);
        return &element;
    }

    void mark_seen(const Node & node) {
        Node_Grid_Element element(node, nullptr, 0, 0);
        nodes.emplace(node.get_id().key(), element);
    }

    Node_Grid_Row & extend() {
        rows.emplace_back(rows.size());
        return rows.back();
    }

    // NOTE: Sometimes, nodes are not placed (e.g. filtered out), in which case the last
    //       placed node above and the node for which dependencies are parsed on the
    //       next deeper dive into add_dependencies() are not the same.
    unsigned int add_dependencies(const Node & node, const Node & placed_above, unsigned int node_row, unsigned int node_col) {
        unsigned int dep_row = node_row + 1;
        if (dep_row >= max_rows) {
            rows_cropped = true;
            return node_col;
        }
        if (dep_row >= rows.size()) {
            extend();
        }

        // Collect node dependencies as elements in the row.
        unsigned int rel_dep_idx = 0;
        const Edges_Set & dep_edges = node.dep_Edges();
        for (const auto & edge_ptr : dep_edges) {
            if (edge_ptr) {
                Node * dep_ptr = edge_ptr->get_dep();
                if (dep_ptr) {
                    bool placed = false;
                    if (!is_in_grid_or_seen_before(*dep_ptr)) { // Unique and avoiding infinite self-recursion.
                        if (node_col < max_columns) {
                            placed = !is_filtered_out(*dep_ptr);
                            if (!placed) { // if minimize_grid leave no vacancy
                                mark_seen(*dep_ptr);
                                node_col = add_dependencies(*dep_ptr, placed_above, dep_row-1, node_col); // *** Should we do dep_row-1??
                            } else {
                                // Add to row.
                                if (!add_to_row(dep_row, node_col, *dep_ptr, &placed_above)) {
                                    errors.emplace_back("Failed to place "+dep_ptr->get_id_str());
                                }
                                // Depth first search.
                                node_col = add_dependencies(*dep_ptr, *dep_ptr, dep_row, node_col);
                            }
                        } else {
                            columns_cropped = true;
                        }
                    }
                    rel_dep_idx++;
                    if (rel_dep_idx < dep_edges.size()) {
                        if (placed) node_col++;
                    }
                }
            }
        }
        return node_col;
    }

    unsigned int add_superiors(const Node & node, const Node & placed_above, unsigned int node_row, unsigned int node_col) {
        unsigned int sup_row = node_row + 1;
        if (sup_row >= max_rows) {
            rows_cropped = true;
            return node_col;
        }
        if (sup_row >= rows.size()) {
            extend();
        }

        // Collect node superiors as elements in the row.
        unsigned int rel_sup_idx = 0;
        const Edges_Set & sup_edges = node.sup_Edges();
        for (const auto & edge_ptr : sup_edges) {
            if (edge_ptr) {
                Node * sup_ptr = edge_ptr->get_sup();
                if (sup_ptr) {
                    bool placed = false;
                    if (!is_in_grid_or_seen_before(*sup_ptr)) { // Unique.
                        if (node_col < max_columns) {
                            placed = !is_filtered_out(*sup_ptr);
                            if (!placed) { // if minimize_grid leave no vacancy
                                mark_seen(*sup_ptr);
                                node_col = add_superiors(*sup_ptr, placed_above, sup_row-1, node_col);
                            } else {
                                // Add to row.
                                if (!add_to_row(sup_row, node_col, *sup_ptr, &node)) {
                                    errors.emplace_back("Failed to place "+sup_ptr->get_id_str());
                                }
                                // Depth first search.
                                node_col = add_superiors(*sup_ptr, *sup_ptr, sup_row, node_col);
                            }
                        } else {
                            columns_cropped = true;
                        }
                    }
                    rel_sup_idx++;
                    if (rel_sup_idx < sup_edges.size()) {
                        if (placed) node_col++;
                    }
                }
            }
        }
        return node_col;
    }

    void find_node_spans(unsigned int row_idx) {
        for (auto & [key, element] : nodes) {
            if (element.above) {
                auto it_above = nodes.find(element.above->get_id().key()); // Find by key, because the map gets rebuilt when more space is needed during element adding.
                if (it_above != nodes.end()) {
                    Node_Grid_Element & above = it_above->second;
                    if ((above.col_pos+(above.span - 1)) < element.col_pos) {
                        above.span = (element.col_pos - above.col_pos) + 1;
                        //FZOUT("col_pos="+std::to_string(above.col_pos)+" span="+std::to_string(above.span)+'\n'); std::cout.flush();
                    }
                }
            }
        }
    }

    std::string errors_str() const {
        std::string errors_list;
        for (auto & err : errors) {
            errors_list += err + '\n';
        }
        return errors_list;
    }

};
