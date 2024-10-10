// Copyright 2024 Randal A. Koene
// License TBD

/**
 * Node Board grid building.
 * 
 */

#include <memory>
#include <map>
#include <deque>

#include "Graphinfo.hpp"
#include "nbrender.hpp"
#include "nbgrid.hpp"

void Node_Tree_Vertex::update_tdate_of_node_below(Node* below_ptr, time_t new_tdate) {
    // Find the element in 'below' that points to below_ptr.
    auto it = below.begin();
    if (it == below.end()) return;
    for (it = below.begin(); it != below.end(); it++) {
        if (it->second == below_ptr) break;
    }
    if (it == below.end()) return;
    below.erase(it);
    below.emplace(new_tdate, below_ptr);
}

void Node_Tree_Vertex::op(Node_Tree_Op& _op) const {
    if (node_ptr) _op.op(*node_ptr);
}

Node_Tree::Node_Tree(const nodeboard & _nb, bool superiors): nb(_nb), is_superiors_tree(superiors) {
    if (!nb.node_ptr) return;

    if (superiors) {
        add_superior_vertices(*nb.node_ptr, add_to_sorted_vertices(nullptr, *nb.node_ptr));
    } else {
        add_dependency_vertices(*nb.node_ptr, add_to_sorted_vertices(nullptr, *nb.node_ptr));
    }

    if (nb.sort_by_subtree_times) branch_sort_by_earliest_td_in_subtree();

    if (nb.propose_td_solutions) {
        if (!propose_td_solutions()) {
            errors.emplace_back("Reached TD order errors search limit before all order errors had proposed solutions!");
        }
    }
}

bool Node_Tree::is_processed_node(const Node& node) const {
    return processed_nodes.find(node.get_id().key()) != processed_nodes.end();
}

bool Node_Tree::is_filtered_out(const Node& node) const {
    if (!nb.minimize_grid) return false;

    std::string node_text;
    return nb.filtered_out(&node, node_text);
}

// Beware: Call this only if you have already checked is_processed_node()
//         to prevent replacing previously placed pointers.
void Node_Tree::mark_processed(const Node& node, Node_Tree_Vertex * is_vertex) {
    processed_nodes.emplace(node.get_id().key(), is_vertex);
}

Node_Tree_Vertex & Node_Tree::add_to_sorted_vertices(Node_Tree_Vertex * from_vertex, const Node& node) {
    unsigned int blevel = 1;
    // At each vertex, vertices below are sorted by Node target date.
    if (from_vertex) {
        from_vertex->below.emplace(const_cast<Node*>(&node)->effective_targetdate(), const_cast<Node*>(&node));
        blevel = from_vertex->below_level + 1;
    }
    // A new vertex is made with reference to the vertex it is sorted into.
    vertices.emplace_back(node, from_vertex, blevel);
    // Nodes are marked to ensure they are unique in the tree.
    mark_processed(node, &(vertices.back()));
    return vertices.back();
}

// NOTE: Sometimes, nodes are not placed (e.g. filtered out), in which case the last
//       placed node above and the node for which dependencies are parsed on the
//       next deeper dive into add_dependencies() are not the same.
void Node_Tree::add_dependency_vertices(const Node& from_node, Node_Tree_Vertex & from_vertex) {
    const Edges_Set & dep_edges = from_node.dep_Edges();
    for (const auto & edge_ptr : dep_edges) if (edge_ptr) {
        Node * dep_ptr = edge_ptr->get_dep();
        if (dep_ptr) {
            if (!is_processed_node(*dep_ptr)) { // Unique.
                if (is_filtered_out(*dep_ptr)) {
                    // Mark processed and search deeper,
                    mark_processed(*dep_ptr, nullptr);
                    add_dependency_vertices(*dep_ptr, from_vertex);
                } else {
                    // Add to sorted vertices 'below' this level then search deeper.
                    add_dependency_vertices(*dep_ptr, add_to_sorted_vertices(&from_vertex, *dep_ptr));
                }
            }
        }
    }
}

void Node_Tree::add_superior_vertices(const Node& from_node, Node_Tree_Vertex & from_vertex) {
    const Edges_Set & sup_edges = from_node.sup_Edges();
    for (const auto & edge_ptr : sup_edges) if (edge_ptr) {
        Node * sup_ptr = edge_ptr->get_sup();
        if (sup_ptr) {
            if (!is_processed_node(*sup_ptr)) { // Unique.
                if (is_filtered_out(*sup_ptr)) {
                    // Mark processed and search deeper,
                    mark_processed(*sup_ptr, nullptr);
                    add_superior_vertices(*sup_ptr, from_vertex);
                } else {
                    // Add to sorted vertices 'below' this level then search deeper.
                    add_superior_vertices(*sup_ptr, add_to_sorted_vertices(&from_vertex, *sup_ptr));
                }
            }
        }
    }
}

