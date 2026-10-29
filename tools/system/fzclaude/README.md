# fzclaude - Scoped, audited access to the Formalizer for Claude

> **Status:** read-only stage, initial testing and tuning. **We are partway
> through the plan, and steps remain:** see **`PLAN.md`** for what is done and
> what to do next.
>
> **Day-to-day way of working** with Claude on Nodes (Log entries for source
> material, Claude Docs, handoffs, keeping chats small): see **`HOWTO.md`**.

`fzclaude` is the only route through which Claude (Claude Code) works with
Formalizer data:

- It reads through the regular Formalizer tools (`fzgraphhtml`, `fzloghtml`)
  in their read-only modes. It never touches the database, and it does not
  create or modify Named Node Lists, even temporarily.
- Everything it returns is filtered by include/exclude rules that only you
  edit.
- Every call made from within Claude Code is recorded, with the Node and Log
  entry IDs it returned and an estimated token count. That output is what
  gets sent to Anthropic.
- It never changes the Graph or Log on Claude's behalf. Claude writes a
  *proposal*, and you review and apply it.

`fzclaude-guard.py` is a Claude Code hook. It blocks Claude from going
around `fzclaude`, for example by calling other Formalizer tools or `psql`
directly, by reaching local web services (the Formalizer CGI scripts,
fzserverpq, fzclaude-web), or by editing the `fzclaude` configuration or
disclosure log.

Its rules are hard or soft:

- **Hard rules always block:**
  - the database programs
  - the Claude Code environment markers
  - commands that are yours to run (`apply`, `triage-apply`, `init`,
    installing)
  - running or loading a fzclaude other than the installed one
  - changes to the protected files and settings
- **Soft rules cover the rest:** running other Formalizer programs, reaching
  local web services, and interpreter code that *mentions* them. During
  development many soft hits are harmless, e.g. a syntax check of a script
  that contains a program's name. A soft hit:
  - goes through if it matches one of your exceptions
  - otherwise becomes a permission prompt for you to allow or deny ("ask")
  - is blocked outright if you turned asking off

Exceptions and asking are set in `~/.formalizer/fzclaude/guard-exceptions.json`,
which only you edit:

```json
{
  "ask": true,
  "exceptions": [
    {"tool": "Bash", "pattern": "fzgraph -h( 2>&1)?", "why": "help text"}
  ]
}
```

- `pattern` is a Python regular expression that must match the whole
  command (for Bash), or the JSON of the tool input (for other tools).
  Keep patterns narrow: a pattern such as `fzgraph .*` would let every
  fzgraph call through.
- Without the file, asking is on and there are no exceptions.
- If the file cannot be read or parsed, soft hits are blocked.
- Every decision (blocked, asked, excepted) is recorded in `guard.log` and
  shown in the Activity tab.

`fzclaude-web.py` is the browser interface to all of this (see below).

Where this project stands and what comes next is in `PLAN.md`.

## Installing changes: `make fzclaude-install`

Claude helps develop fzclaude in this repository. But the copies that
actually run (the scope filter, the guard hook, the web server) are
**installed copies** in `~/.formalizer/fzclaude/bin/`, and the guard keeps
Claude from changing them. A change Claude makes in the repository has no
effect until **you** install it:

```bash
cd ~/src/formalizer
git diff tools/system/fzclaude/     # optional: see what changed
make fzclaude-install
```

`make fzclaude-install` runs `fzclaude-install.sh`, which:

1. Shows the differences between the installed copies and the repository
   versions of `fzclaude.py`, `fzclaude-guard.py`, `fzclaude-web.py`,
   `fzclaude-web-start.py` and `web/`, and asks before installing anything.
2. Copies them to `~/.formalizer/fzclaude/bin/` and appends a line, with the
   git revision, to `~/.formalizer/fzclaude/install.log`.
3. Points `fzclaude`, `fzclaude-guard` and `fzclaude-web-start` in
   `~/.formalizer/bin` at the installed copies. These three are therefore not
   part of `make executables`.
