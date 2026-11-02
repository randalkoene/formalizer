# Tracing Actions

This documents some traces through requested actions to help identify which calls are involved
and possible problems that can occur. Traces will be collected whenever a new trace is needed
to solve an issue.

## Tracing "select" in Incomplete Nodes by Effective Target Date page

Adding this trace, because around May/June 2026 a slowdown began to happen where it can
take up to about a minute to complete a "select" request in order to then specify when a
new Log chunk begins using the Log Time button.

It is initially unclear if this has something to do with a Chrome update, fragmentation of
the Postgres database, or something else.

1. Clicking 'select' invokes a call to URL http://127.0.0.1:8090/fz/graph/namedlists/_select?id=<node-id>
2. `tcpserver.cpp:server_socket_listen()` is waiting on port 8090 and receives the request.
   Note that there is already a comment in there
   `valread = read(new_socket, str, sizeof(str)); // *** right now, this read often hangs until some timeout`.
   The request is sent to `server.handle_special_purpose_request(new_socket, request_str);`.
3. `tcp_server_handlers.cpp:queing_fzserverpq::handle_special_purpose_request()` puts the request into `special_FIFO`.
4. A `fzserverpq.cpp:queue_handling_thread_func()` thread calls `zs_ptr->handle_queued_request();` to notice
   if something is pending in `special_FIFO`.
5. `fzserverpq.cpp:queing_fzserverpq::handle_queued_request()` calls
   `fzserverpq::handle_special_purpose_request()` and after that closes the comms socket involved.
6. `tcp_server_handlers.cpp:fzserverpq::handle_special_purpose_request()` notices that this is a `/fz/`
   request and calls `handle_fz_vfs_request()`.
   Note that this already has a `To_Debug_LogFile()` option in the code, which logs information with a time stamp.
7. `tcp_server_handlers.cpp:handle_fz_vfs_request()` notices that this is a `graph/` request and calls
   `handle_fz_vfs_graph_request()`.
   This also has a `To_Debug_LogFile()` option.
8. `tcp_server_handlers.cpp:handle_fz_vfs_graph_request()` notices that this is a `namedlists/` request and
   calls `handle_named_list_direct_request()`.
9. `tcp_server_handlers.cpp:handle_named_list_direct_request()` detects that there is an argument after `?`
   and extracts `_select` as the named Nodes list intended, as well as the argument pair.
   It detects that `_select` is a known entry in `NNL_underscore_commands` and calls
   `handle_selected_list()`.
10. `tcp_server_handlers.cpp:handle_selected_list()` extracts the `id` key, finds the corresponding Node in
   the Graph, then calls `Graph::add_to_List()` and when lists are persistent (which they normally are),
   calls `Update_Named_Node_List_pq()`.
   It then returns a successful response.
11. Upon return to `handle_fz_vfs_graph_request()` the successful response is given to
   `handle_request_response()`.
12. `tcp_server_handlers.cpp:handle_request_response()` calls `srvtxt.respond(socket)` to send the response
   back to the browser.


## Tracing a Node edit from the Edit page to the database

