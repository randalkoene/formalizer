# fzupdate — Update the Node Schedule

`fzupdate` is the Formalizer core program that automatically updates Node **target
dates** in the Schedule. It handles two independent jobs — advancing **repeating**
Nodes whose target dates have passed, and re-placing **movable** (variable and
unspecified) target-date Nodes into available time — and provides a couple of
time-requirement reports.

Like most Formalizer tools, `fzupdate` does not touch PostgreSQL directly. It talks to
the running **`fzserverpq`** data server over TCP, which holds the graph in shared
memory and serializes changes. `fzserverpq` must be running (`fzserverpqd.sh`).

A companion narrative of the algorithm lives in
[core/doc/schedule-updating.md](../doc/schedule-updating.md); this README is the
reference for options, configuration, and — importantly — how the older and newer
placement mechanisms relate.

---

## Operation modes

Exactly one mode is selected per invocation (the "flow"):

| Flag | Mode | What it does |
|------|------|--------------|
| `-r` | Update repeating | Advances the target dates of incomplete **repeating** Nodes whose target dates have passed (up to the `-T`/reference time). Updated Nodes are collected in NNL `repeating_updated`. |
| `-u` | Update variable | Re-places **movable** Nodes (variable + unspecified/UTD) into available time and proposes new target dates. Updated Nodes are collected in NNL `batch_updated`. This is the main scheduling pass. |
| `-b` | Break EPS group | Splits a group of variable-target-date Nodes that share one target date `-T` into slightly offset target dates so they no longer collide. |
| `-R` | Report repeating time | Prints the total required time for incomplete **repeating** Nodes up to `-T`: `[total minutes] [annual ratio] [hours/week] [hours/day]`. |
| `-N` | Report non-repeating time | Prints the total required minutes for incomplete **non-repeating** Nodes up to `-T`. |

With no mode flag, `fzupdate` prints usage.

---

## Command-line options

