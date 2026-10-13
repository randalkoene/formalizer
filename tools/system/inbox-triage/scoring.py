"""The importance evaluation.

score_message() returns a total, a category, and an itemized list of every rule
that fired with its point contribution. That breakdown is the tuning surface:
nothing influences a score without showing up in the explanation.

All weights live in DEFAULT_CONFIG["weights"] and can be overridden per-key from
config.yaml, so tuning never requires editing this file.
"""

import copy
import os
import re
import time

import yaml

import corrections
from store import domain_of

HERE = os.path.dirname(os.path.realpath(__file__))
CONFIG_FILE = os.path.join(HERE, "config.yaml")

# ---------------------------------------------------------------- categories

NEEDS_REPLY = "needs_reply"
GRANTS = "grants"
DIRECTED = "directed"
THREADS = "threads"
ONBOARDING = "onboarding"
BULK = "bulk"
AUTOMATED = "automated"
SPAM = "spam"

CATEGORY_ORDER = [NEEDS_REPLY, GRANTS, DIRECTED, THREADS, ONBOARDING,
                  BULK, AUTOMATED, SPAM]

# What a message wants from you, independent of what KIND of mail it is. A grant
# message can need a reply; a mailing list you have said matters needs a glance.
NEEDS_REPLY_ACTION = "reply"
NEEDS_READ = "read"
NEEDS_NONE = "none"
NEEDS_ORDER = [NEEDS_REPLY_ACTION, NEEDS_READ, NEEDS_NONE]

NEEDS_LABELS = {
    NEEDS_REPLY_ACTION: ("To reply", "Someone is waiting on an answer from you"),
    NEEDS_READ: ("To read", "Worth a glance, but nothing is being asked"),
    NEEDS_NONE: ("No action", "Nothing here needs you"),
}

# Glanceable subsets, mostly for finding the few things worth seeing inside the
# bulk and automated piles.
TAG_LABELS = {
    "important_list": "Lists you follow",
    "travel": "Travel & bookings",
    "finance": "Invoices & payments",
    "security": "Security & sign-in",
    "calendar": "Calendar invites",
    "muted_list": "Lists you muted",
}
TAG_ORDER = ["important_list", "travel", "finance", "security", "calendar"]
# Muted mail is still filed, still listed, still counted -- it is simply sorted
# to the bottom of Bulk. This tag exists so you can see exactly what you muted.
TAG_MUTED = ["muted_list"]

CATEGORY_LABELS = {
    NEEDS_REPLY: ("\U0001f534", "Needs Reply", "Directed at you, live thread, or a pending question"),
    GRANTS: ("\U0001f7e0", "Grants & Funding", "Touches an application, funder, or proposal"),
    DIRECTED: ("\U0001f7e1", "Directed / Org Business", "For you or Carboncopies, no reply obviously pending"),
    THREADS: ("\U0001f535", "Ongoing Threads (FYI)", "Conversations you are in, no action implied"),
    ONBOARDING: ("\U0001f7e2", "Onboarding", "Arrived via an onboarding group — glance, then keep for reference"),
    BULK: ("⚪", "Bulk / Newsletters", "Mailing lists, announcements, updates"),
    AUTOMATED: ("⚫", "Automated / Transactional", "Machine-generated notifications and receipts"),
    SPAM: ("\U0001f6ab", "Probable Spam / Phishing", "Failed auth, impersonation, or lookalike domains"),
}

# ------------------------------------------------------------------- config

