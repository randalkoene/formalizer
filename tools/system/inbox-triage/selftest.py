#!/usr/bin/env python3
"""Offline checks for the scoring rules. No network, no cache, no mailbox.

Run:  ./selftest.py
"""

import copy
import sys
import time

import actions as gmail_actions
import gmail_client
import scoring
from store import domain_of

ME = "rkoene@carboncopies.org"
NOW = int(time.time() * 1000)

CFG = scoring._deep_merge(
    scoring.DEFAULT_CONFIG,
    {
        "me": {"addresses": [ME], "names": ["randal", "koene"]},
        "org_addresses": ["contact@carboncopies.org"],
        "vips": ["kenneth.hayworth@gmail.com"],
        "grants": [
            {
                "name": "Templeton LOI",
                "domains": ["templetonworldcharity.org"],
                "keywords": ["whole brain emulation loi"],
                "thread_ids": ["thread-grant-1"],
            }
        ],
    },
)
CFG["_me_set"] = {ME}
CFG["_org_set"] = {"contact@carboncopies.org"}
CFG["_vip_set"] = {"kenneth.hayworth@gmail.com"}
CFG["_onboarding_set"] = {"onboarding@carboncopies.org"}


def msg(**kw):
    base = {
        "id": kw.get("id", "m1"),
        "threadId": kw.get("threadId", "t1"),
        "labelIds": ["INBOX", "UNREAD"],
        "internalDate": NOW - 2 * 86400000,
        "snippet": "",
        "subject": "",
        "from_name": "",
        "from_addr": "someone@example.org",
        "to": [],
        "cc": [],
        "reply_to": [],
        "delivered_to": "",
        "body_text": "",
        "attachments": [],
        "has_calendar_part": False,
        "headers": {},
    }
    base.update(kw)
    base["from_domain"] = domain_of(base["from_addr"])
    base["to"] = [{"name": "", "addr": a} for a in kw.get("to", [])]
    base["cc"] = [{"name": "", "addr": a} for a in kw.get("cc", [])]
    return base


CASES = []


def case(name, message, threads, senders, expect_category, expect_rules=()):
    CASES.append((name, message, threads, senders, expect_category, expect_rules))


# 1. A collaborator replies inside a thread you are part of and asks you something.
case(
    "collaborator reply in your thread",
    msg(
        id="m1", threadId="t1",
        from_addr="kenneth.hayworth@gmail.com", from_name="Ken Hayworth",
        to=[ME],
        subject="Re: validation metrics draft",
        body_text="Hi Randal,\n\nCould you review the attached section before Friday?\n"
                  "Does the metric definition still match what you had in mind?\n",
        headers={"in-reply-to": "<mine-123@mail.gmail.com>"},
    ),
    {"t1": {"message_count": 4, "i_sent_count": 2,
            "my_message_ids": ["<mine-123@mail.gmail.com>"],
            "last_from_me": NOW - 3 * 86400000}},
    {"kenneth.hayworth@gmail.com": {"from_count": 20, "i_wrote_to_count": 20}},
    scoring.NEEDS_REPLY,
    ("to_sole_recipient", "thread_you_participated", "direct_reply_to_your_message",
     "action_request", "vip_sender", "personal_salutation"),
)

# 2. A newsletter you are merely on the distribution of.
case(
    "newsletter",
    msg(
        id="m2", threadId="t2",
        from_addr="news@neurotechreports.com", from_name="Neurotech Reports",
        to=["subscribers@neurotechreports.com"],
        subject="December industry roundup",
        body_text="This month in neurotechnology...",
        headers={"list-id": "<news.neurotechreports.com>",
                 "list-unsubscribe": "<https://example.com/u>",
                 "precedence": "bulk"},
        labelIds=["INBOX", "UNREAD", "CATEGORY_PROMOTIONS"],
    ),
    {"t2": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"news@neurotechreports.com": {"from_count": 25, "i_wrote_to_count": 0}},
    scoring.BULK,
    ("list_header", "gmail_promotions", "not_addressed"),
)