4. Reminds you to restart fzclaude-web (⏻ in the page, then the dashboard
   button) if it is running.

It refuses to run from a Claude session, and the guard blocks Claude from
running it, from editing the installed copies or the `~/.formalizer/bin`
links, and from editing Claude Code settings files (where the hook is
registered). The guard also blocks Claude from running a repository copy of
`fzclaude`. Claude can test repository code only with `HOME` set to a
throwaway directory, where the Formalizer tools and data cannot be found.

Run `make fzclaude-install` once to begin with, and again whenever you want
a change to fzclaude to take effect.

## Setup

1. `make fzclaude-install` (see above). The CGI script for the dashboard
   button is installed by `make executables`.
2. Click **Claude Access** (under Research Tools on the dashboard), or run
   `fzclaude init`. Either way, the configuration goes in
   `~/.formalizer/fzclaude/config.json`. This is
   deliberately not in `~/.formalizer/config`, which is a symlink into this
   git repository: the rules name Nodes and topics.
3. Edit the configuration (see below), then look at what Claude would see.
   When you run these commands yourself, nothing is sent anywhere:

   ```bash
   fzclaude scope refresh
   fzclaude map
   fzclaude today
   fzclaude log --days 3
   ```

4. Install the guard in `~/.claude/settings.json` so it applies in every
   project:

   ```json
   "hooks": {
     "PreToolUse": [
       {
         "matcher": "Bash|Edit|Write|NotebookEdit|WebFetch|mcp__.*",
         "hooks": [ {
           "type": "command",
           "command": "G=/home/randalk/.formalizer/fzclaude/bin/fzclaude-guard.py; [ -f \"$G\" ] || G=/home/randalk/.formalizer/bin/fzclaude-guard.py; exec python3 \"$G\"",
           "timeout": 10,
           "onFailure": "block"
         } ]
       }
     ]
   }
   ```

   This runs the installed guard, or the repository copy if nothing is
   installed yet. With `"onFailure": "block"`, a guard that crashes or times
   out blocks the tool call instead of letting it through. `/hooks` in Claude
   Code shows or disables the hook.

   The guard does not cover Claude's Read tool. To keep Claude from reading
   files that may contain Formalizer data, add `permissions.deny` rules, e.g.
   `"Read(//home/randalk/.formalizer/archive/**)"`.

## The web interface

`fzclaude-web.py` serves a page on `http://127.0.0.1:8712/`. It is started
from the dashboard's **Claude Access** button, in the same way as Inbox
Triage: `fzclaude-web-cgi.py` asks fzserverpq to run `fzclaude-web-start.py`
as you, through `CGIbg_run_as_user`. That needs `fzclaude-web-start.py` in
the fzserverpq `predefined_CGIbg` list, followed by an fzserverpq restart.
It stops itself after 3 idle hours, or with the ⏻ button.

The page has four tabs:

- **Use** runs any read command and shows exactly what Claude would receive,
  with an estimated token count. Clicking a Node ID shows that Node. Pages
  are previewed with `changes --no-mark`, so previewing never moves Claude's
  baseline.
- **Settings** is the configuration editor: the scope rules and the other
  settings (Checkboxes, Launching Claude, output sizes). Every setting is explained in
  place, Node IDs show their descriptions, and topics are suggested from
  your topic list. Saving validates the configuration, keeps the previous
  version in `config-history/`, rebuilds the scope, and shows the result.
  Earlier versions can be loaded back.
- **Proposals** lists Claude's proposals with each step's reason and exact
  command. You can apply a proposal (with or without a backup first) or
  decline it.
- **Checkboxes** shows Claude's suggestions for open checkboxes (see
  "Checkbox triage" below) as a form, and processes your choices.
  - **Start in Claude** opens a Claude Code session that starts on the
    request for the next batch.
  - **Resume that session** reopens the session that made a triage, in the
    directory where it was made.

  Both open a tab in a herdr workspace (`launch_workspace`, default
  `fzclaude`), in `launch_dir` (default: your home directory). They need
  the herdr server to be running; the tab says so when it isn't. Run `herdr`
  in a terminal to see the session. These buttons can only start requests
  defined in `fzclaude-web.py` (`LAUNCH_REQUESTS`); the page cannot send any
  other command or request text.