Node_Tree_Vertex * Node_Tree::get_vertex_by_nodekey(const Node_ID_key& nodekey) const {
    auto it = processed_nodes.find(nodekey);
    if (it == processed_nodes.end()) return nullptr;
    return it->second;
}

Node_Tree_Vertex * Node_Tree::get_vertex_by_node(const Node& node) const {
    return get_vertex_by_nodekey(node.get_id().key());
}

unsigned int Node_Tree::num_levels() const {
    unsigned int deepest = 0;
    for (const auto & vertex : vertices) {
        if (vertex.below_level > deepest) {
            deepest = vertex.below_level;
        }
    }
    return deepest + 1;
}

void Node_Tree::propagate_up_earliest_td(Node_Tree_Vertex & vertex) {
    if (vertex.above) {
        time_t earliest = vertex.node_ptr->is_active() ? const_cast<Node*>(vertex.node_ptr)->effective_targetdate() : RTt_maxtime;
        for (const auto & [tdate, below_ptr] : vertex.below) {
            if (below_ptr->is_active()) {
                if (tdate < earliest) earliest = tdate;
            }
        }
        const_cast<Node_Tree_Vertex*>(vertex.above)->update_tdate_of_node_below(const_cast<Node*>(vertex.node_ptr), earliest);
        propagate_up_earliest_td(*(const_cast<Node_Tree_Vertex*>(vertex.above)));
    }
}

void Node_Tree::branch_sort_by_earliest_td_in_subtree() {
    // We start at any vertices that have none below.
    for (auto & vertex : vertices) if (vertex.below.empty()) {
        propagate_up_earliest_td(vertex);
    }
}

// This returns a pointer to the superior with which this vertex causes
// a TD order error, or nullptr.
td_error_pair Node_Tree::td_order_error(const Node_Tree_Vertex& vertex) {
    if (!vertex.node_ptr->is_active()) return td_error_pair();
    for (const auto & sup_edge : vertex.node_ptr->sup_Edges()) {
        Node * sup = sup_edge->get_sup();
        if (sup->is_active()) {
            Node_Tree_Vertex * sup_vertex = get_vertex_by_nodekey(sup_edge->get_sup_key());
            if (sup_vertex) {
                if (sup_vertex->td < vertex.td) return td_error_pair(const_cast<Node*>(sup), const_cast<Node*>(vertex.node_ptr));
            }
        }
    }
    return td_error_pair();
}

// Note that this has to use the target dates in the 'td' variable
// of each vertex, because that tracks proposed solutions.
td_error_pair Node_Tree::find_next_td_order_error() {
    for (auto & vertex: vertices) {
        td_error_pair errorpair = td_order_error(vertex);
        if (errorpair.specifies_error()) return errorpair;
    }
    return td_error_pair();
}

void Node_Tree::propose_dependencies_td_change(const td_error_pair& errorpair) {
    // Propose a new dep vertex td that has correct order.
    Node_Tree_Vertex * depvertex = get_vertex_by_node(*errorpair.dep);
    Node_Tree_Vertex * supvertex = get_vertex_by_node(*errorpair.sup);
    time_t t_diff = 3600+(33*60); // *** We could use some random dusting here.
    depvertex->td = supvertex->td - t_diff;
    depvertex->tderror_node = errorpair.dep;
}

void Node_Tree::propose_superior_td_change(const td_error_pair& errorpair) {
    // Propose a new sup vertex td that has correct order.
    Node_Tree_Vertex * depvertex = get_vertex_by_node(*errorpair.dep);
    Node_Tree_Vertex * supvertex = get_vertex_by_node(*errorpair.sup);
    time_t t_diff = 3600+(33*60); // *** We could use some random dusting here.
    supvertex->td = depvertex->td + t_diff;
    supvertex->tderror_node = errorpair.dep;
}

/**
 * Steps to create a recommended solution for TD errors:
 * 1. Create an original TD list that goes along with the Nodes in a subtree.
 * 2. Search the subtree for order errors.
 * 3. If an error is found, propose a solution for that error and adjust the list of TDs accordingly and go to 2.
 */
