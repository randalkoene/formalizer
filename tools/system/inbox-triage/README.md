# inbox-triage

Sorts the unread mail in `rkoene@carboncopies.org` into categories, showing for
every message exactly why it landed where it did.

**Phase 1 is diagnostic only. It never changes your mailbox.**

## Why it cannot alter your inbox

1. The OAuth token is scoped `gmail.readonly`. Google rejects any mutation at the
   API level — a bug in this code physically cannot modify your mail.
2. No mutating call exists in the source. Verify it yourself:

   ```
   grep -nE "messages\.(modify|trash|delete|send)|batchModify|\.labels\(" *.py
   ```

   The only hits are the comments saying so.
3. `users.messages.get` does not clear the `UNREAD` label. Reading a message
   here leaves it bold in Gmail.
4. Everything written goes to `cache/` and `config.yaml`, both local.

Auth reuses the existing token at `~/.formalizer/config/daywiz/token.json`,
following the same pattern as `formalizer/tools/system/daywiz/email-info.py`.

## Two front ends

Same data, same scoring, same read-only guarantees:

```
./serve.py       # browser UI at http://127.0.0.1:8711
./triage.py …    # terminal CLI
```

## Browser UI

```
./serve.py                 # opens http://127.0.0.1:8711 in your browser
./serve.py --status        # is one running, and as which pid
./serve.py --stop          # stop whichever process holds the port
./serve.py --port 9000
./serve.py --no-browser
```

`Ctrl-C` stops a server running in the foreground. One started in the background
survives it — so if the port is taken, `--stop` is the way. Starting over an
existing instance prints the offending pid and command line rather than an
"address already in use" traceback.

Stopping loses nothing; all state is in `cache/`, `config.yaml` and
`corrections.yaml`. Write mode is the exception: a fresh server is always
read-only.

Binds to `127.0.0.1` only — this serves your mail, so it is not reachable from
the network.

**Triage** — three panes. Every message carries two independent verdicts: its
**category** (what kind of mail it is) and its **needs** (what it wants from
you — `reply`, `read`, or nothing). The rail leads with the two numbers that
matter — how much is waiting on a reply, and how much is worth reading — then
filters by category, then by tag for the things inside Bulk and Automated that
deserve a glance. Category rail with live counts and an inbox-composition
strip; message list with instant search, sort and the top scoring reasons inline;
detail pane with the full score waterfall, signal flags, thread and
correspondence history, body and raw headers.

**Dashboard** — what is in the inbox by category, how old the backlog is, score
distribution, arrivals by month, loudest senders, busiest scoring rules, and the
oldest mail still waiting on you. Bars are clickable: they jump into the filtered
list.

**Data** — everything that used to be terminal-only:

- **Sync** — also in the top bar (<kbd>s</kbd>). Lists what is unread right now,
  drops anything you handled in Gmail, pulls only what is new, re-scores. Seconds.
  This is the one to use after replying.
- **Full rebuild** and **Rebuild, re-probing senders** — with a live progress bar
  and the command's own log streamed into the page. Stoppable mid-run. A run shows
  three states — fetching, scoring (a sweeping bar, since there is no countable
  unit), then done with a green panel and a summary of what landed — so "still
  working" never looks like "stalled".
- **Rebuild config from history** (`seed-config`) — behind a confirmation, because
  it overwrites `config.yaml`. Your `corrections.yaml` is never touched.
- **What you have taught it** — every correction, with *forget* on each row.
- **Your other addresses** — add or remove forwarding aliases.

**Manual** — an in-app manual: what the three refresh options actually do and how
they differ, what every panel and control is for, how teaching works, and the
keyboard and URL reference. Press <kbd>m</kbd> or visit `#manual`.

**Weights** — every weight and threshold with its default, overrides highlighted.
Edit `config.yaml`, press **Re-score**, and the page recomputes from the local
cache. Re-scoring is pure local computation: no network call, no mailbox access.

