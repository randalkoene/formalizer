#!/usr/bin/python3
#
# team_overview.py
#
# Randal A. Koene, 20261127
#
# An overview of CCF team members and interns, built from the person Nodes under
# CCF-ACTIVE-TEAM (Node 20250822084723.1), the Named Node Lists team-interns-active,
# team-staff and team-alumni, the @MEMBER: key=value ...@ line in each person Node,
# their cluster superiors, and Log entries about them. People who need attention
# (an internship that ends soon or has ended, a handoff not done, no notes for a
# while, @NEEDSHELP@ entries, missing or conflicting information) come first.
#
# See Node 20261120081749.1 and the doc "Managing interns and team members in the
# Formalizer" for the conventions:
#   @MEMBER: supervisor=<name> start=<YYYYmm[dd]> [end=<YYYYmm[dd]> handoff=no|scheduled|done|n/a
#            after=continuing|volunteer|staff|left|quit|? contact=yes|no|?]@
#   (end, handoff, after and contact are for interns; ? means not known yet)
#   A Log entry about a person either belongs to the person Node (e.g. made with the
#   Node page "entry" button) or starts with the person Node ID in parentheses.
#   @NEEDSHELP@ in such an entry marks an issue where the person needs help.
#
# Command line:  team_overview.py [-F html|json] [-w <days>] [-q <days>] [-s <days>]
# CGI:           team_overview.py?window=<days>&quiet=<days>&soon=<days>

import os
import sys
import re
import json
import html
import subprocess
from datetime import datetime, date, timedelta

running_as_cgi = 'GATEWAY_INTERFACE' in os.environ

TEAM = '20250822084723.1'
CLUSTERS = '20250822225247.1'
NNLS = ('team-interns-active', 'team-staff', 'team-alumni')
NNL_SHORT = {'team-interns-active': 'intern', 'team-staff': 'staff', 'team-alumni': 'alumni'}

NODE_ID_RE = re.compile(r'^[0-9]{14}\.[0-9]{1,3}$')
MEMBER_RE = re.compile(r'@MEMBER:([^@]*)@')
ROLE_RE = re.compile(r'Onboarded for role</b>:\s*([^\n<]*)')
H3_RE = re.compile(r'<h3>(.*?)</h3>', re.IGNORECASE | re.DOTALL)
PREFIX_RE = re.compile(r'^\s*(?:<[^>]+>\s*)*\(([0-9]{14}\.[0-9]{1,3})\)')
TAG_RE = re.compile(r'<[^>]+>')


class ToolError(Exception):
    pass


def run_tool(prog: str, args: list) -> str:
    """Run a Formalizer tool (./prog in cgi-bin, prog on the PATH otherwise) and return its stdout."""
    cmd = [('./' + prog) if running_as_cgi else prog] + args
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.TimeoutExpired) as e:
        raise ToolError(f'{" ".join(cmd)}: {e}')
    if res.returncode != 0:
        raise ToolError(f'{" ".join(cmd)}: exit code {res.returncode}: {res.stderr.strip()[:300]}')
    return res.stdout


def node_data(node: str) -> dict:
    out = run_tool('fzgraphhtml', ['-q', '-o', 'STDOUT', '-F', 'json', '-n', node])
    try:
        return json.loads(out)
    except json.JSONDecodeError as e:
        raise ToolError(f'fzgraphhtml -n {node}: no valid JSON ({e})')


def id_list(field: str) -> list:
    return [s for s in (field or '').split() if NODE_ID_RE.match(s)]


def plain(text: str) -> str:
    return html.unescape(TAG_RE.sub(' ', text or '')).split()


def plain_str(text: str) -> str:
    return ' '.join(plain(text))


def parse_member(text: str) -> dict:
    m = MEMBER_RE.search(text or '')
    if not m:
        return None
    fields = {}
    for pair in m.group(1).split():
        if '=' in pair:
            k, v = pair.split('=', 1)
            fields[k.strip()] = v.strip()
    return fields


def parse_ymd(s: str, end_of_month: bool = False):
    """YYYYmmdd or YYYYmm to a date (YYYYmm: first or last day of the month); None if unknown."""
    if not s or s == '?':
        return None
    try:
        if len(s) == 8:
            return datetime.strptime(s, '%Y%m%d').date()
        if len(s) == 6:
            d = datetime.strptime(s, '%Y%m').date()
            if end_of_month:
                nxt = (d.replace(day=28) + timedelta(days=4)).replace(day=1)
                return nxt - timedelta(days=1)
            return d
    except ValueError:
        pass
    return None


