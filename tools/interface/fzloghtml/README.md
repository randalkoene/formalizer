# fzloghtml - Generate HTML representation of requested Log records.

## To create a special URL that tells rendering to make a link to local files

In the URL part of a HTML link, put something like:

@FZSERVER@/doc/tex/something.doc

## The `day review` option

The `GraphTypes.hpp:Boolean_Tag_Flags` are used to specify Node category.
Currently, those are (aside from `tzadjust` and `error`):
`work`, `self_work`, `system`, `other`, or `none`.

This is carried out as followed in the `render.cpp:render_Log_review()` function:

1. Retrieve the NNL that lists all sleep Nodes.
2. Identify which of those is the nap Node.
3. Walk through completed Log chunks in the interval that was loaded.
4. Detect sleep chunks and use those to determine wake-up and go-to-sleep times.
5. Naps appear to receive the boolean tag flag `none`.
6. For other Nodes, detect the category and specify it in the boolean tag flag as detailed below.
7. (Also, collect metric tags data for tags such as @NMVIDSTART@ and @NMVIDSTOP@. If found, that is written to a CSV file.)
8. Render as json, html, txt or raw.

Detecting Node/Chunk category:

1. Search for category override tags in the Log entries of the chunk.
2. Get the Boolean Tag Flag with the function `Map_of_Subtrees::node_in_heads_or_any_subtree()`.

The `Map_of_Subtrees` was collected for the head Nodes and their dependencies as
specified in the `config.json` of `fzloghtml`. By default the NNL specified there
is `threads`.

Note that the categories search, as implemented in `fzloghtml`, does not search for
overrides in the Node description, nor does it search for category specification by
superior if not found for a Node. This means that many Nodes that are not covered
by the map of subtrees for the specified NNL receive the `none` Boolean_Tag_Flag.

## Examples of scripts that use this

- fzlog-cgi.py
- fztask-cgi.py
- fztask.py
- fzcatchup.py
- fzlink.py
- checkboxes.py
- fzlog-mostrecent.sh
- fzloghtml-cgi.py
- fzloghtml-term-1920.sh
- fzloghtml-term.sh
- get_log_entry.sh
- dayreview.py

---
Randal A. Koene, 2020
