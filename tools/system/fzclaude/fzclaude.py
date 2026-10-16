#!/usr/bin/env python3
"""fzclaude - Scoped, audited, read-mostly access to the Formalizer for Claude.

fzclaude is the route through which Claude (Claude Code) reads Formalizer
data. It calls the regular Formalizer tools (fzgraphhtml, fzloghtml) in
their read-only modes, never the database, and filters everything they
return through the include/exclude rules in

  ~/.formalizer/fzclaude/config.json

which only you edit (fzclaude-guard.py blocks Claude from changing it).

Every call made from within Claude Code is recorded in

  ~/.formalizer/fzclaude/disclosure.jsonl

with the IDs of the Nodes and Log entries that were returned and an estimate
of their token cost, because that output is what is sent to Anthropic.

fzclaude never changes the Graph or Log on Claude's behalf. Claude can only
write a proposal (fzclaude propose <file>). You review it with
'fzclaude show <id>' and carry it out with 'fzclaude apply <id>'.

See README.md for the visibility rules and the full command list.

Randal A. Koene, 2026
"""

import argparse
import datetime
import hashlib
import html
import json
import math
import os
import re
import shlex
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

HOME = os.path.expanduser('~')
FZBASE = os.path.join(HOME, '.formalizer')
FZBIN = os.path.join(FZBASE, 'bin')
# Kept out of ~/.formalizer/config, which is a symlink into the Formalizer git
# repository: the rules name Nodes and topics and should not end up on GitHub.
STATE_DIR = os.path.join(FZBASE, 'fzclaude')
CONFIG_PATH = os.path.join(STATE_DIR, 'config.json')
SCOPE_PATH = os.path.join(STATE_DIR, 'scope.json')
SNAPSHOT_PATH = os.path.join(STATE_DIR, 'snapshot.json')
DISCLOSURE_PATH = os.path.join(STATE_DIR, 'disclosure.jsonl')
PROPOSALS_DIR = os.path.join(STATE_DIR, 'proposals')

BY_CLAUDE = 'CLAUDECODE' in os.environ
SESSION_ID = os.environ.get('CLAUDE_CODE_SESSION_ID', '')

DEFAULT_CONFIG = {
    'include': [],
    'include_topic': [],
    'include_topic_tree': [],
    'exclude': [],
    'exclude_tree': [],
    'exclude_topic': [],
    'exclude_topic_tree': [],
    'strict_shared': True,
    'show_hidden_counts': True,
    'max_nodes': 50,
    'max_log_entries': 100,
    'log_days_default': 14,
    'excerpt_chars': 100,
    'entry_chars': 600,
    'node_text_chars': 4000,
    'scope_max_age_minutes': 60,
    'scope_max_nodes': 20000,
    'node_log_scan_days': 60,
    'milestone_topic_id': 79,
    'chars_per_token': 4.0,
    'workers': 8,
}

# Descriptions of the configuration variables, used by fzclaude-web.py to build
# its configuration editor. Types: ids (Node IDs), tags (topic tags), bool, int, float.
CONFIG_SCHEMA = [
    ('include', 'ids', 'Include', 'Nodes and everything below them',
     'Each Node listed here is visible to Claude, together with every Node below it: its '
     'dependencies, their dependencies, and so on. A Milestone such as [CARBONCOPIES] is a typical choice. '
     'Nodes added under it later become visible automatically at the next scope rebuild.'),
    ('include_topic', 'tags', 'Include', 'Nodes with a topic',
     'Every Node tagged with one of these topics is visible, but not the Nodes below it.'),
    ('include_topic_tree', 'tags', 'Include', 'Nodes with a topic, and everything below them',
     'Every Node tagged with one of these topics is visible, together with the Nodes below it.'),
    ('exclude', 'ids', 'Exclude', 'Single Nodes',
     'Hides exactly these Nodes. Nodes below them stay visible if they are included some other way. '
     'Any exclude rule wins over any include rule.'),
    ('exclude_tree', 'ids', 'Exclude', 'Nodes and everything below them',
     'Hides these Nodes and every Node below them, even Nodes that an include rule would show.'),
    ('exclude_topic', 'tags', 'Exclude', 'Nodes with a topic',
     'Hides every Node tagged with one of these topics, e.g. family. Their Log entries are hidden too.'),
    ('exclude_topic_tree', 'tags', 'Exclude', 'Nodes with a topic, and everything below them',
     'Hides every Node tagged with one of these topics, and every Node below them.'),
    ('strict_shared', 'bool', 'Safety', 'Hide Nodes that are shared with something not included',
     'A Node can have several superiors. When this is on, a Node that also sits under a superior outside '
     'the included set stays hidden (include roots themselves are exempt). Safer; may hide shared work.'),
    ('show_hidden_counts', 'bool', 'Safety', 'Tell Claude how many items were withheld',
     'Output then says e.g. "(12 outside scope)". It reveals only a number, and helps Claude know that a '
     'list is incomplete.'),
    ('scope_max_nodes', 'int', 'Safety', 'Maximum Nodes in scope',
     'If the include rules reach more Nodes than this, fzclaude refuses to build a scope.'),
    ('scope_max_age_minutes', 'int', 'Safety', 'Rebuild the scope after this many minutes',
     'New Nodes stay invisible until the scope is rebuilt.'),
    ('max_nodes', 'int', 'Output size', 'Nodes per page', 'Longer lists end with a "... N more" line.'),
    ('max_log_entries', 'int', 'Output size', 'Log entries per page', 'The most recent entries are shown first.'),
    ('log_days_default', 'int', 'Output size', 'Default Log window in days',
     'Older Log entries are read only when a request asks for them explicitly.'),
    ('excerpt_chars', 'int', 'Output size', 'Excerpt length in list lines', ''),
    ('entry_chars', 'int', 'Output size', 'Log entry length', 'Longer entries are cut off unless --full is used.'),
    ('node_text_chars', 'int', 'Output size', 'Node description length', 'For the node command, unless --full is used.'),
    ('node_log_scan_days', 'int', 'Output size', 'Days scanned for a Node\'s entries in other chunks',
     'A Log entry can belong to a Node but sit in a chunk of another Node (fzlog -e -n while another chunk is '
     'open; this is how link proposals add entries). For one Node\'s Log, the last this many days are scanned '
     'for such entries. The scan happens locally: only the matching entries reach Claude.'),
    ('milestone_topic_id', 'int', 'Graph', 'Topic ID that marks Milestones',
     'Nodes with this topic are Milestones, as in nodeboard\'s "Milestones Only" view (ROADMAP_TOPIC, 79). '
     'They are marked in Claude\'s output and can be listed on their own with --milestones.'),
    ('chars_per_token', 'float', 'Performance', 'Characters per token',
     'Used for the token estimates in the disclosure log. About 4 for English text.'),
    ('workers', 'int', 'Performance', 'Parallel fzgraphhtml calls', ''),
]

# The configuration variables that determine which Nodes are visible.
SCOPE_RULE_KEYS = (
    'include', 'include_topic', 'include_topic_tree', 'exclude', 'exclude_tree',
    'exclude_topic', 'exclude_topic_tree', 'strict_shared', 'scope_max_nodes',
)

ID_RE = re.compile(r'\d{14}\.\d+')
ID_FULL_RE = re.compile(r'^\d{14}\.\d+$')
NEW_NODE_RE = re.compile(r'New Node: (\d{14}\.\d+)')
URL_RE = re.compile(r'https?://[^\s<>"\'()\[\]]+')

# fzgraphhtml custom templates. With -F json the Node text is JSON-escaped and
# superiors/dependencies are rendered as space separated IDs.
NODE_FIELDS = (
    ('id', 'node-id'), ('comp', 'comp'), ('req_mins', 'req_mins'), ('val', 'val'),
    ('eff_td', 'eff_td'), ('td_prop', 'td_prop'), ('repeats', 'repeats'),
    ('td_patt', 'td_patt'), ('td_every', 'td_every'), ('td_span', 'td_span'),
    ('topics', 'topics'), ('nnls', 'NNLs'), ('sups', 'superiors'),
    ('deps', 'dependencies'), ('text', 'node-text'),
)
NODE_TEMPLATE = 'Node=STRING:{' + ','.join(f'"{k}":"{{{{ {v} }}}}"' for k, v in NODE_FIELDS) + '}'
LIST_TEMPLATE = ('node=STRING:@@R@@{{ node_id }}@@F@@{{ rawtd }}@@F@@{{ req_hrs }}'
                 '@@F@@{{ tdprop }}@@F@@{{ topic }}@@F@@{{ excerpt }}\n')
IDLIST_TEMPLATE = 'node=STRING:@@R@@{{ node_id }}\n'
TOPICS_TEMPLATE = 'topics=STRING:@@R@@{{ tag }}@@F@@{{ title }}\n'


class ScopeError(Exception):
    pass


def fail(msg: str, code=1):
    print(f'fzclaude: {msg}', file=sys.stderr)
    sys.exit(code)


def now_stamp() -> str:
    return datetime.datetime.now().strftime('%Y%m%d%H%M')


def sha1(s: str) -> str:
    return hashlib.sha1(s.encode('utf-8', 'replace')).hexdigest()