DEFAULT_CONFIG = {
    "me": {
        "addresses": ["rkoene@carboncopies.org"],
        # Used to spot a personal salutation in the opening lines.
        "names": ["randal", "koene", "dr. koene", "dr koene"],
        # Other addresses of yours that forward into this mailbox. Mail sent to
        # them counts as sent to you.
        "aliases": [],
    },
    # Mail to these counts as directed at the Foundation rather than at a list.
    "org_addresses": [],
    # Group addresses (or their List-Ids) whose traffic is intake: volunteer
    # offers, resumes, applications. It is neither bulk nor a thread you are in,
    # and it is worth keeping -- hence a category of its own, with no trash.
    "onboarding_addresses": ["onboarding@carboncopies.org"],
    "org_domains": ["carboncopies.org"],
    # Addresses or bare domains that always matter.
    "vips": [],
    # Each entry: {name, keywords: [...], domains: [...], thread_ids: [...]}
    "grants": [],
    "funder_domains": [
        "nih.gov",
        "nsf.gov",
        "grants.gov",
        "era.nih.gov",
        "research.gov",
        "templetonworldcharity.org",
        "templeton.org",
        "sloan.org",
        "simonsfoundation.org",
        "openphilanthropy.org",
        "arpa-h.gov",
        "darpa.mil",
        "ec.europa.eu",
        "erc.europa.eu",
        "wellcome.org",
    ],
    "grant_keywords": [
        "grant", "proposal", "letter of intent", "loi", "rfp", "rfa",
        "request for proposals", "notice of award", "award notice", "funding",
        "submission id", "application id", "panel review", "study section",
        "peer review", "budget justification", "principal investigator",
        "subaward", "no-cost extension", "progress report", "solicitation",
    ],
    "weights": {
        # Directedness
        "to_sole_recipient": 30,
        "to_small_group": 20,
        "to_large_group": 8,
        "cc_only": 8,
        "not_addressed": -6,
        "org_address": 18,
        "personal_salutation": 10,
        # Thread continuity
        "thread_you_participated": 25,
        "direct_reply_to_your_message": 18,
        "thread_multi_message": 6,
        # Grants
        "grant_config_match": 40,
        "grant_thread_match": 40,
        "grant_funder_domain": 22,
        "grant_vocabulary": 14,
        # Reply expected
        "direct_question": 12,
        "action_request": 12,
        "deadline_language": 10,
        # Sender relationship
        "sender_frequent": 15,
        "sender_known": 8,
        "sender_first_contact": -4,
        "vip_sender": 25,
        # Bulk / automated
        "list_header": -25,
        "precedence_bulk": -18,
        "noreply_sender": -15,
        "gmail_promotions": -22,
        "gmail_category_other": -10,
        "auto_submitted": -20,
        "notification_sender": -15,
        "transactional": -18,
        # Spam / phishing
        "auth_failure": -40,
        "display_name_mismatch": -30,
        "lookalike_domain": -35,
        "risky_attachment": -25,
        "bulk_no_unsubscribe": -10,
        # Misc
        "taught_personal": 40,
        "taught_important": 60,
        "taught_bulk": -60,
        "taught_ignore": -100,
        "gmail_personal": 4,
        "important_list": 30,
        "onboarding_group": 12,
        "calendar_invite": 12,
        "pdf_attachment": 4,
        "age_peak": 8,
        "age_floor": -12,
    },
    # Unread mail gains weight for a while -- it has been sitting -- and then
    # loses it. Something unanswered for a year is usually overtaken by events,
    # but it is not worthless, so the floor is finite and it is still listed.
    "age": {
        "grace_days": 2,
        "peak_days": 30,
        "stale_days": 120,
        "dead_days": 365,
    },
    "thresholds": {
        "needs_reply": 55,
        "directed": 25,
        # A message this strongly directed escapes the bulk/automated buckets
        # even if it carries list-ish headers.
        "directed_override": 45,
        # The list UI splits each view at this age.
        "stale_days": 120,
    },
}

RISKY_EXTENSIONS = (
    ".exe", ".scr", ".js", ".jar", ".vbs", ".bat", ".cmd", ".com", ".pif",
    ".hta", ".iso", ".img", ".msi", ".apk", ".lnk", ".docm", ".xlsm", ".pptm",
)

NOREPLY_RE = re.compile(
    r"(?i)(no[-_.]?reply|do[-_.]?not[-_.]?reply|donotreply|notifications?|"
    r"mailer-daemon|bounce|postmaster|automated|auto-confirm)"
)

NOTIFICATION_SENDER_RE = re.compile(
    r"(?i)\b(notifications?|alerts?|updates?|billing|receipts?|invoice|support|"
    r"newsletter|digest|noreply)\b"
)

ACTION_RE = re.compile(
    r"(?i)\b(could you|can you|would you|will you|please (review|send|confirm|"
    r"sign|approve|advise|respond|reply|complete|fill|provide|share|let)|"
    r"let me know|looking forward to your|awaiting your|we need your|"
    r"your thoughts|your feedback|your input|your signature|requesting your|"
    r"action required|response required|rsvp|kindly)\b"
)

DEADLINE_RE = re.compile(
    r"(?i)\b(deadline|due (by|date|on)|no later than|closing date|expires?|"
    r"expiring|time[- ]sensitive|as soon as possible|asap|by (mon|tues|wednes|"
    r"thurs|fri|satur|sun)day|within \d+ (hours|days|business days)|"
    r"final (notice|reminder)|last chance to respond)\b"
)

QUESTION_RE = re.compile(r"(?i)\byou(r|rs)?\b")

