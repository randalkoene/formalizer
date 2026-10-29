#!/usr/bin/env python3
"""fzclaude-guard - Claude Code PreToolUse hook that keeps Claude on fzclaude.

Install it in ~/.claude/settings.json (see README.md). For every Bash, Edit,
Write, NotebookEdit, WebFetch and MCP tool call that Claude makes, it blocks
(exit code 2, with the reason on stderr, which Claude sees):

- running any Formalizer program in ~/.formalizer/bin other than fzclaude
  (fzclaude-guard.py, fzbuild and -h/-v help calls are allowed),
- running 'fzclaude apply', 'fzclaude triage-apply' or 'fzclaude init', which are yours to run,
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

Hard rules always block: the database programs, the environment markers,
the commands that are yours to run, installing fzclaude, running or loading
a fzclaude other than the installed one, and changes to the protected files.
The other rules (Formalizer programs, local web services, and interpreter
code that mentions them) are soft. Many of their hits are harmless during
development, e.g. a script that only mentions a program's name. A soft hit
is let through if it matches an exception that you listed, and is otherwise
put to you as a permission prompt ("ask") instead of being blocked, unless
you turned asking off. Both are set in

  ~/.formalizer/fzclaude/guard-exceptions.json

  {
    "ask": true,
    "exceptions": [
      {"tool": "Bash", "pattern": "fzgraph -h( 2>&1)?", "why": "help text"}
    ]
  }

where each pattern is a Python regular expression that must match the whole
command (for Bash) or the JSON of the tool input (for other tools). Without
the file, asking is on and there are no exceptions. If the file cannot be
read, soft hits are blocked.

This is a tripwire for honest mistakes and for keeping the rules explicit,
not a sandbox: a sufficiently indirect command could get past it. Blocked,
asked and excepted calls are recorded in ~/.formalizer/fzclaude/guard.log.

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
# Overridable only for testing this script by hand: the hook itself runs in Claude Code's environment.
GUARD_LOG = os.environ.get('FZCLAUDE_GUARD_TEST_LOG') or os.path.join(FZBASE, 'fzclaude', 'guard.log')
INSTALLED = os.environ.get('FZCLAUDE_GUARD_TEST_INSTALLED') or os.path.join(FZBASE, 'fzclaude', 'bin')
EXCEPTIONS_PATH = os.environ.get('FZCLAUDE_GUARD_TEST_EXCEPTIONS') or \
    os.path.join(FZBASE, 'fzclaude', 'guard-exceptions.json')
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
USER_ONLY_RE = re.compile(r'fzclaude(?:\.py)?[\'"]?\s+[\'"]?(triage-apply|apply|init)\b')
INTERPRETERS = re.compile(r'^(python[\d.]*|bash|sh|zsh|dash|perl|ruby|node|php)$')
WRAPPERS = {'sudo', 'env', 'nohup', 'time', 'nice', 'ionice', 'exec', 'command', 'stdbuf', 'xargs', 'watch', 'timeout'}
CODE_MARKERS = ['tcpclient', 'Graphaccess', 'Logaccess', 'fzcmdcalls', 'libpq', 'psycopg']
WRITE_MARKERS = re.compile(r'>|\b(rm|mv|cp|tee|truncate|chmod|chown|ln|dd|install|rsync)\b|sed\s+-i|python|perl')


class Hard(str):
    """A reason from a hard rule: always blocked, never asked about or excepted."""


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
    """The reason to stop a command, or None. Every rule is checked, so that a soft hit
    (which may be excepted or asked about) never hides a hard one later in the command."""
    fzprogs = formalizer_programs()
    port = server_port()
    found = []
    if ENV_MARKER_RE.search(command):
        found.append(Hard('commands may not refer to the Claude Code environment markers'))
    hit = USER_ONLY_RE.search(command)
    if hit:
        found.append(Hard(f"'fzclaude {hit.group(1)}' is for the user to run"))
    try:
        cmds = simple_commands(command)
    except ValueError:
        cmds = []
        found.append('the command could not be parsed safely')

    for words in cmds:
        env, prog, args = program_and_args(words)
        if not prog:
            continue
        base = strip_ext(os.path.basename(prog))
        if base == 'fzclaude' and args and args[0] in ('apply', 'triage-apply', 'init'):
            found.append(Hard(f"'fzclaude {args[0]}' is for the user to run"))
        if base in INSTALL_NAMES or (base == 'make' and any(a in INSTALL_NAMES for a in args)):
            found.append(Hard('installing fzclaude is for the user to do (after reviewing the changes)'))
        if base == 'fzclaude' and is_installed() and not under(resolve_program(prog, cwd), INSTALLED):
            found.append(Hard('only the installed fzclaude may be run '
                              '(the user installs changes with make fzclaude-install)'))
        if base in DATABASE_PROGRAMS:
            found.append(Hard(f'{base} accesses the database directly'))
        if base in fzprogs and args not in (['-h'], ['-v'], ['--help']):
            found.append(f'{base} is a Formalizer data tool; use fzclaude instead')
        if base in NETWORK_PROGRAMS and any(LOCAL_RE.search(a) or re.search(rf'(?<!\d){port}(?!\d)', a) for a in args):
            found.append('connections to web services on this machine (Formalizer, fzserverpq, fzclaude-web) '
                         'are not allowed')
        if INTERPRETERS.match(base):
            script = next((a for a in args if not a.startswith('-')), None)
            spath = os.path.join(cwd or os.getcwd(), os.path.expanduser(script)) if script else ''
            sbase = strip_ext(os.path.basename(script)) if script else ''
            if sbase in INSTALL_NAMES:
                found.append(Hard('installing fzclaude is for the user to do (after reviewing the changes)'))
            if sbase == 'fzclaude' and '-c' not in args and is_installed() and not under(spath, INSTALLED) \
                    and not throwaway_home(env):
                found.append(Hard('only the installed fzclaude may be run '
                                  '(the user installs changes with make fzclaude-install)'))
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
                found.append(f'interpreter code mentions {hit}; use fzclaude instead')
            if re.search(rf'(?<!\d){port}(?!\d)', code):
                found.append(f'interpreter code mentions the fzserverpq port {port}')
            if LOCAL_RE.search(code) and NET_CODE_RE.search(code):
                found.append('interpreter code connects to a web service on this machine')
            # Code that loads fzclaude itself (e.g. a test importing the repository copy) only runs
            # with a throwaway HOME, where the Formalizer tools and data cannot be found.
            if LOADS_FZCLAUDE_RE.search(code) and not (sbase in ALLOWED_PROGRAMS and under(spath, INSTALLED)):
                if not throwaway_home(env):
                    found.append(Hard('code that loads fzclaude must run with HOME set to a throwaway directory'))
                elif '.formalizer' in code or HOME in code:
                    found.append(Hard('code run with a throwaway HOME must not refer to the real '
                                      'Formalizer directories'))

    for p in PROTECTED:
        short = p.replace(HOME, '~')
        if (p in command or short in command) and WRITE_MARKERS.search(command):
            found.append(Hard(f'commands that may change {short} are not allowed'))
    if SETTINGS_MENTION_RE.search(command) and WRITE_MARKERS.search(command):
        found.append(Hard('commands that may change Claude Code settings files are not allowed'))
    if re.search(r'\.formalizer/bin|\$FZBIN|/bin/fzclaude', command) and 'fzclaude' in command \
            and WRITE_MARKERS.search(command):
        found.append(Hard('commands that may change the fzclaude links in ~/.formalizer/bin are not allowed'))
    return next((r for r in found if isinstance(r, Hard)), found[0] if found else None)


def check_path(path: str):
    if not path:
        return None
    path = os.path.expanduser(path)
    candidates = {os.path.normpath(path), os.path.realpath(path)}
    if any(SETTINGS_RE.search(c) for c in candidates):
        return Hard('Claude Code settings files (where the guard hook is registered) are managed by the user')
    if os.path.normpath(path) in PROTECTED_LINKS:
        return Hard('the fzclaude links in ~/.formalizer/bin are managed by fzclaude-install')
    for p in PROTECTED:
        for root in {os.path.normpath(p), os.path.realpath(p)}:
            if any(c == root or c.startswith(root + os.sep) for c in candidates):
                return Hard(f'{p} is managed by the user and fzclaude only')
    return None


def load_exceptions():
    """(ask, exceptions) from guard-exceptions.json; asking is on and there are no exceptions
    without the file. A file that cannot be used turns asking off: soft hits are then blocked."""
    if not os.path.exists(EXCEPTIONS_PATH):
        return True, []
    try:
        with open(EXCEPTIONS_PATH) as f:
            data = json.load(f)
        ask = data.get('ask', True)
        excs = data.get('exceptions', [])
        if not isinstance(ask, bool) or not isinstance(excs, list):
            raise ValueError
        for e in excs:
            re.compile(e['pattern'])
            if not isinstance(e.get('tool', 'Bash'), str):
                raise ValueError
    except (OSError, ValueError, TypeError, KeyError, re.error):
        return False, None
    return ask, excs


def excepted(excs: list, tool: str, inp: dict):
    """The first exception that matches this tool call, or None."""
    subject = inp.get('command', '') if tool == 'Bash' else json.dumps(inp, sort_keys=True)
    for e in excs:
        if e.get('tool', 'Bash') == tool and re.fullmatch(e['pattern'], subject, re.S):
            return e
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
    decision, exception = 'blocked', None
    if not isinstance(reason, Hard):
        ask, excs = load_exceptions()
        if excs is None:
            reason += ' (guard-exceptions.json could not be used, so this is blocked, not asked)'
        else:
            exception = excepted(excs, tool, inp)
            decision = 'excepted' if exception else ('asked' if ask else 'blocked')
    try:
        os.makedirs(os.path.dirname(GUARD_LOG), exist_ok=True)
        with open(GUARD_LOG, 'a') as f:
            rec = {'t': datetime.datetime.now().isoformat(timespec='seconds'),
                   'session': data.get('session_id', ''), 'tool': tool,
                   'input': inp, 'reason': str(reason), 'decision': decision}
            if exception:
                rec['exception'] = exception.get('why') or exception['pattern']
            f.write(json.dumps(rec) + '\n')
    except OSError:
        pass
    if decision == 'excepted':
        sys.exit(0)  # on to Claude Code's usual permission handling
    if decision == 'asked':
        print(json.dumps({'hookSpecificOutput': {
            'hookEventName': 'PreToolUse', 'permissionDecision': 'ask',
            'permissionDecisionReason': f'fzclaude-guard: {reason}. Allow it this once?'}}))
        sys.exit(0)
    print(f'fzclaude-guard blocked this: {reason}.', file=sys.stderr)
    sys.exit(2)


if __name__ == '__main__':
    main()
