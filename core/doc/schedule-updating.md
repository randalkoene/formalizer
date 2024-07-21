# Formalizer: Schedule Updating

This document describes up a Schedule is automatically updated with the
Formalizer v2.x. Links within this document lead to additional
information.

This document was started as part of Node [20240614082648.1](http://localhost/cgi-bin/fzlink.py?id=20240614082648.1).

# Programs used to auto-update the Schedule

Updating the Schedule via a form link in an HTML page is done by calling:

```
/cgi-bin/fzupdate-cgi.py
```

The CGI script expects the parameter `update` with one of the following values:

1. `repeating`
2. `variable`
3. `both`

If `both` is selected then it causes 2 successive `fzupdate` calls, first to update `repeating` and then to update `variable`.

An emulated time can be provided through the parameter `T_emulate`.
The number of days to map can be provided through the parameter `map_days` and normally defaults to 14 days. That default value can be modified through the `.formalizer/config/fzupdate/config.json` file.

To carry out the update, a Formalizer core program, `fzupdate` is called. To update `repeating` the call is:

```
fzupdate -r -t <T_emulate> 
```

The Nodes that were modified during the update can be inspected in the NNL `repeating_updated`.

To update `variable` the call is:

```
fzupdate -u -t <T_emulate> -D <map_days>
```

The Nodes that were modified during the update can be inspected in the NNL `batch_updated`.

There are additional ways to call the CGI script with script `update` values `passedfixed` or `convert_passedfixed`:

- `passedfixed`: This uses a serial API request to find fixed target date Nodes with a target date that has passed and puts those into a `passed_fixed` NNL.
- `convert_passedfixed`: This edits the Nodes found in the `passed_fixed` NNL to change their target date property to `variable`.

# Updating repeating Nodes

The `fzupdate -r` call updates repeating Nodes.

This creates a request in shared memory and uses a TCP call to the `fzserverpq` to run the `shm_server_handlers.cpp:handle_request_stack()` function. In that function, the case `batchmod_tpassrepeating` is activated. It carries out the update using the function `Graphmodify.cpp:Graph_modify_batch_node_tpassrepeating()`.

That function does the following:

1. Collects a list of repeating Nodes with target dates up to a specified `tpass` TD through a call to `Graphinfo.cpp:Nodes_incomplete_and_repeating_by_targetdate()`.
2. Updates the target dates of Nodes in that list that have target dates that were passed by calling the function `Graphmodify.cpp:Update_repeating_Nodes()`.

The Nodes that were updated are placed in the NNL `repeating_updated`.

# Updating variable Nodes

The `fzupdate -u` call updates variable target date Nodes with the `update_variable()` function of `fzupdate.cpp`.

1. A time limit, `t_limit`, is set. Normally, this is `t_pass` plus the number of days in `config.map_days`. Alternatively, a specific `t_limit` may have been requested that must be smaller or equal to `t_pass`. To keep updates from exploding, if `t_limit` was set to `RTt_maxtime` it is instead set to a TD that is sufficient to complete all non-repeating Nodes.
2. Constraints are initialized. The `update_constraints` struct is used for that. It contains `t_fetchlimit`, `chunks_per_week`, `num_nodes_with_repeating`, `chunks_req_total`, `weeks_for_nonperiodic`, `days_in_map`. The `f_fetchlimit` is `t_limit` plus `config.fetch_days_beyond_t_limit` (defaults to 30). The `chunks_per_week` is determined based on `config.chunk_minutes`.
3. Edit flags are set to edit target dates.
4. Collect all incomplete Nodes, including repeats up to `constraints.t_fetchlimit`.
5. A vector containing EPS data (`chunks_req`, `epsflags`, `t_eps`) is initialized. There is one set of data for each entry in the list of incomplete Nodes. For each entry, the time required to complete a Node is used to set `chunks_req` (with more than 36 hours being considered 'suspicious'). The `t_eps` variable is initialized to `RTt_maxtime` for each. We now know how many chunks are needed for each Node in the incomplete Nodes list.
6. The total number of chunks needed for all incomplete Nodes up to `t_limit` is calculated. This is used to set necessary values in the `update_constraints` structure, namely values for `num_nodes_with_repeating`, `chunks_req_total`, `weeks_for_nonperiodic` (with a minimum of 1), `days_in_map`.
7. An `EPS_map` is initialized using `t_pass`, `constraints.days_in_map`, the list of incomplete Nodes, and the vector of `epsdata`. A number of so-called 'slots' is allocated for each chunk, because the `EPS_map` works with 5 minute slots for a more detailed mapping. While the `starttime` is set to the time from which to start mapping (usually current time, unless a time is being emulated), the `firstdaystart` is set to the start of the first day in the map and `t_beyond` is set to the start of the first day after the `days_in_map`. Slots already passed before `starttime` are discounted in `firstday_slotspassed` and the `first_slot_td` is determined accordingly. All of the necessary `slots` are initialized with their corresponding TDs. The `next_slot` to handle is initialized to the beginning of that `slots` list. A `node_vector_index` list is created so that every Node in the list of incomplete Nodes with repeats has a corresponding index number. We now have a look-up table that matches Node ID keys to entries in the EPS data vector.
8. **Exact target date Nodes** are placed in the map up to `t_limit`. Slots are reserved for these Nodes in accordance with their chunks required. The reserved slots for each line up to reserve time as if for an appointment that has to happen at exactly that time. Where there is overlap between such Nodes, this is noted. It is also noted if repeated Nodes have periodicity less than a year.
9. **Fixed target date Nodes** are placed in the map up to `t_limit`. Note that this also includes Nodes that specify they **inherit** a target date and do so from an `origin` that is a Node with **fixed** target date. In this case of inheritance, the EPS data `epsflags` note it as `treatgroupable`, and assigning slots is done in the step for *moveable Nodes*. Otherwise, slots are immediately reserved and it is noted whether available slots are insufficient. Slots are reserved from the target date backwards (to earlier times), using any available slots. It is also noted if repeated Nodes have periodicity less than a year.
10. **Moveable Nodes** are placed, meaning Nodes with **variable** or **unspecified** target dates. See detailing in the subsection below.
11. Nodes to update are collected from the EPS map and requests are made to update them to their new target dates.

Note that there is an alternative method called `pack_moveable`, but that method is off by default and is presently typically left off in the `config.json`. The `pack_moveable` option attempts to pack more moveables into the map instead of leaving them undisturbed if their target dates are beyond `t_limit`.

## Exact TD Nodes

The process for Nodes with exact target dates has no optionality, since it is assumed that those must be dealt with at exactly the indicated times.

## Fixed TD Nodes

Time for Nodes with fixed target dates is found in the map such that a Node could by completed by its target date, but without any randomization of assigned slots or chunks at the stage when the EPS map is created. This greatly simplifies the process.

Randomization could be applied as a secondary step when suggesting a day-calendar of Nodes to work on.

## Variable TD Nodes

This process applies to all Nodes marked `variable` TD, `unspecified` TD, or EPS `treatgroupable`.

Processing is done in terms of so-called EPS groups, i.e. groups of Node that are treated as having the same target dates.

For each group, an attempt is made to find enough slots to allocate time for all of the Nodes in the group. Nodes are allocated to slots starting from the earliest available slot to the latest.

A *new target date is proposed* for each Node in a processed group once slots have been found for all the Nodes in the group.

Once insufficient slots are found, remaining Nodes to be processed are marked with the EPS flag `insufficient`. There is some special treatment here in the case of `pack_moveable`.

# New directions for Schedule handling

1. VTD Nodes should continue to be handled as they are, namely that they are supposed to be completed by their specified target dates, and that Nodes where the target date has not been passed don't - normally - get modified.
2. Ordinarily, updating VTD Nodes should only involve updating those where target dates have been passed, so that perhaps we switch away from modifying those to earlier target dates that are within the specified Schedule window.
3. With rules 1. and 2. above, space remains, and that space gets filled with the list of UTD Nodes, Nodes with unspecified target dates. For this to work well, most, or almost all of my typical VTD Nodes will need to become UTD Nodes, because only in a few cases do you really want to get things done by a particular target date - but you are open to shifting those dates as needed.
4. Clearly, this does make FTD and VTD Nodes seem more similar, with the principal difference being that FTD Nodes require manual attention to modify target dates.
5. Wherever there are true dependencies, i.e. something really can only be done when dependency tasks have been completed, those serious dependencies should use the ITD (Inherited Target Date) method. This should apply regardless whether the superior in question is of FTD or VTD type. If the superior is of fixed type then the inheriting dependencies need to be placed in the schedule before VTD Nodes, otherwise along with them. UTD placement always comes last. Note that UTD Nodes should not be scheduled to have the same target date as the superior, because that can cause them to be placed in the Schedule in the wrong order. They should have effective target dates generated for use in Scheduling that are earlier than the superior they make possible (this is wrong in Schedule updates right now).
6. Giving Nodes ITD type should be encouraged. To make that feasible, it needs to be taken into account that one tends to underestimate time needed. That means, ITD Nodes cannot use a placement method that works backwards from the superior's target date, because some extra time needs to be available. Using UTD type more will greatly reduce time needed to update the Schedule manually.
7. UTD Node placement must carefully use the dependency tree hierarchy to decide the order of Nodes.
8. It is good to be able to inspect what Schedule updating is doing by checking an HTML version of the map as it is being generated from step to step.

---
Randal A. Koene, 20240617