# A real booking states that it IS a confirmation and carries a reference.
# Marketing sells you a trip. Keyword matching alone is useless here -- it finds
# twenty airline mailshots for every itinerary -- so the tag also requires the
# message to lack the promotional apparatus (Promotions label, List-Id,
# unsubscribe header).
BOOKING_RE = re.compile(
    r"(?i)\b(booking (confirmation|reference)|confirmation number|record locator"
    r"|e-?ticket|boarding pass|your itinerary|itinerary for|reservation (confirmed|number)"
    r"|check-?in is now open|(flight|trip) (confirmed|details)|pnr)\b"
)
SECURITY_RE = re.compile(
    r"(?i)\b((confirmation|verification|security|login|sign-?in|one-?time|access) code"
    r"|2fa|password (reset|changed)|new sign-?in|unusual (sign-?in|activity)"
    r"|security alert|verify your (email|account))\b"
)
FINANCE_RE = re.compile(
    r"(?i)\b(invoice|receipt for|payment (received|confirmation|due|failed)"
    r"|your (statement|bill)|amount due|has been (charged|paid)|refund)\b"
)
LIST_ID_RE = re.compile(r"<([^>]+)>")

# Transactional mail a machine sent you personally: codes, receipts, statements.
# Addressed to you, but never something to reply to.
TRANSACTIONAL_RE = re.compile(
    r"(?i)\b((confirmation|verification|security|login|sign-?in|one-?time|access) code"
    r"|2fa|password reset|reset your password|verify your (email|account|address)"
    r"|receipt for|your (order|receipt|invoice|statement|subscription)"
    r"|invoice #|statement is ready|payment (received|confirmation|failed)"
    r"|shipping confirmation|has shipped|out for delivery"
    r"|new sign-?in|unusual (sign-?in|activity)|welcome to)\b"
)

AUTH_FAIL_RE = re.compile(r"(?i)\b(spf|dkim|dmarc)\s*=\s*(fail|softfail|permerror)")

# An address embedded in a display name must have a real dotted domain. Google
# Groups rewrites the display name as "someone via Group Name", which otherwise
# reads as user@Group.
EMBEDDED_ADDR_RE = re.compile(r"[\w.+-]+@([\w-]+(?:\.[\w-]+)+)")
VIA_SUFFIX_RE = re.compile(r"(?i)\s+via\s+.*$")

GMAIL_SOFT_CATEGORIES = {
    "CATEGORY_SOCIAL",
    "CATEGORY_FORUMS",
    "CATEGORY_UPDATES",
}


def _deep_merge(base, override):
    out = copy.deepcopy(base)
    for key, val in (override or {}).items():
        if isinstance(val, dict) and isinstance(out.get(key), dict):
            out[key] = _deep_merge(out[key], val)
        else:
            out[key] = val
    return out


def load_config(path=CONFIG_FILE):
    """DEFAULT_CONFIG with config.yaml layered on top (missing file is fine)."""
    user = {}
    if os.path.exists(path):
        with open(path) as fh:
            user = yaml.safe_load(fh) or {}
    cfg = _deep_merge(DEFAULT_CONFIG, user)
    corr = corrections.load()
    cfg["_corrections"] = corr
    cfg["_me_set"] = (
        {a.lower() for a in cfg["me"]["addresses"]}
        | {a.lower() for a in (cfg["me"].get("aliases") or [])}
        | set(corr.get("my_addresses") or [])
    )
    cfg["_org_set"] = {a.lower() for a in cfg.get("org_addresses") or []}
    cfg["_onboarding_set"] = {
        a.lower().strip() for a in cfg.get("onboarding_addresses") or [] if a
    }
    cfg["_vip_set"] = {v.lower() for v in cfg.get("vips") or []}
    return cfg


# ------------------------------------------------------------------ helpers


def _edit_distance(a, b, limit=3):
    """Bounded Levenshtein; returns limit+1 once it is clear we exceed limit."""
    if abs(len(a) - len(b)) > limit:
        return limit + 1
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            cur.append(
                min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (ca != cb))
            )
        if min(cur) > limit:
            return limit + 1
        prev = cur
    return prev[-1]


def _is_vip(msg, cfg):
    return msg["from_addr"] in cfg["_vip_set"] or msg["from_domain"] in cfg["_vip_set"]


def _recipients(msg):
    return [r["addr"] for r in msg["to"]] + [r["addr"] for r in msg["cc"]]


class Score:
    __slots__ = ("total", "category", "needs", "reasons", "flags")

    def __init__(self):
        self.total = 0.0
        self.category = BULK
        self.needs = NEEDS_NONE
        self.reasons = []
        self.flags = {}

    def add(self, rule, points, detail="", always=False):
        if not points and not always:
            return
        self.total += points
        self.reasons.append({"rule": rule, "points": round(points, 1), "detail": detail})

    def as_dict(self):
        return {
            "total": round(self.total, 1),
            "category": self.category,
            "needs": self.needs,
            "tags": self.flags.get("tags") or [],
            "reasons": self.reasons,
            "flags": self.flags,
        }


# -------------------------------------------------------------------- rules


