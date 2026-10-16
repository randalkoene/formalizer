#!/usr/bin/env python3
"""fzclaude-guard - Claude Code PreToolUse hook that keeps Claude on fzclaude.

Install it in ~/.claude/settings.json (see README.md). For every Bash, Edit,
Write, NotebookEdit, WebFetch and MCP tool call that Claude makes, it blocks
(exit code 2, with the reason on stderr, which Claude sees):

- running any Formalizer program in ~/.formalizer/bin other than fzclaude
  (fzclaude-guard.py, fzbuild and -h/-v help calls are allowed),
- running 'fzclaude apply' or 'fzclaude init', which are yours to run,
- psql, pg_dump, pg_dumpall, pg_restore,
- network tools, scripts, WebFetch and browser (MCP) tools that address this
  machine (localhost, 127.x, the fzserverpq port, /cgi-bin/): the Formalizer
  web interface, fzserverpq and fzclaude-web are all ways around fzclaude,
- commands that mention the Claude Code environment markers (CLAUDECODE,
  CLAUDE_CODE_*), so that fzclaude cannot be made to think you ran it,
- interpreter calls (python, bash -c, heredocs, script files) whose code
  mentions any of the above or the Formalizer TCP/Graph Python modules,
- changes to ~/.formalizer/fzclaude (configuration, disclosure log, state,
  and the installed copies of fzclaude and this guard in its bin/),
- changes to the fzclaude links in ~/.formalizer/bin and to Claude Code
  settings files (where this hook is registered),
- running 'make fzclaude-install' or fzclaude-install.sh, which are yours,
- running any fzclaude other than the installed copy, and using the
  repository's fzclaude code with the real home directory. Scripts that
  import it for testing must set HOME to a throwaway directory (where the
  Formalizer tools and data do not exist), and must not mention .formalizer.

This is a tripwire for honest mistakes and for keeping the rules explicit,
not a sandbox: a sufficiently indirect command could get past it. Blocked
calls are recorded in ~/.formalizer/fzclaude/guard.log.

Randal A. Koene, 2026
"""

import datetime
import json
import os
import re
import shlex
import shutil
import sys

HOME = os.path.expanduser('~')
FZBASE = os.path.join(HOME, '.formalizer')
FZBIN = os.path.join(FZBASE, 'bin')
GUARD_LOG = os.path.join(FZBASE, 'fzclaude', 'guard.log')
# Overridable only for testing this script by hand: the hook itself runs in Claude Code's environment.
INSTALLED = os.environ.get('FZCLAUDE_GUARD_TEST_INSTALLED') or os.path.join(FZBASE, 'fzclaude', 'bin')
PROTECTED = [os.path.join(FZBASE, 'fzclaude')]
PROTECTED_LINKS = [os.path.join(FZBIN, n) for n in (
    'fzclaude', 'fzclaude.py', 'fzclaude-guard', 'fzclaude-guard.py', 'fzclaude-web-start', 'fzclaude-web-start.py')]
SETTINGS_RE = re.compile(r'(^|/)\.claude/settings(\.local)?\.json$')
SETTINGS_MENTION_RE = re.compile(r'\.claude/settings(\.local)?\.json|managed-settings')
INSTALL_NAMES = {'fzclaude-install'}
LOADS_FZCLAUDE_RE = re.compile(r'(?<![\w-])fzclaude\.py\b|\bimport\s+fzclaude\b|\bfrom\s+fzclaude\b')

ALLOWED_PROGRAMS = {'fzclaude', 'fzclaude-guard', 'fzbuild', 'md2html', 'boilerplate'}
# Blocked even before they are linked into ~/.formalizer/bin.
ALWAYS_BLOCKED = {'fzclaude-web', 'fzclaude-web-start', 'fzclaude-web-cgi'}
DATABASE_PROGRAMS = {'psql', 'pg_dump', 'pg_dumpall', 'pg_restore'}
NETWORK_PROGRAMS = {'nc', 'ncat', 'netcat', 'curl', 'wget', 'telnet', 'socat', 'http', 'https', 'xh',
                    'lynx', 'w3m', 'links', 'elinks', 'firefox', 'chromium', 'google-chrome', 'xdg-open'}
