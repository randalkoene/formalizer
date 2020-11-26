# logentry - command line interface to fzlog

This command line tool facilitates making of Log entries and Log chunks.
Interactive prompts, a text editor, and a browser are used to collect the
necessary information, and `fzlog` is called to create the Log entry or chunk.

For the web interface to fzlog, see `logentry-form`.

Note: This is the Formalizer 2.x successor to the `makenote` tool.


-----

For comparison, the steps carried out in the `makenote` tool:

1. Output is `tee`d to `/tmp/makenote.log`. (**[LE]**)
2. Call `dil2al -m"$1"`.

The parameter `$1` allows text from a file to be included.

In `dil2al`, that call calls `note.cc:make_note()`, which in turn calls
`note.cc:make_note_TL()`:

1. A temporary file is created, possibly with content read from `$1`. **[LE]**
2. The text editor (e.g. `emacs`) is called with that temporary file. **[LE]**
3. `note.cc:proces_note()` is called, that calls a file type specific `process_note`.

In `process_note_TL()`:

1. Call `pre_process_note()`.
2. Decide if a new Task Log chunk is to be added for the note, `decide_add_TL_chunk()`.
3. Process HTML, `process_HTML()`.
4. Then, `add_TL_chunk_or_entry()`.

In `pre_process_note()`:

1. Read the note from temporary file.
2. Process it in `process_HTML()`.
3. Read the Task Log file into memory.

In `process_HTML()`:

1. Convert line breaks, paragraph breaks, lists.
2. Convert special tags.

In `decide_add_TL_chunk()`:

1. Find time stamp of most recent Task Log cuhnk.
2. Depending on the time since then, and on configured rules, either make a decision or ask the user to make a decision.
3. Possibly make a new Task Log chunk, perhaps even a new section.


In `add_TL_chunk_or_entry()`:

1. Generate all of the standard components of a Task Log entry.
2. Add the entry to the Task Log file.

The process can involve:

- Attaching the new Task Log entry to a different, specified DIL entry (picked from a list, via browser, or entered). **[LE]**
- Starting a new Task Log chunk, with a choice of DIL entry (picked from a list, via browser, or entered). (**[LE]**)

The five places marked with **[LE]** indicate steps that also apply to `logentry`.
