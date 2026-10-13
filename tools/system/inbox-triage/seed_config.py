"""One-time, read-only history scan that drafts config.yaml.

Everything it writes is a *proposal* for you to edit. Deleting a line simply
switches that rule off for that entry -- a bad guess costs a deletion, never a
misclassification you cannot undo.
"""

import collections
import os
import re
from email.utils import getaddresses

import gmail_client
from store import domain_of

HERE = os.path.dirname(os.path.realpath(__file__))
CONFIG_FILE = os.path.join(HERE, "config.yaml")

# Consumer mail hosts are never useful as "funder domain" or VIP-domain signals.
GENERIC_DOMAINS = {
    "gmail.com", "googlemail.com", "yahoo.com", "hotmail.com", "outlook.com",
    "live.com", "icloud.com", "me.com", "aol.com", "protonmail.com", "proton.me",
    "msn.com", "gmx.com", "mail.com", "fastmail.com", "zoho.com", "qq.com",
}

# Big platforms show up as correspondents in grant-ish threads (a Google Ad
# Grants support ticket, a Substack about funding) but are never funders. Left
# in, one such thread turns the whole platform into a "funder domain" and
# captures hundreds of unrelated messages.
PLATFORM_DOMAINS = {
    "google.com", "googlegroups.com", "google.org", "youtube.com",
    "microsoft.com", "apple.com", "amazon.com", "aws.amazon.com",
    "linkedin.com", "facebook.com", "meta.com", "twitter.com", "x.com",
    "substack.com", "medium.com", "wordpress.com", "squarespace.com", "wix.com",
    "slack.com", "zoom.us", "dropbox.com", "box.com", "github.com", "gitlab.com",
    "notion.so", "calendly.com", "eventbrite.com", "mailchimp.com",
    "paypal.com", "stripe.com", "intuit.com", "docusign.net", "adobe.com",
    "atlassian.com", "asana.com", "trello.com", "airtable.com", "typeform.com",
    "surveymonkey.com", "doodle.com", "researchgate.net", "academia.edu",
    "frontiersin.org", "springer.com", "elsevier.com", "wiley.com", "nature.com",
}

ROLE_LOCALPARTS = {
    "info", "contact", "admin", "support", "hello", "team", "office",
    "secretary", "board", "press", "media", "donate", "donations",
}

GRANT_QUERY = (
    'in:sent newer_than:3y ('
    'grant OR proposal OR "letter of intent" OR LOI OR RFP OR RFA OR '
    '"notice of award" OR "submission id" OR "principal investigator" OR '
    '"budget justification" OR solicitation OR "funding opportunity")'
)

SENT_QUERY = "in:sent newer_than:3y"
INBOX_QUERY = "in:inbox newer_than:2y"

GRANT_SCAN_CAP = 250
SENT_SCAN_CAP = 800
INBOX_SCAN_CAP = 600

RE_PREFIX = re.compile(r"(?i)^\s*((re|fwd|fw|aw|vs)\s*:\s*)+")


def _headers(raw):
    return {
        h.get("name", "").lower(): h.get("value", "")
        for h in (raw.get("payload") or {}).get("headers", []) or []
    }


def _addrs(value):
    return [a.lower() for _, a in getaddresses([value or ""]) if a]


def _normalize_subject(subject):
    return RE_PREFIX.sub("", subject or "").strip()


