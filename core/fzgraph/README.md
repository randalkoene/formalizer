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

-----

Randal A. Koene, 2020
