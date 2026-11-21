# fzlog - Log modification utility

The `fzlog` utility is used for a variety of actions that modify the Log.

The `fzlog-cgi.py` script provides a CGI interface to fzlog for web-based
control.

This includes closing Log chunks and starting new Log chunks for a
selected Node.

E.g. `/cgi-bin/fzlog-cgi.py?action=open&n=<node-id>`

Run `fzlog -h` for all command line options.

IDs used below:

- Log chunk ID: the chunk's open time, `YYYYmmddHHMM`, e.g. `202611170930`.
- Log entry ID: chunk ID plus the entry's number in the chunk, e.g. `202611170930.2`.

## Deleting a Log entry

As a safety precaution, only an empty Log entry can be deleted (empty or only
whitespace). So deleting takes two steps: empty the entry, then delete it.

In the browser:

1. Click `edit` next to the entry (Recent Log or any Log page), or open
   `/cgi-bin/fzlog-cgi.py?action=editentry&id=<entry-ID>`.
2. Clear the text and click `Replace entry: Text`. (Until the fix of 2026-11-17
   in `fzlog-cgi.py` is deployed, leave a single space in the text box. A fully
   empty box makes the page fail.)
3. On the Recent Log page an empty entry has a `delete` button next to `edit`.
   Click it, or open `/cgi-bin/fzlog-cgi.py?action=delete&id=<entry-ID>`.

On the command line:

```
fzlog -r <entry-ID> -T " "     # empty the entry
fzlog -D <entry-ID>            # delete it
```

`fzlog -D` refuses with "Entry must be empty to be deletable" if the entry
still has text.

## Changing a Log chunk's open or close time

```
fzlog -m <chunk-ID> -2 <YYYYmmddHHMM>   # new close time
fzlog -m <chunk-ID> -1 <YYYYmmddHHMM>   # new open time
```

In the browser, `/cgi-bin/fzlog-cgi.py?action=modify&id=<chunk-ID>` (click a
chunk's time on the Log page) has forms for both (and for changing the chunk's
Node), and above them a time strip from the start of the previous chunk to the
end of the next one:

- Drag the start or end of the chunk in 5-minute steps (or select a handle and
  use the arrow keys). A handle stops at the limits set by the rules below and
  says which limit it reached.
- By default, dragging a boundary that the chunk shares with a neighbour moves
  the neighbour's edge along. Uncheck "Move the previous chunk's end along" or
  "Move the next chunk's start along" to leave a gap instead (e.g. to insert a
  missing chunk afterwards).
- The page lists the fzlog calls that Apply will make, in an order in which
  every step obeys the rules (`action=modifytimes` runs them and stops at the
  first that fails).
- The logic is in `tools/interface/resources/chunkstrip.js`. Its test,
  `chunkstrip_test.html` next to it, runs in any browser from the source
  directory.

Rules:

- A chunk's open time must be at or before its close time, at or after the close
  time of the chunk before it, after the open time of the chunk before it (its
  ID), and not later than now.
- A chunk's close time must leave room before the next chunk's open time.
  So when two adjacent chunks change, change the one that makes room first:
  - Moving the boundary earlier: first move the close time of the earlier chunk,
    then the open time of the later chunk.
  - Moving the boundary later: first move the open time of the later chunk,
    then the close time of the earlier chunk.
- Changing these times does not change the time logged on the Nodes (their
  completion), only the Log itself.

### Changing the open time of a chunk with Log entries

The open time is the chunk's ID, and each Log entry's ID starts with it
(`YYYYmmddHHMM.nnn`). Since 2026-11-21, `fzlog -m <chunk-ID> -1 <time>` works
for chunks with entries: it changes the chunk ID, the IDs of its entries (they
keep their number, text, Node and order) and a Breakpoint with that ID, all in
one database statement. Either everything changes or nothing does, and the
number of entries changed in the database must equal the number loaded with the
chunk. (Until then the chunk had to be emptied first and the entries re-added by
hand.)

The Node histories cache (`histories`) is not changed, as with other Log
changes. Afterwards, rebuild it ("Update Node histories", or
`fzquerypq -R histories`), or the chunk is missing from its Nodes' history
pages until the next rebuild. (The web server user may only read that table,
which is why fzlog does not update it.)

Links elsewhere that contain the old chunk or entry ID (in Log entries, Node
descriptions or outside the Formalizer) are not changed.

The test `tests/test_chunk_id_change.sh <path-to-fzlog>` runs these changes, and
the cases that must be refused, in a throw-away database (`fztest_chunkid`,
created and dropped by the script). It never touches the Formalizer database.

--

Randal A. Koene, 2020. 2026
