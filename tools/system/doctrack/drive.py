"""Finding works in Google Drive and reading what they are.

READ-ONLY. The token carries only the drive.readonly scope, so Google rejects
any change to Drive. It lives in ~/.formalizer/config/doctrack/, separate from
the Gmail tokens, and is made with the OAuth client that daywiz and
inbox-triage already use.

A work is a Google Doc, Google Slides deck, .docx or .pptx in My Drive, a
shared drive, or shared with you, that you own, last modified, or edited at
some point. Titles and folder names decide locally which ones are internal or
too short; those are never exported.
"""

import io
import os
import re
import sys
import time
import zipfile
from datetime import datetime

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

SCOPES = ["https://www.googleapis.com/auth/drive.readonly"]
HOME = os.path.expanduser("~")
CREDENTIALS_PATH = os.path.join(HOME, ".formalizer/config/daywiz/credentials.json")
TOKEN_PATH = os.path.join(HOME, ".formalizer/config/doctrack/drive-token.json")

DOC = "application/vnd.google-apps.document"
SLIDES = "application/vnd.google-apps.presentation"
FOLDER = "application/vnd.google-apps.folder"
DOCX = "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
PPTX = "application/vnd.openxmlformats-officedocument.presentationml.presentation"
MIME_TYPES = (DOC, SLIDES, DOCX, PPTX)

MIN_WORDS = 300

# Titles or folder names that mark a document as internal or not for
# publication. Matched as whole words, case-insensitive. Edit freely.
INTERNAL = [
    "finance", "finances", "financial", "budget", "budgets", "invoice", "invoices", "expense",
    "expenses", "receipt", "receipts", "payroll", "tax", "taxes", "accounting", "bank", "reimbursement",
    "donation", "donations", "donor list", "minutes", "meeting notes", "notes", "agenda", "agendas",
    "board meeting", "standup", "1:1", "one-on-one", "hr", "hiring", "interview", "interviews",
    "onboarding", "contract", "contracts", "nda", "agreement", "bylaws", "policy", "policies",
    "todo", "to-do", "checklist", "schedule", "calendar", "password", "passwords", "credentials",
    "form responses", "template", "templates", "untitled document", "untitled presentation",
    "copy of", "inventory", "timesheet", "volunteer hours",
]
JOB_APPLICATION = ["cv", "resume", "curriculum vitae", "cover letter", "teaching statement",
                   "research statement"]
# Always kept, even when an internal word also matches.
KEEP = ["proposal", "proposals", "grant", "grants", "application", "applications", "loi",
        "letter of intent", "abstract", "paper", "manuscript", "article", "presentation", "talk",
        "poster", "chapter", "blog", "essay", "white paper", "whitepaper"]


def _words_re(words):
    return re.compile(r"(?<![\w-])(" + "|".join(re.escape(w) for w in words) + r")(?![\w-])", re.I)


INTERNAL_RE, JOB_RE, KEEP_RE = _words_re(INTERNAL), _words_re(JOB_APPLICATION), _words_re(KEEP)


class DriveError(Exception):
    pass


# ---------- auth ----------

def authorize():
    """Run the consent flow for drive.readonly. Interactive: opens a browser."""
    if not os.path.isfile(CREDENTIALS_PATH):
        raise DriveError(f"No OAuth client at {CREDENTIALS_PATH}")
    flow = InstalledAppFlow.from_client_secrets_file(CREDENTIALS_PATH, SCOPES)
    creds = flow.run_local_server(port=0)
    os.makedirs(os.path.dirname(TOKEN_PATH), exist_ok=True)
    with open(TOKEN_PATH, "w") as f:
        f.write(creds.to_json())
    os.chmod(TOKEN_PATH, 0o600)
    return creds


def have_token():
    return os.path.isfile(TOKEN_PATH)


def service():
    if not have_token():
        raise DriveError("No Drive token yet; run: doctrack.py drive-auth")
    creds = Credentials.from_authorized_user_file(TOKEN_PATH, SCOPES)
    if not set(creds.scopes or SCOPES) <= set(SCOPES):
        raise DriveError(f"Drive token carries unexpected scopes: {creds.scopes}")
    if creds.expired and creds.refresh_token:
        creds.refresh(Request())
        with open(TOKEN_PATH, "w") as f:
            f.write(creds.to_json())
    return build("drive", "v3", credentials=creds, cache_discovery=False)


def _retry(fn, attempts=5):
    for i in range(attempts):
        try:
            return fn()
        except HttpError as e:
            if e.resp.status in (403, 429, 500, 503) and i < attempts - 1 and \
                    ("rate" in str(e).lower() or e.resp.status != 403):
                time.sleep(2 ** i)
                continue
            raise


