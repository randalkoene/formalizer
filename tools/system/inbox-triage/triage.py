#!/usr/bin/env python3
"""Unread-inbox triage for rkoene@carboncopies.org.

READ-ONLY. The OAuth scope is gmail.readonly, so Google rejects any attempt to
change the mailbox. There are no modify / batchModify / trash / delete / labels
/ send calls anywhere in this codebase, and fetching a message does not clear
its UNREAD flag. Everything this tool writes goes to inbox-triage/cache/ and
config.yaml.

  triage.py seed-config   draft config.yaml from your mail history
  triage.py sync          fast: drop handled mail, pull only what is new
  triage.py fetch         full rebuild of the cache from scratch
  triage.py score         re-score from the cache (offline) and show categories
  triage.py list <cat>    list one category
  triage.py show <n>      full message plus score breakdown
  triage.py explain <n>   score breakdown only
  triage.py browse        interactive walk through the categories
  triage.py weights       show every tunable weight and its current value
  triage.py teach <n>     teach the classifier about a sender, domain or message
  triage.py taught        list everything you have taught it
  triage.py alias <addr>  declare another address of yours that forwards here

  triage.py authorize-write   grant the write scope (opens a browser, one time)
  triage.py act <what> <n>... mark read / archive / star / trash / report spam
  triage.py actions           show what has been done to your mailbox
"""

import argparse
import json
import os
import sys

import actions
import corrections
import gmail_client
import scoring
import seed_config
import store
import viewer
from scoring import CATEGORY_ORDER

UNREAD_QUERY = "in:inbox is:unread"


def _progress(done, total, label):
    sys.stderr.write("\r  %s: %d/%d   " % (label, done, total))
    sys.stderr.flush()
    if done >= total:
        sys.stderr.write("\n")


def _report_errors(errors, noun):
    """Surface why anything was dropped, rather than only how much."""
    if not errors:
        return
    reasons = {}
    for _key, exc in errors:
        reasons[str(exc)[:120]] = reasons.get(str(exc)[:120], 0) + 1
    print("  WARNING: %d %s could not be fetched after retries:" % (len(errors), noun),
          file=sys.stderr)
    for reason, count in sorted(reasons.items(), key=lambda kv: -kv[1])[:3]:
        print("    %dx %s" % (count, reason), file=sys.stderr)


def _die(msg):
    print(msg, file=sys.stderr)
    sys.exit(1)


def _load_cached():
    messages = store.load_json(store.MESSAGES_FILE)
    if messages is None:
        _die("No cache yet. Run:  ./triage.py fetch")
    return (
        messages,
        store.load_json(store.THREADS_FILE, {}),
        store.load_json(store.SENDERS_FILE, {}),
    )


def _index():
    messages, _, _ = _load_cached()
    scores = store.load_json(store.SCORES_FILE)
    if scores is None:
        _die("No scores yet. Run:  ./triage.py score")
    return viewer.build_index(messages, scores)


# ------------------------------------------------------------------ commands


def cmd_seed_config(args):
    if os.path.exists(seed_config.CONFIG_FILE) and not args.force:
        _die(
            "%s already exists. Re-run with --force to overwrite it "
            "(your edits would be lost)." % seed_config.CONFIG_FILE
        )

    service = gmail_client.get_service()
    profile = gmail_client.get_profile(service)
    me = profile["emailAddress"]
    print("Account: %s" % me)

    findings = seed_config.scan(service, me, progress=_progress)
    with open(seed_config.CONFIG_FILE, "w") as fh:
        fh.write(seed_config.render(findings))

    print("\nScanned %d grant candidates, %d sent, %d inbox messages."
          % (findings["scanned"]["grant_candidates"],
             findings["scanned"]["sent"],
             findings["scanned"]["inbox"]))
    print("Proposed: %d grant conversations, %d VIPs, %d org addresses."
          % (len(findings["grants"]), len(findings["vips"]),
             len(findings["org_addresses"])))
    print("\nWrote %s — review and edit it, then run ./triage.py fetch"
          % seed_config.CONFIG_FILE)