Keyboard: <kbd>j</kbd>/<kbd>k</kbd> move, <kbd>/</kbd> search, <kbd>1</kbd>–<kbd>7</kbd>
pick a category, <kbd>0</kbd> all, <kbd>r</kbd> re-score, and <kbd>t</kbd> <kbd>g</kbd>
<kbd>w</kbd> <kbd>d</kbd> <kbd>m</kbd> for triage / dashboard / weights / data / manual.
URLs are shareable: `#dashboard`, `#weights`, `#data`, `#manual`, `#m93` opens message 93, and
`?theme=dark|light` pins the theme.

### Browser / CLI parity

Everything the CLI can do, the browser can do:

| CLI | In the browser |
|---|---|
| `fetch`, `fetch --force` | Data → the two Fetch buttons, with live progress |
| `seed-config --force` | Data → Rebuild config from history (confirmed) |
| `score` | **Re-score** in the top bar (also automatic after a fetch) |
| `list <category>` | Category rail; `--reasons` is shown inline on every row |
| `show <n>` | Detail pane (full body, scrollable — no `--lines` needed) |
| `explain <n>` | The score waterfall in the detail pane |
| `browse` | The app itself |
| `weights` | Weights tab |
| `teach <n> <kind>` | Teach panel; `--domain` is the scope selector |
| `teach --category` | "File this one message under" |
| `teach --note` | The note field beside the teach buttons |
| `teach --clear` | *clear* in the panel, or *forget* in Data |
| `taught` | Data → What you have taught it |
| `alias`, `alias --remove` | Data → Your other addresses |
| `--json` on any command | The JSON API itself: `/api/overview`, `/api/messages`, `/api/message/<n>`, `/api/weights`, `/api/corrections`, `/api/job` |

The server can also start with **no cache at all** — a fresh install can run its
first fetch from the browser.

### What changed about Gmail access

Adding Fetch to the browser necessarily means the web UI can now reach Gmail.
Two things keep that honest:

- `serve.py` still **never imports a Gmail client and never holds credentials**.
  `fetch` and `seed-config` run by spawning `triage.py` as a subprocess, so the
  browser and the terminal execute the same code path.
- Those reads remain bounded by the `gmail.readonly` scope, so the mailbox still
  cannot be modified from either front end.

### Suggested actions are shown but disabled

Each message lists the actions that would make sense for it — *Draft a reply*,
*Label: Grants*, *Archive*, *Unsubscribe*, *Report spam* — rendered greyed out
with a dashed border. They are deliberately inert in this phase; the token is
scoped `gmail.readonly`, so none of them could run even if wired up. They are
there so you can judge which ones are worth building.

The one button that does leave the app is **Open in Gmail**, and the UI says
plainly that Gmail will mark the message read — this tool will not.

### Colour

The category palette is not decorative. It was validated with a contrast and
colour-vision-deficiency checker in both light and dark mode: red → blue →
yellow → aqua → orange → violet clears the lightness band, chroma floor, CVD
separation, normal-vision and contrast gates in both. Orange (onboarding) is
kept away from yellow (directed) in the category order deliberately: that pair
is the palette's weak link, and the rail puts aqua and a text label between
them. Bulk and automated take a de-emphasis
grey rather than two more competing hues, so the fact that 86% of this inbox is
noise reads as grey at a glance. Every colour is paired with a text label, so no
information rests on colour alone.

## Teaching the classifier

When it gets something wrong, tell it. Corrections live in `corrections.yaml`,
which `seed-config` never regenerates, so what you teach survives a config
rebuild. Teaching changes how the **local cache** is scored — it never touches
your mailbox.

In the browser, every message has a **Teach the classifier** panel: pick a scope
(this sender / everyone at this domain / just this message) and press one of

| | Meaning |
|---|---|
| **Personal — never bulk** | real human correspondence; never filed as bulk or automated |
| **Always important** | always surface it, like a VIP |
| **This really is bulk** | force it into Bulk |
| **Ignore this sender** | noise you never want near the top |
| **I read this list** | a list or group you follow — tagged, counted under *To read*, still filed as Bulk |
| **Mute this list** | sorted to the bottom of Bulk — still filed, listed and counted |

From the CLI:

```
./triage.py teach 117 personal --note "my parents"
./triage.py teach 412 bulk --domain          # everyone at that domain
./triage.py teach 93 --category grants       # pin one message
./triage.py teach 412 important --list       # the list/group it arrived via
./triage.py teach 117 --clear
./triage.py taught                           # list every correction
```