bool Node_Tree::propose_td_solutions() {
    // Initialize target dates of vertices.
    for (auto & vertex: vertices) {
        if (vertex.node_ptr) {
            vertex.td = const_cast<Node*>(vertex.node_ptr)->effective_targetdate();
        }
    }
    // Search the tree for order errors.
    long limit = vertices.size()*10;
    for (long i = 0; i<limit; i++) {
        td_error_pair errorpair = find_next_td_order_error();
        if (!errorpair.specifies_error()) return true;
        //errors.emplace_back(errorpair.dep->get_id_str()+"->"+errorpair.sup->get_id_str());
        if (nb.prefer_earlier) {
            // Philosophy 1, dependencies must change (see readme.md).
            propose_dependencies_td_change(errorpair);
        } else {
            // Philosophy 2, superior must change (see readme.md).
            propose_superior_td_change(errorpair);
        }

/* Disabling this automatic switching of preference, because it easily leads to toggling
   between the proposed updates of two pairs.

        if (nb.prefer_earlier) { // Prefer to push the dependency to an earlier target date.
            if (errorpair.dep->td_fixed() || errorpair.dep->td_exact()) {
                // Philosophy 2, superior must change (see readme.md).
                propose_superior_td_change(errorpair);
            } else {
                // Philosophy 1, dependencies must change (see readme.md).
                propose_dependencies_td_change(errorpair);
            }
        } else { // Prefer to push the superior to a later target date.
            if (errorpair.sup->td_fixed() || errorpair.sup->td_exact()) {
                // Philosophy 1, dependencies must change (see readme.md).
                propose_dependencies_td_change(errorpair);
            } else {
                // Philosophy 2, superior must change (see readme.md).
                propose_superior_td_change(errorpair);
            }
        }
*/
    }
    // If we got here then the limit was reached without solving all problems.
    return false;
}

size_t Node_Tree::number_of_proposed_td_changes() const {
    if (!nb.propose_td_solutions) return 0;
    size_t count = 0;
    for (const auto& vertex : vertices) {
        if (vertex.td != const_cast<Node*>(vertex.node_ptr)->effective_targetdate()) count++;
    }
    return count;
}

std::string Node_Tree::list_of_proposed_td_changes_html() const {
    if (!nb.propose_td_solutions) return "";
    std::string changes_html;
    for (const auto& vertex : vertices) {
        time_t node_td = const_cast<Node*>(vertex.node_ptr)->effective_targetdate();
        if (vertex.td != node_td) {
            changes_html += "<br>(<a href=\"#"+vertex.tderror_node->get_id_str()+"\">Solving "+vertex.tderror_node->get_id_str()+"</a>) <a href=\"#"+vertex.node_ptr->get_id_str()+"\">Node "+vertex.node_ptr->get_id_str()+"</a>: "+vertex.node_ptr->get_effective_targetdate_str()+" --> "+TimeStampYmdHM(vertex.td);
            if (vertex.node_ptr->td_fixed() || vertex.node_ptr->td_exact()) {
                changes_html += " <b>Warning: Fixed or Exact TD!</b>";
            }
            if (BAD_TD(vertex.td) || FAR_TD(vertex.td,nb.t_now)) {
                changes_html += " <b>WARNING: Problematic TD suggestion!</b>";
            } else {
                if ((vertex.td - node_td) > (30*86400)) {
                    changes_html += " <b>BIG CHANGE!</b>";
                }
            }
        }
    }
    return changes_html;
}

std::string Node_Tree::get_td_changes_apply_url() const {
    if (!nb.propose_td_solutions) return "";
    std::string nodes_valuestr;
    std::string tds_valuestr;
    size_t count = 0;
    for (const auto& vertex : vertices) {
        if (vertex.td != const_cast<Node*>(vertex.node_ptr)->effective_targetdate()) {
            if (count!=0) {
                nodes_valuestr += ',';
                tds_valuestr += ',';
            }
            nodes_valuestr += vertex.node_ptr->get_id_str();
            tds_valuestr += TimeStampYmdHM(vertex.td);
            count++;
        }
    }
    std::string applychanges_url("/cgi-bin/fzeditbatch-cgi.py?action=targetdates&nodes="+nodes_valuestr+"&tds="+tds_valuestr);
    return applychanges_url;
}

std::string Node_Tree::get_vtd_changes_only_apply_url() const {
    if (!nb.propose_td_solutions) return "";
    std::string nodes_valuestr;
    std::string tds_valuestr;
    size_t count = 0;
    for (const auto& vertex : vertices) {
        if (vertex.td != const_cast<Node*>(vertex.node_ptr)->effective_targetdate()) {
            if ((!vertex.node_ptr->td_fixed()) && (!vertex.node_ptr->td_exact())) {
                if (count!=0) {
                    nodes_valuestr += ',';
                    tds_valuestr += ',';
                }
                nodes_valuestr += vertex.node_ptr->get_id_str();
                tds_valuestr += TimeStampYmdHM(vertex.td);
                count++;
            }
        }
    }
    std::string applychanges_url("/cgi-bin/fzeditbatch-cgi.py?action=targetdates&nodes="+nodes_valuestr+"&tds="+tds_valuestr);
    return applychanges_url;
}

