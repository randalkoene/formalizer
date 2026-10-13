"""Terminal presentation: category summary, listings, message detail, browse.

Every command here reads only the local cache -- no network calls, no mailbox
access of any kind.
"""

import datetime
import textwrap

from rich.console import Console
from rich.panel import Panel
from rich.table import Table
from rich.text import Text

from scoring import CATEGORY_LABELS, CATEGORY_ORDER

console = Console()

CATEGORY_STYLE = {
    "needs_reply": "bold red",
    "grants": "bold dark_orange",
    "directed": "bold yellow",
    "threads": "bold blue",
    "onboarding": "bold cyan",
    "bulk": "white",
    "automated": "bright_black",
    "spam": "bold magenta",
}


def build_index(messages, scores):
    """Assign stable display numbers: category order, then score descending."""
    by_cat = {c: [] for c in CATEGORY_ORDER}
    for msg in messages:
        score = scores.get(msg["id"])
        if score:
            by_cat.setdefault(score["category"], []).append((msg, score))

    for cat in by_cat:
        by_cat[cat].sort(key=lambda p: -p[1]["total"])

    ordered = []
    n = 1
    for cat in CATEGORY_ORDER:
        for msg, score in by_cat[cat]:
            ordered.append({"n": n, "msg": msg, "score": score, "category": cat})
            n += 1

    numbered = {e["n"]: e for e in ordered}
    cat_entries = {c: [e for e in ordered if e["category"] == c] for c in CATEGORY_ORDER}
    return ordered, numbered, cat_entries


def _when(msg):
    ts = msg["internalDate"] / 1000.0
    dt = datetime.datetime.fromtimestamp(ts)
    age = (datetime.datetime.now() - dt).days
    return "%s (%dd)" % (dt.strftime("%b %d"), age)


def _sender(msg, width=28):
    label = msg["from_name"] or msg["from_addr"]
    return textwrap.shorten(label, width=width, placeholder="…")


def print_summary(cat_entries, total):
    table = Table(
        title="Unread triage — %d messages" % total,
        title_style="bold",
        header_style="bold",
        expand=False,
    )
    table.add_column("", width=2)
    table.add_column("Category", min_width=24)
    table.add_column("Count", justify="right", width=6)
    table.add_column("Share", justify="right", width=6)
    table.add_column("Top", justify="right", width=5)
    table.add_column("Numbers", justify="right", width=11)

    for cat in CATEGORY_ORDER:
        entries = cat_entries.get(cat, [])
        icon, label, _blurb = CATEGORY_LABELS[cat]
        if entries:
            span = "%d-%d" % (entries[0]["n"], entries[-1]["n"])
            top = "%.0f" % entries[0]["score"]["total"]
            share = "%.0f%%" % (100.0 * len(entries) / total) if total else "—"
        else:
            span, top, share = "—", "—", "—"
        table.add_row(
            icon,
            Text(label, style=CATEGORY_STYLE[cat]),
            str(len(entries)),
            share,
            top,
            span,
        )

    console.print(table)
    console.print(
        "\n[bright_black]list <category>  ·  show <n>  ·  explain <n>  ·  browse"
        "[/bright_black]"
    )
    console.print(
        "[bright_black]categories: %s[/bright_black]\n" % "  ".join(CATEGORY_ORDER)
    )


def print_category(cat, entries, reasons=2):
    icon, label, blurb = CATEGORY_LABELS[cat]
    console.print()
    console.print(Text("%s  %s  — %d messages" % (icon, label, len(entries)),
                       style=CATEGORY_STYLE[cat]))
    console.print("[bright_black]%s[/bright_black]" % blurb)

    if not entries:
        console.print("[bright_black]  (none)[/bright_black]\n")
        return

    table = Table(header_style="bold", expand=True, show_lines=False)
    table.add_column("#", justify="right", width=4)
    table.add_column("Score", justify="right", width=5)
    table.add_column("Date", width=12)
    table.add_column("From", width=28)
    table.add_column("Subject", min_width=30, max_width=52)
    table.add_column("Why", style="bright_black", min_width=24)

    for entry in entries:
        msg, score = entry["msg"], entry["score"]
        why = "; ".join(r["detail"] or r["rule"] for r in score["reasons"][:reasons])
        table.add_row(
            str(entry["n"]),
            "%.0f" % score["total"],
            _when(msg),
            _sender(msg),
            textwrap.shorten(msg["subject"] or "(no subject)", 52, placeholder="…"),
            textwrap.shorten(why, 60, placeholder="…"),
        )

    console.print(table)