def _rule_directedness(msg, cfg, score, w):
    me = cfg["_me_set"]
    org = cfg["_org_set"]
    org_domains = {d.lower() for d in cfg.get("org_domains") or []}

    to_addrs = [r["addr"] for r in msg["to"]]
    cc_addrs = [r["addr"] for r in msg["cc"]]
    total_recipients = len(set(to_addrs) | set(cc_addrs))

    in_to = bool(me & set(to_addrs))
    in_cc = bool(me & set(cc_addrs))

    org_hit = (org & set(to_addrs + cc_addrs)) or (
        {a for a in to_addrs + cc_addrs if domain_of(a) in org_domains and a not in me}
    )

    if in_to and len(to_addrs) == 1 and not cc_addrs:
        score.add("to_sole_recipient", w["to_sole_recipient"], "you are the only recipient")
    elif in_to and total_recipients <= 4:
        score.add(
            "to_small_group",
            w["to_small_group"],
            "named on To: with %d recipients" % total_recipients,
        )
    elif in_to:
        score.add(
            "to_large_group",
            w["to_large_group"],
            "named on To: with %d recipients" % total_recipients,
        )
    elif in_cc:
        score.add("cc_only", w["cc_only"], "you are on Cc: only")

    if org_hit:
        score.add(
            "org_address",
            w["org_address"],
            "addressed to Carboncopies (%s)" % ", ".join(sorted(org_hit)[:2]),
        )

    if not in_to and not in_cc:
        # Delivered via a list or Bcc: your address appears nowhere.
        if not org_hit:
            score.add(
                "not_addressed",
                w["not_addressed"],
                "your address is not in To: or Cc:",
            )

    opening = msg["body_text"][:400].lower()
    for name in cfg["me"]["names"]:
        if re.search(r"\b%s\b" % re.escape(name.lower()), opening):
            score.add("personal_salutation", w["personal_salutation"], 'opens with "%s"' % name)
            break

    score.flags["directed_at_me"] = in_to or in_cc
    score.flags["directed_strong"] = bool(
        in_to and total_recipients <= 4
    ) or bool(org_hit)
    return score


def _rule_thread(msg, thread, cfg, score, w):
    if not thread:
        return score

    count = thread.get("message_count", 1)
    mine = thread.get("i_sent_count", 0)

    if mine:
        score.add(
            "thread_you_participated",
            w["thread_you_participated"],
            "you have sent %d message(s) in this thread" % mine,
        )

    in_reply_to = (msg["headers"].get("in-reply-to") or "").strip()
    references = msg["headers"].get("references") or ""
    my_ids = set(thread.get("my_message_ids") or [])
    if my_ids and (in_reply_to in my_ids or any(mid in references for mid in my_ids)):
        score.add(
            "direct_reply_to_your_message",
            w["direct_reply_to_your_message"],
            "replies directly to a message you sent",
        )
        score.flags["replied_to_you"] = True

    if count > 1:
        score.add("thread_multi_message", w["thread_multi_message"], "thread of %d messages" % count)

    score.flags["thread_ongoing"] = count > 1
    score.flags["thread_you_participated"] = bool(mine)
    return score


def _rule_grants(msg, cfg, score, w):
    haystack = " ".join(
        [msg["subject"], msg["snippet"], msg["body_text"][:6000]]
    ).lower()
    matched = False

    for grant in cfg.get("grants") or []:
        name = grant.get("name", "grant")
        if msg["threadId"] in (grant.get("thread_ids") or []):
            score.add("grant_thread_match", w["grant_thread_match"], "known %s thread" % name)
            matched = True
            continue
        domains = {d.lower() for d in (grant.get("domains") or [])}
        if msg["from_domain"] and msg["from_domain"] in domains:
            score.add("grant_config_match", w["grant_config_match"], "from %s funder domain" % name)
            matched = True
            continue
        for kw in grant.get("keywords") or []:
            if kw.lower() in haystack:
                score.add(
                    "grant_config_match",
                    w["grant_config_match"],
                    '%s: matched "%s"' % (name, kw),
                )
                matched = True
                break

    funder_domains = {d.lower() for d in cfg.get("funder_domains") or []}
    if msg["from_domain"] and (
        msg["from_domain"] in funder_domains
        or any(msg["from_domain"].endswith("." + d) for d in funder_domains)
    ):
        score.add("grant_funder_domain", w["grant_funder_domain"], "from funder %s" % msg["from_domain"])
        matched = True

    hits = [kw for kw in cfg.get("grant_keywords") or [] if kw.lower() in haystack]
    weak = False
    if hits:
        score.add(
            "grant_vocabulary",
            w["grant_vocabulary"],
            "grant language: %s" % ", ".join(hits[:3]),
        )
        weak = len(hits) >= 2

    # A configured grant, a known thread, or a curated funder domain files the
    # message under Grants unconditionally. Generic vocabulary is only enough
    # when the message is not obviously a newsletter or a robot -- otherwise
    # every fundraising mailshot lands in the grants bucket.
    score.flags["grant_related"] = matched
    score.flags["grant_vocabulary_only"] = weak and not matched
    return score