# 3. Phishing: lookalike domain, failed auth, executable attachment.
case(
    "phishing with lookalike domain",
    msg(
        id="m3", threadId="t3",
        from_addr="billing@carboncopies.org.secure-verify.net",
        from_name="Carboncopies Finance <finance@carboncopies.org>",
        to=[ME],
        subject="Urgent: verify your account",
        body_text="Please confirm your credentials as soon as possible.",
        attachments=[{"filename": "invoice.exe", "mimeType": "application/x-msdownload", "size": 1}],
        headers={"authentication-results": "mx.google.com; spf=fail; dkim=fail; dmarc=fail"},
    ),
    {"t3": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {},
    scoring.SPAM,
    ("auth_failure", "display_name_mismatch", "risky_attachment"),
)

# 4. A funder writes about a submitted application.
case(
    "funder about an application",
    msg(
        id="m4", threadId="thread-grant-1",
        from_addr="programs@templetonworldcharity.org", from_name="TWCF Programs",
        to=[ME],
        subject="Your letter of intent — next steps",
        body_text="Dear Dr. Koene, the review panel has completed its assessment "
                  "of your proposal. The submission id is TWCF-2024-118.",
    ),
    {"thread-grant-1": {"message_count": 2, "i_sent_count": 1, "my_message_ids": []}},
    {"programs@templetonworldcharity.org": {"from_count": 3, "i_wrote_to_count": 2}},
    scoring.GRANTS,
    ("grant_thread_match", "grant_funder_domain", "grant_vocabulary"),
)

# 5. Machine-generated notification.
case(
    "automated notification",
    msg(
        id="m5", threadId="t5",
        from_addr="notifications@github.com", from_name="GitHub",
        to=[ME],
        subject="[org/repo] Run failed",
        body_text="The workflow run failed.",
        headers={"auto-submitted": "auto-generated"},
    ),
    {"t5": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"notifications@github.com": {"from_count": 25, "i_wrote_to_count": 0}},
    scoring.AUTOMATED,
    ("auto_submitted", "notification_sender"),
)

# 6. Cold outreach addressed to the Foundation, not to a list.
case(
    "cold outreach to the Foundation",
    msg(
        id="m6", threadId="t6",
        from_addr="student@university.edu", from_name="A Student",
        to=["contact@carboncopies.org"],
        subject="Question about your research direction",
        body_text="Hello, I am writing to ask whether your team takes interns. "
                  "Would you be able to point me to the right person?",
    ),
    {"t6": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"student@university.edu": {"from_count": 1, "i_wrote_to_count": 0}},
    scoring.DIRECTED,
    ("org_address", "sender_first_contact"),
)


# 7. Forwarded list mail: DMARC fails on the forwarding hop but ARC vouches for
#    it. This must not read as phishing -- it is how most legitimate mailing-list
#    and alias-forwarded mail arrives.
case(
    "forwarded list mail with dmarc=fail",
    msg(
        id="m7", threadId="t7",
        from_addr="b@nectome.com", from_name="Someone via BPF Staff",
        to=["bpf-staff@googlegroups.com"],
        subject="Re: items for tonight's meeting",
        body_text="Agenda attached.",
        headers={
            "authentication-results":
                "mx.google.com; dkim=pass header.i=@googlegroups.com; arc=pass "
                "(i=3 dkim=pass); spf=pass; dmarc=fail",
            "list-id": "<bpf-staff.googlegroups.com>",
        },
    ),
    {"t7": {"message_count": 3, "i_sent_count": 0, "my_message_ids": []}},
    {"b@nectome.com": {"from_count": 5, "i_wrote_to_count": 0}},
    scoring.BULK,
    ("list_header",),
)

# 8. Google Groups rewrites the display name as "someone via Group Name". That
#    is not an embedded address and must not read as impersonation.
case(
    "google groups via-rewrite is not impersonation",
    msg(
        id="m8", threadId="t8",
        from_addr="website@carboncopies.org",
        from_name="noreply-spamdigest via Website@Carboncopies",
        to=[ME],
        subject="[website] Moderator's spam report",
        body_text="Weekly digest.",
        headers={"authentication-results": "mx.google.com; dkim=pass; spf=pass; dmarc=pass",
                 "auto-submitted": "auto-generated"},
    ),
    {"t8": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"website@carboncopies.org": {"from_count": 25, "i_wrote_to_count": 0}},
    scoring.AUTOMATED,
    (),
)

# 9. A real phish sent directly: hard SPF failure, no ARC to vouch for it,
#    unknown sender.
case(
    "direct phish with hard spf failure",
    msg(
        id="m9", threadId="t9",
        from_addr="secure@account-verify-mail.top",
        to=[ME],
        subject="Mailbox suspension notice",
        body_text="Verify your password immediately.",
        headers={"authentication-results": "mx.google.com; spf=fail; dkim=fail; dmarc=fail"},
    ),
    {"t9": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"secure@account-verify-mail.top": {"from_count": 1, "i_wrote_to_count": 0}},
    scoring.SPAM,
    ("auth_failure",),
)


# 10. A forwarded alias: addressed to another address of yours, not to the
#     mailbox address. Without the alias this scores not_addressed and falls
#     through to bulk -- which is how personal mail got filed as a newsletter.
ALIAS_CFG = scoring._deep_merge(CFG, {})
ALIAS_CFG["_me_set"] = CFG["_me_set"] | {"randal.alias@gmail.com"}
ALIAS_CFG["_org_set"] = CFG["_org_set"]
ALIAS_CFG["_vip_set"] = CFG["_vip_set"]
ALIAS_CFG["_corrections"] = {"senders": {}, "domains": {}, "messages": {}, "my_addresses": []}

# 11-13 exercise the corrections layer.
TAUGHT_PERSONAL = {
    "senders": {"mum@example.net": {"kind": "personal", "note": "my parents"}},
    "domains": {}, "messages": {}, "my_addresses": [],
}
TAUGHT_BULK = {
    "senders": {}, "domains": {"newsy.example": {"kind": "bulk"}},
    "messages": {}, "my_addresses": [],
}
TAUGHT_PIN = {
    "senders": {}, "domains": {},
    "messages": {"pinme": {"category": "grants"}}, "my_addresses": [],
}


# A volunteer application arriving through the onboarding group. It carries
# every marker of a mailing list, and must still escape Bulk.
case(
    "volunteer application via the onboarding group",
    msg(
        id="m20", threadId="t20",
        from_addr="hopeful@gmail.com", from_name="A Hopeful Volunteer",
        to=["onboarding@carboncopies.org"],
        subject="[onboarding] Application for an internship",
        body_text="Dear Carboncopies, please find my resume attached.",
        attachments=[{"filename": "resume.pdf", "mimeType": "application/pdf", "size": 90000}],
        headers={"list-id": "<onboarding.carboncopies.org>",
                 "precedence": "list",
                 "sender": "onboarding@carboncopies.org",
                 "list-unsubscribe": "<https://groups.google.com/a/carboncopies.org/x>"},
    ),
    {"t20": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {},
    scoring.ONBOARDING,
    ("onboarding_group", "list_header"),
)

# Taught rules still outrank it: muting the group puts it back in Bulk.
def _cfg_with(corr, me_extra=()):
    cfg = scoring._deep_merge(CFG, {})
    cfg["_me_set"] = CFG["_me_set"] | set(me_extra)
    cfg["_org_set"] = CFG["_org_set"]
    cfg["_vip_set"] = CFG["_vip_set"]
    cfg["_onboarding_set"] = CFG["_onboarding_set"]
    cfg["_corrections"] = corr
    return cfg


EXTRA = []


def extra_case(name, message, threads, senders, cfg, expect_category, expect_rules=()):
    EXTRA.append((name, message, threads, senders, cfg, expect_category, expect_rules))


extra_case(
    "mail to a forwarded alias of yours",
    msg(id="m10", threadId="t10", from_addr="friend@example.net", from_name="A Friend",
        to=["randal.alias@gmail.com"],
        subject="Winterwetter",
        body_text="Hallo Randal, hier ist es kalt.",
        labelIds=["INBOX", "UNREAD", "CATEGORY_PERSONAL"]),
    {"t10": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"friend@example.net": {"from_count": 20, "i_wrote_to_count": 20}},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": []},
              me_extra=["randal.alias@gmail.com"]),
    scoring.DIRECTED,
    ("to_sole_recipient", "gmail_personal"),
)

extra_case(
    "taught personal beats every bulk signal",
    msg(id="m11", threadId="t11", from_addr="mum@example.net",
        to=["list@example.net"],
        subject="Family news",
        body_text="Hello Randal.",
        headers={"list-id": "<fam.example.net>", "precedence": "bulk"},
        labelIds=["INBOX", "UNREAD", "CATEGORY_PROMOTIONS"]),
    {"t11": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"mum@example.net": {"from_count": 25, "i_wrote_to_count": 25}},
    _cfg_with(TAUGHT_PERSONAL),
    scoring.DIRECTED,
    ("taught_personal",),
)

extra_case(
    "taught bulk at the domain level forces bulk",
    msg(id="m12", threadId="t12", from_addr="editor@newsy.example",
        to=[ME], subject="A personal note just for you",
        body_text="Dear Randal, could you reply to this?"),
    {"t12": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"editor@newsy.example": {"from_count": 5, "i_wrote_to_count": 2}},
    _cfg_with(TAUGHT_BULK),
    scoring.BULK,
    ("taught_bulk",),
)

extra_case(
    "pinning one message wins outright",
    msg(id="pinme", threadId="t13", from_addr="whoever@example.net",
        to=[ME], subject="Nothing grant-like here", body_text="Hi."),
    {"t13": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {},
    _cfg_with(TAUGHT_PIN),
    scoring.GRANTS,
    ("taught_category",),
)

extra_case(
    "no_reply with an underscore is still a robot",
    msg(id="m14", threadId="t14", from_addr="no_reply@utility.example",
        to=[ME], subject="Your statement",
        body_text="Your statement is ready. Could you review it by Friday?"),
    {"t14": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"no_reply@utility.example": {"from_count": 9, "i_wrote_to_count": 0}},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": []}),
    scoring.AUTOMATED,
    ("noreply_sender", "transactional"),
)

extra_case(
    "a confirmation code is transactional, not a conversation",
    msg(id="m15", threadId="t15", from_addr="info@bigsite.example", from_name="BigSite",
        to=[ME], subject="Your BigSite confirmation code is 9yq4ecbv",
        body_text="Enter this code to continue. Did you not request this?",
        labelIds=["INBOX", "UNREAD", "CATEGORY_PERSONAL"]),
    {"t15": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"info@bigsite.example": {"from_count": 12, "i_wrote_to_count": 0}},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": []}),
    scoring.AUTOMATED,
    ("transactional",),
)


# ---- age curve, tags, lists and the needs axis -------------------------

extra_case(
    "a year-old unanswered message is demoted, not deleted",
    msg(id="m16", threadId="t16", from_addr="someone@example.net", to=[ME],
        subject="Re: that thing we discussed",
        body_text="Could you let me know?",
        internalDate=NOW - 400 * 86400000),
    {"t16": {"message_count": 2, "i_sent_count": 1, "my_message_ids": []}},
    {"someone@example.net": {"from_count": 4, "i_wrote_to_count": 4}},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": [], "lists": {}}),
    # Still needs a reply -- age demotes it by 20 points against a fresh copy,
    # but does not pretend the question went away. The list view folds it under
    # "older than N days" rather than hiding it.
    scoring.NEEDS_REPLY,
    ("age",),
)

extra_case(
    "a list you follow is still bulk, but worth reading",
    msg(id="m17", threadId="t17", from_addr="colleague@example.org",
        to=["bpf-staff@googlegroups.com"],
        subject="Agenda for Thursday",
        body_text="Notes attached.",
        headers={"list-id": "<bpf-staff.googlegroups.com>"}),
    {"t17": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": [],
               "lists": {"bpf-staff.googlegroups.com": {"kind": "important"}}}),
    scoring.BULK,
    ("important_list",),
)

extra_case(
    "muting the onboarding group still outranks the category",
    msg(id="m21", threadId="t21", from_addr="hopeful2@gmail.com",
        to=["onboarding@carboncopies.org"],
        subject="[onboarding] Volunteering",
        body_text="I would like to help.",
        headers={"list-id": "<onboarding.carboncopies.org>"}),
    {"t21": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": [],
               "lists": {"onboarding@carboncopies.org": {"kind": "ignore"}}}),
    scoring.BULK,
    ("taught_ignore", "onboarding_group"),
)

