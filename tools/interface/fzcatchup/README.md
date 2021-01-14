# fzcatchup - Command Line tool to simplify Log catch-up

The `fzcatchup(.py)` tool implements a process flow that is streamlined to help with the iterative catch-up of many Log entries and Log chunk switches, including any necessary calls to the `addnode` command line tool to create new Nodes as needed.

The alternative to using this tool (or a web interface equivalent) is to use the `fztask` tool with emulated time selections, i.e. proceeding as if Log chunks were being generated in the normal course of daily application of the Formalizer. That alternative is weighed down by including the normal process of Schedule updates during Log chunk switches. The `fzcatchup` tool skips the updates and instead focuses entirely on quickly iterating through Log chunks to catch up.

**Note**: It is important to carefully read the instructions on the command line during the interactive process, because the order of Log entry content creation and the time stamps requested is purposely somewhat different than during the normal `fztask` process flow. This is done to better match up with the manner in which temporary Log notes tend to be taken.

---
Randal A. Koene  
2021-01-14