def cluster_name(text: str) -> str:
    s = plain_str(text)
    i = s.find(' Lead:')
    return s[:i] if i > 0 else s[:40]


def person_name(text: str) -> str:
    m = H3_RE.search(text or '')
    if m:
        return plain_str(m.group(1))
    return plain_str(text)[:40]


def collect_people() -> tuple:
    team = node_data(TEAM)
    clusters = {}
    for c in id_list(node_data(CLUSTERS).get('dependencies')):
        clusters[c] = cluster_name(node_data(c).get('node-text', ''))
    people = {}
    for n in id_list(team.get('dependencies')):
        d = node_data(n)
        text = d.get('node-text', '')
        if 'CCF career path' not in text:
            continue  # a task or other Node under CCF-ACTIVE-TEAM, not a person
        try:
            completion = float(d.get('completion', '0'))
        except ValueError:
            completion = 0.0
        if completion >= 1.0 or completion < 0.0:
            continue  # completed or obsolete Node
        role = ROLE_RE.search(text)
        nnls = [s.strip() for s in (d.get('NNLs') or '').split(',') if s.strip() in NNLS]
        people[n] = {
            'id': n,
            'name': person_name(text),
            'role': plain_str(role.group(1)) if role else '',
            'member': parse_member(text),
            'nnls': nnls,
            'clusters': [clusters[s] for s in id_list(d.get('superiors')) if s in clusters],
            'last_note': None,
            'needshelp': [],
        }
    return people, clusters


def parse_raw_log(out: str):
    """Yield (chunk_open_YYYYmmddHHMM, entry_node_or_chunk_node, entry_text) from fzloghtml -F raw output."""
    chunk_open, chunk_node = '', ''
    lines = out.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith('CHUNK:'):
            parts = line.split(':')
            chunk_open = parts[1] if len(parts) > 1 else ''
            chunk_node = parts[-1].strip() if NODE_ID_RE.match(parts[-1].strip()) else ''
        elif line.startswith('ENTRY:'):
            parts = line.split(':')
            entry_node = parts[2].strip() if len(parts) > 2 else ''
            text = []
            i += 1
            if i < len(lines) and lines[i] == '__BEGIN__':
                i += 1
            while i < len(lines) and lines[i] != '__END__':
                text.append(lines[i])
                i += 1
            yield chunk_open, (entry_node or chunk_node), '\n'.join(text)
        i += 1


def collect_log(people: dict, window_days: int):
    t_from = (date.today() - timedelta(days=window_days)).strftime('%Y%m%d')
    outputs = []
    for nnl in NNLS:
        outputs.append(run_tool('fzloghtml', ['-q', '-o', 'STDOUT', '-N', '-F', 'raw', '-1', t_from, '-2', 'now', '-l', nnl]))
    outputs.append(run_tool('fzloghtml', ['-q', '-o', 'STDOUT', '-N', '-F', 'raw', '-1', t_from, '-2', 'now',
                                          '-x', '[(][0-9]{14}[.][0-9]{1,3}[)]']))
    seen = set()
    for out in outputs:
        for t_open, node, text in parse_raw_log(out):
            about = set()
            if node in people:
                about.add(node)
            m = PREFIX_RE.match(text)
            if m and m.group(1) in people:
                about.add(m.group(1))
            for p in about:
                key = (p, t_open, text)
                if key in seen:
                    continue
                seen.add(key)
                person = people[p]
                if (person['last_note'] is None) or (t_open > person['last_note']):
                    person['last_note'] = t_open
                if '@NEEDSHELP@' in text:
                    person['needshelp'].append((t_open, plain_str(text.replace('@NEEDSHELP@', ''))[:160]))


