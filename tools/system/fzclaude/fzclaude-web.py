#!/usr/bin/env python3
"""fzclaude-web - Browser interface for fzclaude, on 127.0.0.1:8712.

Lets you edit the fzclaude scope rules with explanations alongside, see
exactly what Claude would receive from each fzclaude command, review and
apply Claude's proposals, and see what was sent to Claude.

Everything goes through fzclaude itself: reads run 'fzclaude <command>' as a
subprocess, configuration changes use fzclaude's own validation, and
proposals are applied with 'fzclaude apply'. What you look at here goes to
your browser only, never to Claude.

Safeguards:

  * Listens on 127.0.0.1 only and accepts only Host: 127.0.0.1/localhost.
  * Each start creates a random token that the page sends with every API
    request, so other web pages open in your browser cannot use the API.
  * fzclaude-guard.py blocks Claude's Bash, WebFetch and browser tools from
    reaching this port.
  * Claude Code environment markers are removed from the environment of
    every fzclaude call made from here.

  ./fzclaude-web.py                  # http://127.0.0.1:8712
  ./fzclaude-web.py --idle-exit 180  # quit after 3 idle hours (0 = never)
  ./fzclaude-web.py --no-browser

Normally started from the Formalizer dashboard (see fzclaude-web-start.py)
and stopped with the Stop button in the page.

Randal A. Koene, 2026
"""

import argparse
import json
import math
import os
import re
import secrets
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

HERE = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, HERE)
import fzclaude  # noqa: E402

HOST = '127.0.0.1'
PORT = 8712
WEB_DIR = os.path.join(HERE, 'web')
FZCLAUDE = os.path.join(HERE, 'fzclaude.py')
TOKEN = secrets.token_urlsafe(24)
STATIC = {'/app.css': 'text/css', '/app.js': 'application/javascript'}

# Read commands the page may run, with the options each accepts and their kinds.
RUN_SPECS = {
    'scope': {'action': 'pos:status|refresh'},
    'map': {'depth': 'int', 'width': 'int', 'all': 'flag', 'milestones': 'flag'},
    'today': {'days': 'int', 'max': 'int', 'offset': 'int'},
    'node': {'id': 'pos:id', 'full': 'flag', 'all': 'flag', 'max': 'int'},
    'subtree': {'id': 'pos:id', 'depth': 'int', 'all': 'flag', 'milestones': 'flag', 'max': 'int', 'offset': 'int'},
    'search': {'text': 'pos:text', 'regex': 'flag', 'all': 'flag', 'milestones': 'flag', 'max': 'int', 'offset': 'int'},
    'log': {'days': 'int', 'since': 'stamp', 'until': 'stamp', 'all-time': 'flag', 'node': 'id',
            'search': 'text', 'count': 'flag', 'full': 'flag', 'max': 'int', 'offset': 'int'},
    'topics': {},
    'nnls': {},
    'nnl': {'name': 'pos:text', 'all': 'flag', 'milestones': 'flag', 'max': 'int', 'offset': 'int'},
    'changes': {'max': 'int'},  # always run with --no-mark: previews must not move Claude's baseline
    'disclosures': {'since': 'stamp', 'session': 'text', 'ids': 'flag'},
    'stats': {'since': 'stamp'},
    'proposals': {},
    'show': {'pid': 'pos:pid'},
}


def clean_env() -> dict:
    return {k: v for k, v in os.environ.items() if k != 'CLAUDECODE' and not k.startswith('CLAUDE_')}


def run_fzclaude(args: list, timeout=600) -> dict:
    try:
        p = subprocess.run([sys.executable, FZCLAUDE] + args, capture_output=True, text=True,
                           errors='replace', timeout=timeout, env=clean_env(), stdin=subprocess.DEVNULL)
        return {'rc': p.returncode, 'output': p.stdout, 'stderr': p.stderr}
    except subprocess.TimeoutExpired:
        return {'rc': -1, 'output': '', 'stderr': f'fzclaude took longer than {timeout} s'}