def scan(service, me_addr, progress=None, verbose=print):
    me = me_addr.lower()
    my_domain = domain_of(me)

    # ---- pass 1: grant-looking mail you have SENT ------------------------
    verbose("Scanning Sent mail for grant correspondence…")
    grant_ids = [i for i, _ in gmail_client.list_query_ids(service, GRANT_QUERY, GRANT_SCAN_CAP)]
    grant_msgs, _ = gmail_client.fetch_messages(
        service, grant_ids, progress=progress, fmt="metadata",
        headers=["To", "Cc", "Subject", "Date"],
    )

    grant_groups = collections.OrderedDict()
    for raw in grant_msgs.values():
        hdrs = _headers(raw)
        subject = _normalize_subject(hdrs.get("subject", ""))
        if not subject:
            continue
        group = grant_groups.setdefault(
            subject, {"thread_ids": set(), "domains": set(), "count": 0}
        )
        group["thread_ids"].add(raw.get("threadId"))
        group["count"] += 1
        for addr in _addrs(hdrs.get("to", "")) + _addrs(hdrs.get("cc", "")):
            dom = domain_of(addr)
            if (
                dom
                and dom != my_domain
                and dom not in GENERIC_DOMAINS
                and dom not in PLATFORM_DOMAINS
            ):
                group["domains"].add(dom)

    # Keep the threads that look like a real back-and-forth with an outside
    # organization, not a one-off mention of the word "proposal".
    grants = [
        {"name": subject, "data": data}
        for subject, data in grant_groups.items()
        if data["domains"]
    ]
    grants.sort(key=lambda g: (-g["data"]["count"], g["name"]))
    grants = grants[:25]

    # ---- pass 2: who you actually write to -------------------------------
    verbose("Ranking your correspondents from Sent mail…")
    sent_ids = [i for i, _ in gmail_client.list_query_ids(service, SENT_QUERY, SENT_SCAN_CAP)]
    sent_msgs, _ = gmail_client.fetch_messages(
        service, sent_ids, progress=progress, fmt="metadata", headers=["To", "Cc"]
    )

    counts = collections.Counter()
    for raw in sent_msgs.values():
        hdrs = _headers(raw)
        for addr in set(_addrs(hdrs.get("to", "")) + _addrs(hdrs.get("cc", ""))):
            if addr == me or domain_of(addr) in ("",):
                continue
            counts[addr] += 1

    vips = [(a, n) for a, n in counts.most_common(40) if n >= 3][:25]

    # ---- pass 3: Carboncopies role addresses you receive on --------------
    verbose("Looking for Carboncopies role addresses in your inbox…")
    inbox_ids = [i for i, _ in gmail_client.list_query_ids(service, INBOX_QUERY, INBOX_SCAN_CAP)]
    inbox_msgs, _ = gmail_client.fetch_messages(
        service, inbox_ids, progress=progress, fmt="metadata", headers=["To", "Cc"]
    )

    role_counts = collections.Counter()
    other_org_counts = collections.Counter()
    alias_counts = collections.Counter()
    for raw in inbox_msgs.values():
        hdrs = _headers(raw)
        recips = set(_addrs(hdrs.get("to", "")) + _addrs(hdrs.get("cc", "")))
        # Mail delivered to you but addressed to some other address is almost
        # always a forwarding alias of yours (or a list). Mailing lists are
        # excluded by their List-Id; what remains is worth proposing.
        if me not in recips and not hdrs.get("list-id"):
            for addr in recips:
                if domain_of(addr) not in (my_domain,):
                    alias_counts[addr] += 1
        for addr in recips:
            if addr == me:
                continue
            local, _, dom = addr.partition("@")
            if dom != my_domain:
                continue
            if local in ROLE_LOCALPARTS:
                role_counts[addr] += 1
            else:
                other_org_counts[addr] += 1

    org_addresses = [a for a, n in role_counts.most_common(15)]
    # Colleagues at your own domain: suggested, but commented out, since they
    # are people rather than Foundation role inboxes.
    other_org = [a for a, n in other_org_counts.most_common(10) if n >= 3]

    # Only propose aliases that carry your surname -- a shared list address would
    # otherwise look identical from the headers alone.
    surname = me.split("@")[0].replace(".", "").replace("_", "")[-5:]
    aliases = [(a, n) for a, n in alias_counts.most_common(8)
               if n >= 3 and (surname[-4:] in a.replace(".", "") or n >= 40)]

    return {
        "me": me,
        "grants": grants,
        "vips": vips,
        "aliases": aliases,
        "org_addresses": org_addresses,
        "other_org": other_org,
        "scanned": {
            "grant_candidates": len(grant_ids),
            "sent": len(sent_ids),
            "inbox": len(inbox_ids),
        },
    }