| Option | Meaning |
|--------|---------|
| `-r` / `-u` / `-b` / `-R` / `-N` | Select the operation mode (see above). |
| `-T <t_max\|full>` | Time limit to update up to (inclusive). A timestamp `YYYYmmddHHMM`, or `full` for a complete update of all movable Nodes (see [`-T full`](#-t-full-in-depth)). Overrides the `map_days` configuration for `-u`. Also the target date argument for `-b`, and the end time for `-R`/`-N`. |
| `-D <days>` | Number of days to map with `-u` (overrides config `map_days`; default 14). Ignored when `-T` gives an explicit limit. |
| `-m <margin>` | Safety margin applied to the computed demand in `-T full` (overrides config `full_overhead_multiplier`; default 1.1). |
| `-F <days>` | Cap the `-T full` map horizon to `<days>`; overflow UTD Nodes are tail-packed beyond the map (overrides config `full_map_days_max`; `0` = uncapped). |
| `-P <seconds\|none>` | **Legacy** "pack moveable" mode: turns on `pack_moveable` and sets the beyond-map packing interval to `<seconds>`; `-P none` turns it off. See [Placement mechanisms](#placement-mechanisms-legacy-vs-new). Off by default and normally left off. |
| `-c <chain>` | Placer chain for `-u`, e.g. `VTD,UTD` (comma or semicolon separated). Overrides config `chain`. |
| `-B <btf_days>` | Boolean-Tag-Flag day map, e.g. `SELFWORK:WED,SAT_WORK:MON,TUE,THU,SUN` — restricts categorized UTD Nodes to specific weekdays. |
| `-S <NNL_name>` | Use dependency subtrees of Nodes in the named NNL for BTF categorization (e.g. `threads`). |
| `-M <days>` | Show the generated maps for this many days (only with verbose `-V`). |
| `-e` | Disable end-of-day target-time priorities. |
| `-Z <HHMM>` | End-of-day time (e.g. `1940`). |
| `-d` | **Dry run** — compute and report everything, but do not modify the graph. Always preview with this first. |
| `-t <time>` | Emulated ("reference") time, e.g. `-t 202607171200`. Lets you compute an update as if run at another time. (From the shared ReferenceTime options.) |
| `-V` / `-v` | Verbose / very-verbose output (progress, the constraints report, and — with `-M` — the maps). |
| `-E <target>` | Redirect error/verbose stream, e.g. `-E STDOUT`. |

---

## Configuration

Config lives in `~/.formalizer/config/fzupdate/config.json` (flat JSON of string
values). Initialize config directories with `fzsetup.py -C`.

| Key | Default | Meaning |
|-----|---------|---------|
| `chunk_minutes` | `20` | Granularity of a work "chunk". The map allocates time in 5-minute slots; `chunk_minutes/5` slots per chunk. |
| `map_days` | `14` | Default horizon for `-u` when no `-T`/`-D` is given. |
| `map_multiplier` | `3` | Scales the computed `days_in_map` from the weeks of non-periodic work required. |
| `full_overhead_multiplier` | `1.1` | Safety margin on computed demand for `-T full`. Recommended `1.05`–`1.2`; larger values only enlarge the map and runtime. (Formerly a raw multiplier on a rough estimate — a large value like `2.0` is no longer needed.) |
| `full_map_days_max` | `0` (off) | Caps the `-T full` map horizon (in days). When set, overflow UTD Nodes are tail-packed beyond the map (see below). `0` = uncapped. Also settable per-run with `-F <days>`. |
| `fetch_days_beyond_t_limit` | `30` | Fetch incomplete Nodes this many days past `t_limit` (so nearby fixed/exact Nodes are visible). |
| `pack_moveable` | `false` | **Legacy** alternative placement (see [Placement mechanisms](#placement-mechanisms-legacy-vs-new)). Normally off. |
| `pack_interval_beyond` | `86400` (1 day) | Spacing between successive target dates assigned beyond the map. Used both by legacy `pack_moveable` and by the new tail-packing. |
| `chain` | *(empty → default placer)* | Placer chain. A typical value is `VTD;BTF;UTD`. |
| `NNL_name` | *(empty)* | NNL whose subtrees drive BTF categorization (e.g. `threads`). |
| `btf_days` | *(empty)* | Weekday restrictions for BTF categories (same format as `-B`). |
| `UTD_is_priority_queue` | `true` | Treat UTD Nodes as a priority-ordered to-do stack (order preserved, dates shifted freely). |
| `update_to_earlier_allowed` | `false` | Allow moving VTD Nodes to *earlier* target dates. |
| `endofday_priorities` | `true` | Snap proposed target dates to end-of-day priority times. |
| `dolater_endofday` / `doearlier_endofday` | `73800` / `68400` | End-of-day times (seconds since midnight, or `HH:MM`) for lower/higher urgency. |
| `eps_group_offset_mins` | `2` | Minutes of offset used to keep grouped target dates distinct and ordered. |
| `timezone_offset_hours` | `0` | Timezone offset applied to end-of-day adjustments. |
| `showmaps` / `showmaps_days` | `true` / `30` | Whether to render maps (very-verbose) and for how many days. |
| `warn_repeating_too_tight` | `true` | Warn when repeating Nodes are packed too tightly. |

---

## The `-u` variable update, in brief

1. Determine `t_limit` (from `-T`, `-D`, or `map_days`).
2. Fetch all incomplete Nodes + repeat instances up to `t_limit + fetch_days_beyond_t_limit`.
3. Compute the chunks required for each Node.
4. Build an **EPS map** of 5-minute slots spanning `days_in_map`.
5. Place Nodes in order: **exact** target dates → **fixed** (and inherited-from-fixed) →
   **movable** (variable + unspecified). Movable placement uses the configured `chain`
   (typically `VTD;BTF;UTD`) or, if no chain is set, the default `group_and_place_movable`.
6. Collect the Nodes whose target dates changed and send one batch update to `fzserverpq`.

See [schedule-updating.md](../doc/schedule-updating.md) for the full description,
including the ETD/FTD/VTD/ITD/UTD Node categories.

---

## `-T full` in depth

`-T full` re-places **every** movable Node, not just a bounded window. Because the map
must be large enough to hold everything, `fzupdate` sizes `t_limit` from the **actual
placement demand** rather than guessing:

- It runs a day-aligned fixed-point loop: fetch → compute required chunks (all UTD Nodes
  regardless of date, plus everything else up to the candidate limit) → grow the limit
  until slot capacity ≥ demand × margin. The loop is accelerated by the measured
  demand-growth rate and typically converges in 2–4 iterations.
- `full_overhead_multiplier` (or `-m`) is the **safety margin** on that demand.

**When the schedule is over-committed.** If repeating and dated Nodes consume ~100% or
more of the available time, no finite horizon can slot-place every UTD Node. Then:

- If **`full_map_days_max` is set**, the map horizon is capped at that many days, and any
  UTD Nodes that do not fit are given order-preserving target dates at
  `pack_interval_beyond` spacing **beyond the map** (*tail-packing*). Every UTD Node still
  receives an updated date; the fetch still covers all UTD Nodes so none are dropped; and
  the map — hence runtime and memory — stays bounded.
- If **`full_map_days_max` is not set**, `fzupdate` exits with a diagnostic reporting what
  fraction of available time the repeating and dated Nodes consume, and suggests enabling
  tail-packing, reducing repeating commitments, or using a bounded `-D`/`-T`.

If the schedule *does* fit within the cap, tail-packing simply never triggers and you get
a fully slot-accurate update.

---

## Placement mechanisms: legacy vs. new

There are two independent ways to deal with movable Nodes that don't fit in the mapped
window. They do **not** need each other, and mixing them is discouraged.

### `pack_moveable` / `-P` — legacy, off by default

The older mechanism, part of the **default** (non-chain) `group_and_place_movable` path.
When on, instead of leaving movable Nodes with target dates beyond `t_limit` undisturbed,
it keeps going past the map and assigns them target dates at successive
`pack_interval_beyond` offsets. It is off by default and is normally left off in
`config.json`. Turning it on also changes how movable Nodes are fetched.

### `full_map_days_max` tail-packing — new, recommended for `-T full`

The tail-packing described above is driven **solely** by `full_map_days_max`. It works
inside the placer **chain** (e.g. `VTD;BTF;UTD`), where the uncategorized-UTD placer now
gives overflow Nodes order-preserving dates beyond the map. It does **not** consult
`pack_moveable`. It only borrows the `pack_interval_beyond` spacing value (which has a
non-zero default regardless of `-P`).

### Which to use

- To make `-T full` complete on an over-committed schedule: set **`full_map_days_max`**
  only. **Leave `pack_moveable` off** (do not add `-P`). The two mechanisms are unrelated,
  and enabling both mixes the legacy default-path behavior into a chain-based run.
- `pack_moveable`/`-P` remains available for the legacy default placement path but is
  largely superseded by the chain + tail-packing approach.

---

## Recommended configuration for `-T full`

```json
{
    "full_overhead_multiplier": "1.1",
    "full_map_days_max": "120",
    "chain": "VTD;BTF;UTD",
    "UTD_is_priority_queue": "true"
}
```

`full_map_days_max` is the horizon you want planned to the exact slot; everything past it
is tail-packed in priority order. Choose it to taste (a larger cap plans more accurately
but builds a larger map).

---

## Examples

```bash
# Preview a full update (no changes written); verbose with the sizing report:
fzupdate -E STDOUT -u -T full -B WORK:SUN,MON,TUE,WED,THU,FRI_SELFWORK:SAT -d -V

# Apply a full update (requires full_map_days_max set if the schedule is over-committed):
fzupdate -u -T full -B WORK:SUN,MON,TUE,WED,THU,FRI_SELFWORK:SAT

# Full update with a one-off 90-day cap and margin (overriding config), dry run:
fzupdate -u -T full -F 90 -m 1.1 -B WORK:SUN,MON,TUE,WED,THU,FRI_SELFWORK:SAT -d -V

# Update repeating Nodes whose target dates have passed (as of an emulated time):
fzupdate -r -t 202607171200

# Bounded variable update over the next 14 days (default), dry run:
fzupdate -u -d -V

# Variable update up to an explicit date:
fzupdate -u -T 202610150000

# How much time do repeating Nodes require over the next ~90 days?
fzupdate -R -T 202610150000
```

---

## Notes

- Always dry-run (`-d`) and read the verbose (`-V`) **constraints report** before applying:
  it shows demand vs. available chunks, the chosen `t_limit`, and `days_in_map`.
- Updated Nodes are recorded in NNLs `repeating_updated` (`-r`) and `batch_updated` (`-u`)
  for inspection.
- The CGI wrapper `fzupdate-cgi.py` drives `-r`/`-u` from the web interface; see
  [schedule-updating.md](../doc/schedule-updating.md).

---
Randal A. Koene, 20201126 (updated 20260717)