def _rule_reply_expected(msg, cfg, score, w):
    body = msg["body_text"][:8000]
    asked = False

    for line in body.splitlines():
        if "?" in line and QUESTION_RE.search(line):
            score.add("direct_question", w["direct_question"], "asks you a question: %s" % line.strip()[:70])
            asked = True
            break

    match = ACTION_RE.search(body) or ACTION_RE.search(msg["subject"])
    if match:
        score.add("action_request", w["action_request"], 'requests action: "%s"' % match.group(0))
        asked = True

    match = DEADLINE_RE.search(body) or DEADLINE_RE.search(msg["subject"])
    if match:
        score.add("deadline_language", w["deadline_language"], 'time-bound: "%s"' % match.group(0))
        asked = True

    if msg["has_calendar_part"]:
        score.add("calendar_invite", w["calendar_invite"], "contains a calendar invite")
        asked = True

    score.flags["reply_expected"] = asked
    return score


def _rule_sender(msg, sender_ctx, cfg, score, w):
    if _is_vip(msg, cfg):
        score.add("vip_sender", w["vip_sender"], "sender is on your VIP list")
        score.flags["vip"] = True

    ctx = (sender_ctx or {}).get(msg["from_addr"])
    if not ctx:
        return score

    wrote_to = ctx.get("i_wrote_to_count", 0)
    from_count = ctx.get("from_count", 0)

    if wrote_to >= 3:
        score.add("sender_frequent", w["sender_frequent"], "you have written to them %d+ times" % wrote_to)
    elif wrote_to >= 1:
        score.add("sender_known", w["sender_known"], "you have written to them before")
    elif from_count <= 1:
        score.add("sender_first_contact", w["sender_first_contact"], "no prior correspondence")

    score.flags["known_sender"] = wrote_to >= 1
    return score


def _rule_bulk(msg, cfg, score, w):
    hdrs = msg["headers"]
    bulk = False

    if hdrs.get("list-id") or hdrs.get("list-post"):
        score.add("list_header", w["list_header"], "List-Id: %s" % (hdrs.get("list-id", "")[:50]))
        bulk = True

    precedence = (hdrs.get("precedence") or "").lower()
    if precedence in ("bulk", "list", "junk"):
        score.add("precedence_bulk", w["precedence_bulk"], "Precedence: %s" % precedence)
        bulk = True

    if hdrs.get("list-unsubscribe") and not bulk:
        score.add("list_header", w["list_header"], "carries an unsubscribe header")
        bulk = True

    if NOREPLY_RE.search(msg["from_addr"]):
        score.add("noreply_sender", w["noreply_sender"], "sender is %s" % msg["from_addr"])
        bulk = True

    labels = set(msg["labelIds"])
    if "CATEGORY_PROMOTIONS" in labels:
        score.add("gmail_promotions", w["gmail_promotions"], "Gmail filed this as Promotions")
        bulk = True
    elif labels & GMAIL_SOFT_CATEGORIES:
        which = sorted(labels & GMAIL_SOFT_CATEGORIES)[0]
        score.add("gmail_category_other", w["gmail_category_other"], "Gmail category %s" % which)
        bulk = True

    if "CATEGORY_PERSONAL" in labels and not bulk:
        score.add("gmail_personal", w["gmail_personal"], "Gmail filed this as Personal")
        score.flags["gmail_personal"] = True

    score.flags["bulk"] = bulk
    return score


def _rule_automated(msg, cfg, score, w):
    hdrs = msg["headers"]
    automated = False

    auto = (hdrs.get("auto-submitted") or "").lower()
    if auto and auto != "no":
        score.add("auto_submitted", w["auto_submitted"], "Auto-Submitted: %s" % auto)
        automated = True

    local = msg["from_addr"].split("@", 1)[0]
    if NOTIFICATION_SENDER_RE.search(local):
        score.add("notification_sender", w["notification_sender"], "sender local-part is %s" % local)
        automated = True

    match = TRANSACTIONAL_RE.search(msg["subject"]) or TRANSACTIONAL_RE.search(msg["snippet"])
    if match:
        score.add("transactional", w["transactional"],
                  'transactional: "%s"' % match.group(0))
        automated = True

    score.flags["automated"] = automated
    return score