def cmd_fetch(args):
    service = gmail_client.get_service()
    profile = gmail_client.get_profile(service)
    me = profile["emailAddress"]
    print("Account: %s (read-only)" % me)

    print("Listing unread inbox mail…")
    pairs = gmail_client.list_query_ids(service, UNREAD_QUERY)
    ids = [i for i, _ in pairs]
    print("  %d unread messages" % len(ids))
    if not ids:
        return

    print("Fetching messages…")
    raw_msgs, errors = gmail_client.fetch_messages(service, ids, progress=_progress)
    _report_errors(errors, "message(s)")

    messages = [store.normalize(raw) for raw in raw_msgs.values()]
    messages.sort(key=lambda m: -m["internalDate"])
    store.save_json(store.MESSAGES_FILE, messages)

    thread_ids = sorted({m["threadId"] for m in messages if m["threadId"]})
    print("Fetching thread context for %d threads…" % len(thread_ids))
    raw_threads, t_errors = gmail_client.fetch_threads(
        service, thread_ids, progress=_progress
    )
    threads = {
        tid: store.normalize_thread(raw, me) for tid, raw in raw_threads.items()
    }
    store.save_json(store.THREADS_FILE, threads)
    _report_errors(t_errors, "thread(s)")

    known = store.load_json(store.SENDERS_FILE, {}) if not args.force else {}
    senders = sorted({m["from_addr"] for m in messages if m["from_addr"]})
    todo = [s for s in senders if s not in known]
    print("Probing correspondence history for %d sender(s)…" % len(todo))
    if todo:
        probed, s_errors = gmail_client.probe_correspondence(
            service, todo, progress=_progress
        )
        known.update(probed)
        _report_errors(s_errors, "sender probe(s)")
    store.save_json(store.SENDERS_FILE, known)

    print("\nCached %d messages, %d threads, %d senders in %s"
          % (len(messages), len(threads), len(known), store.CACHE_DIR))
    print("Nothing in your mailbox was modified. Next:  ./triage.py score")


def cmd_sync(args):
    """Reconcile with Gmail cheaply.

    Anything you handled elsewhere -- replied to in Gmail, read on your phone,
    archived -- is no longer unread-in-inbox, so it simply leaves. Only genuinely
    new messages are downloaded. One listing plus the new bodies, rather than
    re-fetching everything.
    """
    service = gmail_client.get_service()
    print("Account: %s (read-only)" % gmail_client.get_profile(service)["emailAddress"])
    print("Listing unread inbox mail…")
    current = [i for i, _ in gmail_client.list_query_ids(service, UNREAD_QUERY)]
    current_set = set(current)

    messages = store.load_json(store.MESSAGES_FILE)
    if messages is None:
        _die("No cache yet. Run:  ./triage.py fetch")
    cached = {m["id"]: m for m in messages}

    gone = [mid for mid in cached if mid not in current_set]
    fresh = [mid for mid in current if mid not in cached]
    print("  %d unread in Gmail now" % len(current))
    print("  %d no longer unread here — handled in Gmail or elsewhere" % len(gone))
    print("  %d new" % len(fresh))

    if not gone and not fresh:
        print("\nAlready in sync. Nothing to do.")
        return

    kept = [m for mid, m in cached.items() if mid in current_set]

    if fresh:
        print("Fetching the new ones…")
        raw, errors = gmail_client.fetch_messages(service, fresh, progress=_progress)
        _report_errors(errors, "message(s)")
        kept.extend(store.normalize(r) for r in raw.values())

    kept.sort(key=lambda m: -m["internalDate"])
    store.save_json(store.MESSAGES_FILE, kept)

    threads = store.load_json(store.THREADS_FILE, {}) or {}
    senders = store.load_json(store.SENDERS_FILE, {}) or {}

    # A reply of yours changes a thread, so refresh the threads of anything new
    # as well as any thread we have never seen.
    touched = {m["threadId"] for m in kept if m["id"] in set(fresh)}
    missing = {m["threadId"] for m in kept if m["threadId"] not in threads}
    want = sorted(t for t in (touched | missing) if t)
    if want:
        print("Refreshing %d thread(s)…" % len(want))
        me = gmail_client.get_profile(service)["emailAddress"]
        raw_threads, t_errors = gmail_client.fetch_threads(service, want, progress=_progress)
        for tid, raw in raw_threads.items():
            threads[tid] = store.normalize_thread(raw, me)
        _report_errors(t_errors, "thread(s)")
    # Forget threads nothing refers to any more.
    alive = {m["threadId"] for m in kept}
    threads = {t: v for t, v in threads.items() if t in alive}
    store.save_json(store.THREADS_FILE, threads)

    todo = sorted({m["from_addr"] for m in kept if m["from_addr"]} - set(senders))
    if todo:
        print("Probing %d new sender(s)…" % len(todo))
        probed, s_errors = gmail_client.probe_correspondence(service, todo, progress=_progress)
        senders.update(probed)
        _report_errors(s_errors, "sender probe(s)")
    store.save_json(store.SENDERS_FILE, senders)

    cfg = scoring.load_config()
    store.save_json(store.SCORES_FILE, scoring.score_all(kept, threads, senders, cfg))
    print("\n%d messages in triage (was %d). Nothing in your mailbox was modified."
          % (len(kept), len(messages)))