LOCAL_RE = re.compile(r'(localhost|127\.\d+\.\d+\.\d+|0\.0\.0\.0|\[?::1\]?(?=[:/])|/cgi-bin/|/fz/)', re.IGNORECASE)
NET_CODE_RE = re.compile(r'\b(socket|urllib|requests|http\.client|httpx|aiohttp|fetch|XMLHttpRequest|webbrowser)\b')
ENV_MARKER_RE = re.compile(r'CLAUDECODE|CLAUDE_CODE_')
USER_ONLY_RE = re.compile(r'fzclaude(?:\.py)?[\'"]?\s+[\'"]?(apply|init)\b')
INTERPRETERS = re.compile(r'^(python[\d.]*|bash|sh|zsh|dash|perl|ruby|node|php)$')
WRAPPERS = {'sudo', 'env', 'nohup', 'time', 'nice', 'ionice', 'exec', 'command', 'stdbuf', 'xargs', 'watch', 'timeout'}
CODE_MARKERS = ['tcpclient', 'Graphaccess', 'Logaccess', 'fzcmdcalls', 'libpq', 'psycopg']
WRITE_MARKERS = re.compile(r'>|\b(rm|mv|cp|tee|truncate|chmod|chown|ln|dd|install|rsync)\b|sed\s+-i|python|perl')


def strip_ext(name: str) -> str:
    return re.sub(r'\.(py|sh)$', '', name)


def formalizer_programs() -> set:
    try:
        names = {strip_ext(n) for n in os.listdir(FZBIN)}
    except OSError:
        names = set()
    return (names - ALLOWED_PROGRAMS) | ALWAYS_BLOCKED


def server_port() -> str:
    try:
        with open(os.path.join(FZBASE, 'server_address')) as f:
            return f.read().strip().rsplit(':', 1)[-1] or '8090'
    except OSError:
        return '8090'


def simple_commands(command: str):
    """Split a shell command into simple commands (lists of words). Raises ValueError."""
    lex = shlex.shlex(command.replace('\n', ' ; '), posix=True, punctuation_chars=True)
    lex.whitespace_split = True
    cmds, cur = [], []
    for tok in lex:
        if tok and all(c in ';&|()<>' for c in tok):
            if cur:
                cmds.append(cur)
            cur = []
            continue
        cur.append(tok)
    if cur:
        cmds.append(cur)
    return cmds


def program_and_args(words: list):
    """(environment assignments, program, arguments) of a simple command."""
    env, i = {}, 0
    while i < len(words):
        w = words[i]
        if re.match(r'^[A-Za-z_]\w*=', w):
            k, v = w.split('=', 1)
            env[k] = v
            i += 1
        elif os.path.basename(w) in WRAPPERS:
            i += 1
            while i < len(words) and (words[i].startswith('-') or re.match(r'^\d+[smh]?$', words[i])):
                i += 1
        else:
            return env, w, words[i + 1:]
    return env, None, []


def under(path: str, root: str) -> bool:
    p, r = os.path.realpath(path), os.path.realpath(root)
    return p == r or p.startswith(r + os.sep)


def is_installed() -> bool:
    return os.path.isfile(os.path.join(INSTALLED, 'fzclaude.py'))


def throwaway_home(env: dict) -> bool:
    """True if the command runs with HOME pointing somewhere other than the real home directory."""
    h = env.get('HOME')
    return bool(h) and not under(os.path.expanduser(h), HOME) and not under(HOME, os.path.expanduser(h))


def resolve_program(prog: str, cwd: str) -> str:
    if '/' in prog or prog.startswith('~'):
        return os.path.realpath(os.path.join(cwd or os.getcwd(), os.path.expanduser(prog)))
    return os.path.realpath(shutil.which(prog) or os.path.join(FZBIN, prog))


def mentions(text: str, names: set):
    for n in names:
        if re.search(rf'(?<![\w-]){re.escape(n)}(\.py|\.sh)?(?![\w-])', text):
            return n
    return None


