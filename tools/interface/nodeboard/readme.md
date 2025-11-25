# `nodeboard` - Kanban board tool with Node hierarchy support

## Programs and scripts

- `nodeboard`: Generate Kanban board representations of Node hierarchies and Named
  Nodes Lists in a variety of useful ways.
- `nodeboard-cgi.py`: Web interface CGI scripted access to `nodeboard`.

## Examples

The following examples demonstrate some of the principal ways to use
nodeboard. Note that many of the options available on the command line
can also be modified from the `options` pane in the top-right corner
of many generated boards.

### `nodeboard -g -n <node-id>`

Generate a grid-constrained board that presents the Node specified by
`<node-id>` and its hierarchy of superiors.

E.g. the board created by:

```
/cgi-bin/nodeboard-cgi.py?n=20100907053900.1&g=true&I=true&T=true&B=79&tdorder=true&tdbad=true&tdfar=true&i=79
```

### `nodeboard -G -n <node-id>`

Generate a grid-constrained board that presents the Node specified by
`<node-id>` and its hierarchy of dependencies.

E.g. the board created by:

```
/cgi-bin/nodeboard-cgi.py?n=20100907053900.1&G=true&I=true&T=true&B=79&tdorder=true&tdbad=true&tdfar=true&i=79
```

### `nodeboard -f <sysmet-json-file>`

Generate a board from data indicated in a special system/metrics format.

E.g. the board created by:

```
/cgi-bin/nodeboard-cgi.py?f=main2023
```

### `nodeboard -n <node-id>`

Generate a columns-constrained board that presents the Node specified
by `<node-id>`, its direct dependencies as column headers, and further
dependencies within the columns.

E.g. the board created by:

```
/cgi-bin/nodeboard-cgi.py?n=20250822225247.1&Z=true
```

### `nodeboard -D <name>`

Generate a board where the column headers are the Nodes in the NNL specified
by `<name>` and their dependencies fill the columns.

E.g. the baord created by:

```
/cgi-bin/nodeboard-cgi.py?D=week_main_goals&T=true&u=204512311159&r=100&U=true
```

### `nodeboard -c <csv-path>`

Generate a schedule based on information in a CSV file indicated by
`<csv-path>`.

E.g. the board created by:

```
/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20
```

### `nodeboard -l "{milestones_formalizer,procrastination,group_sleep}"`

Generate a board from the specified list of NNLs.

### `nodeboard -t "{carboncopies,cc-research,cc-operations}"`

Generate a board from the specified list of Topic keys.

### `nodeboard -m "{carboncopies,NNL:milestones_formalizer}"`

Generate a board from the specified mixed list of topics and NNLs.

## Detection and auto-solving of target date order problems

Assumptions:

- The dependency structure in the Node hierarchy is correct.
- Where dependencies can be completed right up until their superior, the 'inherit'
  TD is used, so, when 'variable' TD is used ite means they should be completed
  somewhat sooner.

Solving target date order problems can be done according to several possible
philosophies:

1. The superior must indeed be achieved by its specified target date. That means,
   its dependencies must be completed earlier. Therefore, dependency target dates
   need to be updated to earlier times to solve a target date order problem.

2. Dependencies have their target dates due to schedule constraints caused by
   time needed to accomplish a hierarchy of dependencies and other Nodes. They
   cannot be earlier, because there is no room (without manual changes in the
   overall Schedule). Therefore, the superior target date needs to be updated to
   a later time to solve a target date order problem.

One way to choose between these philosophies is to consider the TD property of
the superior. If the superior has a fixed target date then choose philosophy 1.
If it has a variable target date then choose philosophy 2.

A simple way to propose changes is to apply one of the philosophies and to merely
propose a change that solves the situation for one superior-dependency pair, and
then to keep looking for more errors until they are all solved.

---
Randal A. Koene, 2024