def print_breakdown(entry):
    msg, score = entry["msg"], entry["score"]
    icon, label, _ = CATEGORY_LABELS[entry["category"]]

    table = Table(show_header=True, header_style="bold", box=None, pad_edge=False)
    table.add_column("Points", justify="right", width=7)
    table.add_column("Rule", width=28)
    table.add_column("Detail", style="bright_black")

    for reason in score["reasons"]:
        pts = reason["points"]
        style = "green" if pts > 0 else "red"
        table.add_row(
            Text("%+g" % pts, style=style), reason["rule"], reason["detail"]
        )
    table.add_row(Text("%.0f" % score["total"], style="bold"),
                  Text("TOTAL", style="bold"),
                  Text("%s %s" % (icon, label), style=CATEGORY_STYLE[entry["category"]]))

    console.print(
        Panel(
            table,
            title="[bold]#%d  %s[/bold]" % (entry["n"], msg["subject"] or "(no subject)"),
            subtitle="[bright_black]%s[/bright_black]" % msg["from_addr"],
            border_style=CATEGORY_STYLE[entry["category"]],
        )
    )


def print_message(entry, body_lines=60):
    msg = entry["msg"]
    header = Table(box=None, show_header=False, pad_edge=False)
    header.add_column(style="bold bright_black", width=10)
    header.add_column()

    header.add_row("From", "%s <%s>" % (msg["from_name"], msg["from_addr"]))
    header.add_row("To", ", ".join(r["addr"] for r in msg["to"]) or "—")
    if msg["cc"]:
        header.add_row("Cc", ", ".join(r["addr"] for r in msg["cc"]))
    header.add_row("Date", _when(msg))
    header.add_row("Subject", msg["subject"] or "(no subject)")
    if msg["attachments"]:
        header.add_row(
            "Attach",
            ", ".join("%s (%s)" % (a["filename"], a["mimeType"]) for a in msg["attachments"]),
        )
    if msg["headers"].get("list-id"):
        header.add_row("List-Id", msg["headers"]["list-id"])

    console.print()
    console.print(header)
    console.print()

    body = msg["body_text"] or msg["snippet"] or "(no body captured)"
    lines = body.splitlines()
    shown = "\n".join(lines[:body_lines])
    console.print(Panel(shown, border_style="bright_black", title="body"))
    if len(lines) > body_lines:
        console.print(
            "[bright_black]… %d more lines (raw text is in cache/messages.json)"
            "[/bright_black]" % (len(lines) - body_lines)
        )
    console.print()
    print_breakdown(entry)


def browse(numbered, cat_entries):
    """Interactive category/message walker. Read-only throughout."""
    cats = [c for c in CATEGORY_ORDER if cat_entries.get(c)]
    if not cats:
        console.print("[yellow]Nothing to browse. Run fetch and score first.[/yellow]")
        return

    while True:
        console.print()
        for i, cat in enumerate(cats, 1):
            icon, label, _ = CATEGORY_LABELS[cat]
            console.print(
                "  [bold]%d[/bold]) %s %s [bright_black](%d)[/bright_black]"
                % (i, icon, label, len(cat_entries[cat]))
            )
        choice = console.input(
            "\n[bold]category number[/bold] (or [bold]#N[/bold] for a message, "
            "[bold]q[/bold] to quit): "
        ).strip()

        if choice.lower() in ("q", "quit", "exit"):
            return
        if choice.startswith("#") and choice[1:].isdigit():
            entry = numbered.get(int(choice[1:]))
            if entry:
                print_message(entry)
            continue
        if not choice.isdigit() or not 1 <= int(choice) <= len(cats):
            continue

        cat = cats[int(choice) - 1]
        entries = cat_entries[cat]
        pos = 0
        print_category(cat, entries)

        while True:
            prompt = (
                "\n[bold]n[/bold]ext / [bold]p[/bold]rev / number to open / "
                "[bold]l[/bold]ist / [bold]b[/bold]ack: "
            )
            cmd = console.input(prompt).strip().lower()
            if cmd in ("b", "back", ""):
                break
            if cmd in ("q", "quit"):
                return
            if cmd == "l":
                print_category(cat, entries)
                continue
            if cmd == "n":
                if pos < len(entries) - 1:
                    pos += 1
                print_message(entries[pos])
                continue
            if cmd == "p":
                if pos > 0:
                    pos -= 1
                print_message(entries[pos])
                continue
            if cmd.lstrip("#").isdigit():
                entry = numbered.get(int(cmd.lstrip("#")))
                if entry:
                    print_message(entry)
                    if entry["category"] == cat:
                        pos = entries.index(entry)