def check_bash(command: str, cwd: str = ''):
    fzprogs = formalizer_programs()
    port = server_port()
    if ENV_MARKER_RE.search(command):
        return 'commands may not refer to the Claude Code environment markers'
    hit = USER_ONLY_RE.search(command)
    if hit:
        return f"'fzclaude {hit.group(1)}' is for the user to run"
    try:
        cmds = simple_commands(command)
    except ValueError:
        return 'the command could not be parsed safely'

    for words in cmds:
        env, prog, args = program_and_args(words)
        if not prog:
            continue
        base = strip_ext(os.path.basename(prog))
        if base == 'fzclaude' and args and args[0] in ('apply', 'init'):
            return f"'fzclaude {args[0]}' is for the user to run"
        if base in INSTALL_NAMES or (base == 'make' and any(a in INSTALL_NAMES for a in args)):
            return 'installing fzclaude is for the user to do (after reviewing the changes)'
        if base == 'fzclaude' and is_installed() and not under(resolve_program(prog, cwd), INSTALLED):
            return 'only the installed fzclaude may be run (the user installs changes with make fzclaude-install)'
        if base in DATABASE_PROGRAMS:
            return f'{base} accesses the database directly'
        if base in fzprogs:
            if args in (['-h'], ['-v'], ['--help']):
                continue
            return f'{base} is a Formalizer data tool; use fzclaude instead'
        if base in NETWORK_PROGRAMS and any(LOCAL_RE.search(a) or re.search(rf'(?<!\d){port}(?!\d)', a) for a in args):
            return 'connections to web services on this machine (Formalizer, fzserverpq, fzclaude-web) are not allowed'
        if INTERPRETERS.match(base):
            script = next((a for a in args if not a.startswith('-')), None)
            spath = os.path.join(cwd or os.getcwd(), os.path.expanduser(script)) if script else ''
            sbase = strip_ext(os.path.basename(script)) if script else ''
            if sbase in INSTALL_NAMES:
                return 'installing fzclaude is for the user to do (after reviewing the changes)'
            if sbase == 'fzclaude' and '-c' not in args and is_installed() and not under(spath, INSTALLED) \
                    and not throwaway_home(env):
                return 'only the installed fzclaude may be run (the user installs changes with make fzclaude-install)'
            if sbase in ALLOWED_PROGRAMS and '-c' not in args:
                continue
            code = command
            if spath and os.path.isfile(spath):
                try:
                    with open(spath, errors='replace') as f:
                        code += '\n' + f.read(1_000_000)
                except OSError:
                    pass
            hit = mentions(code, fzprogs | DATABASE_PROGRAMS | set(CODE_MARKERS))
            if hit:
                return f'interpreter code mentions {hit}; use fzclaude instead'
            if re.search(rf'(?<!\d){port}(?!\d)', code):
                return f'interpreter code mentions the fzserverpq port {port}'
            if LOCAL_RE.search(code) and NET_CODE_RE.search(code):
                return 'interpreter code connects to a web service on this machine'
            # Code that loads fzclaude itself (e.g. a test importing the repository copy) only runs
            # with a throwaway HOME, where the Formalizer tools and data cannot be found.
            if LOADS_FZCLAUDE_RE.search(code) and not (sbase in ALLOWED_PROGRAMS and under(spath, INSTALLED)):
                if not throwaway_home(env):
                    return 'code that loads fzclaude must run with HOME set to a throwaway directory'
                if '.formalizer' in code or HOME in code:
                    return 'code run with a throwaway HOME must not refer to the real Formalizer directories'

    for p in PROTECTED:
        short = p.replace(HOME, '~')
        if (p in command or short in command) and WRITE_MARKERS.search(command):
            return f'commands that may change {short} are not allowed'
    if SETTINGS_MENTION_RE.search(command) and WRITE_MARKERS.search(command):
        return 'commands that may change Claude Code settings files are not allowed'
    if re.search(r'\.formalizer/bin|\$FZBIN|/bin/fzclaude', command) and 'fzclaude' in command \
            and WRITE_MARKERS.search(command):
        return 'commands that may change the fzclaude links in ~/.formalizer/bin are not allowed'
    return None


def check_path(path: str):
    if not path:
        return None
    path = os.path.expanduser(path)
    candidates = {os.path.normpath(path), os.path.realpath(path)}
    if any(SETTINGS_RE.search(c) for c in candidates):
        return 'Claude Code settings files (where the guard hook is registered) are managed by the user'
    if os.path.normpath(path) in PROTECTED_LINKS:
        return 'the fzclaude links in ~/.formalizer/bin are managed by fzclaude-install'
    for p in PROTECTED:
        for root in {os.path.normpath(p), os.path.realpath(p)}:
            if any(c == root or c.startswith(root + os.sep) for c in candidates):
                return f'{p} is managed by the user and fzclaude only'
    return None


def main():
    try:
        data = json.load(sys.stdin)
    except ValueError:
        sys.exit(0)
    tool = data.get('tool_name', '')
    inp = data.get('tool_input', {}) or {}
    reason = None
    if tool == 'Bash':
        reason = check_bash(inp.get('command', ''), data.get('cwd', ''))
    elif tool in ('Edit', 'Write', 'MultiEdit'):
        reason = check_path(inp.get('file_path', ''))
    elif tool == 'NotebookEdit':
        reason = check_path(inp.get('notebook_path', ''))
    elif tool == 'WebFetch' or tool.startswith('mcp__'):
        if LOCAL_RE.search(json.dumps(inp)):
            reason = 'web requests to this machine (Formalizer, fzserverpq, fzclaude-web) are not allowed'
    if not reason:
        sys.exit(0)
    try:
        os.makedirs(os.path.dirname(GUARD_LOG), exist_ok=True)
        with open(GUARD_LOG, 'a') as f:
            f.write(json.dumps({'t': datetime.datetime.now().isoformat(timespec='seconds'),
                                'session': data.get('session_id', ''), 'tool': tool,
                                'input': inp, 'reason': reason}) + '\n')
    except OSError:
        pass
    print(f'fzclaude-guard blocked this: {reason}.', file=sys.stderr)
    sys.exit(2)


if __name__ == '__main__':
    main()