def cmd_sync_one(args):
    """Check one message against Gmail and drop it if it is no longer waiting.

    Used right after replying in Gmail. Cheaper than a full sync, and it asks
    Gmail rather than taking anyone's word for it: if the message is somehow
    still unread in the inbox, it says so and changes nothing.
    """
    ordered, numbered, _ = _index()
    entry = _entry_or_die(numbered, args.n)
    msg = entry["msg"]

    service = gmail_client.get_service()
    try:
        live = service.users().messages().get(
            userId="me", id=msg["id"], format="minimal").execute()
        labels = live.get("labelIds", [])
        handled = not ("UNREAD" in labels and "INBOX" in labels)
    except Exception as exc:
        # A 404 means it is gone entirely, which certainly counts as handled.
        if "404" in str(exc):
            labels, handled = [], True
        else:
            raise

    result = {"n": args.n, "id": msg["id"], "labels": labels, "handled": handled,
              "subject": msg["subject"] or "(no subject)"}

    if handled:
        actions.drop_from_cache([msg["id"]])
        messages, threads, senders = _load_cached()
        cfg = scoring.load_config()
        store.save_json(store.SCORES_FILE,
                        scoring.score_all(messages, threads, senders, cfg))
        result["remaining"] = len(messages)

    if args.json:
        print(json.dumps(result))
        return
    if handled:
        print("Removed from triage: %s" % result["subject"][:60])
        print("%d messages remain." % result["remaining"])
    else:
        print("Gmail still shows this as unread in your inbox — nothing changed.")
        print("  %s" % result["subject"][:60])


def cmd_score(args):
    messages, threads, senders = _load_cached()
    cfg = scoring.load_config()
    scores = scoring.score_all(messages, threads, senders, cfg)
    store.save_json(store.SCORES_FILE, scores)

    ordered, numbered, cat_entries = viewer.build_index(messages, scores)
    if args.json:
        print(json.dumps(
            [{"n": e["n"], "id": e["msg"]["id"], "category": e["category"],
              "score": e["score"]["total"], "subject": e["msg"]["subject"],
              "from": e["msg"]["from_addr"]} for e in ordered], indent=1))
        return

    if not os.path.exists(scoring.CONFIG_FILE):
        print("[note] no config.yaml — scoring with built-in defaults only.")
        print("       ./triage.py seed-config will draft one from your history.\n")
    viewer.print_summary(cat_entries, len(messages))


def cmd_list(args):
    ordered, numbered, cat_entries = _index()
    cat = args.category
    if cat not in CATEGORY_ORDER:
        _die("Unknown category %r. Choose from: %s" % (cat, ", ".join(CATEGORY_ORDER)))
    entries = cat_entries.get(cat, [])
    if args.json:
        print(json.dumps([{"n": e["n"], "id": e["msg"]["id"],
                           "score": e["score"]["total"],
                           "from": e["msg"]["from_addr"],
                           "subject": e["msg"]["subject"],
                           "reasons": e["score"]["reasons"]} for e in entries], indent=1))
        return
    viewer.print_category(cat, entries, reasons=args.reasons)


def _entry_or_die(numbered, n):
    entry = numbered.get(n)
    if not entry:
        _die("No message #%d. Run ./triage.py score to see the valid range." % n)
    return entry


