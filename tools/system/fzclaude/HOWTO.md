# How to work with Claude on Formalizer Nodes

This is the day-to-day way of working with Claude (Claude Code) and `fzclaude`.
`README.md` describes the tools, and `PLAN.md` says where the project stands.

## The principle: the state of the work lives outside the chat

A chat's context window fills up and eventually gets summarized or cleared. So
nothing important should live only in the chat. Everything needed to pick up a
piece of work is kept where any new chat can find it cheaply:

| What | Where it lives | How a new chat gets it |
|---|---|---|
| Source material (a request, requirements, a chat message) | Pasted into a **Log entry** of the Node | `fzclaude context <id>` (long entries: `fzclaude log --node <id> --full`) |
| Progress, decisions, what is open | A **handoff Log entry** at the end of each session | `fzclaude context <id>` |
| Work products (an abstract, a letter, a plan) | A **Claude Doc**, linked from the Node's Log | `context` lists the link; Claude opens the doc |
| How your Formalizer is organized | The **`formalizer-map` skill** (`~/.claude/skills/`) | Loads by itself when relevant |
| The access rules | `~/.claude/CLAUDE.md`, the guard hook, the scope rules | Always active |
| Where fzclaude development stands | `PLAN.md` | Claude reads it when asked |

With that in place, a new chat needs only one sentence: *"Let's work on Node
20261008094529.1"*. One `fzclaude context` call (about 1,000 to 2,000 tokens)
brings back the Node, its Milestone, its Log with the source material and the
last handoff, and the links to its documents.

## Working on a Node, step by step

1. **Before starting** (optional, but it saves the most time): paste the
   source material into a Log entry for the Node, e.g. the chat message in
   which someone describes what they need.
2. **Start a chat** and name the Node. Claude runs `fzclaude context <id>` and
   reads long entries in full.
3. **Work products go in a Claude Doc.** Claude creates it, and you can edit
   or comment in the browser or here. The doc header lists the related Node
   IDs, but never copies Node or Log content: a doc persists on claude.ai.
4. **Claude proposes recording the doc** with `fzclaude link <id> <url>`. You
   apply it (Claude Access, Proposals tab).
5. **At the end of the session, Claude proposes a handoff** with
   `fzclaude note <id> --text "..."`: what was done, what is open, where to
   continue, and the command to resume the session. You apply it.
6. **Changes to the Formalizer itself** (new Nodes, target dates, completion)
   are always proposals that you review and apply.

## Triaging open checkboxes

You ask: *"Triage my open checkboxes (a batch of 25)."* Claude makes
suggestions, and you decide in the **Checkboxes** tab of fzclaude-web. Claude
does this in batches, so that you can process one batch while Claude
prepares the next.

What Claude does (the triage file format is in `README.md`, "Checkbox
triage"):

1. `fzclaude checkboxes --count`, then `fzclaude checkboxes --max 25`. The
   list starts with the oldest entries. Checkboxes from a batch you haven't
   processed yet are still open, so the next batch starts after them
   (`--offset`). Add `--entry` when a checkbox line only makes sense with the
   rest of its entry.
2. `fzclaude today --days 14` once, for the Nodes coming up.
3. For each checkbox, decide what to suggest:
   - **`obsolete`**: it is clearly outdated (an event that has passed,
     something since done another way).
   - **`link`**: an existing Node already covers it.
   - **`new`**: otherwise, if it is something to do.
   - **`leave`**: when unsure. **`other`** holds an idea for handling it
     differently by hand.

   For `new` and `link`, it finds 1 to 3 candidate Nodes with `search`,
   checked with `node`. Candidates must be in scope.
4. For `new`:
   - **Description:** clean up the checkbox text into a Node description.
   - **Hours:** an estimate (0.5 when there is nothing to go on).
   - **Target date:** if it relates to a Node in the next 14 days, set it
     before that Node's target date. Otherwise set it after those 14 days,
     spread out rather than all on one day.
   - **`td_prop`:** `variable`, and `fixed` only for a real deadline.
   - **`why`:** one line, which the form shows next to the suggestion.
5. Claude writes the JSON to its scratchpad and runs `fzclaude triage <file>`.
   If it is rejected, it fixes the errors and saves again. Then it tells you
   how many checkboxes are in the triage, and what it was unsure about.

## Keeping the context window manageable

- **One chat per Node**, or per small cluster of closely related Nodes. When
  switching to unrelated work, use `/clear` or start a new `claude`. Don't
  carry one Node's material into another Node's work.
- **Keep development and Node work apart.** Building fzclaude (or other
  Formalizer tools) happens in its own sessions, in `~/src/formalizer`. Node
  work can start in any directory.
- **Read narrowly.** `context`, `node`, `log --node` and `search` are cheap.
  `map`, `--all` and long Log windows are for orientation, not routine.
- **`/compact`** when moving to a different part of the same task, and
  **`/context`** to see how full the window is.
- **Resume or start fresh?** Resume (`claude --resume <session-id>`, which the
  handoff entry records) when a session was in the middle of something on the
  same Node. Otherwise start fresh: `context` plus the handoff is cheaper than
  an old transcript.
- **Remote Control** (`/remote-control`) belongs to one session. Start it again
  in each new chat that you want to continue in the browser.

## What reaches Anthropic

Everything Claude reads through `fzclaude` is logged (Claude Access, Activity
tab). The orientation skill and Claude Docs are sent when they load, but outside
that log. Keep Node and Log details out of docs, and docs to the work product
and Node IDs.