def _rule_spam(msg, cfg, score, w):
    hdrs = msg["headers"]
    suspicious = False

    auth = hdrs.get("authentication-results") or ""
    lowered = auth.lower()
    fails = AUTH_FAIL_RE.findall(auth)
    # DMARC alignment fails routinely on legitimate forwarded and mailing-list
    # mail. ARC exists precisely to vouch for that case, and a passing SPF+DKIM
    # pair says the same thing, so neither should read as a phishing signal.
    vouched = "arc=pass" in lowered or ("spf=pass" in lowered and "dkim=pass" in lowered)
    if fails and not vouched:
        detail = ", ".join("%s=%s" % (m, r) for m, r in fails)
        score.add("auth_failure", w["auth_failure"], "authentication %s" % detail)
        suspicious = True

    # Classic impersonation: the display name claims one domain, the envelope
    # address is another.
    name = VIA_SUFFIX_RE.sub("", msg["from_name"] or "")
    embedded = EMBEDDED_ADDR_RE.search(name)
    if embedded and msg["from_domain"] and embedded.group(1).lower() != msg["from_domain"]:
        score.add(
            "display_name_mismatch",
            w["display_name_mismatch"],
            'display name claims "%s" but sent from %s' % (embedded.group(1), msg["from_domain"]),
        )
        suspicious = True

    # Lookalike of a domain you actually deal with.
    trusted = set(cfg.get("org_domains") or [])
    trusted |= {v for v in cfg["_vip_set"] if "@" not in v}
    trusted |= {domain_of(v) for v in cfg["_vip_set"] if "@" in v}
    trusted |= set(cfg.get("funder_domains") or [])
    trusted.discard("")
    if msg["from_domain"] and msg["from_domain"] not in trusted:
        for good in trusted:
            if 0 < _edit_distance(msg["from_domain"], good) <= 2:
                score.add(
                    "lookalike_domain",
                    w["lookalike_domain"],
                    "%s closely resembles %s" % (msg["from_domain"], good),
                )
                suspicious = True
                score.flags["hard_suspicious"] = True
                break

    risky = [
        a["filename"]
        for a in msg["attachments"]
        if a["filename"].lower().endswith(RISKY_EXTENSIONS)
    ]
    if risky:
        score.add("risky_attachment", w["risky_attachment"], "attachment: %s" % ", ".join(risky[:2]))
        suspicious = True
        score.flags["hard_suspicious"] = True

    if score.flags.get("bulk") and not hdrs.get("list-unsubscribe"):
        score.add("bulk_no_unsubscribe", w["bulk_no_unsubscribe"], "bulk mail with no unsubscribe path")

    if "SPAM" in msg["labelIds"]:
        score.add("auth_failure", w["auth_failure"], "Gmail marked this as spam")
        suspicious = True
        score.flags["hard_suspicious"] = True

    score.flags["suspicious"] = suspicious
    return score


def list_keys(msg, cfg):
    """Every identity this message could be taught under as a list/group.

    The List-Id when there is one, plus any recipient address that is not you --
    group mail inside an organisation often carries no List-Id at all, and the
    group address is the only handle on it.
    """
    keys = []
    raw = msg["headers"].get("list-id") or ""
    found = LIST_ID_RE.search(raw)
    list_id = (found.group(1) if found else raw).strip().lower()
    if list_id:
        keys.append(list_id)
        # A List-Id and the group's address are the same list under two
        # spellings ("contact.carboncopies.org" / "contact@carboncopies.org").
        # Teaching either must match both.
        head, _, tail = list_id.partition(".")
        if head and tail:
            keys.append("%s@%s" % (head, tail))
    me = cfg["_me_set"]
    for entry in msg["to"] + msg["cc"]:
        addr = entry["addr"]
        if addr and addr not in me and addr not in keys:
            keys.append(addr)
    return keys


def _rule_onboarding(msg, cfg, score, w):
    """Mail that arrived through an onboarding group address.

    Checked against every identity the message carries as a group -- its
    List-Id, the address form of that List-Id, and its recipients -- so either
    spelling works in config. The Sender: header is included because a message
    Bcc'd to the group names it nowhere else.
    """
    want = cfg.get("_onboarding_set") or set()
    if not want:
        return score

    hit = next((k for k in list_keys(msg, cfg) if k in want), None)
    if not hit:
        sender = (msg["headers"].get("sender") or "").lower()
        hit = next((a for a in want if a and a in sender), None)
    if not hit:
        return score

    score.add("onboarding_group", w["onboarding_group"],
              "arrived via the onboarding group %s" % hit)
    score.flags["onboarding"] = True
    return score


def _has_promo_apparatus(msg):
    return ("CATEGORY_PROMOTIONS" in msg["labelIds"]
            or bool(msg["headers"].get("list-unsubscribe"))
            or bool(msg["headers"].get("list-id")))