- **Activity** shows what was sent to Claude per session and per call, with
  the Nodes and Log entries each call returned. It also shows token costs and
  everything the guard stopped (blocked, asked about, or let through by an
  exception).

The web server uses the same code paths as the command line: reads run
`fzclaude <command>`, saving uses fzclaude's validation, and applying runs
`fzclaude apply --yes`. Its safeguards:

- It listens on 127.0.0.1 only and checks the Host header.
- A random token, created at each start, is required on every API request,
  so other web pages in your browser cannot use the API.
- Claude Code environment markers are stripped from every call it makes.
- The guard keeps Claude's Bash, WebFetch and browser tools away from it.

## Visibility rules

| Rule | Effect |
|---|---|
| `include: [<node-id>]` | The Node and its full dependency subtree are visible. |
| `include_topic: [<tag>]` | Nodes with the topic are visible. |
| `include_topic_tree: [<tag>]` | Nodes with the topic, and their subtrees, are visible. |
| `exclude: [<node-id>]` | Hides that one Node. |
| `exclude_tree: [<node-id>]` | Hides the Node and its subtree. |
| `exclude_topic: [<tag>]` | Hides Nodes that have the topic. |
| `exclude_topic_tree: [<tag>]` | Hides Nodes that have the topic, and their subtrees. |
| `strict_shared: true` | Also hides a Node that has a superior outside the included set (it is shared with something you did not include). Include roots are exempt. |