# ---------- listing ----------

def _list(svc, q, fields):
    out, token = [], None
    while True:
        resp = _retry(lambda: svc.files().list(
            q=q, fields=f"nextPageToken, files({fields})", pageSize=1000, pageToken=token,
            corpora="allDrives", includeItemsFromAllDrives=True, supportsAllDrives=True).execute())
        out.extend(resp.get("files", []))
        token = resp.get("nextPageToken")
        if not token:
            return out


def folder_paths(svc):
    """Folder id -> path such as 'My Drive/Research/WBE' or '<shared drive>/Grants'."""
    drives = {}
    token = None
    while True:
        resp = _retry(lambda: svc.drives().list(pageSize=100, pageToken=token).execute())
        for d in resp.get("drives", []):
            drives[d["id"]] = d["name"]
        token = resp.get("nextPageToken")
        if not token:
            break
    root = _retry(lambda: svc.files().get(fileId="root", fields="id").execute())["id"]
    folders = {f["id"]: f for f in _list(svc, f"mimeType = '{FOLDER}' and trashed = false",
                                          "id, name, parents")}
    cache = {root: "My Drive", **drives}

    def path(fid, depth=0):
        if fid in cache:
            return cache[fid]
        f = folders.get(fid)
        if not f or depth > 40:
            return "Shared with me"
        parent = (f.get("parents") or [None])[0]
        cache[fid] = (path(parent, depth + 1) if parent else "Shared with me") + "/" + f["name"]
        return cache[fid]

    return path


def list_files(svc):
    types = " or ".join(f"mimeType = '{m}'" for m in MIME_TYPES)
    return _list(svc, f"({types}) and trashed = false",
                 "id, name, mimeType, parents, createdTime, modifiedTime, webViewLink, driveId, "
                 "ownedByMe, owners(displayName, me), lastModifyingUser(displayName, me)")


def edited_by_me(svc, file_id):
    """True when any revision of the file was made by you."""
    token = None
    while True:
        try:
            resp = _retry(lambda: svc.revisions().list(
                fileId=file_id, fields="nextPageToken, revisions(lastModifyingUser(me))",
                pageSize=1000, pageToken=token).execute())
        except HttpError:
            return False  # revisions not readable with this access level
        if any(r.get("lastModifyingUser", {}).get("me") for r in resp.get("revisions", [])):
            return True
        token = resp.get("nextPageToken")
        if not token:
            return False


# ---------- text ----------

def _xml_text(xml, tag):
    parts = re.findall(rf"<{tag}(?:\s[^>]*)?>([^<]*)</{tag}>", xml)
    return " ".join(p for p in parts)


def _office_text(data, mime):
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        if mime == DOCX:
            xml = z.read("word/document.xml").decode("utf-8", "replace")
            paras = re.split(r"</w:p>", xml)
            return "\n".join(_xml_text(p, "w:t") for p in paras)
        slides = sorted((n for n in z.namelist() if re.fullmatch(r"ppt/slides/slide\d+\.xml", n)),
                        key=lambda n: int(re.search(r"(\d+)\.xml", n).group(1)))
        return "\n\f\n".join(_xml_text(z.read(n).decode("utf-8", "replace"), "a:t") for n in slides)


def text_of(svc, f):
    import html
    if f["mimeType"] in (DOC, SLIDES):
        data = _retry(lambda: svc.files().export(fileId=f["id"], mimeType="text/plain").execute())
        return data.decode("utf-8", "replace")
    data = _retry(lambda: svc.files().get_media(fileId=f["id"], supportsAllDrives=True).execute())
    return html.unescape(_office_text(data, f["mimeType"]))


# ---------- classification ----------

def kind_of(f, path):
    t = f["name"].lower()
    if f["mimeType"] in (SLIDES, PPTX):
        return "poster" if "poster" in t else "slides"
    if re.search(r"proposal|grant|application|\bloi\b|letter of intent", t):
        return "proposal"
    if "abstract" in t:
        return "abstract"
    if "poster" in t:
        return "poster"
    return "document"


def exclusion_by_name(name, folder):
    """Reason to leave a file out, judged by title and folder only, or ''."""
    if KEEP_RE.search(name):
        return ""
    if JOB_RE.search(name):
        return "job application"
    if INTERNAL_RE.search(name) or INTERNAL_RE.search(folder):
        return "internal"
    return ""


def epoch(ts):
    return datetime.fromisoformat(ts.replace("Z", "+00:00")).timestamp()