def attention(p: dict, today: date, quiet_days: int, soon_days: int) -> list:
    flags = []
    nnls = p['nnls']
    cat = nnls[0] if len(nnls) == 1 else None
    if not nnls:
        flags.append('in none of the team NNLs')
    elif len(nnls) > 1:
        flags.append('in more than one team NNL: ' + ', '.join(nnls))
    m = p['member']
    if m is None:
        flags.append('no @MEMBER line')
        m = {}
    else:
        if m.get('supervisor', '?') == '?':
            flags.append('supervisor unknown')
    end = parse_ymd(m.get('end'), end_of_month=True)
    handoff = m.get('handoff', '?')
    if cat == 'team-interns-active':
        if end is None:
            flags.append('end date unknown')
        elif end < today:
            flags.append(f'internship ended {end.isoformat()}: record after= and move to team-alumni')
        elif (end - today).days <= soon_days and handoff in ('no', '?'):
            flags.append(f'ends {end.isoformat()}, handoff not scheduled')
        if handoff == '?' and m:
            flags.append('handoff state missing')
    if cat == 'team-alumni' and 'end' in m:  # a former intern
        if m.get('after', '?') == '?':
            flags.append('after= unknown')
        if handoff in ('no', 'scheduled', '?'):
            flags.append(f'handoff {handoff}')
    if cat == 'team-alumni' and p['clusters']:
        flags.append('alumni still in cluster ' + ', '.join(p['clusters']))
    if cat in ('team-interns-active', 'team-staff'):
        recent_help = [h for h in p['needshelp'] if h[0][:8] >= (today - timedelta(days=quiet_days)).strftime('%Y%m%d')]
        if recent_help:
            flags.append(f'@NEEDSHELP@: {len(recent_help)} in the last {quiet_days} days')
        if p['last_note'] is None:
            flags.append('no notes in the Log window')
        else:
            last = datetime.strptime(p['last_note'][:8], '%Y%m%d').date()
            if (today - last).days > quiet_days:
                flags.append(f'no notes for {(today - last).days} days')
    return flags


PAGE_HEAD = '''<html>
<head>
<meta charset="utf-8" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Team Overview</title>
<style>
table { border-collapse: collapse; width: 100%%; }
th, td { border: 1px solid #ccc; padding: 4px 6px; text-align: left; vertical-align: top; font-size: 0.9em; }
th { background-color: var(--tr-bg-2); color: var(--tr-fg); }
td.flags { color: #a00000; }
[data-theme="darkblue1"] td.flags, [data-theme="darkblue2"] td.flags, [data-theme="black"] td.flags { color: #ffcc66; }
tr.alumni td { color: var(--inactive-node-color); }
</style>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>
<h1>Team Overview</h1>
<p>%s. Log window: %d days. Quiet after %d days without notes. Handoff warning %d days before the end date.</p>
'''

PAGE_TAIL = '''<hr>
<p>[<a href="/index.html">fz: Top</a>] [<a href="/cgi-bin/nodeboard-cgi.py?n=20250822225247.1&Z=true" target="_blank">CCF Clusters</a>]
[<a href="/cgi-bin/nodeboard-cgi.py?n=20250822084723.1&G=true&Z=true&T=true&B=79" target="_blank">CCF Team</a>]</p>
</body>
</html>
'''

COLUMNS = ('Person', 'NNL', 'Role', 'Clusters', 'Supervisor', 'Start', 'End', 'Handoff', 'After', 'Contact', 'Last note', 'Attention')


def row_html(p: dict) -> str:
    m = p['member'] or {}
    n = html.escape(p['id'])
    person = (f'<a href="/cgi-bin/fzlink.py?id={n}" target="_blank">{html.escape(p["name"])}</a> '
              f'<a href="/cgi-bin/logentry-form.py?makeentry=fornode&node={n}" target="_blank" title="add a Log entry about this person to the open chunk">[entry]</a>')
    last = p['last_note'] or ''
    if last:
        last = f'<a href="/cgi-bin/fzlink.py?id={n}&alt=hist50#END" target="_blank">{last[:4]}-{last[4:6]}-{last[6:8]}</a>'
    flags = '<br>'.join(html.escape(f) for f in p['flags'])
    if p['needshelp']:
        flags += ''.join(f'<br><i>{html.escape(t[:8])}: {html.escape(txt)}</i>' for t, txt in p['needshelp'][-3:])
    cells = [person, ', '.join(NNL_SHORT.get(x, x) for x in p['nnls']), html.escape(p['role']),
             html.escape(', '.join(p['clusters'])), html.escape(m.get('supervisor', '')), html.escape(m.get('start', '')),
             html.escape(m.get('end', '')), html.escape(m.get('handoff', '')), html.escape(m.get('after', '')),
             html.escape(m.get('contact', '')), last, flags]
    cls = ' class="alumni"' if p['nnls'] == ['team-alumni'] else ''
    return f'<tr{cls}>' + ''.join((f'<td class="flags">{c}</td>' if i == len(cells) - 1 else f'<td>{c}</td>')
                                  for i, c in enumerate(cells)) + '</tr>\n'