def _rule_tags(msg, cfg, score, w):
    """Glanceable subsets. Tags describe a message; they do not re-file it."""
    tags = []
    haystack = (msg["subject"] or "") + " " + msg["snippet"][:300]

    # Order matters: "confirmation code" is a security code, not a booking.
    if SECURITY_RE.search(haystack) and not _has_promo_apparatus(msg):
        tags.append("security")
    elif BOOKING_RE.search(haystack) and not _has_promo_apparatus(msg):
        tags.append("travel")
    if FINANCE_RE.search(haystack) and not _has_promo_apparatus(msg):
        tags.append("finance")
    if msg["has_calendar_part"]:
        tags.append("calendar")

    hit = corrections.lookup_list(cfg.get("_corrections") or {}, list_keys(msg, cfg))
    if hit:
        key, kind, _meta = hit
        if kind == "important":
            score.add("important_list", w["important_list"],
                      "you follow the list %s" % key)
            tags.append("important_list")
            score.flags["important_list"] = True
        elif kind == "ignore":
            score.add("taught_ignore", w["taught_ignore"],
                      "you muted the list %s — filed in Bulk" % key)
            tags.append("muted_list")
            score.flags["force_bulk"] = True
            score.flags["muted_list"] = True

    score.flags["tags"] = tags
    score.flags["list_keys"] = list_keys(msg, cfg)[:3]
    return score


def _rule_corrections(msg, cfg, score, w):
    """Apply what you have taught the tool about this sender, domain or message."""
    hit = corrections.lookup(
        cfg.get("_corrections") or {}, msg["from_addr"], msg["from_domain"], msg["id"]
    )
    if not hit:
        return score
    scope, kind, entry = hit
    where = {"message": "this message", "sender": "this sender", "domain": "this domain"}[scope]
    note = (" — %s" % entry["note"]) if entry.get("note") else ""

    if entry.get("category"):
        score.flags["pin_category"] = entry["category"]
        score.add("taught_category", 0,
                  "you filed %s under %s%s" % (where, entry["category"], note), always=True)

    if kind == "personal":
        score.add("taught_personal", w["taught_personal"],
                  "you marked %s as personal correspondence%s" % (where, note))
        score.flags["never_bulk"] = True
    elif kind == "important":
        score.add("taught_important", w["taught_important"],
                  "you marked %s as always important%s" % (where, note))
        score.flags["never_bulk"] = True
    elif kind == "bulk":
        score.add("taught_bulk", w["taught_bulk"],
                  "you marked %s as bulk%s" % (where, note))
        score.flags["force_bulk"] = True
    elif kind == "ignore":
        score.add("taught_ignore", w["taught_ignore"],
                  "you marked %s as noise to ignore%s" % (where, note))
        score.flags["force_bulk"] = True

    score.flags["taught"] = "%s:%s" % (scope, kind or "category")
    return score


def _age_points(age_days, cfg, w):
    """Rise to a peak, then decay past it and go negative for very old mail."""
    a = cfg["age"]
    if age_days <= a["grace_days"]:
        return 0.0
    if age_days <= a["peak_days"]:
        span = max(a["peak_days"] - a["grace_days"], 1)
        return w["age_peak"] * (age_days - a["grace_days"]) / span
    if age_days <= a["stale_days"]:
        span = max(a["stale_days"] - a["peak_days"], 1)
        return w["age_peak"] * (1.0 - (age_days - a["peak_days"]) / span)
    span = max(a["dead_days"] - a["stale_days"], 1)
    frac = min(1.0, (age_days - a["stale_days"]) / span)
    return w["age_floor"] * frac


def _rule_misc(msg, cfg, score, w, now_ms):
    age_days = max(0.0, (now_ms - msg["internalDate"]) / 86400000.0)
    points = _age_points(age_days, cfg, w)
    if round(points, 1):
        if points > 0:
            detail = "unread for %d days" % int(age_days)
        else:
            detail = "%d days old — probably overtaken by events" % int(age_days)
        score.add("age", points, detail)

    pdfs = [a for a in msg["attachments"] if a["filename"].lower().endswith(".pdf")]
    if pdfs:
        score.add("pdf_attachment", w["pdf_attachment"], "%d PDF attachment(s)" % len(pdfs))

    score.flags["age_days"] = round(age_days, 1)
    return score


# --------------------------------------------------------------- categorize


