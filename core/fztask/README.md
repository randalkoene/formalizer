# fztask - Task Control

The fztask tool is a command line tool that persists, presently running
in a while-loop, iteratively carrying out the sequence:

1. Make a Log entry.
2. Choose a new Node for the next Log Chunk or simply close the Log Chunk.
3. Update the Schedule.
4. Start new Log Chunk (if applicable).
5. Set a timer for the Log Chunk.

The Node for the next Log Chunk is chosen:

1. From a shortlist of Nodes.
2. By browsing available Nodes.

The shortlist is updated very simply. It is composed of two parts:

1. The 5 most recent Nodes.
2. The 5 next Nodes in the Schedule.

No attempt is made to shuffle or randomize, or to prioritize in a smart way.

**Note**: When using a modern browser, the functionality of fztask is included
in the options available as follows:

- Log Chunk entry and closing: On the page generated through the "Recent Log" button
of the main Formalizer index.
- Selecting a new Node: Via the "Log Time" button on that page, then through the
time selection and then the "Select Node for next Log Chunk" button, which offers
access to the list of Next Nodes, Recent Log, and much more.
- A timer: Via the "Timer" button on the main Formalizer index page.

--
Randal A. Koene, 20201125