(This is about editing parameters of an existing Node. Adding Nodes and Edges is
traced in the [fzgraph README](https://github.com/randalkoene/formalizer/blob/master/core/fzgraph/README.md).)

1. `fzgraphhtml-cgi.py?edit=<node-id>` generates the Edit page, a form with the
   Node's data.
2. Pressing `modify` submits the form to `fzedit-cgi.py` with `action=modify`.
   `modify_node()` collects the form values, writes the description to
   `/var/www/webdata/formalizer/node-text.html` and calls `fzedit -E STDOUT -M <node-id> -f <file> -c ... -H ... -a ... -t ... -p ... -r ... -e ... -s ...`.
3. In `fzedit`, each parameter option sets the corresponding flag in
   `fze.editflags` (an `Edit_flags` object) and stores the value in `fze.nd`
   (a `Node_data` object). Configuration and defaults have already been applied
   by the standard init.
4. `fzedit.cpp:edit_node()` prepares a shared-memory segment and calls
   `Graphmodify.cpp:Graph_modifications::request_edit_Node()`. That creates a
   Node object (the editing object, with the same ID) in the segment and appends
   a `Graphmod_data` entry with request `graphmod_edit_node` to the
   `Graph_modifications` stack. `Graphmod_data` inherits `Edit_flags`.
5. The edit flags are copied to that `Graphmod_data` entry, and `reduce_edits()`
   compares the requested values with the Node in the resident Graph and clears
   the flags of parameters that do not change. (The Edit page sends all
   parameters, so this keeps database updates to what actually changed.)
6. `Node_data::copy()` copies the values from `fze.nd` into the editing object.
7. `server_request_with_shared_data()` sends the segment name to the `fzserverpq`
   TCP port and waits for the response.
8. In `fzserverpq`, `tcpserver.cpp:server_socket_listen()` receives a string that
   is not `GET`, `PATCH`, `FZ`, `STOP` or `PING`, so it is treated as the name of a
   shared-memory segment. `queing_fzserverpq::handle_request_with_data_share()`
   puts it in `shm_FIFO`, and the queue handling thread passes it to
   `fzserverpq::handle_request_with_data_share()`.
9. `shm_server_handlers.cpp:handle_request_stack()` finds the
   `Graph_modifications` object and validates the whole stack with
   `request_stack_valid()` (for Node edits this includes checking that the Topics
   exist). If anything is invalid, nothing is changed.
10. For `graphmod_edit_node`, `Graphmodify.cpp:Graph_modify_edit_node()` finds the
    real Node in the resident Graph and calls `Node::edit_content()` with the
    editing object and the `Graphmod_data` flags.
11. `Graphtypes.cpp:Node::edit_content()` copies each flagged parameter from the
    editing object and then stores the flags in the Node (`editflags`) to mark
    what changed. `handle_request_stack()` updates the Node's modification time
    and adds the Node to the results.
12. `Graphpostgres.cpp:handle_Graph_modifications_pq()` opens a database
    connection and, for each result, calls `handle_one_modification_pq()`, which
    for a Node edit calls `update_Node_pq()` with the Node and its edit flags.
13. `update_Node_pq()` builds an `UPDATE` command that sets only the flagged
    fields, and runs it.
14. The server replies `RESULTS` (or `ERROR`) on the socket; `fzedit` exits with
    the corresponding status.
15. `fzedit-cgi.py` shows the outcome on the resulting web page.

### Why repeating Nodes are updated inside the server

Updating repeating Nodes during Schedule updating could have been done like any
other multi-parameter edit: `fzupdate` would compute the new values on copies of
the Nodes and send only the changes, with `Edit_flags`, along the path above.

Instead, it is a separate shared-memory request that the server carries out
itself, for two reasons:

1. Less overhead, so a better response time.
2. It makes target date properties and repeat patterns (`tdproperty`,
   `tdpattern`) part of what a Node *is*, rather than the result of one tool's
   editing procedure. Repeating Nodes are conceptually different (for example,
   logged time accrues over many repetitions, not against one required time), so
   the Graph server should understand them.

The server also selects which Nodes to update, rather than receiving a list. It
already has the Nodes sorted by target date and can stop at the right point, and
copying a list into shared memory would be extra work. Special cases (Nodes that
should not be advanced) have to be handled by other means, e.g. before the update.

The path for `fzupdate` repeating updates:

1. `fzupdate.cpp:update_repeating()` calls
   `Graph_modifications::request_Batch_Node_Tpass(t_pass)`, which adds a
   `batchmod_tpassrepeating` request with only the time `t_pass` to the stack, and
   sends it with `server_request_with_shared_data()`.
2. `handle_request_stack()` calls
   `Graphmodify.cpp:Graph_modify_batch_node_tpassrepeating()`, which gets the
   incomplete repeating Nodes sorted by target date
   (`Nodes_incomplete_and_repeating_by_targetdate()`), advances those before
   `t_pass` (`Update_repeating_Nodes()`), and puts the updated Nodes in the
   `repeating_updated` NNL.
3. `handle_Graph_modifications_pq()` calls `update_batch_nodes_pq()`, which writes
   the Nodes in `repeating_updated` to the database. If no Nodes were updated, the
   NNL is deleted and nothing is written.

Variable and unspecified target date Nodes are updated differently: `fzupdate`
computes the new target dates itself and sends them as a `batchmod_targetdates`
request (see [Schedule Updating](https://github.com/randalkoene/formalizer/blob/master/core/doc/schedule-updating.md)).

Source: Trello card eUjjF1yZ (archived in ~/.formalizer/archive/trello/cards/).
