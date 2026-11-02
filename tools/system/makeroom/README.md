# makeroom: make room in the coming week

Urgent Nodes with unspecified or variable target dates (the UTD todo stack)
get pushed back too far when the week is full of repeating Nodes and VTD
Nodes that could wait. `makeroom-cgi.py` is a form that proposes making room:

- **Skip repeating Nodes.** An approved skip moves the Node to its next
  regular occurrence (`/fz/graph/nodes/<id>?skip=<n>`, as the skip button on
  Node pages does). Nodes listed in `skip_default` start out approved, all
  others start out declined.
- **Move VTD Nodes to later.** Each one is proposed for the same weekday and
  time `move_days` later (`/fz/graph/nodes/<id>?targetdate=<new>`). These
  start out declined, so that moving one is a single click. The new date can
  be changed in the form.

Only approved rows are carried out. **Dry run** shows the commands without
running them. No backup is made: this is routine Schedule work, like using
the skip button.

The window starts when the form is opened and covers the next `days` days
(or `?days=<n>`). Open it from the dashboard (**Make Room**) or at
`/cgi-bin/makeroom-cgi.py`.

## Configuration

`~/.formalizer/config/makeroom/makeroom.json`, linked into
`/var/www/webdata/formalizer/` by `make executables`:

| Variable | Meaning |
|---|---|
| `days` | Days to look ahead (default 7). |
| `skip_default` | Repeating Nodes whose skip starts out approved. |
| `skip_offer` | Repeating Nodes always offered (declined by default), even exact ones. |
| `hide` | Nodes never offered. |
| `list_fixed_repeating` | Offer every repeating Node with a fixed target date (default true). |
| `list_exact_repeating` | Offer every repeating Node with an exact target date, e.g. meetings (default false). |
| `move_tdprops` | Target date properties of Nodes offered for moving (default `["variable"]`). |
| `move_days` | How far a move goes (default 7). |
| `history_weeks` | How far back "skipped recently" looks (default 8). |

To add a Node, put its ID in the list in the JSON file. The form reads the
file each time it opens.

## History

Every change the form carries out is appended to
`/var/www/webdata/formalizer/makeroom-history.jsonl`. The form uses it to
show in how many recent weeks a Node was skipped. A Node that is skipped
every week is probably due to be replaced or changed rather than skipped
again.

---
Randal A. Koene, 2026
