# fzgraph - Modify the essential components of the Graph.

This includes adding Nodes, adding Edges, etc.



## About *removing* (deleting) Nodes

Removing a Node should normally not be possible, except by following an undo-chain back or by reverting to a backup of the database.

When a Node has accumulated History in the form of Log entries, attempting to remove the Node would be akin to attempting to undo or selectively erase history. That should not be done, even though it is entirely possible to mark a Node *complete*, or *obsolete*, or *replaced*, or *done differently*, or *no longer possible*. It is also possible to change a Node's description.

### Undoing Node creation without detrimental consequences

There is technically a window of opportunity, before the first Log entry has been made for a Node, where removal of that Node could be accomplished without detrimental consequences. This possibility may even be desirable for those situations where a Node was hastily, almost accidentally, created, and where one wishes to undo that mistake. This belongs to the implementation of **undo**.

### Node removal for reasons of technical mistakes during the transition phase

There is one other case: Following the first, still rather experimental, instances of Node creation during the transition to the Formalizer 2.x environment, there may be a need to remove Nodes that were incorrectly created. Methods for this are:

1. Restore a backup of the database.
2. Use `psql` to manually remove corresponding table entries.
3. Add an actual, limited-use, Node removal option to `fzgraph`.

The third method may be worthwhile if experience shows it to be so. Unless that happens, it is likely that the work involved would consume more time than the time spent correcting Node creation mistakes by other means.

### Background information about how an Edge is added

One way to create and Edge is to call fzgraph as follows:

`fzgraph -M edges -S <superior-id> -D <dependency-id>`

Within `fzgraph`, this leads to the function `addedge.cpp:make_edges()`,
which does:

1. Read the superiors and dependencies lists.
2. Create a shared memory chunk of the necessary size.
3. For each superior-dependency pair, call `add_Edge_request()` to
   prepare a request in the shared memory area.
4. Call `server_request_with_shared_data()` to give the request to
   the active `fzserverpq` instance via TCP.

In `fzserverpq`, once the Graph has been loaded and remains resident,
the `tcpserver.cpp:server_socket_listen()` function enters a loop where
it listens for incoming TCP requests. The incoming request is not
preceded by GET, PATCH, FZ, STOP or PING, so it is interpreted as
a request through shared memory. That is handled by the member function
`fzserverpq::handle_request_with_data_share()`:

1. Calls `shm_server_handlers.cpp:handle_request_stack()` with the
   shared memory segment.
2. Sends a TCP response.
3. Removes the reference to the shared memory segment.

In `handle_request_stack()`:

1. Find the Graph modifications that were provided in the shared
   memory segment.
2. Check if the request stack is valid.
3. Prepare a part of the shared memory segment for responses.
4. For each modification request, make one of many possible calls to
   carry out Graph modifications in memory. In this case, a call is
   made to `Graph_modify_add_edge()`.
5. Call `handle_Graph_modifications_pq()` to carry out modifications
   in the stored database. These modifications are obtained from a
   results object that was prepared while carrying out in-memory
   modifications.

In `Graphmodify.cpp:Graph_modify_add_edge()`:

1. A shared memory segment is activated to access requested Edge data,
   including superior and dependency.
2. The `Graph::create_and_add_Edge()` function is called to modify the
   graph by adding the new edge.
3. Once the new Edge object has been created in the shared-memory
   Graph, its `copy_content()` member function is called to obtain its
   parameter settings from the data that was supplied.

It is interesting to note how this differs from modification requests
made directly to fzserverpq through the FZ TCP API. An FZ TCP request
requires quite a bit of routing. First, it leads to
`fzserverpq::handle_special_purpose_request()`. That leads to
one of:

- `handle_serialized_data_request()`
- `handle_fz_vfs_request()`
- `direct_tcpport_api_file_serving()`

Our Graph request goes to `handle_fz_vfs_request()`, which uses virtual
paths as a way to specify actions. This function can make a number of
calls to short-cut commands, or lead to database or graph requests. Ours
is a graph request via `handle_fz_vfs_graph_request()`, which can handle:

- Log time requests via `node_add_logged_time()`.
- Nodes requests via `handle_node_direct_request()`.
- NNL requests vai `handle_named_list_direct_request()`.

Ours is a Nodes request, and `handle_node_direct_request()` can handle:

- A show request via `handle_node_direct_show()`. These will make it possible
  to receive output in HTML and other formats.
- An edit or modification request via `handle_node_direct_edit_multiple_pars()`.
  These requests are Formalizer System actions that affect multiple parameters
  of a Node, such as completing a Node, skipping an instance of a repeated
  Node, or possibly skipping multiple instances.
- A parameter request via `handle_node_direct_parameter()`. Such a request
  addresses a specific parameter of a Node and can include requests that act
  on a specific superior or dependency (i.e. can also affect an Edge).

Ours is a request to add a specific superior, so it leads to the function
`handle_node_direct_parameter()`. This function first looks for actions on
the Node (which contain a '?' for parameter values), then show actions
(which contain a '.'), and for actions that refer to further branches of
information, such as "topics", "superiors", or "dependencies". Our request
contains a "/superiors/" token as part of the route. Via a map, this leads
to `handle_node_superiors_edit()`, which recognizes the actions:

- `add`
- `remove`
- `addlist`

Ours is an "/add?" call, so that leads to
`tcp_server_handlers.cpp:handle_node_superiors_add()`, which, once properly
implemented, will do:

1. Find the intended superior Node in the Graph by its ID.
2. Carry out the Edge creation and Node modification in memory.
3. Set Edit flags.

Once these modifications have been made in memory and the function returns
successfully, remaining processing in `handle_node_direct_parameter()` does:

1. Update the database based on edit flags via `Update_Node_pq()`.
2. Carry out a validity test checking Edit errors.
3. Create the response HTML, which is ultimately sent back through the TCP
   link.

An important question is how `Update_Node_pq()` differs from
`handle_Graph_modifications_pq()`. Both are in `Graphpostgres.cpp`.

`Update_Node_pq()` does:

1. Create a connection to the database.
2. Call `update_Node_pq()` with the edit flags.
3. Clear edit flags.
4. Finish the database action and connection.

**Note: The `update_Node_pq()` function may not take care of Edge changes.
Indeed, the function only includees calls to Node edits.**

`handle_Graph_modifications_pq()` does:

1. Create a connection to the database.
2. Use the `Graphmod_results:results[i].request_handled` identifiers to
   call specific database update functions. In our case, `add_Edge_pq()`.
   The `update_Node_pq()` option also exists for Node edits.
3. Finish the database action and connection.

The `handle_Graph_modifications_pq()` function is far more capable than
the `Update_Node_pq()` function, as it pertains to many more components
than just the Nodes.

Note that adding an edge in the database requires only a call from
`handle_Graph_modifications_pq()` to `add_Edge_pq()`, because the
necessary Graph connections from Nodes are created purely from the list
of Node and Edges in the database.

There is additional information in [https://trello.com/c/eUjjF1yZ](https://trello.com/c/eUjjF1yZ).

#### Aside:

A Graph modification request that goes to `handle_serialized_data_request()`,
does:

1. Requests are tokenized.
2. Requests are sent to `handle_request_args()`.

That handler uses a map to call one of several functions for NNL or Node
modifications. For our modification request, we are sent to `Nodes_match()`,
which identifies the Node(s) involved and leads to the function
`handle_serialized_data_request_response()`, which then returns the set of
Nodes in CSV format.



-----

Randal A. Koene, 2020