def write_json(path: str, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + '.tmp'
    with open(tmp, 'w') as f:
        json.dump(data, f, indent=1)
    os.replace(tmp, path)


def read_json(path: str):
    with open(path) as f:
        return json.load(f)


# ---------------------------------------------------------------- configuration

def check_config(data) -> tuple:
    """Configuration merged with defaults, and a list of errors (empty if valid)."""
    if not isinstance(data, dict):
        return None, ['the configuration must be a JSON object']
    errors = []
    # A typo in a safety rule (e.g. "exlude") must not silently do nothing.
    unknown = set(data) - set(DEFAULT_CONFIG)
    if unknown:
        errors.append('unknown configuration variables: ' + ', '.join(sorted(unknown)))
    cfg = dict(DEFAULT_CONFIG)
    cfg.update(data)
    for key, typ, *_ in CONFIG_SCHEMA:
        val = cfg[key]
        if typ in ('ids', 'tags'):
            if not isinstance(val, list) or not all(isinstance(x, str) for x in val):
                errors.append(f"'{key}' must be a list of strings")
            elif typ == 'ids':
                bad = [x for x in val if not ID_FULL_RE.match(x)]
                if bad:
                    errors.append(f"'{key}' contains values that are not Node IDs: {', '.join(bad)}")
            else:
                bad = [x for x in val if not re.fullmatch(r'[^\s,()]+', x)]
                if bad:
                    errors.append(f"'{key}' contains values that are not topic tags: {', '.join(bad)}")
        elif typ == 'bool' and not isinstance(val, bool):
            errors.append(f"'{key}' must be true or false")
        elif typ == 'int' and (not isinstance(val, int) or isinstance(val, bool) or val < 1):
            errors.append(f"'{key}' must be a whole number of at least 1")
        elif typ == 'float' and (not isinstance(val, (int, float)) or isinstance(val, bool) or val <= 0):
            errors.append(f"'{key}' must be a positive number")
    return cfg, errors


def load_config() -> dict:
    if not os.path.exists(CONFIG_PATH):
        fail(f'No configuration at {CONFIG_PATH}.\n'
             "Run 'fzclaude init' yourself (not from Claude), or use the fzclaude web page.")
    try:
        data = read_json(CONFIG_PATH)
    except ValueError as e:
        fail(f'Configuration file is not valid JSON: {e}')
    cfg, errors = check_config(data)
    if errors:
        fail('Invalid configuration (refusing to run):\n  ' + '\n  '.join(errors))
    return cfg


def save_config(data: dict):
    """Validate and save a configuration, keeping the previous one in config-history/."""
    cfg, errors = check_config(data)
    if errors:
        return errors
    if os.path.exists(CONFIG_PATH):
        hist = os.path.join(STATE_DIR, 'config-history')
        os.makedirs(hist, exist_ok=True)
        stamp = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
        os.replace(CONFIG_PATH, os.path.join(hist, f'config-{stamp}.json'))
    write_json(CONFIG_PATH, {k: cfg[k] for k in DEFAULT_CONFIG})
    return []


def config_hash(cfg: dict) -> str:
    return sha1(json.dumps({k: cfg[k] for k in SCOPE_RULE_KEYS}, sort_keys=True))


# ------------------------------------------------------- calling Formalizer tools

def run_tool(prog: str, args: list, timeout=120):
    try:
        p = subprocess.run([os.path.join(FZBIN, prog)] + args, capture_output=True,
                           text=True, errors='replace', timeout=timeout)
    except (OSError, subprocess.TimeoutExpired):
        return -1, ''
    return p.returncode, p.stdout


def fetch_node(nid: str):
    """Full data of one Node as a dict of strings, or None."""
    rc, out = run_tool('fzgraphhtml', ['-n', nid, '-F', 'json', '-o', 'STDOUT', '-E', os.devnull,
                                       '-q', '-T', NODE_TEMPLATE], timeout=30)
    if rc != 0:
        return None
    try:
        d = json.loads(out, strict=False)
    except ValueError:
        return None
    if d.get('id') != nid:
        return None
    return d


def list_ids(args: list):
    """Node IDs listed by an fzgraphhtml list request (-L, -t), or None on failure."""
    rc, out = run_tool('fzgraphhtml', args + ['-N', 'all', '-F', 'txt', '-e', '-o', 'STDOUT',
                                              '-E', os.devnull, '-q', '-T', IDLIST_TEMPLATE])
    if rc != 0:
        return None
    return [r.strip() for r in out.split('@@R@@')[1:] if ID_FULL_RE.match(r.strip())]


def parse_topics(s: str) -> list:
    return re.findall(r'([^\s,()]+)\s*\(', s)


def to_float(s, default=0.0):
    try:
        return float(s)
    except (TypeError, ValueError):
        return default


def to_int(s, default=0):
    try:
        return int(float(s))
    except (TypeError, ValueError):
        return default


def html_to_text(s: str) -> str:
    s = re.sub(r'(?is)<(script|style)\b.*?</\1>', '', s)

    def link(m):
        url, label = m.group(1), re.sub(r'<[^>]+>', '', m.group(2))
        return label if url in label else f'{label} ({url})'  # not <url>: tags are stripped below

    s = re.sub(r'(?is)<a\b[^>]*?href\s*=\s*["\']([^"\']*)["\'][^>]*>(.*?)</a>', link, s)
    s = re.sub(r'(?i)<br\s*/?>|</p>|</li>|</div>|</h\d>|</tr>', '\n', s)
    s = re.sub(r'(?i)<li\b[^>]*>', '- ', s)
    s = re.sub(r'<[^>]+>', '', s)
    s = html.unescape(s)
    s = re.sub(r'[ \t]+', ' ', s)
    s = re.sub(r'\n\s*\n\s*\n+', '\n\n', s)
    return s.strip()


def one_line(s: str, n: int) -> str:
    s = ' '.join(s.split())
    return s if len(s) <= n else s[:max(n - 1, 0)] + '…'


def node_meta(d: dict) -> dict:
    text = html_to_text(d['text'])
    return {
        'sups': [x for x in d['sups'].split() if ID_FULL_RE.match(x)],
        'deps': [x for x in d['deps'].split() if ID_FULL_RE.match(x)],
        'topics': parse_topics(d['topics']),
        'nnls': [x.strip() for x in d['nnls'].split(',') if x.strip()],
        'comp': to_float(d['comp']),
        'req_mins': to_int(d['req_mins']),
        'td': d['eff_td'],
        'td_prop': d['td_prop'],
        'repeats': d['repeats'] == 'Yes',
        # Content hash excludes completion and target date, which change often.
        'chash': sha1('\x1f'.join([text, d['sups'], d['deps'], d['topics']]))[:16],
    }


class Fetcher:
    def __init__(self, workers: int):
        self.workers = max(1, workers)
        self.cache = {}

    def get_many(self, ids) -> dict:
        todo = [i for i in dict.fromkeys(ids) if i not in self.cache]
        if todo:
            with ThreadPoolExecutor(self.workers) as ex:
                for nid, d in zip(todo, ex.map(fetch_node, todo)):
                    self.cache[nid] = d
        return {i: self.cache[i] for i in ids}


# -------------------------------------------------------------------- the scope

def walk(fetcher: Fetcher, roots, limit: int):
    """Breadth-first over dependencies. Returns (reached IDs, IDs that could not be read)."""
    seen, failed = set(), set()
    frontier = list(dict.fromkeys(roots))
    while frontier:
        res = fetcher.get_many(frontier)
        nxt, nxt_set = [], set()
        for nid in frontier:
            seen.add(nid)
            d = res[nid]
            if d is None:
                failed.add(nid)
                continue
            for dep in d['deps'].split():
                if ID_FULL_RE.match(dep) and dep not in seen and dep not in nxt_set:
                    nxt.append(dep)
                    nxt_set.add(dep)
        if len(seen) + len(nxt) > limit:
            raise ScopeError(f'more than {limit} Nodes reached (scope_max_nodes); refusing to continue')
        if not BY_CLAUDE:
            print(f'\r  {len(seen)} Nodes read...', end='', file=sys.stderr)
        frontier = nxt
    if not BY_CLAUDE:
        print('', file=sys.stderr)
    return seen, failed


def compute_scope(cfg: dict) -> dict:
    f = Fetcher(cfg['workers'])
    limit = cfg['scope_max_nodes']
    notes = []

    roots = f.get_many(cfg['include'])
    missing = [r for r, d in roots.items() if d is None]
    if missing:
        raise ScopeError(f"include Node(s) not found: {', '.join(missing)}")

    tree_roots = list(cfg['include'])
    for tag in cfg['include_topic_tree']:
        ids = list_ids(['-t', tag])
        if ids is None:
            notes.append(f"include_topic_tree '{tag}': topic could not be listed, skipped")
            continue
        tree_roots += ids
    singles = []
    for tag in cfg['include_topic']:
        ids = list_ids(['-t', tag])
        if ids is None:
            notes.append(f"include_topic '{tag}': topic could not be listed, skipped")
            continue
        singles += ids

    reachable, failed = walk(f, tree_roots, limit)
    for nid, d in f.get_many(singles).items():
        if d is not None:
            reachable.add(nid)
    explicit = set(tree_roots) | set(singles)

    # Exclusion trees: explicit roots, plus every Node tagged with an exclude_topic_tree topic,
    # whether listed by fzgraphhtml -t or found among the reachable Nodes.
    excl_tree_topics = set(cfg['exclude_topic_tree'])
    excl_roots = set(cfg['exclude_tree'])
    for tag in excl_tree_topics:
        ids = list_ids(['-t', tag])
        if ids is None:
            raise ScopeError(f"exclude_topic_tree '{tag}': topic could not be listed; refusing to build scope")
        excl_roots |= set(ids)
    if excl_tree_topics:
        for nid in reachable:
            d = f.cache.get(nid)
            if d and set(parse_topics(d['topics'])) & excl_tree_topics:
                excl_roots.add(nid)
    excluded_tree = set()
    if excl_roots:
        excluded_tree, efailed = walk(f, excl_roots, limit * 4)
        if efailed - set(cfg['exclude_tree']):
            raise ScopeError(f'{len(efailed)} Nodes in excluded subtrees could not be read; refusing to build scope')

    excl_single = set(cfg['exclude'])
    excl_topics = set(cfg['exclude_topic'])
    hidden = {'unreadable': 0, 'exclude': 0, 'exclude_tree': 0, 'exclude_topic': 0, 'shared_outside': 0}
    visible = {}
    for nid in reachable:
        d = f.cache.get(nid)
        if d is None:
            hidden['unreadable'] += 1
        elif nid in excl_single:
            hidden['exclude'] += 1
        elif nid in excluded_tree:
            hidden['exclude_tree'] += 1
        elif set(parse_topics(d['topics'])) & excl_topics:
            hidden['exclude_topic'] += 1
        else:
            m = node_meta(d)
            if cfg['strict_shared'] and nid not in explicit and any(s not in reachable for s in m['sups']):
                hidden['shared_outside'] += 1
            else:
                visible[nid] = m

    scope = {
        'created': now_stamp(),
        'created_epoch': time.time(),
        'config_hash': config_hash(cfg),
        'roots': sorted(explicit & set(visible)),
        'reachable': len(reachable),
        'hidden': hidden,
        'notes': notes,
        'visible': visible,
    }
    write_json(SCOPE_PATH, scope)
    return scope


def load_scope(cfg: dict, force=False) -> dict:
    if not force and os.path.exists(SCOPE_PATH):
        try:
            s = read_json(SCOPE_PATH)
            fresh = time.time() - s['created_epoch'] < cfg['scope_max_age_minutes'] * 60
            if s['config_hash'] == config_hash(cfg) and fresh:
                return s
        except (ValueError, KeyError):
            pass
    print('fzclaude: refreshing scope...', file=sys.stderr)
    return compute_scope(cfg)


# --------------------------------------------------------- output and disclosure

class Out:
    def __init__(self):
        self.lines = []
        self.nodes = set()
        self.entries = set()

    def p(self, s=''):
        self.lines.append(s)


OUT = Out()


def emit(cmd: str, argv: list, cfg):
    text = '\n'.join(OUT.lines)
    if text:
        print(text)
    cpt = cfg['chars_per_token'] if cfg else DEFAULT_CONFIG['chars_per_token']
    tokens = math.ceil(len(text) / cpt)
    rec = {
        't': datetime.datetime.now().isoformat(timespec='seconds'),
        'session': SESSION_ID,
        'by': 'claude' if BY_CLAUDE else 'user',
        'cmd': cmd,
        'argv': argv,
        'nodes': sorted(OUT.nodes),
        'log_entries': sorted(OUT.entries),
        'chars': len(text),
        'est_tokens': tokens,
    }
    os.makedirs(STATE_DIR, exist_ok=True)
    with open(DISCLOSURE_PATH, 'a') as f:
        f.write(json.dumps(rec) + '\n')
    if not BY_CLAUDE:
        print(f'[fzclaude: run by you, not sent to Claude. ~{tokens} tokens, '
              f'{len(OUT.nodes)} Nodes, {len(OUT.entries)} Log entries]', file=sys.stderr)


def fmt_td(td: str) -> str:
    if re.fullmatch(r'\d{12}', td or ''):
        return f'{td[0:4]}-{td[4:6]}-{td[6:8]} {td[8:10]}:{td[10:12]}'
    return td or '-'


# Completion codes as in Graphtypes.hpp: active means 0 <= completion < 1 (Node::is_active()).
SPECIAL_COMPLETION = {-1: 'obsolete', -2: 'replaced', -3: 'done-differently', -4: 'no-longer-possible'}
CFG = DEFAULT_CONFIG  # the loaded configuration, set in main()
_milestone_tags = {}


def is_active(m: dict) -> bool:
    return 0.0 <= m['comp'] < 1.0


def status_word(comp: float) -> str:
    if 0.0 <= comp < 1.0:
        return f'{int(comp * 100)}%'
    if comp >= 1.0:
        return 'done'
    return SPECIAL_COMPLETION.get(round(comp), 'inactive')


def milestone_tag():
    """The tag of the topic that marks Milestones (looked up once per run), or None."""
    tid = str(CFG['milestone_topic_id'])
    if tid not in _milestone_tags:
        rc, out = run_tool('fzgraphhtml', ['-t', '?', '-F', 'txt', '-e', '-o', 'STDOUT', '-E', os.devnull, '-q',
                                           '-T', 'topics=STRING:@@R@@{{ topic_id }}@@F@@{{ tag }}\n'])
        tag = None
        if rc == 0:
            for rec in out.split('@@R@@')[1:]:
                f = rec.rstrip('\n').split('@@F@@')
                if len(f) >= 2 and f[0].strip() == tid:
                    tag = f[1].strip()
        _milestone_tags[tid] = tag
    return _milestone_tags[tid]


def is_milestone(m: dict) -> bool:
    tag = milestone_tag()
    return bool(tag) and tag in m['topics']


def node_line(nid: str, m: dict, excerpt: str) -> str:
    OUT.nodes.add(nid)
    topic = m['topics'][0] if m['topics'] else '-'
    rep = '*' if m['repeats'] else ''
    ms = ' MILESTONE' if is_milestone(m) else ''
    return (f"{nid} {fmt_td(m['td'])} {m['td_prop'][:5]:<5}{rep} {status_word(m['comp']):>4}{ms} "
            f"{m['req_mins'] / 60:5.2f}h [{topic}] {excerpt}")


def node_filter(args, vis: dict):
    """Which Nodes a listing shows: active ones unless --all, only Milestones with --milestones.
    Returns (predicate, description of what it leaves out)."""
    show_all = getattr(args, 'all', False)
    milestones = getattr(args, 'milestones', False)

    def pred(n):
        m = vis[n]
        return (show_all or is_active(m)) and (not milestones or is_milestone(m))

    parts = ([] if show_all else ['inactive']) + (['non-Milestone'] if milestones else [])
    what = ' or '.join(parts) + ' Nodes' if parts else ''
    hint = ' (use --all to include inactive Nodes)' if not show_all else ''
    return pred, what, hint


def shown_children(n: str, pred, vis: dict):
    """The nearest Nodes below n that pred accepts, looking through those it rejects.
    Returns (accepted Nodes, rejected Nodes passed through, dependencies outside scope)."""
    found, skipped, outside = [], set(), 0
    seen, stack = {n}, list(vis[n]['deps'])
    while stack:
        c = stack.pop()
        if c in seen:
            continue
        seen.add(c)
        if c not in vis:
            outside += 1
        elif pred(c):
            found.append(c)
        else:
            skipped.add(c)
            stack.extend(vis[c]['deps'])
    return found, skipped, outside


def left_out_note(n: int, what: str, hint: str) -> str:
    return f' {n} {what} left out{hint}.' if (what and n) else ''


def excerpts(cfg: dict, ids, n=None) -> dict:
    n = n or cfg['excerpt_chars']
    res = Fetcher(cfg['workers']).get_many(list(ids))
    return {i: (one_line(html_to_text(d['text']), n) if d else '(could not be read)') for i, d in res.items()}


def paged(items: list, maximum: int, offset: int, what: str):
    """The requested page of items and a trailer line (or '')."""
    page = items[offset:offset + maximum]
    rest = len(items) - offset - len(page)
    trailer = f'... {rest} more {what} (use --offset {offset + len(page)})' if rest > 0 else ''
    return page, trailer


def hidden_note(cfg: dict, n: int, what='outside scope') -> str:
    return f' ({n} {what})' if (cfg['show_hidden_counts'] and n) else ''


def out_of_scope(nid: str):
    OUT.p(f'{nid}: not in scope (outside scope or no such Node).')


# ------------------------------------------------------------ read commands

def cmd_scope(args, cfg, scope):
    if args.action == 'refresh':
        scope = load_scope(cfg, force=True)
    vis = scope['visible']
    active = sum(1 for m in vis.values() if is_active(m))
    ms = sum(1 for m in vis.values() if is_milestone(m))
    ms_active = sum(1 for m in vis.values() if is_milestone(m) and is_active(m))
    OUT.p(f"Scope built {fmt_td(scope['created'])}: {len(vis)} visible Nodes ({active} active, "
          f"{len(vis) - active} inactive; {ms} Milestones, {ms_active} of them active), "
          f"{scope['reachable']} reachable from include rules.")
    if milestone_tag() is None:
        OUT.p(f"Note: no topic with ID {cfg['milestone_topic_id']} found, so no Milestones are marked.")
    OUT.p('Hidden: ' + ', '.join(f'{k} {v}' for k, v in scope['hidden'].items()))
    for note in scope['notes']:
        OUT.p(f'Note: {note}')
    if not cfg['include'] and not cfg['include_topic'] and not cfg['include_topic_tree']:
        OUT.p('No include rules are configured, so nothing is visible.')
    roots = scope['roots']
    if roots:
        OUT.p('Roots:')
        ex = excerpts(cfg, roots)
        for r in roots:
            OUT.p('  ' + node_line(r, vis[r], ex[r]))


def cmd_today(args, cfg, scope):
    vis = scope['visible']
    rc, out = run_tool('fzgraphhtml', ['-I', '-r', '-N', 'all', '-D', str(args.days),
                                       '-x', str(cfg['excerpt_chars']), '-F', 'txt', '-e', '-o', 'STDOUT',
                                       '-E', os.devnull, '-q', '-T', LIST_TEMPLATE])
    if rc != 0:
        raise ScopeError('fzgraphhtml could not produce the Schedule list')
    rows, hidden = [], 0
    for rec in out.split('@@R@@')[1:]:
        f = rec.rstrip('\n').split('@@F@@')
        if len(f) < 6 or not ID_FULL_RE.match(f[0].strip()):
            continue
        nid = f[0].strip()
        if nid not in vis:
            hidden += 1
            continue
        prop = html_to_text(re.sub(r'(?is)<button\b.*?</button>', '', f[3]))
        rows.append((nid, f[1].strip(), f[2].strip(), prop, f[4].strip(), html_to_text(f[5])))
    OUT.p(f'Schedule, next {args.days} day(s): {len(rows)} entries in scope' + hidden_note(cfg, hidden) + '.')
    page, trailer = paged(rows, args.max or cfg['max_nodes'], args.offset, 'entries')
    for nid, td, hrs, prop, topic, exc in page:
        OUT.nodes.add(nid)
        OUT.p(f'{fmt_td(td)} {prop:<8} {hrs:>5}h {nid} [{topic}] {one_line(exc, cfg["excerpt_chars"])}')
    if trailer:
        OUT.p(trailer)


def cmd_node(args, cfg, scope):
    vis = scope['visible']
    nid = args.id
    if nid not in vis:
        return out_of_scope(nid)
    d = fetch_node(nid)
    if d is None:
        return OUT.p(f'{nid}: could not be read.')
    m = node_meta(d)
    OUT.nodes.add(nid)
    rep = f", repeats {d['td_patt']} every {d['td_every']} span {d['td_span']}" if m['repeats'] else ''
    OUT.p(f'Node {nid}' + (' (Milestone)' if is_milestone(m) else ''))
    OUT.p(f"status {status_word(m['comp'])} ({'active' if is_active(m) else 'inactive'}), completion {m['comp']:.2f}, "
          f"required {m['req_mins'] / 60:.2f}h, valuation {d['val']}")
    OUT.p(f"target date {fmt_td(m['td'])} ({m['td_prop']}){rep}")
    OUT.p('topics: ' + (', '.join(m['topics']) or '-'))
    OUT.p('NNLs: ' + (', '.join(m['nnls']) or '-'))

    pred, what, hint = node_filter(args, vis)
    sups_vis = [s for s in m['sups'] if s in vis]
    deps_in_scope = [s for s in m['deps'] if s in vis]
    deps_vis = sorted((s for s in deps_in_scope if pred(s)), key=lambda x: vis[x]['td'])
    deps_page, deps_trailer = paged(deps_vis, args.max or cfg['max_nodes'], 0, 'dependencies')
    ex = excerpts(cfg, sups_vis + deps_page)
    OUT.p(f'superiors: {len(sups_vis)}' + hidden_note(cfg, len(m['sups']) - len(sups_vis)))
    for s in sups_vis:
        OUT.p('  ' + node_line(s, vis[s], ex[s]))
    OUT.p(f'dependencies: {len(deps_vis)}' + hidden_note(cfg, len(m['deps']) - len(deps_in_scope))
          + left_out_note(len(deps_in_scope) - len(deps_vis), what, hint))
    for s in deps_page:
        OUT.p('  ' + node_line(s, vis[s], ex[s]))
    if deps_trailer:
        OUT.p('  ' + deps_trailer.replace('use --offset', f'use subtree {nid}, skip'))

    text = html_to_text(d['text'])
    limit = len(text) if args.full else cfg['node_text_chars']
    OUT.p('text:')
    OUT.p(text[:limit])
    if len(text) > limit:
        OUT.p(f'... ({len(text) - limit} more characters, use --full)')


def cmd_subtree(args, cfg, scope):
    vis = scope['visible']
    root = args.id
    if root not in vis:
        return out_of_scope(root)
    pred, what, hint = node_filter(args, vis)
    rows, seen, skipped, hidden = [], set(), set(), 0

    def visit(n, depth):
        nonlocal hidden
        if n in seen:
            rows.append((depth, n, True))
            return
        seen.add(n)
        rows.append((depth, n, False))
        if depth >= args.depth:
            return
        kids, passed, outside = shown_children(n, pred, vis)
        skipped.update(passed)
        hidden += outside
        for c in sorted(kids, key=lambda x: vis[x]['td']):
            visit(c, depth + 1)

    visit(root, 0)  # the requested Node itself is always shown
    page, trailer = paged(rows, args.max or cfg['max_nodes'], args.offset, 'lines')
    ex = excerpts(cfg, [n for _, n, again in page if not again])
    OUT.p(f'Subtree of {root} to depth {args.depth}: {len(seen)} Nodes shown'
          + hidden_note(cfg, hidden, 'dependencies outside scope') + '.'
          + left_out_note(len(skipped - seen), what, hint))
    for depth, n, again in page:
        OUT.nodes.add(n)
        OUT.p('  ' * depth + (f'{n} (shown above)' if again else node_line(n, vis[n], ex[n])))
    if trailer:
        OUT.p(trailer)


def cmd_search(args, cfg, scope):
    vis = scope['visible']
    pred, what, hint = node_filter(args, vis)
    ids = [n for n in vis if pred(n)]
    texts = Fetcher(cfg['workers']).get_many(ids)
    pattern = re.compile(args.text if args.regex else re.escape(args.text), re.IGNORECASE)
    matches = []
    for nid in ids:
        d = texts[nid]
        if d is None:
            continue
        text = ' '.join(html_to_text(d['text']).split())
        mt = pattern.search(text)
        if mt:
            a = max(0, mt.start() - 40)
            snippet = ('…' if a else '') + text[a:mt.end() + 60] + ('…' if mt.end() + 60 < len(text) else '')
            matches.append((nid, snippet))
    matches.sort(key=lambda x: vis[x[0]]['td'])
    OUT.p(f'{len(matches)} Nodes in scope match.'
          + (f' Searched {len(ids)} of {len(vis)}; {what} were not searched{hint}.' if what else ''))
    page, trailer = paged(matches, args.max or cfg['max_nodes'], args.offset, 'matches')
    for nid, snippet in page:
        OUT.p(node_line(nid, vis[nid], snippet))
    if trailer:
        OUT.p(trailer)


def parse_log_raw(text: str) -> list:
    chunks, cur, entry, buf, in_text = [], None, None, [], False
    for line in text.split('\n'):
        if in_text:
            if line == '__END__':
                entry['text'] = '\n'.join(buf)
                buf, in_text = [], False
            else:
                buf.append(line)
            continue
        if line.startswith('CHUNK:'):
            parts = line[6:].split(':')
            if len(parts) < 4:
                cur = None
                continue
            # With -n, fzloghtml wraps the open time in a temporal-context link: keep only the stamp.
            stamp = re.search(r'\d{12}', parts[0])
            cur = {'open': stamp.group(0) if stamp else parts[0].strip(), 'close': parts[1].strip(),
                   'diff': ':'.join(parts[2:-1]).strip(), 'node': parts[-1].strip(), 'entries': []}
            chunks.append(cur)
        elif line.startswith('ENTRY:') and cur is not None:
            parts = line[6:].split(':')
            own = parts[1].strip() if len(parts) > 1 else ''
            entry = {'minor': parts[0].strip(), 'own_node': own, 'node': own or cur['node'], 'text': ''}
            cur['entries'].append(entry)
        elif line == '__BEGIN__' and entry is not None:
            in_text = True
    return chunks


def log_entries(cfg, vis, t_from, t_to, node=None, search=None):
    """Visible Log entries as a chronological list of (chunk, entry), plus hidden counts."""
    a = ['-1', t_from, '-2', t_to, '-F', 'raw', '-o', 'STDOUT', '-E', os.devnull, '-q']
    if node:
        a += ['-n', node]
    if search:
        a += ['-f', search, '-A', '-C']  # -A: all search terms must be present
    rc, out = run_tool('fzloghtml', a)
    if rc != 0:
        raise ScopeError('fzloghtml could not produce the requested Log interval')
    rows, hidden_entries, hidden_chunks = [], 0, 0
    for c in parse_log_raw(out):
        shown = [e for e in c['entries'] if e['node'] in vis]
        hidden_entries += len(c['entries']) - len(shown)
        if not shown:
            hidden_chunks += 1
        for e in shown:
            rows.append((c, e))
    return rows, hidden_entries, hidden_chunks


def node_log_entries(cfg, vis, nid, t_from, t_to, search=None):
    """One Node's Log: its own chunks (fzloghtml -n selects chunks by their Node), plus entries that
    belong to it inside other chunks, e.g. those added with fzlog -e -n while another chunk was open.
    The second part scans only the last node_log_scan_days, filtered here, not sent."""
    rows, h_entries, h_chunks = log_entries(cfg, vis, t_from, t_to, node=nid, search=search)
    scan_from = (datetime.datetime.now() - datetime.timedelta(days=cfg['node_log_scan_days'])).strftime('%Y%m%d%H%M')
    scan_from = max(scan_from, t_from)
    if t_to == 'now' or t_to >= scan_from:
        have = {(c['open'], e['minor']) for c, e in rows}
        extra, _, _ = log_entries(cfg, vis, scan_from, t_to, search=search)
        rows += [(c, e) for c, e in extra if e['node'] == nid and (c['open'], e['minor']) not in have]
        rows.sort(key=lambda r: (r[0]['open'], to_int(r[1]['minor'])))
    return rows, h_entries, h_chunks


def print_log_rows(cfg, vis, rows, full=False):
    chunk_nodes = list(dict.fromkeys(c['node'] for c, _ in rows if c['node'] in vis))
    ex = excerpts(cfg, chunk_nodes, 60)
    last = None
    for c, e in rows:
        if c is not last:
            close = 'OPEN' if c['close'] in ('', 'OPEN') else fmt_td(c['close'])[11:]
            if c['node'] in vis:
                OUT.nodes.add(c['node'])
                OUT.p(f"== {fmt_td(c['open'])}-{close} ({c['diff']}) {c['node']} {ex[c['node']]}")
            else:
                OUT.p(f"== {fmt_td(c['open'])}-{close} ({c['diff']}) (chunk Node outside scope)")
            last = c
        OUT.entries.add(f"{c['open']}.{e['minor']}")
        OUT.nodes.add(e['node'])
        text = html_to_text(e['text'])
        text = text if full else one_line(text, cfg['entry_chars'])
        own = f" ({e['own_node']})" if e['own_node'] else ''
        OUT.p(f"  [{e['minor']}]{own} {text}")


def cmd_log(args, cfg, scope):
    vis = scope['visible']
    if args.node and args.node not in vis:
        return out_of_scope(args.node)
    if args.all_time:
        t_from = '199001010000'
    elif args.since:
        t_from = args.since
    else:
        days = args.days or cfg['log_days_default']
        t_from = (datetime.datetime.now() - datetime.timedelta(days=days)).strftime('%Y%m%d%H%M')
    t_to = args.until or 'now'
    if args.node:
        rows, h_entries, h_chunks = node_log_entries(cfg, vis, args.node, t_from, t_to, args.search)
    else:
        rows, h_entries, h_chunks = log_entries(cfg, vis, t_from, t_to, search=args.search)
    OUT.p(f'Log {fmt_td(t_from)} to {fmt_td(t_to) if t_to != "now" else "now"}: {len(rows)} entries in scope'
          + hidden_note(cfg, h_entries, f'entries outside scope, {h_chunks} chunks entirely') + '.')
    if args.count:
        return
    # Most recent first when paging, printed in chronological order.
    maximum = args.max or cfg['max_log_entries']
    end = len(rows) - args.offset
    start = max(0, end - maximum)
    print_log_rows(cfg, vis, rows[start:max(end, 0)], args.full)
    if start > 0:
        OUT.p(f'... {start} earlier entries (use --offset {args.offset + (end - start)})')


def milestone_paths(nid: str, vis: dict, limit=3) -> list:
    """Paths upward from nid through visible superiors, each ending at the first Milestone (or a top)."""
    paths, stack = [], [[nid]]
    while stack and len(paths) < limit:
        path = stack.pop(0)
        sups = [s for s in vis[path[-1]]['sups'] if s in vis and s not in path]
        if len(path) > 1 and (is_milestone(vis[path[-1]]) or not sups):
            paths.append(path[1:])
            continue
        if not sups:
            continue
        stack.extend(path + [s] for s in sups)
    return paths


def cmd_context(args, cfg, scope):
    """Everything needed to start working on a Node, in one call."""
    vis = scope['visible']
    nid = args.id
    if nid not in vis:
        return out_of_scope(nid)
    d = fetch_node(nid)
    if d is None:
        return OUT.p(f'{nid}: could not be read.')
    m = node_meta(d)
    text = html_to_text(d['text'])

    OUT.p('Node: ' + node_line(nid, m, ''))
    limit = len(text) if args.full else cfg['node_text_chars']
    OUT.p('  ' + text[:limit].replace('\n', '\n  ') + (f' ... ({len(text) - limit} more characters, use --full)'
                                                        if len(text) > limit else ''))

    paths = milestone_paths(nid, vis)
    ex = excerpts(cfg, {n for p in paths for n in p}, 70)
    OUT.p('Under:' + ('' if paths else ' (no visible superiors)'))
    for p in paths:
        OUT.p('  ' + ' > '.join(f'{n} {ex[n]}' for n in p))
        OUT.nodes.update(p)

    deps = [c for c in m['deps'] if c in vis]
    active = sorted((c for c in deps if is_active(vis[c])), key=lambda x: vis[x]['td'])
    OUT.p(f'Active dependencies: {len(active)}' + (f' ({len(deps) - len(active)} inactive left out)'
                                                   if len(deps) > len(active) else ''))
    dex = excerpts(cfg, active[:cfg['max_nodes']])
    for c in active[:cfg['max_nodes']]:
        OUT.p('  ' + node_line(c, vis[c], dex[c]))

    # The Node's whole Log history is read for links; only the most recent entries are shown.
    rows, _, _ = node_log_entries(cfg, vis, nid, '199001010000', 'now')
    rows = [(c, e) for c, e in rows if e['node'] == nid]
    links, seen = [], set()
    for src, s in [('description', text)] + [(f"Log {c['open']}.{e['minor']}", html_to_text(e['text']))
                                             for c, e in rows]:
        for url in URL_RE.findall(s):
            url = url.rstrip('.,;:')
            if url not in seen:
                seen.add(url)
                links.append((src, url))
    OUT.p(f'Links: {len(links)}' + (' (newest last)' if links else ''))
    for src, url in links[-args.links:]:
        kind = ' [Claude Doc/artifact]' if re.match(r'https://claude\.ai/(code/)?artifact/', url) else ''
        OUT.p(f'  {src}: {url}{kind}')

    recent = rows[-args.log:]
    OUT.p(f'Log: {len(rows)} entries for this Node' + (f', the last {len(recent)} shown' if rows else ''))
    if recent:
        print_log_rows(cfg, vis, recent)


def cmd_topics(args, cfg, scope):
    vis = scope['visible']
    counts = {}
    for m in vis.values():
        for t in m['topics']:
            c = counts.setdefault(t, [0, 0])
            c[0] += 1
            c[1] += is_active(m)
    titles = {}
    rc, out = run_tool('fzgraphhtml', ['-t', '?', '-F', 'txt', '-e', '-o', 'STDOUT', '-E', os.devnull,
                                       '-q', '-T', TOPICS_TEMPLATE])
    if rc == 0:
        for rec in out.split('@@R@@')[1:]:
            f = rec.rstrip('\n').split('@@F@@')
            if len(f) >= 2:
                titles[f[0].strip()] = f[1].strip()
    OUT.p(f'{len(counts)} topics on Nodes in scope (Nodes, active):')
    for t, (n, inc) in sorted(counts.items(), key=lambda x: -x[1][0]):
        OUT.p(f'  {t} ({n}, {inc}) {titles.get(t, "")}'.rstrip())


def cmd_nnls(args, cfg, scope):
    counts = {}
    for m in scope['visible'].values():
        for name in m['nnls']:
            counts[name] = counts.get(name, 0) + 1
    OUT.p(f'{len(counts)} Named Node Lists contain Nodes in scope:')
    for name, n in sorted(counts.items(), key=lambda x: -x[1]):
        OUT.p(f'  {name} ({n})')


def cmd_nnl(args, cfg, scope):
    vis = scope['visible']
    pred, what, hint = node_filter(args, vis)
    ids = list_ids(['-L', args.name]) or []
    in_scope = [n for n in ids if n in vis]
    shown = [n for n in in_scope if pred(n)]
    if not in_scope:
        return OUT.p(f"No Nodes in scope in Named Node List '{args.name}'.")
    OUT.p(f"Named Node List '{args.name}': {len(shown)} Nodes shown" + hidden_note(cfg, len(ids) - len(in_scope)) + '.'
          + left_out_note(len(in_scope) - len(shown), what, hint))
    page, trailer = paged(shown, args.max or cfg['max_nodes'], args.offset, 'Nodes')
    ex = excerpts(cfg, page)
    for n in page:
        OUT.p(node_line(n, vis[n], ex[n]))
    if trailer:
        OUT.p(trailer)


def cmd_map(args, cfg, scope):
    vis = scope['visible']
    pred, what, hint = node_filter(args, vis)
    memo = {}

    def below(n):
        if n not in memo:
            seen, stack = set(), [c for c in vis[n]['deps'] if c in vis]
            while stack:
                c = stack.pop()
                if c not in seen:
                    seen.add(c)
                    stack.extend(x for x in vis[c]['deps'] if x in vis)
            memo[n] = (len(seen), sum(1 for c in seen if is_active(vis[c])))
        return memo[n]

    rows, skipped = [], set()

    def visit(n, depth, path):
        rows.append((depth, n))
        if depth >= args.depth:
            return
        found, passed, _ = shown_children(n, pred, vis)
        skipped.update(passed)
        kids = sorted((c for c in found if c not in path), key=lambda c: -below(c)[1])
        for c in kids[:args.width]:
            visit(c, depth + 1, path | {c})
        if len(kids) > args.width:
            rows.append((depth + 1, f'... {len(kids) - args.width} more'))

    for r in sorted(scope['roots'], key=lambda x: vis[x]['td']):
        visit(r, 0, {r})  # roots are always shown
    active = sum(1 for m in vis.values() if is_active(m))
    shown = {n for _, n in rows if n in vis}
    OUT.p(f"Scope: {len(vis)} Nodes ({active} active), {len(scope['roots'])} roots. "
          'Each line: Node, then (Nodes below it, active Nodes below it).'
          + left_out_note(len(skipped - shown), what, hint))
    ex = excerpts(cfg, [n for _, n in rows if n in vis], 70)
    for depth, n in rows:
        if n in vis:
            tot, ninc = below(n)
            OUT.p('  ' * depth + node_line(n, vis[n], ex[n]) + f' ({tot}, {ninc})')
        else:
            OUT.p('  ' * depth + n)
    counts = {}
    for m in vis.values():
        for t in m['topics']:
            counts[t] = counts.get(t, 0) + 1
    top = sorted(counts.items(), key=lambda x: -x[1])[:25]
    OUT.p('Topics: ' + ', '.join(f'{t} {n}' for t, n in top))


def cmd_changes(args, cfg, scope):
    scope = load_scope(cfg, force=True)
    vis = scope['visible']
    current = {n: [m['chash'], m['comp'], m['td']] for n, m in vis.items()}
    if not os.path.exists(SNAPSHOT_PATH):
        write_json(SNAPSHOT_PATH, {'t': scope['created'], 'nodes': current})
        return OUT.p(f"No earlier snapshot. Baseline marked at {fmt_td(scope['created'])} ({len(vis)} Nodes).")
    snap = read_json(SNAPSHOT_PATH)
    old = snap['nodes']
    new = [n for n in vis if n not in old]
    gone = [n for n in old if n not in vis]
    content = [n for n in vis if n in old and old[n][0] != current[n][0]]
    progress = [n for n in vis if n in old and n not in content and old[n][1] != current[n][1]]
    td_only = [n for n in vis if n in old and n not in content and n not in progress and old[n][2] != current[n][2]]
    OUT.p(f"Changes since {fmt_td(snap['t'])}: {len(new)} new, {len(content)} content changed, "
          f"{len(progress)} completion changed, {len(td_only)} target date only, {len(gone)} no longer in scope.")
    maximum = args.max or cfg['max_nodes']
    for title, ids in (('New', new), ('Content changed', content), ('Completion changed', progress)):
        if not ids:
            continue
        ids.sort(key=lambda x: vis[x]['td'])
        page, trailer = paged(ids, maximum, 0, 'Nodes')
        ex = excerpts(cfg, page)
        OUT.p(f'{title}:')
        for n in page:
            OUT.p('  ' + node_line(n, vis[n], ex[n]))
        if trailer:
            OUT.p('  ' + trailer.split(' (use')[0])
    rows, h_entries, h_chunks = log_entries(cfg, vis, snap['t'], 'now')
    OUT.p(f'Log entries since then: {len(rows)} in scope' + hidden_note(cfg, h_entries) + '.')
    lmax = cfg['max_log_entries']
    if rows:
        print_log_rows(cfg, vis, rows[-lmax:])
        if len(rows) > lmax:
            OUT.p(f'... {len(rows) - lmax} earlier entries (use log --since {snap["t"]})')
    if not args.no_mark:
        write_json(SNAPSHOT_PATH, {'t': scope['created'], 'nodes': current})


# --------------------------------------------------------- disclosure reports

def read_disclosures(since=None):
    recs = []
    if os.path.exists(DISCLOSURE_PATH):
        with open(DISCLOSURE_PATH) as f:
            for line in f:
                try:
                    r = json.loads(line)
                except ValueError:
                    continue
                if since and r['t'] < since:
                    continue
                recs.append(r)
    return recs


def iso_since(s):
    if not s:
        return None
    if re.fullmatch(r'\d{8}', s):
        s += '0000'
    if re.fullmatch(r'\d{12}', s):
        return f'{s[0:4]}-{s[4:6]}-{s[6:8]}T{s[8:10]}:{s[10:12]}'
    return s


def cmd_disclosures(args, cfg, scope):
    recs = [r for r in read_disclosures(iso_since(args.since)) if r['by'] == 'claude']
    if args.session:
        recs = [r for r in recs if r['session'] == args.session]
    sessions = {}
    for r in recs:
        s = sessions.setdefault(r['session'] or '(unknown)', {'calls': 0, 'nodes': set(), 'entries': set(),
                                                              'tokens': 0, 'first': r['t'], 'last': r['t']})
        s['calls'] += 1
        s['nodes'] |= set(r['nodes'])
        s['entries'] |= set(r['log_entries'])
        s['tokens'] += r['est_tokens']
        s['last'] = r['t']
    all_nodes = set().union(*(s['nodes'] for s in sessions.values())) if sessions else set()
    all_entries = set().union(*(s['entries'] for s in sessions.values())) if sessions else set()
    OUT.p(f'Sent to Claude{" since " + args.since if args.since else ""}: {len(recs)} calls, '
          f'{len(all_nodes)} distinct Nodes, {len(all_entries)} distinct Log entries, '
          f'~{sum(s["tokens"] for s in sessions.values())} tokens.')
    for sid, s in sessions.items():
        OUT.p(f"  session {sid}: {s['first']} to {s['last']}, {s['calls']} calls, "
              f"{len(s['nodes'])} Nodes, {len(s['entries'])} entries, ~{s['tokens']} tokens")
    if args.ids:
        OUT.p('Nodes: ' + ' '.join(sorted(all_nodes)))
        OUT.p('Log entries: ' + ' '.join(sorted(all_entries)))


def cmd_stats(args, cfg, scope):
    recs = read_disclosures(iso_since(args.since))
    by_cmd = {}
    for r in recs:
        s = by_cmd.setdefault(r['cmd'], {'n': 0, 'tok': [], 'items': 0})
        s['n'] += 1
        s['tok'].append(r['est_tokens'])
        s['items'] += len(r['nodes']) + len(r['log_entries'])
    OUT.p('Estimated tokens per call, by command (runs by you and by Claude):')
    OUT.p(f"  {'command':<12} {'calls':>5} {'avg':>7} {'max':>7} {'per item':>8}")
    for c, s in sorted(by_cmd.items()):
        per = sum(s['tok']) / s['items'] if s['items'] else 0
        OUT.p(f"  {c:<12} {s['n']:>5} {sum(s['tok']) // s['n']:>7} {max(s['tok']):>7} {per:>8.1f}")


# --------------------------------------------------------------- proposals

FLAG_SPECS = {
    'fzgraph': {'value': set('-M -C -T -H -a -S -D -t -g -p -r -e -s -Y -G -I -U -P'.split()), 'novalue': {'-q'}},
    'fzedit': {'value': set('-M -T -c -H -a -t -g -p -r -e -s -Y -G -I -U -P'.split()), 'novalue': {'-q'}},
    'fzlog': {'value': {'-T', '-n', '-c'}, 'novalue': {'-e', '-C', '-q'}},
}
# Flags whose values may hold Node IDs or @name references (never free text).
ID_FLAGS = {'-S', '-D', '-M', '-n', '-c', '-C'}
REF = r'(?:\d{14}\.\d+|@[\w-]+)'
API_PATTERNS = [
    re.compile(rf'^/fz/graph/nodes/({REF})/completion\?(?:set|add)=-?[0-9.]+m?$'),
    re.compile(rf'^/fz/graph/nodes/({REF})/required\?(?:set|add)=-?[0-9.]+[mh]$'),
    re.compile(rf'^/fz/graph/nodes/({REF})\?targetdate=\d{{12}}(?:&required=[0-9.]+)?$'),
    re.compile(rf'^/fz/graph/nodes/({REF})\?skip=(?:\d+|toT(?:&T=\d{{12}})?)$'),
    re.compile(rf'^/fz/graph/nodes/({REF})/topics/(?:add|remove)\?((?:[\w-]+=\[?[0-9.]*\]?&?)+)$'),
    re.compile(rf'^/fz/graph/nodes/({REF})/(?:superiors|dependencies)/(?:add|remove)\?({REF})=$'),
]


def parse_flags(prog: str, args: list) -> list:
    spec = FLAG_SPECS[prog]
    res, i = [], 0
    while i < len(args):
        a = args[i]
        if a in spec['value']:
            if i + 1 >= len(args):
                raise ValueError(f'{a} needs a value')
            res.append((a, args[i + 1]))
            i += 2
        elif a in spec['novalue']:
            res.append((a, None))
            i += 1
        else:
            raise ValueError(f'{prog} option not allowed in proposals: {a}')
    flags = [f for f, _ in res]
    if len(flags) != len(set(flags)):
        raise ValueError('an option is given more than once')
    return res


def validate_proposal(prop: dict, vis: dict, cfg: dict) -> list:
    errors = []
    if not isinstance(prop, dict) or not isinstance(prop.get('title'), str) or not prop['title'].strip():
        return ['proposal needs a "title"']
    steps = prop.get('steps')
    if not isinstance(steps, list) or not steps:
        return ['proposal needs a non-empty "steps" list']
    excluded_tags = set(cfg['exclude_topic']) | set(cfg['exclude_topic_tree'])
    names = {}

    for i, step in enumerate(steps, 1):
        def err(msg):
            errors.append(f'step {i}: {msg}')

        def check_ref(x, where):
            if x.startswith('@'):
                if x[1:] not in names:
                    err(f'{where} {x} does not name a Node created by an earlier step')
            elif not ID_FULL_RE.match(x) or x not in vis:
                err(f'{where} {x} is not a Node in scope')

        def check_tags(tags):
            bad = set(tags) & excluded_tags
            if bad:
                err(f'uses excluded topic(s): {", ".join(sorted(bad))}')

        if not isinstance(step, dict) or not isinstance(step.get('why'), str) or not step['why'].strip():
            err('needs a "why" explaining the step')
            continue
        argv = step.get('argv')
        if not isinstance(argv, list) or not argv or not all(isinstance(a, str) for a in argv):
            err('needs an "argv" list of strings')
            continue
        prog = argv[0]
        if prog not in FLAG_SPECS:
            err(f'program not allowed: {prog}')
            continue
        try:
            fl = parse_flags(prog, argv[1:])
        except ValueError as e:
            err(str(e))
            continue
        fd = dict(fl)
        name = step.get('name')

        if prog == 'fzgraph':
            if ('-M' in fd) == ('-C' in fd):
                err('fzgraph needs exactly one of -M or -C')
            elif fd.get('-M') == 'node':
                if '-S' not in fd or '-T' not in fd:
                    err('a new Node needs explicit superiors (-S) and a description (-T)')
                if '-q' in fd:
                    err('do not use -q when creating a Node (its new ID is read from the output)')
                for s in fd.get('-S', '').split(','):
                    if s:
                        check_ref(s.strip(), 'superior')
                for s in fd.get('-D', '').split(','):
                    if s:
                        check_ref(s.strip(), 'dependency')
                check_tags([t.strip() for t in fd.get('-g', '').split(',') if t.strip()])
                if name:
                    if not re.fullmatch(r'[\w-]+', name) or name in names:
                        err(f'invalid or duplicate step name: {name}')
                    else:
                        names[name] = i
            elif fd.get('-M') == 'edges':
                if '-S' not in fd or '-D' not in fd or '-T' in fd:
                    err('new edges need -S and -D (and no -T)')
                for s in fd.get('-S', '').split(',') + fd.get('-D', '').split(','):
                    if s:
                        check_ref(s.strip(), 'edge Node')
            elif '-C' in fd:
                extra = set(fd) - {'-C', '-q'}
                if extra:
                    err(f'options not allowed with -C: {", ".join(sorted(extra))}')
                api = fd['-C']
                mt = next((p.match(api) for p in API_PATTERNS if p.match(api)), None)
                if not mt:
                    err(f'API request not allowed: {api}')
                else:
                    for x in re.findall(REF, api):
                        check_ref(x, 'Node')
                    if '/topics/add?' in api:
                        check_tags(re.findall(r'([\w-]+)=', api.split('?', 1)[1]))
            else:
                err('fzgraph -M must be node or edges')
        elif prog == 'fzedit':
            if '-M' not in fd:
                err('fzedit needs -M <node-id>')
            else:
                ids = ID_RE.findall(fd['-M'])
                if not ids or len(ids) > 2:
                    err(f'-M {fd["-M"]} is not a Node or Edge ID')
                for x in ids:
                    check_ref(x, 'Node')
            if not set(fd) - {'-M', '-q'}:
                err('fzedit step changes nothing')
            check_tags([t.strip() for t in fd.get('-g', '').split(',') if t.strip()])
        elif prog == 'fzlog':
            if '-e' in fd:
                if '-T' not in fd or '-n' not in fd:
                    err('a Log entry needs -T text and -n <node-id>')
                else:
                    check_ref(fd['-n'], 'entry Node')
                if '-c' in fd or '-C' in fd:
                    err('make the entry a separate step from opening/closing chunks')
            elif '-c' in fd:
                check_ref(fd['-c'], 'chunk Node')
                if set(fd) - {'-c', '-q'}:
                    err('opening a chunk takes only -c <node-id>')
            elif '-C' in fd:
                if set(fd) - {'-C', '-q'}:
                    err('closing a chunk takes only -C')
            else:
                err('fzlog step must be -e (entry), -c (open chunk) or -C (close chunk)')
    return errors


def proposal_path(pid: str) -> str:
    if not re.fullmatch(r'[\w-]+', pid):
        fail(f'not a proposal ID: {pid}')
    return os.path.join(PROPOSALS_DIR, pid + '.json')


def load_proposal(pid: str) -> dict:
    path = proposal_path(pid)
    if not os.path.exists(path):
        fail(f'no proposal {pid}')
    return read_json(path)


def cmd_propose(args, cfg, scope):
    try:
        with open(args.file) as f:
            prop = json.load(f)
    except (OSError, ValueError) as e:
        return OUT.p(f'Could not read proposal file: {e}')
    save_proposal(prop, scope, cfg)


def save_proposal(prop: dict, scope: dict, cfg: dict):
    errors = validate_proposal(prop, scope['visible'], cfg)
    if errors:
        OUT.p('Proposal rejected:')
        for e in errors:
            OUT.p('  ' + e)
        return
    slug = re.sub(r'[^\w]+', '-', prop['title'].lower()).strip('-')[:40]
    pid = datetime.datetime.now().strftime('%Y%m%d%H%M%S') + '-' + slug
    record = {
        'pid': pid, 'title': prop['title'], 'context_node': prop.get('context_node', ''),
        'created': now_stamp(), 'session': SESSION_ID, 'status': 'pending',
        'steps': [{k: s[k] for k in ('name', 'why', 'argv') if k in s} for s in prop['steps']],
    }
    write_json(proposal_path(pid), record)
    OUT.p(f"Proposal {pid} saved with {len(record['steps'])} steps. "
          f'It changes nothing until the user runs: fzclaude show {pid}; fzclaude apply {pid}')


def cmd_link(args, cfg, scope):
    """Propose a Log entry that links a work product (a document, chat, file) to a Node."""
    if args.id not in scope['visible']:
        return out_of_scope(args.id)
    if not URL_RE.fullmatch(args.url):
        return OUT.p(f'Not a link: {args.url}')
    text = f'{args.text.strip()}: {args.url}' if args.text else args.url
    save_proposal({
        'title': f'Link to {args.id}: ' + (args.text.strip() if args.text else args.url)[:60],
        'context_node': args.id,
        'steps': [{'why': 'Record the link in the Log of the Node it belongs to, so it can be found from there '
                          '(fzclaude context lists it)',
                   'argv': ['fzlog', '-e', '-n', args.id, '-T', text]}],
    }, scope, cfg)


def cmd_proposals(args, cfg, scope):
    files = sorted(os.listdir(PROPOSALS_DIR)) if os.path.isdir(PROPOSALS_DIR) else []
    if not files:
        return OUT.p('No proposals.')
    for fn in files:
        if fn.endswith('.json'):
            p = read_json(os.path.join(PROPOSALS_DIR, fn))
            OUT.p(f"{p['pid']} [{p['status']}] {len(p['steps'])} steps: {p['title']}")


def show_proposal(p: dict, write=print):
    write(f"Proposal {p['pid']} [{p['status']}]: {p['title']}")
    if p.get('context_node'):
        write(f"context Node: {p['context_node']}")
    for i, s in enumerate(p['steps'], 1):
        label = f" (creates @{s['name']})" if s.get('name') else ''
        write(f"{i}. {s['why']}{label}")
        write(f"   {shlex.join(s['argv'])}")
    for r in p.get('results', []):
        created = f", created {r['created']}" if r.get('created') else ''
        write(f"result step {r['step']}: exit {r['rc']}{created}")
        if r.get('output'):
            write('   ' + r['output'].replace('\n', '\n   '))


def cmd_show(args, cfg, scope):
    show_proposal(load_proposal(args.pid), OUT.p)


def cmd_apply(args, cfg):
    if BY_CLAUDE:
        fail('apply must be run by you, not by Claude.')
    p = load_proposal(args.pid)
    if args.decline:
        p['status'] = 'declined'
        write_json(proposal_path(args.pid), p)
        return print(f'Proposal {args.pid} marked declined.')
    if p['status'] != 'pending':
        fail(f"proposal is {p['status']}, not pending")
    scope = load_scope(cfg, force=True)
    errors = validate_proposal(p, scope['visible'], cfg)
    if errors:
        print('Proposal no longer validates against the current scope:')
        for e in errors:
            print('  ' + e)
        sys.exit(1)
    show_proposal(p)
    if args.yes:
        print('\nConfirmed with --yes.')
    elif input(f"\nApply these {len(p['steps'])} steps to your Formalizer? Type 'yes': ").strip() != 'yes':
        return print('Not applied. (Use --decline to mark it declined.)')
    if not args.no_backup:
        print('Making a backup with fzbackup.py...')
        if subprocess.run([os.path.join(FZBIN, 'fzbackup.py')]).returncode != 0:
            fail('backup failed; nothing was applied')

    created, results, status = {}, [], 'applied'
    for i, s in enumerate(p['steps'], 1):
        prog = s['argv'][0]
        args_out = []
        for flag, val in parse_flags(prog, s['argv'][1:]):
            if val is not None and flag in ID_FLAGS:
                val = re.sub(r'@([\w-]+)', lambda m: created[m.group(1)], val)
            args_out += [flag] if val is None else [flag, val]
        print(f'\n{i}. {s["why"]}\n   {shlex.join([prog] + args_out)}')
        r = subprocess.run([os.path.join(FZBIN, prog)] + args_out, capture_output=True, text=True, errors='replace')
        output = (r.stdout + r.stderr).strip()
        print(output)
        res = {'step': i, 'rc': r.returncode, 'output': output[-500:]}
        if r.returncode == 0 and prog == 'fzgraph' and dict(parse_flags(prog, s['argv'][1:])).get('-M') == 'node':
            mt = NEW_NODE_RE.search(output)
            if mt:
                res['created'] = mt.group(1)
                if s.get('name'):
                    created[s['name']] = mt.group(1)
            elif s.get('name'):
                res['rc'] = -1
                res['output'] += '\n(fzclaude: could not find the new Node ID in the output)'
        results.append(res)
        if res['rc'] != 0:
            status = 'failed'
            print(f'Step {i} failed; stopping here.')
            break
    p.update({'status': status, 'applied': now_stamp(), 'results': results})
    write_json(proposal_path(args.pid), p)
    if os.path.exists(SCOPE_PATH):
        os.remove(SCOPE_PATH)  # new Nodes: rebuild the scope on the next call
    print(f'\nProposal {args.pid}: {status}.')


def cmd_init(args):
    if BY_CLAUDE:
        fail('init must be run by you, not by Claude.')
    if os.path.exists(CONFIG_PATH):
        return print(f'{CONFIG_PATH} already exists.')
    os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
    write_json(CONFIG_PATH, DEFAULT_CONFIG)
    print(f'Wrote {CONFIG_PATH}. Add Node IDs to "include" (and exclusions), then try:\n'
          '  fzclaude scope refresh\n  fzclaude map\n  fzclaude today')


# ---------------------------------------------------------------------- main

def build_parser():
    ap = argparse.ArgumentParser(prog='fzclaude', description=__doc__.split('\n\n')[0])
    sub = ap.add_subparsers(dest='cmd', required=True)

    def paging(p, offset=True):
        p.add_argument('--max', type=int, default=0, help='maximum items shown')
        if offset:
            p.add_argument('--offset', type=int, default=0, help='skip this many items')

    p = sub.add_parser('init', help='write a default configuration (you only)')
    p = sub.add_parser('scope', help='scope status, or rebuild with "refresh"')
    p.add_argument('action', nargs='?', choices=['status', 'refresh'], default='status')
    def filters(p):
        p.add_argument('--all', action='store_true', help='include inactive Nodes (complete, obsolete, ...)')
        p.add_argument('--milestones', action='store_true', help='only Milestones')

    p = sub.add_parser('map', help='outline of the visible part of the Graph')
    p.add_argument('--depth', type=int, default=2)
    p.add_argument('--width', type=int, default=25, help='children shown per Node')
    filters(p)
    p = sub.add_parser('today', help='Schedule entries in scope')
    p.add_argument('--days', type=int, default=1)
    paging(p)
    p = sub.add_parser('node', help='one Node in detail')
    p.add_argument('id')
    p.add_argument('--full', action='store_true', help='complete description text')
    p.add_argument('--all', action='store_true', help='also list inactive dependencies')
    paging(p, offset=False)
    p = sub.add_parser('subtree', help='dependencies of a Node, as an indented tree')
    p.add_argument('id')
    p.add_argument('--depth', type=int, default=3)
    filters(p)
    paging(p)
    p = sub.add_parser('search', help='search descriptions of Nodes in scope')
    p.add_argument('text')
    p.add_argument('--regex', action='store_true')
    filters(p)
    paging(p)
    p = sub.add_parser('log', help='Log entries in scope (default: recent days)')
    p.add_argument('--days', type=int, default=0)
    p.add_argument('--since', help='YYYYmmddHHMM')
    p.add_argument('--until', help='YYYYmmddHHMM')
    p.add_argument('--all-time', action='store_true')
    p.add_argument('--node')
    p.add_argument('--search')
    p.add_argument('--count', action='store_true')
    p.add_argument('--full', action='store_true', help='complete entry text')
    paging(p)
    sub.add_parser('topics', help='topics on Nodes in scope')
    sub.add_parser('nnls', help='Named Node Lists holding Nodes in scope')
    p = sub.add_parser('nnl', help='Nodes in scope in one Named Node List')
    p.add_argument('name')
    filters(p)
    paging(p)
    p = sub.add_parser('context', help='what is needed to work on a Node: Node, Milestone path, '
                                       'active dependencies, links, recent Log')
    p.add_argument('id')
    p.add_argument('--log', type=int, default=10, help='recent Log entries shown')
    p.add_argument('--links', type=int, default=20, help='links shown (newest)')
    p.add_argument('--full', action='store_true', help='complete description text')
    p = sub.add_parser('link', help='propose a Log entry that links a document or other work to a Node')
    p.add_argument('id')
    p.add_argument('url')
    p.add_argument('--text', default='', help='what the link is, e.g. "Abstract draft (Claude Doc)"')
    p = sub.add_parser('changes', help='what changed since the last call')
    p.add_argument('--no-mark', action='store_true', help='do not move the snapshot forward')
    paging(p, offset=False)
    p = sub.add_parser('disclosures', help='what was sent to Claude')
    p.add_argument('--since', help='YYYYmmdd[HHMM]')
    p.add_argument('--session')
    p.add_argument('--ids', action='store_true')
    p = sub.add_parser('stats', help='estimated token cost per command')
    p.add_argument('--since', help='YYYYmmdd[HHMM]')
    p = sub.add_parser('propose', help='save a proposal file for the user to review')
    p.add_argument('file')
    sub.add_parser('proposals', help='list proposals')
    p = sub.add_parser('show', help='show a proposal')
    p.add_argument('pid')
    p = sub.add_parser('apply', help='carry out a proposal (you only)')
    p.add_argument('pid')
    p.add_argument('--decline', action='store_true')
    p.add_argument('--no-backup', action='store_true')
    p.add_argument('--yes', action='store_true', help='do not ask for confirmation (used by fzclaude-web)')
    return ap


COMMANDS = {
    'scope': cmd_scope, 'map': cmd_map, 'today': cmd_today, 'node': cmd_node,
    'subtree': cmd_subtree, 'search': cmd_search, 'log': cmd_log, 'topics': cmd_topics,
    'nnls': cmd_nnls, 'nnl': cmd_nnl, 'changes': cmd_changes, 'propose': cmd_propose,
    'context': cmd_context, 'link': cmd_link,
}
NO_SCOPE_COMMANDS = {
    'disclosures': cmd_disclosures, 'stats': cmd_stats, 'proposals': cmd_proposals, 'show': cmd_show,
}


def main():
    os.umask(0o077)
    argv = sys.argv[1:]
    args = build_parser().parse_args(argv)
    if args.cmd == 'init':
        return cmd_init(args)
    cfg = load_config()
    global CFG
    CFG = cfg
    if args.cmd == 'apply':
        try:
            return cmd_apply(args, cfg)
        except ScopeError as e:
            fail(str(e))
    try:
        if args.cmd in NO_SCOPE_COMMANDS:
            NO_SCOPE_COMMANDS[args.cmd](args, cfg, None)
        else:
            COMMANDS[args.cmd](args, cfg, load_scope(cfg))
    except ScopeError as e:
        OUT.p(f'fzclaude: {e}')
    emit(args.cmd, argv, cfg)


if __name__ == '__main__':
    main()