Either way the cache is re-scored immediately and the CLI prints which messages
moved. The teach buttons are solid and live — deliberately unlike the dashed,
inert *Suggested actions* beside them, which cannot run in this phase.

### Forwarding aliases

If mail reaches you at another address that forwards here, say so once:

```
./triage.py alias randal.a.koene@gmail.com
```

or press **That address is me** in the browser. This matters more than it
sounds: 348 messages in this inbox (29%) are addressed to a forwarded alias, and
without declaring it they each take a `not_addressed` penalty and earn no
directedness credit at all — which is exactly how personal mail ends up in Bulk.
`seed-config` now proposes likely aliases under `me.aliases`.

## CLI usage

```
./triage.py seed-config    # once: draft config.yaml from your mail history
./triage.py sync           # seconds: drop handled mail, pull only what is new
./triage.py fetch          # full rebuild of the cache (minutes)
./triage.py score          # score offline and print the category summary
./triage.py list needs_reply
./triage.py show 12        # full message plus its score breakdown
./triage.py explain 12     # breakdown only
./triage.py browse         # interactive walk through categories
./triage.py weights        # every tunable weight and its current value
```

Add `--json` to `score`, `list`, `show`, or `explain` for machine-readable output.

`fetch` is the only command that touches the network. `score`, `list`, `show`,
`explain`, and `browse` all read the local cache — so re-scoring after a config
change is instant and free.

## Categories

| | Category | Meaning |
|---|---|---|
| 🔴 | `needs_reply` | Directed at you, live thread, or a pending question |
| 🟠 | `grants` | Touches an application, funder, or proposal |
| 🟡 | `directed` | For you or Carboncopies, no reply obviously pending |
| 🔵 | `threads` | Conversations you are in, no action implied |
| 🟢 | `onboarding` | Arrived via an onboarding group — glance, then keep for reference |
| ⚪ | `bulk` | Mailing lists, newsletters, announcements |
| ⚫ | `automated` | Machine-generated notifications and receipts |
| 🚫 | `spam` | Failed auth, impersonation, lookalike domains |

Grant relevance and spam suspicion override the numeric score when assigning a
bucket; within each bucket, messages sort by score descending.

## How scoring works

`scoring.py` runs nine rule groups over each message. Every rule that fires
records its name, its point contribution, and a human-readable detail — that
list is printed by `show` and `explain`, so no influence on a score is hidden.

**Directedness** — sole `To:` recipient, small named group, large group, `Cc:`
only, or not addressed to you at all (list delivery). Mail to a Carboncopies
role address forms its own tier. A personal salutation in the opening lines adds
weight.

**Thread continuity** — whether you have *sent* in the thread (the strongest
"someone is waiting on you" signal), whether this message replies directly to
one of your messages, and thread length. Derived from real thread metadata, not
from the `Re:` prefix.

**Grants** — matches against the `grants:` records in `config.yaml` (thread ids,
funder domains, keywords), known funder domains, and generic grant vocabulary.

**Reply expected** — questions addressed to you, imperative asks, deadline
language, calendar invites.

**Sender relationship** — how often you have written *to* this address, measured
from your own Sent mail. Stranger vs. regular correspondent is the sharpest
single discriminator available.

**VIP** — config-driven boost.

**Bulk** — `List-Id`, `List-Unsubscribe`, `Precedence: bulk`, `noreply@` senders,
Gmail's own Promotions/Updates/Forums categories. These demote; they never hide.

**Automated** — `Auto-Submitted` header, notification-style sender local-parts.

**Onboarding** — arrived through one of the group addresses in
`onboarding_addresses` (matched on `List-Id`, its address spelling, the
recipients, or `Sender:`). Intake mail carries every marker of a mailing list
and is none of it, so it escapes Bulk into its own category, is marked *read*
rather than *reply*, and is offered no trash. Anything you have taught — muting
the group, marking a sender bulk — still outranks it.

**Spam / phishing** — SPF/DKIM/DMARC failures parsed from `Authentication-Results`,
display-name-vs-From-domain mismatch, domains within edit distance 2 of one you
actually deal with, risky attachment extensions. Since `in:inbox` already
excludes Gmail's spam folder, this targets what slipped through.

