# fzclaude plan: Claude's access to the Formalizer

This file tracks where we are in giving Claude access to the Formalizer, and
what comes next. For how the tools work, see `README.md`.

## Ground rules (all phases)

1. **No direct database access.** No `psql`, `pg_dump` or SQL. Claude only
   uses Formalizer tools, through `fzclaude`.
2. **Reading first, writing later.** Each step toward writing needs explicit
   approval.
3. **Everything is auditable.** `disclosure.jsonl` records what was sent to
   Anthropic, and `guard.log` records what was blocked.
4. **Enforced by the harness, not by Claude's good behavior.** The
   `fzclaude-guard.py` hook does the blocking.

## Phases

| Phase | What | Status |
|---|---|---|
| 0 | Orientation: a private map of how the scoped part of the Formalizer is organized | **next**, after the scope is tuned |
| 1 | Read-only `fzclaude` wrapper with include/exclude scope, disclosure log, token stats, paging, incremental `changes` | **done** (2026-10-14) |
| 1b | Browser interface (Claude Access on the dashboard): scope editor, preview, proposals, activity | **done** (2026-10-14) |
| 1c | Guard hook in `~/.claude/settings.json` (fails closed) | **done** (2026-10-14) |
| 1d | Installed copies that Claude cannot change; changes take effect only through `make fzclaude-install` | **done** (2026-10-14), first install pending |
| 2 | Proposals: Claude writes `fzclaude propose`, you review and apply (with a backup first) | **built, not yet used** |
| 3 | Sandbox: rehearse proposals on a copy of the database before applying them for real | not started |
| 4 | Narrow direct writes for specific low-risk operations, each enabled separately | not started, only if wanted |

## Where to continue

### Now: initial testing and tuning of the read-only stage

- [ ] Run `make fzclaude-install` (the first install), then restart
      fzclaude-web (⏻, then the dashboard button).
- [ ] Set the scope in **Scope rules**: `[CARBONCOPIES]` under Include, and
      `family` under Exclude (topic, or topic tree).
- [ ] Preview in **Use** (Map, Today, Log, Node). Check:
  - [ ] Is anything visible that shouldn't be? Add exclusions.
  - [ ] Is anything missing that should be visible? Check how many Nodes
        `shared_outside` hides (strict mode), and decide between keeping
        strict mode on and adding explicit excludes.
  - [ ] Are the page sizes, excerpt lengths and the 14-day Log window right?
- [ ] Commit `tools/system/fzclaude/` and the related `Makefile`,
      `config/fzserverpq/config.json` and `tools/system/top/index.json`
      changes.
- [ ] Consider a `permissions.deny` Read rule for `~/.formalizer/archive/**`
      (backups). The guard does not cover Claude's Read tool.
- [ ] Phase 0: Claude runs `fzclaude map` and writes the private orientation
      skill in `~/.claude/skills/formalizer-map/`, holding:
  - a factual part, generated from `fzclaude map`, which can be refreshed
  - your annotations, edited by hand
- [ ] Use it for real work for a while, e.g. working through Nodes on the
      day's Schedule. Then review **Activity**: what was sent, the tokens per
      command, and anything blocked.

### Tuning questions to answer from real use

- Which commands are expensive (`fzclaude stats`)? Do any need a terser
  format or smaller default pages?
- Is `changes` enough to catch up between sessions, or is a per-session
  snapshot needed?
- Should the disclosure log stay on permanently, or switch to summary-only
  once the process is trusted?
- Does `fzgraphhtml -t` list all Nodes with a topic? This matters for
  `exclude_topic_tree` without strict mode.

### Then: Phase 2 (proposals) in practice

- [ ] First real proposal, for example "Send Yasufumi the information for
      Nov. 24 and put the event and its preparation on my Schedule". This
      means creating an event Node at a fixed time and a preparation Node
      under it, using an `@name` reference.
- [ ] Check that `apply` creates the Nodes correctly, that the backup runs,
      and that the result is recorded in the proposal.
- [ ] Decide which step types are missing (e.g. NNL edits, `fzlog` chunk
      handling, skip/update of repeating Nodes).

### Later: Phase 3 (sandbox)

- A second `fzserverpq` on another port (`-p`), against a copy of the schema
  (`-s`).
- Clients find the server through `~/.formalizer/server_address`, so the
  sandbox needs its own Formalizer base directory or a client override.
- `fzclaude apply --sandbox` would run a proposal there first and show the
  result before the real apply.

### Later, possibly: Phase 4 and beyond

- Narrow direct writes, each type enabled separately in the config: e.g.
  appending a Log entry to a Node in scope, or creating Nodes under an
  approved superior. Never deletions. Always a backup first.
- Access from claude.ai web conversations, which would need something
  different: an MCP connector to fzclaude, with the same scope filter.
- Bringing in context from outside the Formalizer, e.g. the chat message a
  Node refers to. That needs its own scope and disclosure rules.

## Decisions made so far

- **Scope:** include rules (a Node with its subtree, or a topic) plus exclude
  rules (a single Node, a subtree, or a topic). Exclude always wins. Strict
  shared-dependency mode is on by default.
- **Config location:** `~/.formalizer/fzclaude/config.json`, deliberately
  outside the repo, because `~/.formalizer/config` is a symlink into git.
- **Orientation map:** a private user-level skill in `~/.claude/skills/`,
  never in the repo.
- **Read side uses only read-only tool modes.** No scratch NNLs. The scope is
  computed by walking the graph with `fzgraphhtml -n` (about 11 ms per Node).
- **Claude develops, you deploy:** what runs is the installed copy in
  `~/.formalizer/fzclaude/bin/`. Changes in the repository take effect only
  after you review and install them (`make fzclaude-install`). The guard
  protects the installed copies, the `~/.formalizer/bin` links and Claude
  Code's settings files.
- **Active Nodes by default:** Node listings show only active Nodes
  (`0 <= completion < 1`, as `Node::is_active()`) unless `--all` is given.
  `--milestones` gives nodeboard's "Milestones Only" view (topic 79). This
  saves tokens and keeps attention on what needs attention.
- **Web UI:** runs as you on `127.0.0.1:8712`, uses a per-start token, and is
  started through `CGIbg_run_as_user`, like Inbox Triage.

---
Randal A. Koene, 2026