def cmd_show(args):
    ordered, numbered, _ = _index()
    entry = _entry_or_die(numbered, args.n)
    if args.json:
        print(json.dumps({"n": entry["n"], "message": entry["msg"],
                          "score": entry["score"]}, indent=1))
        return
    viewer.print_message(entry, body_lines=args.lines)


def cmd_explain(args):
    ordered, numbered, _ = _index()
    entry = _entry_or_die(numbered, args.n)
    if args.json:
        print(json.dumps(entry["score"], indent=1))
        return
    viewer.print_breakdown(entry)


def cmd_browse(args):
    ordered, numbered, cat_entries = _index()
    viewer.browse(numbered, cat_entries)


def _rescore_and_report(before):
    """Re-score from cache and report which messages changed category."""
    messages, threads, senders = _load_cached()
    cfg = scoring.load_config()
    after = scoring.score_all(messages, threads, senders, cfg)
    store.save_json(store.SCORES_FILE, after)

    moved = []
    for msg in messages:
        was = (before.get(msg["id"]) or {}).get("category")
        now = after[msg["id"]]["category"]
        if was and was != now:
            moved.append((msg, was, now))

    if not moved:
        print("\nNo messages changed category.")
        return
    print("\n%d message(s) moved:" % len(moved))
    for msg, was, now in moved[:25]:
        print("   %-10s -> %-11s %-28s %s"
              % (was, now, (msg["from_name"] or msg["from_addr"])[:28],
                 (msg["subject"] or "(no subject)")[:44]))
    if len(moved) > 25:
        print("   … and %d more" % (len(moved) - 25))
    print("\nRun ./triage.py score to see the new summary.")


def cmd_teach(args):
    ordered, numbered, _ = _index()
    entry = _entry_or_die(numbered, args.n)
    msg = entry["msg"]

    if args.list:
        keys = scoring.list_keys(msg, scoring.load_config())
        if not keys:
            _die("Message #%d has no list or group address to teach." % args.n)
        scope, target = "list", (args.list_key or keys[0])
        label = "list %s" % target
    elif args.category:
        scope, target = "message", msg["id"]
        label = 'message #%d pinned to "%s"' % (args.n, args.category)
    elif args.domain:
        scope, target = "domain", msg["from_domain"]
        label = "domain %s" % target
    else:
        scope, target = "sender", msg["from_addr"]
        label = "sender %s" % target

    before = store.load_json(store.SCORES_FILE, {})

    if args.clear:
        corrections.teach(target, None, scope=scope)
        print("Cleared the correction for %s." % label)
    elif args.category:
        data = corrections.load()
        data["messages"][msg["id"].lower()] = {
            "category": args.category,
            "taught": __import__("datetime").date.today().isoformat(),
        }
        corrections.save(data)
        print("Pinned %s." % label)
    else:
        if not args.kind:
            _die("Say what to teach: %s (or use --clear / --category)"
                 % ", ".join(corrections.LIST_KINDS if args.list else corrections.KINDS))
        allowed = corrections.LIST_KINDS if scope == "list" else corrections.KINDS
        if args.kind not in allowed:
            _die("For %s use one of: %s" % (scope, ", ".join(allowed)))
        corrections.teach(target, args.kind, scope=scope, note=args.note)
        print("Taught: %s is %s — %s" % (label, args.kind, allowed[args.kind]))

    _rescore_and_report(before)


def cmd_taught(args):
    data = corrections.load()
    if args.json:
        print(json.dumps(data, indent=1))
        return
    any_rows = False
    for scope, key in (("sender", "senders"), ("domain", "domains"),
                       ("list", "lists"), ("message", "messages")):
        rows = data.get(key) or {}
        if not rows:
            continue
        any_rows = True
        print("\n%s:" % key)
        for target, entry in sorted(rows.items()):
            what = entry.get("kind") or ("category=%s" % entry.get("category"))
            note = ("  # %s" % entry["note"]) if entry.get("note") else ""
            print("   %-44s %-12s %s%s"
                  % (target, what, entry.get("taught", ""), note))
    if data.get("my_addresses"):
        any_rows = True
        print("\nmy_addresses (mail to these counts as mail to you):")
        for addr in data["my_addresses"]:
            print("   %s" % addr)
    if not any_rows:
        print("Nothing taught yet. Use:  ./triage.py teach <n> personal")
    else:
        print("\nStored in %s" % corrections.CORRECTIONS_FILE)