**Age** — a curve, not a ramp. Unread mail gains weight for about a month (it has
been sitting), loses it again by four months, and becomes a penalty by a year,
floored at −12. Old mail is demoted, never hidden: every list splits at 120 days
with the older band collapsed behind its count.

**Tags** — glanceable subsets, mostly for finding the few things worth seeing
inside the noise: `important_list`, `travel`, `finance`, `security`, `calendar`.
Travel and the rest require transactional structure *and* the absence of
promotional apparatus — keyword matching alone finds twenty airline mailshots per
real itinerary.

## Tuning

1. Edit `config.yaml`: trim `vips:` to people who really are VIPs, delete grant
   entries that are not real applications, add keywords and funder domains.
2. Override any weight or threshold under `weights:` / `thresholds:`. Run
   `./triage.py weights` to see every key and whether you have overridden it.
3. Re-run `./triage.py score`. No API calls, so iterate as often as you like.

Use `./triage.py explain <n>` on messages that landed in the wrong bucket — the
breakdown tells you which weight to move.

## Files

| File | Purpose |
|---|---|
| `triage.py` | CLI |
| `gmail_client.py` | Read-only auth, paged listing, batched fetch |
| `store.py` | Payload normalization and the JSON cache |
| `scoring.py` | All rules, weights, and category assignment |
| `seed_config.py` | History scan that drafts `config.yaml` |
| `corrections.py` | Your taught corrections, stored in `corrections.yaml` |
| `viewer.py` | Terminal rendering and interactive browse |
| `serve.py` | Local web server for the browser UI (no Gmail access) |
| `web/` | Browser UI: `index.html`, `app.css`, `app.js` (no JS dependencies) |
| `selftest.py` | Offline checks for the scoring rules (`./selftest.py`) |
| `config.yaml` | Your tuning surface (regenerated by `seed-config`) |
| `corrections.yaml` | What you have taught it (never regenerated) |
| `cache/` | `messages.json`, `threads.json`, `sender_history.json`, `scores.json` |

## What the first run already showed

Run against 1209 unread messages:

| Category | Count | Share |
|---|---:|---:|
| Needs Reply | 92 | 8% |
| Grants & Funding | 12 | 1% |
| Directed / Org Business | 54 | 4% |
| Ongoing Threads | 5 | 0% |
| Bulk / Newsletters | 753 | 62% |
| Automated / Transactional | 291 | 24% |
| Probable Spam / Phishing | 2 | 0% |

Six things worth knowing, because they shaped the rules:

- **Gmail's `resultSizeEstimate` is not a count.** It reported ~201 unread when
  the real figure was 1209. Only paging the full result list is reliable.
- **Batched requests get rate-limited per sub-request.** Gmail's per-user ceiling
  is 250 quota units/sec and `messages.get` costs 5, so a 50-request batch lands
  exactly on the limit. Those failures are delivered to the batch *callback*, not
  raised, so they are silently lost unless retried explicitly. `gmail_client.py`
  now uses 25-request batches and re-submits failed keys across several rounds.
- **`dmarc=fail` on its own does not mean phishing.** Most of your list mail and
  everything forwarded through `onboarding@` / `website@` fails DMARC on the
  forwarding hop while `arc=pass` vouches for it. Treating that as a spam signal
  wrongly condemned 52 messages from real collaborators. The rule now ignores
  auth failures that ARC (or a passing SPF+DKIM pair) vouches for, and never
  condemns someone you have written to unless there is a hard signal — a
  lookalike domain or a hostile attachment.

- **A forwarded alias is invisible to the scorer unless you declare it.** 29% of
  this inbox is addressed to `randal.a.koene@gmail.com`. Until that was declared,
  every one of those messages scored `not_addressed` and earned no directedness
  credit, which is how mail from close family was filed as a newsletter.
- **Bulk must never be the fallback bucket.** It used to be: anything neither
  addressed to you nor part of a thread landed in "Bulk / Newsletters" even with
  zero bulk signals and Gmail's own `CATEGORY_PERSONAL` label. Now a message is
  only called bulk when there is actual evidence of bulk.
- **"Needs Reply" must mean something is being asked.** It used to fire on thread
  participation alone, which swallowed the Threads category whole.