def build_argv(cmd: str, opts: dict) -> list:
    spec = RUN_SPECS.get(cmd)
    if spec is None:
        raise ValueError(f'command not available here: {cmd}')
    argv, positional = [cmd], []
    for key, kind in spec.items():
        val = opts.get(key)
        if val in (None, '', False):
            if kind.startswith('pos:') and kind != 'pos:status|refresh':
                raise ValueError(f'{key} is required')
            continue
        sval = str(val).strip()
        if kind.startswith('pos:'):
            k = kind[4:]
            if k == 'id' and not fzclaude.ID_FULL_RE.match(sval):
                raise ValueError(f'not a Node ID: {sval}')
            if k == 'pid' and not re.fullmatch(r'[\w-]+', sval):
                raise ValueError(f'not a proposal ID: {sval}')
            if '|' in k and sval not in k.split('|'):
                raise ValueError(f'{key} must be one of {k}')
            positional.append(sval)
        elif kind == 'flag':
            argv.append(f'--{key}')
        elif kind == 'int':
            if not re.fullmatch(r'\d{1,6}', sval):
                raise ValueError(f'{key} must be a whole number')
            argv += [f'--{key}', sval]
        elif kind == 'stamp':
            if not re.fullmatch(r'\d{8}(\d{4})?', sval):
                raise ValueError(f'{key} must look like YYYYmmdd or YYYYmmddHHMM')
            argv += [f'--{key}', sval]
        elif kind == 'id':
            if not fzclaude.ID_FULL_RE.match(sval):
                raise ValueError(f'not a Node ID: {sval}')
            argv += [f'--{key}', sval]
        else:
            argv += [f'--{key}', sval]
    if cmd == 'changes':
        argv.append('--no-mark')
    # Positional values go after '--' so text such as '-x' is never taken for an option.
    return argv + (['--'] + positional if positional else [])


# ------------------------------------------------------------------ jobs (apply)

JOBS = {}
JOBS_LOCK = threading.Lock()


def start_job(args: list) -> str:
    jid = secrets.token_hex(6)
    job = {'status': 'running', 'output': '', 'started': time.time()}
    with JOBS_LOCK:
        JOBS[jid] = job

    def worker():
        p = subprocess.Popen([sys.executable, FZCLAUDE] + args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             stdin=subprocess.DEVNULL, text=True, errors='replace', env=clean_env())
        for line in p.stdout:
            job['output'] += line
        job['rc'] = p.wait()
        job['status'] = 'done' if job['rc'] == 0 else 'failed'

    threading.Thread(target=worker, daemon=True).start()
    return jid


# ------------------------------------------------------------------ helpers

def config_payload() -> dict:
    exists = os.path.exists(fzclaude.CONFIG_PATH)
    data, errors = {}, []
    if exists:
        try:
            data = fzclaude.read_json(fzclaude.CONFIG_PATH)
        except ValueError as e:
            errors = [f'config.json is not valid JSON: {e}']
    cfg, check = fzclaude.check_config(data if isinstance(data, dict) else {})
    hist_dir = os.path.join(fzclaude.STATE_DIR, 'config-history')
    history = sorted(os.listdir(hist_dir), reverse=True)[:30] if os.path.isdir(hist_dir) else []
    return {
        'exists': exists, 'path': fzclaude.CONFIG_PATH, 'config': cfg or fzclaude.DEFAULT_CONFIG,
        'defaults': fzclaude.DEFAULT_CONFIG, 'errors': errors + check, 'history': history,
        'schema': [{'key': k, 'type': t, 'group': g, 'label': lab, 'help': h}
                   for k, t, g, lab, h in fzclaude.CONFIG_SCHEMA],
    }