def table_html(title: str, rows: list) -> str:
    if not rows:
        return ''
    head = '<tr>' + ''.join(f'<th>{c}</th>' for c in COLUMNS) + '</tr>\n'
    return f'<h2>{html.escape(title)} ({len(rows)})</h2>\n<table><tbody>\n{head}{"".join(row_html(p) for p in rows)}</tbody></table>\n'


def sort_key(p: dict):
    order = {'team-interns-active': 0, 'team-staff': 1, 'team-alumni': 2}
    return (order.get(p['nnls'][0], 3) if len(p['nnls']) == 1 else -1, p['name'].lower())


def build(window_days: int, quiet_days: int, soon_days: int) -> dict:
    people, _ = collect_people()
    collect_log(people, window_days)
    today = date.today()
    for p in people.values():
        p['flags'] = attention(p, today, quiet_days, soon_days)
    return people


def render_html(people: dict, window_days: int, quiet_days: int, soon_days: int) -> str:
    rows = sorted(people.values(), key=sort_key)
    attn = [p for p in rows if p['flags'] and p['nnls'] != ['team-alumni']]
    interns = [p for p in rows if p['nnls'] == ['team-interns-active'] and not p['flags']]
    staff = [p for p in rows if p['nnls'] == ['team-staff'] and not p['flags']]
    alumni = [p for p in rows if p['nnls'] == ['team-alumni']]
    summary = (f'{len(people)} people: {sum(p["nnls"] == ["team-interns-active"] for p in rows)} active interns, '
               f'{sum(p["nnls"] == ["team-staff"] for p in rows)} staff, {len(alumni)} alumni; '
               f'{len(attn)} need attention')
    out = PAGE_HEAD % (summary, window_days, quiet_days, soon_days)
    out += table_html('Needs attention', attn)
    out += table_html('Active interns', interns)
    out += table_html('Staff', staff)
    out += f'<details><summary>Alumni ({len(alumni)})</summary>\n{table_html("Alumni", alumni)}</details>\n'
    return out + PAGE_TAIL


def int_arg(value, default: int) -> int:
    try:
        v = int(value)
        return v if 0 < v <= 3650 else default
    except (TypeError, ValueError):
        return default


def process_as_cgi():
    import cgi
    print('Content-type:text/html\n')
    form = cgi.FieldStorage()
    window_days = int_arg(form.getvalue('window'), 180)
    quiet_days = int_arg(form.getvalue('quiet'), 30)
    soon_days = int_arg(form.getvalue('soon'), 21)
    try:
        people = build(window_days, quiet_days, soon_days)
    except ToolError as e:
        print(f'<html><body><h1>Team Overview</h1><p class="fail">{html.escape(str(e))}</p></body></html>')
        return
    print(render_html(people, window_days, quiet_days, soon_days))


def process_as_cmdline():
    import argparse
    parser = argparse.ArgumentParser(description='Overview of CCF team members and interns.')
    parser.add_argument('-F', dest='outformat', default='html', choices=('html', 'json'), help='output format (default: html)')
    parser.add_argument('-w', dest='window', type=int, default=180, help='days of Log to search (default: 180)')
    parser.add_argument('-q', dest='quiet', type=int, default=30, help='flag active people without notes for this many days (default: 30)')
    parser.add_argument('-s', dest='soon', type=int, default=21, help='flag an internship ending within this many days without a scheduled handoff (default: 21)')
    args = parser.parse_args()
    try:
        people = build(args.window, args.quiet, args.soon)
    except ToolError as e:
        print(f'Error: {e}', file=sys.stderr)
        sys.exit(1)
    if args.outformat == 'json':
        print(json.dumps(sorted(people.values(), key=sort_key), indent=1))
    else:
        print(render_html(people, args.window, args.quiet, args.soon))


if __name__ == '__main__':
    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