def _categorize(score, cfg):
    f = score.flags
    t = cfg["thresholds"]
    f["reply_needed"] = bool(
        score.total >= t["needs_reply"]
        and (f.get("reply_expected") or f.get("replied_to_you"))
        and (f.get("directed_at_me") or f.get("directed_strong"))
    )
    strong = f.get("directed_strong") and score.total >= t["directed_override"]
    # What you have taught the tool outranks every inferred signal.
    never_bulk = f.get("never_bulk")

    pin = f.get("pin_category")
    if pin in CATEGORY_ORDER:
        return pin
    if f.get("force_bulk"):
        return BULK

    # A soft signal alone must not condemn someone you actually correspond with;
    # a lookalike domain or a hostile attachment condemns regardless.
    if f.get("hard_suspicious"):
        return SPAM
    if f.get("suspicious") and not (
        f.get("vip") or f.get("thread_you_participated") or f.get("known_sender")
        or never_bulk
    ):
        return SPAM
    # Intake mail is its own pile. It outranks the bulk/automated headers it
    # carries (a Google Group always looks like a list) but not spam, and not
    # anything you have explicitly taught.
    if f.get("onboarding"):
        return ONBOARDING
    if f.get("grant_related"):
        return GRANTS
    if f.get("grant_vocabulary_only") and not (
        f.get("bulk") or f.get("automated") or f.get("suspicious")
    ):
        return GRANTS
    if f.get("automated") and not strong and not never_bulk:
        return AUTOMATED
    if f.get("bulk") and not strong and not never_bulk:
        return BULK
    # "Needs Reply" must mean something is actually being asked of you. Merely
    # having spoken in a thread is not that -- it is what Threads is for.
    if f.get("reply_needed"):
        return NEEDS_REPLY
    # A conversation you are part of where nothing is being asked of you is the
    # whole point of the Threads bucket. Without this it never fires, because an
    # addressed thread message always clears the Directed threshold first.
    if f.get("thread_you_participated") and not f.get("reply_expected"):
        return THREADS
    if score.total >= t["directed"] and (f.get("directed_at_me") or f.get("directed_strong")):
        return DIRECTED
    if f.get("thread_ongoing"):
        return THREADS
    # Nothing matched. Only call it Directed when the message is actually
    # addressed to you -- otherwise it stays in bulk. Promoting everything with
    # no list header would hand the Directed bucket to every cold mailshot that
    # omits an unsubscribe link.
    if f.get("directed_at_me") or f.get("directed_strong"):
        return DIRECTED
    # If you have said this sender is never bulk, the fallback must honour that
    # too -- otherwise "never bulk" quietly fails for anything scoring low.
    if never_bulk:
        return DIRECTED
    return BULK


def _decide_needs(score, cfg, category):
    """What this message wants from you, cutting across categories.

    Kept separate from the category on purpose: a grant message can need a
    reply, and a mailing list you follow can need a glance even though it is,
    by kind, bulk.
    """
    f = score.flags
    if category == SPAM or f.get("force_bulk"):
        return NEEDS_NONE
    if f.get("reply_needed"):
        return NEEDS_REPLY_ACTION
    if category in (GRANTS, DIRECTED, THREADS, NEEDS_REPLY, ONBOARDING):
        return NEEDS_READ
    tags = f.get("tags") or []
    if (f.get("important_list") or f.get("never_bulk") or f.get("vip")
            or "travel" in tags or "security" in tags or "calendar" in tags):
        return NEEDS_READ
    return NEEDS_NONE


def score_message(msg, thread_ctx, sender_ctx, cfg, now_ms=None):
    """Evaluate one message. Returns a Score with an itemized breakdown."""
    now_ms = now_ms or int(time.time() * 1000)
    w = cfg["weights"]
    score = Score()

    _rule_directedness(msg, cfg, score, w)
    _rule_thread(msg, (thread_ctx or {}).get(msg["threadId"]), cfg, score, w)
    _rule_grants(msg, cfg, score, w)
    _rule_reply_expected(msg, cfg, score, w)
    _rule_sender(msg, sender_ctx, cfg, score, w)
    _rule_onboarding(msg, cfg, score, w)
    _rule_bulk(msg, cfg, score, w)
    _rule_automated(msg, cfg, score, w)
    _rule_spam(msg, cfg, score, w)
    _rule_tags(msg, cfg, score, w)
    _rule_corrections(msg, cfg, score, w)
    _rule_misc(msg, cfg, score, w, now_ms)

    score.reasons.sort(key=lambda r: -abs(r["points"]))
    score.category = _categorize(score, cfg)
    score.needs = _decide_needs(score, cfg, score.category)
    return score


def score_all(messages, thread_ctx, sender_ctx, cfg):
    """Score every message and return {id: score dict}, sorted per category."""
    now_ms = int(time.time() * 1000)
    return {
        m["id"]: score_message(m, thread_ctx, sender_ctx, cfg, now_ms).as_dict()
        for m in messages
    }


# --------------------------------------------------------------- LLM hookup
#
# Phase-2 slot. Wire a callable here that takes (msg, score) and returns extra
# (rule, points, detail) tuples; score_message stays deterministic until then.
LLM_HOOK = None
