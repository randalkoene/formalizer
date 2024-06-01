# `nodeboard` - Kanban board tool with Node hierarchy support

(( ...this would be a good place for general documentation of the tool... ))

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
