# Formalizer tool: daywiz

The `daywiz` tool is a wizard tool that helps carry out a set of System recommended
steps throughout the day.

## Files

1. `daywiz.py`: The principal DayWiz script. Displays and provides interface to DayWiz.
2. `consumed.py`: Generates a helpful table of known foods for consumption entry.
3. `metrics.py`: Use this to load metrics data, parse it, and produce useful output.
4. `nutrition.py`: (Under construction. Intended to help with nutrition planning.)
5. `score.py`: Generate evaluated and scored output for the day and recent days.
6. `wiztable.py`: Contains the definition of the table on the left-hand side of the DayWiz page.
7. `calendar-info.py`: Pull Google Calendar data.
8. `convert_to_database.py`: Conversion from JSON data file to Formalizer database table.
9. `daydata_from_database.py`: Testing retrieval of a day's data from the Formalizer database table.
10. `daywiz-autodata-cgi.py`: CGI entry point to retrieve number of emails, numbe of checkboxes, upcoming calendar entries, and 24 hour emails received
11. `daywiz-autodata.py`: Retrieve number of emails, numbe of checkboxes, upcoming calendar entries, and 24 hour emails received.
12. `email-info-cgi.py`: CGI entry point to run `email-info.py`.
13. `email-info.py`: Retrieve number of emails unread and optionally the last 24 hours of emails received.
14. `inspect_data.py`: Inspect the data format and more for data stored in the Formalizer database table.
15. `daywiz-set.py`: Set the state of a Wiztable item for a day from a script (see "Setting values from scripts").
16. `daywiz-collect.py`: Collect values (e.g. open Nodes for `nodebalance`) and fill them in with `daywiz-set.py`.

Note that `daywiz_json.py` is like `daywiz.py`, but works with the JSON data source
and is now deprecated.

## Additional dependencies

1. `core/include/fznutrition.py`: Data file for known foods, categories, etc.
2. `core/include/fzhtmlpage.py`: HTML page builder template classes.

## Database storage

The default version of `daywiz.py` now uses the Formalizer database to store and
retrieve data.

The data is stored in multiple tables by datestamp key. The content of each table is
in the form of a JSON string containing the relevant data belonging to that table for
that particular day.

The nature of the content is transparent to the database, because it is within each
JSON construct.

Updating the variables included is relatively easy, because storing and retrieval is
done by datestamp key and involves a replacement of all the data in that cell.

The `daywiz.py` is careful to map available data to the up-to-date data structure
expected. Missing data is therefore handled gracefully.

When the data structure is updated in `daywiz.py` the new data structure will be used
and stored when new data is saved. Hence, old data formats will load without causing
an error and new formats are adopted smoothly. (See `get_day_data()`,
`get_wiz_data()` and `daypage_wiztable.merge_data()` in `daywiz.py`.)

Therefore, to update the Wiztable data structure, just update `wiztable.py`.

## Setting values from scripts

`daywiz.py` accepts `cmd=set` with the form fields `item`, `value`, and optionally
`date` (YYYY.MM.DD, default today) and `overwrite=true`. It responds with JSON whose
`status` is `set`, `skipped` (already filled in and no overwrite) or `error`. The update
goes through the same code as a change made on the page, and the stored value is read
back from the database to confirm it. An invalid date is an error, not today.

Other Python scripts in this directory can call `set_day_value()` directly, but only
when running as the web server user, because saving writes temporary files in
`/var/www/webdata/formalizer`. From the command line, use `daywiz-set.py`, which
calls `daywiz.py` through the web server:

```
daywiz-set.py nodebalance 3533 -d 2026.10.26
daywiz-collect.py              # collect and fill all items that have a collector
daywiz-collect.py -n           # only show the collected values
```

## Candidates for daywiz-collect

Wiztable items that could be filled in automatically by adding collectors to
`daywiz-collect.py`, from most to least ready. (Survey of 2026-10-26, based on the
line definitions in `wiztable.py` and the tools behind their buttons.)

### Ready now (a tool already produces the value)

| Item | Type | Source |
|---|---|---|
| `emailunread` | number | `daywiz-autodata.py` (via `email-info.py`), already run by the Sync button |
| `openchkbx` | number | `daywiz-autodata.py`: chunks with open checkboxes |
| `backup` | checkbox | `fzbackup.py` writes `~/.archive/postgres/formalizer-postgres-backup-<YYYYmmddHHMM>.gz`; one dated today means checked |

`daywiz-autodata.py` normally runs through fzserverpq (as the user with Gmail access).
A collector would either run it directly or read the `daywiz_autodata.json` it leaves
behind in `/var/www/webdata/formalizer`.

### From DayWiz's own data (the day's nutrition table, with times)

| Item | Type | Rule |
|---|---|---|
| `morningfast` | checkbox | No nutrition entries before a morning cutoff (to decide) |
| `lessthan300` | checkbox | Calories before the cutoff under 300 (`consumed_calories()` already sums them) |
| `fastafter8` | checkbox | No nutrition entries after 20:00 |
| `finalized` | checkbox | The previous day's DayWiz data meets a definition of "finalized" (e.g. `reviewscore` filled, or a minimum score) |
| `caffeine`, `alcohol` | number | Only if these are logged as foods in the nutrition table |

These can only be judged once the day is over, so a run late that evening or early the
next morning would fill the previous day with `-d`.

### From the Log and the Schedule (needs a rule)

| Item | Type | Possible rule |
|---|---|---|
| `log` | checkbox | Newest Log chunk is open, or closed within the last N minutes |
| `startedearly` | checkbox | At least 1 hour logged on priority Nodes by 10:00 |
| `firstwork` / `secondwork` | checkbox | At least 4 hours logged on work Nodes by 14:00 / after 14:00 |
| `challenge` | checkbox | Time logged today on a Node in the `challenges_open` NNL |
| `passedfixed` / `passedrepeat` | checkbox | No passed Fixed (or repeating) Nodes remain |
| `priority` | checkbox | Every Promise above some priority has a scheduled slot |
| `schedfits` | checkbox | Schedule's required time fits in the time available |
| `reviewscore` | number | Day review page (`fzloghtml-cgi.py?review=true`); where the score number is produced still needs checking |

"Priority Nodes" and "work Nodes" need a definition, e.g. a set of NNLs or the
`threads`/`week_main_goals` subtrees.

### Partly automatable

- `calsync`: a collector could list calendar events without a matching Node, but
  whether calendar and Schedule agree is a judgment call.
- `checkboxes`, `promises`, `milestones`, `decisions`, `variable`, `realistic`,
  `relations`: review steps. A collector could fill a supporting number (e.g. a count
  of overdue Promises) if number lines are added for that.

### Manual only

The morning ritual, health and body lines (`weight`, `bloodpressure`, `pushup*`,
`shower`, `teeth`, etc.), messaging catch-ups (Signal, WhatsApp, Discord, etc.), and
reflections (`lovedbody`, `lovedmind`, `visible`). `weight` and `bloodpressure` would
need a connected scale or blood-pressure monitor.

### Suggested order

1. `emailunread`, `openchkbx`, `backup`: a few lines each, no decisions needed.
2. `morningfast`, `lessthan300`, `fastafter8`: once the cutoffs and run time are chosen.
3. The Log-based items, once work and priority Nodes are defined.

Note that a checkbox set by a script means "the rule held", which is not quite "I did
it". A collector could instead fill a number line and leave the checkbox manual.

## More

See https://trello.com/c/JssbodOF .

Also, note that the metrics defined and calculated in the Formalizer component
`tools/system/metrics` are separate from those in DayWiz.

--
Randal A. Koene, 2023
