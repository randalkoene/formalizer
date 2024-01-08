# Notes about scheduling

## Dealing with `inherit` and `unspecified` targetdate properties

This scheduling attempts to adhere to the strategies determined for the
meaning of `inherit` and `unspecified` targetdate properties, as
described in the comments of `Graphtypes.cpp:Node::effective_targetdate`.

1. Unless a Node has the `inherit` or `unspecified` targetdate property,
the locally specified targetdate is used (and repeats are applied as
needed).

2. When a Node has the `inherit` or `unspecified` targetdate property
then the `effective_targetdate()` member function is used to obtain the
targetdate. (This is automatic, since the targetdate sorted collection
obtained with `Graphinfo:Nodes_incomplete_by_targetdate()` already uses
`effective_targetdate()`.)

3. Coloring indicates the targetdate property, and `inherit` and
`unspecified` are presented in their own distinctive colors. (This is
done by the `nodeboard` tool.)

4. During the scheduling steps (**exact**, **fixed**, **variable**),
Nodes with `unspecified` targetdates are included in the **variable**
scheduling step, while those with `inherit` targetdates are included
according to the following protocol:

```
a: Use the effective_targetdate() function to find the origin Node for
   the targetdate used.
b: If the origin Node is the same Node or nullptr then include it in
   the variable scheduling step.
c: If the origin Node has inherit or unspecified property then include
   it in the variable scheduling step. (This is not ideal!)
d: If the origin Node has fixed or exact property then include it in
   the fixed scheduling step.
e: If the origin Node has variable property then include it in the
   variable scheduling step.
```

It is sufficient to implement protocol points a, b and d, as the rest
can be derived to mean inclusion in the variable scheduling step.


--
Randal A. Koene, 2024