def cmd_alias(args):
    before = store.load_json(store.SCORES_FILE, {})
    corrections.add_my_address(args.address, remove=args.remove)
    if args.remove:
        print("Removed %s from your addresses." % args.address)
    else:
        print("Added %s — mail addressed to it now counts as addressed to you."
              % args.address)
    _rescore_and_report(before)


def cmd_authorize_write(args):
    print("inbox-triage is about to ask Google for the gmail.modify scope.")
    print()
    print("  That scope can mark read, archive, label, and move to Trash.")
    print("  It CANNOT delete permanently -- Google reserves that for a broader")
    print("  scope this tool never requests, so trashed mail stays recoverable")
    print("  for about 30 days.")
    print()
    print("  The grant is stored separately at:")
    print("    %s" % gmail_client.WRITE_TOKEN_PATH)
    print("  Your existing read-only token is not touched, and read-only mode")
    print("  never loads this file.")
    print()
    if not args.yes:
        # Several ways of launching this have no interactive stdin; asking there
        # would just crash on EOF.
        if not sys.stdin.isatty():
            print("Not an interactive terminal, so nothing was asked and nothing")
            print("was changed. Re-run with --yes to start the consent flow:")
            print()
            print("    ./triage.py authorize-write --yes")
            return
        try:
            answer = input("Open the Google consent page now? [y/N] ").strip().lower()
        except EOFError:
            print("\nNo answer possible here. Re-run with --yes.")
            return
        if answer not in ("y", "yes"):
            print("Nothing changed.")
            return

    print()
    print("Starting the consent flow. A browser should open; if it does not,")
    print("copy the URL printed below into one yourself.")
    print()
    path = gmail_client.authorize_write()
    print("\nWrite authorization saved to %s" % path)
    print("Revoke any time by deleting that file, or at")
    print("  https://myaccount.google.com/permissions")


def cmd_act(args):
    ordered, numbered, _ = _index()
    entries = [_entry_or_die(numbered, n) for n in args.n]
    spec = actions.ALL_ACTIONS.get(args.what)
    if not spec:
        _die("Unknown action %r. Choose from: %s"
             % (args.what, ", ".join(sorted(actions.ALL_ACTIONS))))

    print("%s  %d message(s):" % (spec["label"], len(entries)))
    for e in entries[:20]:
        print("   #%-5d %-26s %s" % (e["n"], (e["msg"]["from_name"] or e["msg"]["from_addr"])[:26],
                                     (e["msg"]["subject"] or "(no subject)")[:48]))
    if len(entries) > 20:
        print("   … and %d more" % (len(entries) - 20))
    print("\n%s" % spec["blurb"])

    if args.dry_run:
        print("\nDry run — nothing was changed.")
        return
    if not args.yes:
        if input("\nGo ahead? [y/N] ").strip().lower() not in ("y", "yes"):
            print("Nothing changed.")
            return

    ids = [e["msg"]["id"] for e in entries]
    result = actions.apply(args.what, ids, note="cli")
    print("\nDone: %d succeeded, %d failed." % (len(result["succeeded"]), len(result["failed"])))
    if result["removes_from_triage"]:
        dropped = actions.drop_from_cache(result["succeeded"])
        print("Removed %d message(s) from the local triage cache." % dropped)
        messages, threads, senders = _load_cached()
        cfg = scoring.load_config()
        store.save_json(store.SCORES_FILE,
                        scoring.score_all(messages, threads, senders, cfg))
        print("Re-scored. %d messages remain." % len(messages))


def cmd_actions(args):
    rows = actions.read_log(limit=args.limit)
    if args.json:
        print(json.dumps(rows, indent=1))
        return
    if not rows:
        print("Nothing has been done to your mailbox yet.")
        return
    for row in rows:
        if row.get("result") == "done":
            print("   %s  %-12s succeeded %s, failed %s"
                  % (row["at"], row["action"], row["succeeded"], row["failed"]))
        else:
            print("%s  %-12s %d message(s)%s"
                  % (row["at"], row["action"], row.get("count", 0),
                     "  [dry run]" if row.get("dry_run") else ""))


