# fzedit - Edit components of the Graph (Nodes, Edges, Topics)

The `fzedit` tools is the `core` tool for editing parameters of existing Nodes, Edges and Topics of a Graph.

## Editing Nodes

Node edits are possible through both the **shared-memory API** (using tools that run on the same logical
machine as the `fzserverpq` and the Graph shared memory segment), as well as the **direct TCP-port API**.

### Editing Nodes with the shared-memory API

1. The Formalizer tool, such as `fzedit` collects the Node edit data it needs. For example, this could be received
   through command line parameters, or from a file, or it might be generated, as when `fzupdate` determines new
   target date parameter data for multiple Nodes.
2. A `Graph_modifications` object is prepared in a new shared-memory segment. It contains a stack (vector) of
   `Graphmod_data` objects. Each one of those can be used to request modifications of one Node.
3. To provide the edited parameter data for a specific Node, a `Node` object is created in the shared-memory
   segment and connected with its `Graphmod_data` object. The ID of the `Node` object must be the same ID as
   that of an existing Node in the Graph.
4. The `Node` object can include any of the information that a `Node` would receive when being created, but does not
   need to include all of it. An `Edit_flags` object is used to specify exactly for which parameters modifications
   are being requested.

The modifications stack is then handed over to `fzserverpq` in the same way as during Node creation (see `fzgraph`).
The server validates the modifications stack and carries out the requested modifications of the in-memory Graph and
calls `Graphpostgres` functions to write those changes to the database as updates of existing records.


### Editing Nodes with the direct TCP-port API

Using the shared-memory API can be most efficient when changing multiple parameters, or when changing parameters of
multiple Nodes at once. To edit just one or a few parameters of a specific Node, it can be easier to send that
request straight to the `fzserverpq` using the direct TCP-port API.

The following are `GET` URL paths that can be used to edit specific Node parameters:

```
    /fz/graph/nodes/20200901061505.1/edit?completion=1.0&repeats=no
    /fz/graph/nodes/20200901061505.1/edit?required=+45m
    /fz/graph/nodes/20200901061505.1?completion=1.0&repeats=no
    /fz/graph/nodes/20200901061505.1?required=+45m
    /fz/graph/nodes/20200901061505.1/completion?set=1.0
    /fz/graph/nodes/20200901061505.1/required?add=45m
    /fz/graph/nodes/20200901061505.1/required?add=-45m
    /fz/graph/nodes/20200901061505.1/required?add=0.75h
    /fz/graph/nodes/20200901061505.1/required?set=2h
    /fz/graph/nodes/20200901061505.1/topics/add?organization=1.0&oop-change=1.0
    /fz/graph/nodes/20200901061505.1/topics/remove?literature=[1.0]
    /fz/graph/logtime?20200901061505.1=45
    /fz/graph/logtime?20200901061505.1=45&T=202012080713
```

Note A: The `logtime` format is a shorthand provided, because updating for time logged after an interval of
activity is extremely common.

Note B: The path to change a single parameter is very similar to the path used to inspect a specific parameter, for example, `/fz/graph/nodes/20200901061505.1/completion[.html]`.

A call to `fzserverpq -h` will list implemented URL methods.

### The Edit Web Page

A Node editing web page is generated by a call to `fzgraphhtml-cgi.py?edit=<node-id>`.

Submitting changes on that page through the `modify` button calls `fzedit-cgi.py`.

This receives the input `action=modify` and `id=<node-id>`, as well as numerous other variable values.

That calls the `modify_node()` function in `fzedit-cgi.py`, which collects the necessary variable values and then calls `fzedit`.


## Editing Edges

TBD

This will be similar to editing Node parameters. The source (dependency) and target (superior) Node IDs that are integral parts of an Edge ID cannot be modified, because those are structural necessities for the Edge to exist.
To change the connection that an Edge creates, remove (delete) the old Edge and create a new one.

```
    /fz/graph/edges/20200901061505.1>20200904135323.1/edit?urgency=3.0
```

## Editing Topics

TBD

Randal A. Koene, 20201126