extra_case(
    "teaching a list by its address matches its List-Id too",
    msg(id="m18", threadId="t18", from_addr="someone@example.org",
        to=["contact@carboncopies.org"],
        subject="New enquiry",
        body_text="Hello.",
        headers={"list-id": "<contact.carboncopies.org>"}),
    {"t18": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": [],
               "lists": {"contact@carboncopies.org": {"kind": "important"}}}),
    scoring.BULK,
    ("important_list",),
)

extra_case(
    "a real booking confirmation is tagged travel",
    msg(id="m19", threadId="t19", from_addr="confirm@southwest.example", from_name="Southwest",
        to=[ME], subject="You're going to Portland on 09/18 (C8ZESI)!",
        body_text="Your itinerary for the trip. Confirmation number C8ZESI."),
    {"t19": {"message_count": 1, "i_sent_count": 0, "my_message_ids": []}},
    {"confirm@southwest.example": {"from_count": 2, "i_wrote_to_count": 0}},
    _cfg_with({"senders": {}, "domains": {}, "messages": {}, "my_addresses": [], "lists": {}}),
    scoring.DIRECTED,
    (),
)


def _assert_no_rule(name, message, threads, senders, rule):
    score = scoring.score_message(message, threads, senders, CFG, now_ms=NOW)
    fired = {r["rule"] for r in score.reasons}
    if rule in fired:
        print("FAIL  %-38s rule %s should not have fired" % (name, rule))
        return 1
    print("ok    %-38s (%s correctly silent)" % (name, rule))
    return 0