def cmd_weights(args):
    cfg = scoring.load_config()
    print("Current weights (override any of these under `weights:` in config.yaml):\n")
    for key, val in scoring.DEFAULT_CONFIG["weights"].items():
        current = cfg["weights"].get(key, val)
        flag = "" if current == val else "   <- overridden (default %s)" % val
        print("  %-32s %8s%s" % (key, current, flag))
    print("\nThresholds:\n")
    for key, val in scoring.DEFAULT_CONFIG["thresholds"].items():
        current = cfg["thresholds"].get(key, val)
        flag = "" if current == val else "   <- overridden (default %s)" % val
        print("  %-32s %8s%s" % (key, current, flag))


def main():
    parser = argparse.ArgumentParser(
        prog="triage.py",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("seed-config", help="draft config.yaml from mail history")
    p.add_argument("--force", action="store_true", help="overwrite an existing config.yaml")
    p.set_defaults(func=cmd_seed_config)

    p = sub.add_parser("fetch", help="pull unread mail and context into the cache")
    p.add_argument("--force", action="store_true", help="re-probe sender history from scratch")
    p.set_defaults(func=cmd_fetch)

    p = sub.add_parser("sync", help="fast: drop handled mail, pull only what is new")
    p.set_defaults(func=cmd_sync)

    p = sub.add_parser("sync-one", help="check one message and drop it if handled")
    p.add_argument("n", type=int)
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_sync_one)

    p = sub.add_parser("score", help="score the cache offline and show categories")
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_score)

    p = sub.add_parser("list", help="list one category")
    p.add_argument("category", choices=CATEGORY_ORDER)
    p.add_argument("--reasons", type=int, default=2, help="how many reasons per row")
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_list)

    p = sub.add_parser("show", help="full message plus score breakdown")
    p.add_argument("n", type=int)
    p.add_argument("--lines", type=int, default=60, help="body lines to display")
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_show)

    p = sub.add_parser("explain", help="score breakdown only")
    p.add_argument("n", type=int)
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_explain)

    p = sub.add_parser("browse", help="interactive walk through the categories")
    p.set_defaults(func=cmd_browse)

    p = sub.add_parser("weights", help="show every tunable weight")
    p.set_defaults(func=cmd_weights)

    p = sub.add_parser("teach", help="teach the classifier about a sender, domain or message")
    p.add_argument("n", type=int, help="message number, as shown by list/score")
    p.add_argument("kind", nargs="?",
                   choices=sorted(set(corrections.KINDS) | set(corrections.LIST_KINDS)),
                   help="what this sender/domain/list really is")
    p.add_argument("--domain", action="store_true",
                   help="apply to the whole sending domain, not just this address")
    p.add_argument("--list", action="store_true",
                   help="apply to the mailing list / group this arrived via "
                        "(kinds: important, ignore)")
    p.add_argument("--list-key", help="which list identity to use, if several")
    p.add_argument("--category", choices=CATEGORY_ORDER,
                   help="pin THIS ONE message to a category")
    p.add_argument("--note", help="why — recorded alongside the correction")
    p.add_argument("--clear", action="store_true", help="remove the correction")
    p.set_defaults(func=cmd_teach)

    p = sub.add_parser("taught", help="list everything you have taught the classifier")
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_taught)

    p = sub.add_parser("authorize-write",
                       help="grant the gmail.modify scope (opens a browser, one time)")
    p.add_argument("--yes", action="store_true",
                   help="skip the confirmation (needed when stdin is not a terminal)")
    p.set_defaults(func=cmd_authorize_write)

    p = sub.add_parser("act", help="apply an action to one or more messages")
    p.add_argument("what", choices=sorted(actions.ALL_ACTIONS))
    p.add_argument("n", type=int, nargs="+", help="message numbers")
    p.add_argument("--dry-run", action="store_true", help="show what would happen")
    p.add_argument("--yes", action="store_true", help="skip the confirmation")
    p.set_defaults(func=cmd_act)

    p = sub.add_parser("actions", help="show what has been done to your mailbox")
    p.add_argument("--limit", type=int, default=50)
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=cmd_actions)

    p = sub.add_parser("alias", help="declare another address of yours that forwards here")
    p.add_argument("address")
    p.add_argument("--remove", action="store_true")
    p.set_defaults(func=cmd_alias)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