def lookup_node(nid: str) -> dict:
    """For the configuration editor: what a Node is, whether or not it is in scope."""
    d = fzclaude.fetch_node(nid)
    if d is None:
        return {'id': nid, 'found': False}
    m = fzclaude.node_meta(d)
    in_scope = None
    if os.path.exists(fzclaude.SCOPE_PATH):
        try:
            in_scope = nid in fzclaude.read_json(fzclaude.SCOPE_PATH)['visible']
        except (ValueError, KeyError):
            pass
    return {'id': nid, 'found': True, 'excerpt': fzclaude.one_line(fzclaude.html_to_text(d['text']), 140),
            'topics': m['topics'], 'comp': m['comp'], 'td': fzclaude.fmt_td(m['td']),
            'deps': len(m['deps']), 'in_scope': in_scope}


def all_topics() -> list:
    rc, out = fzclaude.run_tool('fzgraphhtml', ['-t', '?', '-F', 'txt', '-e', '-o', 'STDOUT', '-E', os.devnull,
                                                '-q', '-T', fzclaude.TOPICS_TEMPLATE])
    topics = []
    if rc == 0:
        for rec in out.split('@@R@@')[1:]:
            f = rec.rstrip('\n').split('@@F@@')
            if len(f) >= 2 and f[0].strip():
                topics.append({'tag': f[0].strip(), 'title': f[1].strip()})
    return topics


def read_jsonl_tail(path: str, limit: int) -> list:
    if not os.path.exists(path):
        return []
    with open(path, errors='replace') as f:
        lines = f.readlines()[-limit:]
    out = []
    for line in lines:
        try:
            out.append(json.loads(line))
        except ValueError:
            pass
    return out[::-1]


def list_proposals() -> list:
    d = fzclaude.PROPOSALS_DIR
    res = []
    if os.path.isdir(d):
        for fn in sorted(os.listdir(d), reverse=True):
            if fn.endswith('.json'):
                try:
                    res.append(fzclaude.read_json(os.path.join(d, fn)))
                except ValueError:
                    pass
    return res


# ------------------------------------------------------------------ HTTP

class State:
    last_activity = time.time()
    server = None