The practical consequence: because `in:inbox` already excludes Gmail's spam
folder, the spam rule catches very little here. The real noise in your inbox is
**86% bulk and automated mail**, which is what the categories are for.

### A known soft spot

`direct_question` fires on any line containing a "?" and a second-person
pronoun, which matches 365 messages — including newsletters asking rhetorical
questions. It only matters where it combines with a high score and a direct
address, but it is the loosest rule in the set and the first one to tighten if
Needs Reply feels padded.

## The categorisation invariant

Every message in the cache is in **exactly one** category, always. Muting a list,
ignoring a sender and year-old mail all demote; none of them remove. Nothing is
dropped, nothing is invisible, and the per-category counts always sum to the
message total — `./selftest.py` asserts this against both synthetic cases and the
real cache, and `/api/overview` reports an `unscored` count that surfaces in the
UI if it ever stops holding.

This matters for what comes next: actions that modify Gmail will be driven off
these buckets, so a message falling out of all of them would be a message
silently exempt from whatever you chose to do.

Muting never files anything as **Spam**. That bucket means failed authentication,
impersonation or a lookalike domain; a newsletter you are tired of is not spam.

## Actions

The badge at the top left switches between **read-only** (the default, every time
the server starts) and **write mode** (armed for 15 minutes, then automatic).

Read-only is enforced, not promised: the write credential lives in its own file
(`~/.formalizer/config/inbox-triage/gmail-write-token.json`, outside the repo)
that is loaded *only* while write mode is armed. In
read-only mode the process holds no credential capable of mutation. Your existing
read-only daywiz token is untouched and still does all the reading.

One-time grant:

```
./triage.py authorize-write     # asks Google for gmail.modify
```

`gmail.modify` can mark read, archive, star, trash and report spam. It **cannot
delete permanently** — that needs a far broader scope this tool never requests —
so trashed mail stays recoverable in Gmail for ~30 days.

| Action | Effect | Reversible by |
|---|---|---|
| Reply in Gmail | opens the message in Gmail; no write scope needed | — |
| Mark read | removes `UNREAD` | marking unread |
| Archive | removes `INBOX` | All Mail |
| Star | adds `STARRED` | unstarring |
| Trash | `messages.trash` | Trash, ~30 days |
| Report spam | adds `SPAM` | Spam folder |

Which actions appear depends on the view. Categories where every message matters
get star / mark read / archive — **no trash**. Bulk and Automated add trash.
Spam offers trash and report spam. **"All messages" offers none**, so that
choosing an action follows from having looked at a category — and the server
enforces that, not just the buttons.

Every view that offers actions also offers **multi-select**: tick several rows
and act on them together. Batch never widens what is possible — trash is absent
from Needs Reply, Grants, Directed and Threads in bulk exactly as it is singly.

Every action is appended to `cache/actions.log` *before* the call is made.

**Replying is deliberately outside the app.** Pressing *Reply in Gmail* opens
Gmail in a new tab and puts a prompt in the app naming that message. Reply, come
back, press **Replied / read — remove it**, and that one message is checked
against Gmail and dropped from triage in about a second — so you settle each
reply as you go instead of batching five and losing track.

The prompt verifies rather than assumes: if Gmail still reports the message
unread in the inbox, it says so and changes nothing.

```
./triage.py sync-one 42    # the same check from the terminal
```

**Sync** in the top bar remains for everything else — mail read on your phone,
archived elsewhere, or newly arrived.

```
./triage.py act mark_read 12 13 14
./triage.py act trash 300 --dry-run
./triage.py actions
```

### Suggested actions that were dropped

These had no implementable Gmail API equivalent and were removed rather than left
as buttons that lie: **Snooze** and **Mute thread** (not exposed by the API at
all), **Block sender** and **Filter future ones** (need a settings scope this tool
deliberately does not request), and **Add to grant tracker** (no such thing
exists). **Unsubscribe** survives as a plain link to the sender's own
`List-Unsubscribe` URL.

## Still not implemented

Drafting or sending mail from inside the app — replying goes through Gmail on
purpose. Custom labels, and scheduled digests. `scoring.py` also carries an
`LLM_HOOK` slot for a future model-assisted pass.