Any exclude beats any include. Log chunks and entries follow the Nodes they
belong to: an entry is visible only if its own Node (or, if it has none, its
chunk's Node) is visible.

Unknown configuration variables, malformed Node IDs, include Nodes that
cannot be found, and exclusion subtrees that cannot be fully read all stop
`fzclaude` from building a scope. It refuses rather than guessing.

The scope is cached in `~/.formalizer/fzclaude/scope.json` and is rebuilt
after `scope_max_age_minutes` or whenever the rules change. A Node created
after the last rebuild stays invisible until the next one.

`checkboxes_all` (default off) and `checkboxes_since` (default 20240401)
control the `checkboxes` command (see below). They do not change the scope.

Other configuration variables: `max_nodes`, `max_log_entries` (page sizes),
`log_days_default` (default Log window), `excerpt_chars`, `entry_chars`,
`node_text_chars` (truncation), `show_hidden_counts` (whether output says
how many items were withheld), `scope_max_nodes` (safety cap on the walk),
`chars_per_token` (token estimate), and `workers` (parallel `fzgraphhtml`
calls).

## Commands

Reading (filtered, paged, and recorded when run by Claude).

Node listings (`map`, `subtree`, `search`, `nnl`, and the dependencies in
`node`) show only **active** Nodes by default, as `Node::is_active()`
defines them: completion is at least 0 and below 1. Each listing says how
many inactive Nodes it left out, and `--all` includes them. With
`--milestones`, only Milestones are shown (Nodes with topic
`milestone_topic_id`, 79 by default, as in nodeboard's "Milestones Only").
They are nested under their nearest Milestone ancestor. Every Node line
gives its status in words (`40%`, `done`, `obsolete`, `replaced`,
`done-differently`, `no-longer-possible`) and marks Milestones. `today` and
`log` are not filtered this way.

- `scope [status|refresh]`: counts of visible and hidden Nodes, and the roots.
- `map [--depth 2] [--width 25]`: an outline of the visible Graph with
  subtree sizes. This is the source for Claude's orientation notes.
- `today [--days N]`: Schedule entries in scope (same data as `schedule`).
- `node <id> [--full]`: one Node, its visible superiors and dependencies,
  and its description.
- `context <id> [--log 10] [--links 20] [--full]`: one call with what is
  needed to start working on a Node:
  - the Node and its description
  - its path up through visible superiors to the nearest Milestone(s)
  - its active dependencies
  - links found in its description and in its whole Log history, with Claude
    Docs/artifacts marked
  - its most recent Log entries

  A Node's Log (here and in `log --node`) includes entries that belong to the
  Node but sit in another Node's chunk. `fzlog -e -n` adds them that way
  while another chunk is open, and so does every `link` proposal. They come
  from two sources:
  - **`fzloghtml -n`**, like the Node pages: the `histories` cache table,
    rebuilt daily by `local_node_histories_refresh.py`.
  - **A local scan of the last `node_log_scan_days`** (default 60), for
    entries added since the last refresh. `fzlog -e` does not update the
    cache. Only the matching entries are returned, and duplicates are
    removed.
- `subtree <id> [--depth 3] [--all] [--milestones]`
- `search <text> [--regex] [--all] [--milestones]`: the search runs inside
  fzclaude, so only matches are returned.
- `log [--days N | --since T] [--until T] [--node id] [--search text] [--count] [--full]`
- `checkboxes [--since YYYYmmdd[HHMM]] [--entry] [--count]`: open (unchecked)
  checkboxes in the Log since `checkboxes_since`, grouped by Log entry. Each
  one is named `[<entry-id>:<n>]`, the n-th open checkbox in that entry.
  `--entry` adds the entry's text for context. With `checkboxes_all` off, only
  entries whose Node is in scope are shown. With it on, every open checkbox
  is shown except those in entries of Nodes hidden by an exclude rule. For
  entries outside scope, only the entry, its checkboxes, and the Node's ID and
  a short excerpt are shown. `node`, `search`, etc. still do not show that
  Node.
- `topics`, `nnls`, `nnl <name>`
- `changes [--no-mark]`: new or changed Nodes and new Log entries since the
  previous `changes`. This is how Claude catches up without rereading
  everything.

Accounting:

- `disclosures [--since YYYYmmdd] [--session id] [--ids]`: what was sent to
  Claude, per session.
- `stats`: estimated tokens per call, for each command.

Proposals:

- `propose <file.json>`: Claude saves a proposal. It is validated against
  the current scope.
- `link <id> <url> [--text "what it is"]`: a one-step proposal for a Log
  entry on the Node, recording a document or other work product (e.g. a
  Claude Doc started for the Node). `context` then finds it again.
- `note <id> --text "..."`: a one-step proposal for a plain Log entry, e.g.
  the handoff at the end of a session. See `HOWTO.md`.
- `proposals`, `show <id>`
- `apply <id> [--no-backup] [--decline]`: **you only**. It validates the
  proposal again, shows it, asks for `yes`, runs `fzbackup.py`, and then runs
  the steps in order, stopping at the first failure.
- `triage <file.json>`: Claude saves suggestions for open checkboxes (see
  "Checkbox triage").
- `triage-apply <id> --decisions <file> [--dry-run] [--no-backup] [--yes]`:
  **you only**, normally run by the Checkboxes form.

## Checkbox triage

How to ask for one, and what Claude does step by step, is in `HOWTO.md`.

Claude reads `fzclaude checkboxes`, looks for suitable superiors (`search`,
`node`, `subtree`) and for related Nodes in the next two weeks (`today --days
14`), and saves its suggestions:

```json
{
  "title": "Open checkboxes, first batch",
  "items": [
    { "ref": "202610200900.1:1", "checkbox": "Ask Ken about budget",
      "suggest": "new", "why": "Belongs with the budget review",
      "superiors": [ {"id": "20260101090000.1", "why": "Budget review Node"} ],
      "node_text": "Ask Ken about the Q1 budget", "hours": 0.5,
      "targetdate": "202611101700", "td_prop": "variable", "topics": "" },
    { "ref": "202610200900.1:2", "checkbox": "Send slides",
      "suggest": "obsolete", "why": "The event was in March" }
  ]
}
```

`suggest` is one of `new`, `link` (it is already covered by an existing
Node, listed in `link` like `superiors`), `checked`, `obsolete`, or `leave`.
`other` may hold a suggestion for handling it differently by hand. Saving
checks that every checkbox is still open and visible to Claude, that the
text matches, and that every candidate Node is in scope.

In the **Checkboxes** tab, each checkbox starts out set to Claude's
suggestion. You can change the action, the superior (or type in any Node
ID), the description, hours, target date and property, and topics. With
**Dry run** ticked (the default), processing shows exactly what would
happen and changes nothing. Untick it to process for real:

1. A backup with `fzbackup.py` (unless you untick it).
2. For each Log entry: Nodes are made with `fzgraph -M node`, and then the
   entry is rewritten with `fzlog -r` (keeping the entry's own Node). Each
   handled `<input type="checkbox">` becomes `<input type="checkbox"
   checked>`, followed by `[ <node-id> ]` (new or linked Node) or
   `[ obsolete ]`, as "Use Selected" on the Log entry edit page does.
3. A checkbox whose text has changed since the triage is left alone.
4. The result says how many checkboxes were handled. The triage records the
   results, so handled checkboxes show as done and the rest can be
   processed later.

## Proposal format

```json
{
  "title": "Nov 24 event and preparation",
  "context_node": "20261001090000.1",
  "steps": [
    { "name": "event", "why": "The event itself, at its fixed time",
      "argv": ["fzgraph", "-M", "node", "-S", "20261001090000.1", "-T", "Event ...",
               "-t", "202611241000", "-p", "fixed", "-H", "1.5"] },
    { "why": "Preparation, due two days before",
      "argv": ["fzgraph", "-M", "node", "-S", "@event", "-T", "Prepare ...",
               "-t", "202611220900", "-p", "variable", "-H", "3"] }
  ]
}
```

`@name` refers to a Node created by an earlier step. Its new ID is read from
fzgraph's `New Node:` output when the proposal is applied.

Allowed steps:

- `fzgraph -M node` (requires `-S` and `-T`, no `-q`)
- `fzgraph -M edges`
- `fzgraph -C` with completion, required, targetdate, skip, topics, or
  superiors/dependencies add/remove requests
- `fzedit -M`
- `fzlog -e -n <id> -T`, `fzlog -c <id>`, `fzlog -C`

Every Node a step refers to must be in scope, and excluded topics cannot be
added. Text has to be given inline with `-T`, so you see all of it when you
review the proposal.

## Files

All of these are in `~/.formalizer/fzclaude/`:

- `config.json`: the rules (yours).
- `scope.json`: the cached visible set, holding structure and hashes but no
  text.
- `snapshot.json`: the baseline for `changes`.
- `disclosure.jsonl`: one line per call.
- `proposals/`: proposals and their results.
- `triage/`: checkbox triages, with your choices' results.
- `tmp/`: the latest choices sent from the Checkboxes form, and entry text
  while it is being rewritten.
- `guard.log`: commands the guard blocked, asked about, or let through by an
  exception.
- `guard-exceptions.json`: your guard exceptions, and whether soft hits are
  asked about (yours; optional).
- `config-history/`: earlier configurations, saved by the web interface.
- `web.log`: output of fzclaude-web.
- `bin/`: the installed copies of fzclaude, the guard and fzclaude-web.
- `install.log`: when each version was installed, and from which git
  revision.

## Limitations

- The guard does not see computer-use clicks inside a browser window. Keep
  computer use away from the fzclaude page.
- The guard checks command text. It is a tripwire, not a sandbox, and a
  sufficiently indirect command could get past it.
- Token counts are estimates (characters / `chars_per_token`). Exact counts
  would require sending the text to Anthropic's API.
- Output in Claude's context is re-sent with every later request in that
  session. The disclosure log records what was disclosed and to which
  session, not each re-send.
- `fzgraphhtml -t` may not list every Node with a topic. Strict mode covers
  the gap for `exclude_topic_tree`.

---
Randal A. Koene, 2026
