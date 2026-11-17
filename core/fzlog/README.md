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

In the browser, `/cgi-bin/fzlog-cgi.py?action=modify&id=<chunk-ID>` has forms
for both (and for changing the chunk's Node).

Rules:

- A chunk's open time must be at or before its close time, at or after the close
  time of the chunk before it, and not later than now.
- A chunk's close time must leave room before the next chunk's open time.
  So when two adjacent chunks change, change the one that makes room first:
  - Moving the boundary earlier: first move the close time of the earlier chunk,
    then the open time of the later chunk.
  - Moving the boundary later: first move the open time of the later chunk,
    then the close time of the earlier chunk.
- The open time is the chunk's ID, so changing it changes the ID. Links that
  point to the old chunk ID stop working.

**For now, the chunk must have no Log entries to change its open time.**
`fzlog -m <chunk-ID> -1 ...` otherwise stops with "Sorry! For now, a Log chunk
must be emptied to change its ID." The Log entries carry the chunk ID in their
own IDs, and changing those is not implemented yet. To do it by hand:

1. Copy the text of each entry in the chunk (and note its Node, if it differs
   from the chunk's Node). The `edit` page of each entry shows both.
2. Empty and delete each entry, as above.
3. Change the open time: `fzlog -m <old-chunk-ID> -1 <YYYYmmddHHMM>`.
   The new chunk ID is the new open time.
4. Re-add the entries in their original order, each at the end of the chunk:
   `fzlog -i <new-chunk-ID> -T "<text>" [-n <node-ID>]`, or with `-f <file>`
   for longer text.

### Planned: keep the entries when changing the open time

The plan is to let `fzlog -m <chunk-ID> -1 <time>` do the steps above itself:
keep the entries (text, Node, order), remove them, change the open time, and
add them back under the new chunk ID. A copy of the entries is written to a
temporary JSON file first, so they can be restored if a step fails. The
`Change Open timestamp` form on the `action=modify` page would then work for
chunks that have entries, with no change needed in the page itself. A better
version would change the IDs inside a single database transaction.

--

Randal A. Koene, 2020. 2026