class Handler(BaseHTTPRequestHandler):
    server_version = 'fzclaude-web'

    def log_message(self, fmt, *args):
        sys.stderr.write('%s %s\n' % (time.strftime('%H:%M:%S'), fmt % args))

    def send(self, code: int, body, ctype='application/json'):
        if not isinstance(body, (bytes, str)):
            body = json.dumps(body)
        if isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header('Content-Type', ctype + '; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Referrer-Policy', 'no-referrer')
        if ctype == 'text/html':
            self.send_header('Content-Security-Policy',
                             "default-src 'self'; style-src 'self'; script-src 'self'; frame-ancestors 'none'")
        self.end_headers()
        self.wfile.write(body)

    def host_ok(self) -> bool:
        return self.headers.get('Host', '') in (f'{HOST}:{PORT}', f'localhost:{PORT}')

    def api_ok(self) -> bool:
        origin = self.headers.get('Origin')
        if origin and origin not in (f'http://{HOST}:{PORT}', f'http://localhost:{PORT}'):
            return False
        return secrets.compare_digest(self.headers.get('X-FZC-Token', ''), TOKEN)

    def body(self) -> dict:
        n = int(self.headers.get('Content-Length') or 0)
        if n > 1_000_000:
            raise ValueError('request too large')
        data = json.loads(self.rfile.read(n) or b'{}')
        if not isinstance(data, dict):
            raise ValueError('expected a JSON object')
        return data

    def do_GET(self):
        self.route('GET')

    def do_POST(self):
        self.route('POST')

    def route(self, method: str):
        State.last_activity = time.time()
        if not self.host_ok():
            return self.send(403, {'error': 'wrong Host'})
        url = urlparse(self.path)
        path = url.path
        if method == 'GET' and path in ('/', '/index.html'):
            with open(os.path.join(WEB_DIR, 'index.html')) as f:
                return self.send(200, f.read().replace('@@TOKEN@@', TOKEN), 'text/html')
        if method == 'GET' and path in STATIC:
            with open(os.path.join(WEB_DIR, path[1:]), 'rb') as f:
                return self.send(200, f.read(), STATIC[path])
        if not path.startswith('/api/'):
            return self.send(404, {'error': 'not found'})
        if not self.api_ok():
            return self.send(403, {'error': 'missing or wrong token (reload the page)'})
        q = {k: v[0] for k, v in parse_qs(url.query).items()}
        try:
            return self.send(200, self.api(method, path[5:], q))
        except ValueError as e:
            return self.send(400, {'error': str(e)})

    def api(self, method: str, name: str, q: dict):
        if method == 'GET':
            if name == 'config':
                return config_payload()
            if name == 'history':
                fn = q.get('name', '')
                if not re.fullmatch(r'config-\d{14}\.json', fn):
                    raise ValueError('not a history file')
                return fzclaude.read_json(os.path.join(fzclaude.STATE_DIR, 'config-history', fn))
            if name == 'lookup':
                nid = q.get('id', '')
                if not fzclaude.ID_FULL_RE.match(nid):
                    raise ValueError('not a Node ID')
                return lookup_node(nid)
            if name == 'topics':
                return {'topics': all_topics()}
            if name == 'proposals':
                return {'proposals': list_proposals()}
            if name == 'disclosure-log':
                recs = read_jsonl_tail(fzclaude.DISCLOSURE_PATH, 2000)
                if q.get('who') != 'all':
                    recs = [r for r in recs if r.get('by') == 'claude']
                return {'records': recs[:int(q.get('limit', '200'))]}
            if name == 'guard-log':
                return {'records': read_jsonl_tail(os.path.join(fzclaude.STATE_DIR, 'guard.log'), 200)}
            if name == 'job':
                job = JOBS.get(q.get('id', ''))
                if not job:
                    raise ValueError('no such job')
                return job
            raise ValueError(f'unknown request: {name}')

        data = self.body()
        if name == 'config':
            errors = fzclaude.save_config(data.get('config'))
            if errors:
                return {'ok': False, 'errors': errors}
            return {'ok': True, 'scope': run_fzclaude(['scope', 'refresh'])}
        if name == 'run':
            argv = build_argv(str(data.get('cmd', '')), data.get('opts') or {})
            res = run_fzclaude(argv)
            cpt = fzclaude.DEFAULT_CONFIG['chars_per_token']
            try:
                cpt = fzclaude.load_config()['chars_per_token']
            except SystemExit:
                pass
            res['argv'] = ['fzclaude'] + argv
            res['est_tokens'] = math.ceil(len(res['output'].rstrip('\n')) / cpt)
            return res
        if name in ('apply', 'decline'):
            pid = str(data.get('pid', ''))
            if not re.fullmatch(r'[\w-]+', pid):
                raise ValueError('not a proposal ID')
            if name == 'decline':
                return run_fzclaude(['apply', pid, '--decline'])
            args = ['apply', pid, '--yes'] + ([] if data.get('backup', True) else ['--no-backup'])
            return {'job': start_job(args)}
        if name == 'stop':
            threading.Thread(target=State.server.shutdown, daemon=True).start()
            return {'ok': True}
        raise ValueError(f'unknown request: {name}')


def idle_watch(minutes: int):
    while True:
        time.sleep(30)
        busy = any(j['status'] == 'running' for j in JOBS.values())
        if not busy and time.time() - State.last_activity > minutes * 60:
            print(f'Idle for {minutes} minutes, exiting.', file=sys.stderr)
            State.server.shutdown()
            return


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--port', type=int, default=PORT)
    ap.add_argument('--idle-exit', type=int, default=180, metavar='MINUTES')
    ap.add_argument('--no-browser', action='store_true')
    args = ap.parse_args()
    globals()['PORT'] = args.port
    os.umask(0o077)
    State.server = ThreadingHTTPServer((HOST, args.port), Handler)
    if args.idle_exit > 0:
        threading.Thread(target=idle_watch, args=(args.idle_exit,), daemon=True).start()
    url = f'http://{HOST}:{args.port}/'
    print(f'fzclaude-web on {url}', file=sys.stderr)
    if not args.no_browser:
        webbrowser.open(url)
    State.server.serve_forever()


if __name__ == '__main__':
    main()