def render(findings):
    """Render the scan into a commented config.yaml."""
    lines = [
        "# inbox-triage configuration",
        "#",
        "# Drafted by `triage.py seed-config` from your own mail history. Every",
        "# entry below is a PROPOSAL -- edit, delete, or extend freely. Removing an",
        "# entry just switches that rule off; it never deletes anything in Gmail.",
        "#",
        "# After editing, re-run `triage.py score` (offline, no API calls) to see",
        "# what moved.",
        "",
        "me:",
        "  addresses:",
        "    - %s" % findings["me"],
        "  # Names checked against the opening lines, for a personal salutation.",
        "  names:",
        "    - randal",
        "    - koene",
        "  # Other addresses of yours that forward into this mailbox. Mail sent to",
        "  # them counts as sent to you. (`triage.py alias <addr>` adds one too,",
        "  # storing it in corrections.yaml instead.)",
        "  aliases:"] + (
        ["    - %s  # %d message(s) delivered here but addressed to it" % (a, n)
         for a, n in findings.get("aliases") or []]
        or ["    []"]) + [
        "",
        "# Mail to these counts as directed at the Foundation rather than at a list.",
        "org_addresses:",
    ]
    if findings["org_addresses"]:
        lines += ["  - %s" % a for a in findings["org_addresses"]]
    else:
        lines += ["  # none found -- add e.g. info@carboncopies.org", "  []"]
    if findings.get("other_org"):
        lines.append("  # Other addresses at your own domain seen in your inbox.")
        lines.append("  # Uncomment any that are shared Foundation inboxes:")
        lines += ["  # - %s" % a for a in findings["other_org"]]

    lines += [
        "",
        "# Group addresses whose traffic is intake -- volunteer offers, resumes,",
        "# applications. Mail arriving through one of these is filed under",
        "# Onboarding rather than Bulk, and that category offers no Trash.",
        "onboarding_addresses:",
        "  - onboarding@carboncopies.org",
        "",
        "org_domains:",
        "  - carboncopies.org",
        "",
        "# Addresses (or bare domains) that always matter. Ranked by how often you",
        "# have written to them. Trim this to the people who really are VIPs --",
        "# a long list dilutes the signal.",
        "vips:",
    ]
    if findings["vips"]:
        for addr, n in findings["vips"]:
            lines.append("  - %s  # %d messages sent to them" % (addr, n))
    else:
        lines.append("  []")

    lines += [
        "",
        "# Grant applications and funder relationships found in your Sent mail.",
        "# A message matching a grant's thread_ids, domains, or keywords is filed",
        "# under Grants & Funding. Delete the ones that are not real applications.",
        "grants:",
    ]
    if findings["grants"]:
        for grant in findings["grants"]:
            data = grant["data"]
            name = grant["name"].replace('"', "'")
            lines.append('  - name: "%s"' % name[:120])
            lines.append("    # %d message(s) you sent in this conversation" % data["count"])
            lines.append("    thread_ids:")
            for tid in sorted(t for t in data["thread_ids"] if t):
                lines.append("      - %s" % tid)
            lines.append("    # Domains seen in this conversation. Uncomment one ONLY if it")
            lines.append("    # really is a funder -- every message from that domain will then")
            lines.append("    # be filed under Grants & Funding.")
            lines.append("    domains: []")
            for dom in sorted(data["domains"])[:6]:
                lines.append("    #   - %s" % dom)
            lines.append("    keywords: []")
            lines.append("")
    else:
        lines += ["  []", ""]

    lines += [
        "# Weight overrides. Anything omitted keeps the default from scoring.py",
        "# (see `triage.py weights` for the full list of tunable keys).",
        "# weights:",
        "#   to_sole_recipient: 30",
        "#   thread_you_participated: 25",
        "#   grant_config_match: 40",
        "#   list_header: -25",
        "",
        "# thresholds:",
        "#   needs_reply: 55",
        "#   directed: 25",
        "#   directed_override: 45",
        "",
    ]
    return "\n".join(lines)