def main():
    failures = 0
    for name, message, threads, senders, expect_cat, expect_rules in CASES:
        score = scoring.score_message(message, threads, senders, CFG, now_ms=NOW)
        fired = {r["rule"] for r in score.reasons}
        problems = []

        if score.category != expect_cat:
            problems.append("category %s, expected %s" % (score.category, expect_cat))
        missing = [r for r in expect_rules if r not in fired]
        if missing:
            problems.append("rules did not fire: %s" % ", ".join(missing))

        status = "FAIL" if problems else "ok  "
        print("%s  %-38s score=%6.1f  %s" % (status, name, score.total, score.category))
        for problem in problems:
            print("      -> %s" % problem)
            failures += 1
        if problems:
            for r in score.reasons:
                print("         %+7g  %-30s %s" % (r["points"], r["rule"], r["detail"]))

    for name, message, threads, senders, cfg, expect_cat, expect_rules in EXTRA:
        score = scoring.score_message(message, threads, senders, cfg, now_ms=NOW)
        fired = {r["rule"] for r in score.reasons}
        problems = []
        if score.category != expect_cat:
            problems.append("category %s, expected %s" % (score.category, expect_cat))
        missing = [r for r in expect_rules if r not in fired]
        if missing:
            problems.append("rules did not fire: %s" % ", ".join(missing))
        print("%s  %-38s score=%6.1f  %s"
              % ("FAIL" if problems else "ok  ", name, score.total, score.category))
        for problem in problems:
            print("      -> %s" % problem)
            failures += 1
        if problems:
            for r in score.reasons:
                print("         %+7g  %-30s %s" % (r["points"], r["rule"], r["detail"]))

    # ---- the needs axis and tags, checked directly --------------------
    def check(name, got, want):
        nonlocal_failures[0] += 0 if got == want else 1
        print("%s  %-38s %s" % ("ok  " if got == want else "FAIL", name,
                                got if got == want else "got %r, want %r" % (got, want)))

    nonlocal_failures = [0]
    base = {"senders": {}, "domains": {}, "messages": {}, "my_addresses": [], "lists": {}}

    # A grant message that asks something must read as "reply", even though its
    # category is Grants -- that is the point of a separate axis.
    grant_ask = msg(id="g1", threadId="tg", from_addr="programs@templetonworldcharity.org",
                    to=[ME], subject="Your letter of intent — action needed",
                    body_text="Dear Dr. Koene, could you confirm the budget by Friday?")
    sc = scoring.score_message(grant_ask, {"tg": {"message_count": 1, "i_sent_count": 0,
                                                 "my_message_ids": []}},
                               {}, _cfg_with(base), now_ms=NOW)
    check("grant that asks something needs a reply", (sc.category, sc.needs),
          (scoring.GRANTS, scoring.NEEDS_REPLY_ACTION))

    # Onboarding mail is a glance, not a reply -- and it is read, not noise,
    # even though every list header on it says otherwise.
    intake = msg(id="o1", threadId="to", from_addr="hopeful@gmail.com",
                 to=["onboarding@carboncopies.org"],
                 subject="[onboarding] Resume for your consideration",
                 body_text="Hello, I would like to volunteer.",
                 headers={"list-id": "<onboarding.carboncopies.org>",
                          "precedence": "list"})
    sc = scoring.score_message(intake, {"to": {"message_count": 1, "i_sent_count": 0,
                                               "my_message_ids": []}},
                               {}, _cfg_with(base), now_ms=NOW)
    check("onboarding mail is filed to read, not to reply", (sc.category, sc.needs),
          (scoring.ONBOARDING, scoring.NEEDS_READ))

    # Airline marketing must NOT be tagged travel.
    promo = msg(id="p1", threadId="tp", from_addr="deals@delta.example", from_name="Delta",
                to=[ME], subject="Last Chance: Get Up To 50% More When You Buy Miles",
                body_text="Your trip to anywhere. Book your flight now.",
                headers={"list-unsubscribe": "<https://x/u>"},
                labelIds=["INBOX", "UNREAD", "CATEGORY_PROMOTIONS"])
    sc = scoring.score_message(promo, {"tp": {"message_count": 1, "i_sent_count": 0,
                                              "my_message_ids": []}},
                               {}, _cfg_with(base), now_ms=NOW)
    check("airline marketing is not tagged travel", "travel" in (sc.flags.get("tags") or []), False)
    check("airline marketing needs nothing", sc.needs, scoring.NEEDS_NONE)

    # An ignored list is forced down regardless of how personal it looks.
    ign = msg(id="i1", threadId="ti", from_addr="person@example.org", to=[ME],
              subject="Could you take a look at this?",
              body_text="Dear Randal, please review.",
              headers={"list-id": "<chatter.example.org>"})
    sc = scoring.score_message(ign, {"ti": {"message_count": 1, "i_sent_count": 0,
                                            "my_message_ids": []}}, {},
                               _cfg_with(dict(base, lists={"chatter.example.org": {"kind": "ignore"}})),
                               now_ms=NOW)
    check("a muted list is filed in Bulk, not removed", (sc.category, sc.needs),
          (scoring.BULK, scoring.NEEDS_NONE))
    check("a muted list is tagged so you can still find it",
          "muted_list" in (sc.flags.get("tags") or []), True)

    # ---- the invariant everything else depends on ----------------------
    # Every message must end up in exactly one real category. Nothing may be
    # dropped, hidden, or left uncategorised -- actions that modify Gmail will
    # be driven off these buckets, so a message falling out of all of them
    # would be a message silently exempt from whatever you decide to do.
    every = [c for (_n, m, th, sn, cfg_, _c, _r) in
             [(n, m, th, sn, cf, c, r) for n, m, th, sn, cf, c, r in EXTRA]
             for c in [scoring.score_message(m, th, sn, cfg_, now_ms=NOW).category]]
    every += [scoring.score_message(m, th, sn, CFG, now_ms=NOW).category
              for _n, m, th, sn, _c, _r in CASES]
    check("every message lands in a valid category",
          all(c in scoring.CATEGORY_ORDER for c in every), True)
    check("no message is left uncategorised", len(every), len(EXTRA) + len(CASES))

    # Same check against the real cache, when there is one.
    try:
        import store as _store
        cached = _store.load_json(_store.MESSAGES_FILE)
        cached_scores = _store.load_json(_store.SCORES_FILE)
        if cached and cached_scores:
            cats = [cached_scores.get(m["id"], {}).get("category") for m in cached]
            check("real cache: every message categorised",
                  all(c in scoring.CATEGORY_ORDER for c in cats), True)
            check("real cache: category counts sum to the message total",
                  len(cats), len(cached))
    except Exception as exc:
        print("ok    (no cache to check: %s)" % exc)

    failures += nonlocal_failures[0]

    # ---- the action layer ---------------------------------------------
    # The catch-all view must offer nothing, so that choosing an action always
    # means having looked at a category first.
    check("'all messages' offers no actions", gmail_actions.available(None, None), [])
    check("bulk offers trash", "trash" in [a["key"] for a in gmail_actions.available("bulk")], True)
    check("onboarding offers mark read",
          "mark_read" in [a["key"] for a in gmail_actions.available("onboarding")], True)
    check("onboarding offers NO trash — these are kept for reference",
          "trash" in [a["key"] for a in gmail_actions.available("onboarding")], False)
    check("needs_reply does NOT offer trash",
          "trash" in [a["key"] for a in gmail_actions.available("needs_reply")], False)
    check("multi-select covers every real category",
          gmail_actions.MULTI_SELECT_CATEGORIES == set(gmail_actions.PER_CATEGORY), True)
    for cat in ("needs_reply", "grants", "directed", "threads", "onboarding"):
        check("batch in %s still offers no trash" % cat,
              "trash" in [a["key"] for a in gmail_actions.available(cat)], False)
    check("trash is marked destructive",
          gmail_actions.describe("trash")["destructive"], True)
    check("mark_read is not marked destructive",
          gmail_actions.describe("mark_read")["destructive"], False)

    # The write scope must stay at gmail.modify. mail.google.com would allow
    # permanent deletion, which would make trash irreversible.
    check("write scope cannot delete permanently",
          gmail_client.WRITE_SCOPES, ["https://www.googleapis.com/auth/gmail.modify"])
    check("write token is kept out of the shared read-only token",
          gmail_client.WRITE_TOKEN_PATH != gmail_client.TOKEN_PATH, True)

    # A dry run must not touch Gmail or the log.
    before = len(gmail_actions.read_log(limit=10000))
    res = gmail_actions.apply("trash", ["fake1", "fake2"], dry_run=True)
    check("dry run reports without acting", (res["ok"], res["dry_run"]), (True, True))
    check("dry run writes nothing to the action log",
          len(gmail_actions.read_log(limit=10000)), before)

    try:
        gmail_actions.apply("not_a_real_action", ["x"])
        check("unknown actions are refused", False, True)
    except ValueError:
        check("unknown actions are refused", True, True)

    # Negative checks: rules that must stay silent.
    failures += _assert_no_rule(
        "via-rewrite display name",
        msg(id="n1", from_addr="website@carboncopies.org",
            from_name="noreply-spamdigest via Website@Carboncopies", to=[ME]),
        {}, {}, "display_name_mismatch")
    failures += _assert_no_rule(
        "arc-vouched forwarded mail",
        msg(id="n2", from_addr="b@nectome.com", to=[ME],
            headers={"authentication-results": "arc=pass; spf=pass; dkim=pass; dmarc=fail"}),
        {}, {}, "auth_failure")

    print()
    if failures:
        print("%d problem(s)" % failures)
        return 1
    print("all %d cases pass" % (len(CASES) + len(EXTRA) + 29))
    return 0


if __name__ == "__main__":
    sys.exit(main())