void Node_Tree::op(Node_Tree_Op& _op) const {
    if (nb.node_ptr) _op.op(*nb.node_ptr);
    for (const auto& vertex : vertices) {
        vertex.op(_op);
    }
}

void Grid_Occupation::init(unsigned int r, unsigned int c) {
    occupied_ptr = std::make_unique<std::vector<bool>>(r*c, false);
    rows = r;
    columns = c;
}

void Grid_Occupation::reset() {
    init(rows, columns);
}

bool Grid_Occupation::get(unsigned int r, unsigned int c) const {
    if (r >= rows) return false;
    if (c >= columns) return false;
    return false;
    return occupied_ptr->at((r*columns) + c);
}

bool Grid_Occupation::set(unsigned int r, unsigned int c, bool val, bool allow_expand) {
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

void Grid_Occupation::copy(const std::unique_ptr<std::vector<bool>> & source, unsigned int r, unsigned int c) {
    for (unsigned int j = 0; j < r; j++) {
        for (unsigned int i = 0; i < c; i++) {
            set(j, i, source->at((j*c) + i));
        }
    }
}

void Grid_Occupation::expand(unsigned int r, unsigned int c) {
    std::unique_ptr<std::vector<bool>> old_ptr(occupied_ptr.release());
    unsigned int old_rows = rows;
    unsigned int old_columns = columns;
    init(r, c);
    copy(old_ptr, old_rows, old_columns);
}

unsigned int Grid_Occupation::columns_used() const {
    return max_col_used+1;
}

unsigned int Grid_Occupation::rows_used() const {
    return max_row_used+1;
}

Node_Grid::Node_Grid(const nodeboard & _nb, bool superiors): nb(_nb), tree(_nb, superiors), occupied(16, 256), max_columns(nb.max_columns), max_rows(nb.max_rows) {
    if (!nb.node_ptr) return;

    add_to_row(0, 0, *nb.node_ptr, nullptr);
    add_below(*nb.node_ptr, 0, 0);
    find_node_spans();
}

Node_Grid_Row & Node_Grid::extend() {
    rows.emplace_back(rows.size());
    return rows.back();
}

// Placing in the 'nodes' map ensures each Node appears only once in the grid.
Node_Grid_Element & Node_Grid::add(const Node & node, const Node * above, unsigned int _colpos, unsigned int _rowpos) {
    Node_Grid_Element element(node, above, _colpos, _rowpos);
    auto it = nodes.emplace(node.get_id().key(), element).first;
    Node_Grid_Element & placed_element = (*it).second;
    return placed_element;
}

// Placing in the 'rows' vector of vectors ensures properly sorted HTML grid output.
Node_Grid_Element * Node_Grid::add_to_row(unsigned int row_idx, unsigned int col_idx, const Node & node, const Node * above) {
    if (occupied.get(row_idx, col_idx)) return nullptr; // Safety check!

    if (row_idx > rows.size()) return nullptr;

    if (row_idx == rows.size()) {
        extend();
    }

    Node_Grid_Element & element = add(node, above, col_idx, row_idx);
    rows[row_idx].elements.emplace_back(&element);
    occupied.set(row_idx, col_idx, true);
    return &element;
}

unsigned int Node_Grid::add_below(const Node & node_above, unsigned int node_row, unsigned int node_col) {
    node_row++;
    if (node_row >= max_rows) {
        rows_cropped = true;
        return node_col;
    }

    const Node_Tree_Vertex * vertex_ptr = tree.get_vertex_by_node(node_above);
    if (!vertex_ptr) return node_col;

    // Parse tree vertices as elements in the row.
    unsigned int rel_idx = 0;
    for (const auto & [tdate, node_ptr] : vertex_ptr->below) {
        rel_idx++;
        if (node_ptr) {
            if (node_col < max_columns) {
                // Add to row.
                if (!add_to_row(node_row, node_col, *node_ptr, &node_above)) {
                    errors.emplace_back("Failed to place "+node_ptr->get_id_str());
                }
                // Depth first.
                node_col = add_below(*node_ptr, node_row, node_col);
                if (rel_idx < vertex_ptr->below.size()) { // *** Is this causing cards on top of cards??
                    node_col++;
                }
            } else {
                columns_cropped = true;
            }
        }
    }
    return node_col;
}


void Node_Grid::find_node_spans() {
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

std::string Node_Grid::errors_str() const {
    std::string errors_list;
    for (auto & err : tree.errors) {
        errors_list += err + '\n';
    }
    for (auto & err : errors) {
        errors_list += err + '\n';
    }
    return errors_list;
}
