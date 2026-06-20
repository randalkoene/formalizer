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

