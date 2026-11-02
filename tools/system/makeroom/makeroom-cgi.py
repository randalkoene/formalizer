#!/usr/bin/python3
#
# Randal A. Koene, 20261102
#
# Make room in the coming week: a form that proposes skipping repeating Nodes and moving
# VTD Nodes to next week, so that the UTD Nodes on the todo stack can find a place in the
# Schedule. Each proposal is approved or declined in the form, and only approved ones are
# carried out (with fzgraph -C, as the skip button on Node pages does).
#
# Expects GET or POST data of the following forms:
#   (nothing)              show the form for the next 'days' days (see config)
#   days=<n>               show the form for the next n days
#   action=apply           carry out the approved proposals posted by the form
#   help=on                show these options
#
# Which Nodes are proposed is set in makeroom.json (see README.md).

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, re, json, html
sys.stderr = sys.stdout
from datetime import datetime, timedelta
from subprocess import run

print("Content-type:text/html\n\n")

webdata_path = "/var/www/webdata/formalizer"
configfile = webdata_path+'/makeroom.json'
historyfile = webdata_path+'/makeroom-history.jsonl'

DEFAULT_CONFIG = {
    'days': 7,
    'skip_default': [],
    'skip_offer': [],
    'hide': [],
    'list_fixed_repeating': True,
    'list_exact_repeating': False,
    'move_tdprops': ['variable'],
    'move_days': 7,
    'history_weeks': 8,
}

ID_RE = re.compile(r'^\d{14}\.\d+$')
LIST_TEMPLATE = ('node=STRING:@@R@@{{ node_id }}@@F@@{{ rawtd }}@@F@@{{ req_hrs }}'
                 '@@F@@{{ tdprop }}@@F@@{{ topic }}@@F@@{{ excerpt }}\n')

form = cgi.FieldStorage()

interface_options_help = '''
<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>makeroom-cgi API</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h1>makeroom-cgi API</h1>

Propose skips of repeating Nodes and moves of VTD Nodes, to make room in the coming week.

<p>
Options:
<ul>
<li><code>days</code>: Number of days to look ahead (default from makeroom.json)
<li><code>action=apply</code>: Carry out the approved proposals (posted by the form)
<li><code>help</code>: This page
</ul>
</p>
<p>Configuration: <code>%s</code></p>

</body>
</html>
''' % configfile

PAGE_HEAD = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Make Room</title>
<style>
table.mr { border-collapse: collapse; margin-bottom: 1em; }
table.mr th { text-align: left; padding: 4px 8px; border-bottom: 1px solid #888; }
table.mr td { padding: 3px 8px; vertical-align: top; }
table.mr tr.approved td { background: rgba(80, 160, 80, 0.15); }
.muted { opacity: 0.7; }
.note { font-size: 90%%; }
.fail { color: #c33; font-weight: bold; }
.success { color: #393; }
</style>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>
<h1>Make Room: %s</h1>
'''

PAGE_TAIL = '''
<hr>
<p class="note">Configuration: <code>%s</code></p>
<p>[<a href="/index.html">fz: Top</a>]</p>
<script>
function markRow(chkbx) {
    chkbx.closest('tr').classList.toggle('approved', chkbx.checked);
}
document.querySelectorAll('input.approve').forEach(c => { markRow(c); c.addEventListener('change', () => markRow(c)); });
</script>
</body>
</html>
''' % configfile


def load_config() -> tuple:
    cfg = dict(DEFAULT_CONFIG)
    try:
        with open(configfile, 'r') as f:
            data = json.load(f)
    except FileNotFoundError:
        return cfg, f'No configuration file at {configfile}, using defaults.'
    except (OSError, ValueError) as e:
        return cfg, f'Could not read {configfile}: {e}. Using defaults.'
    for k, v in data.items():
        if k in cfg:
            cfg[k] = v
    return cfg, ''


def esc(s) -> str:
    return html.escape(str(s), quote=True)


def plain(s: str) -> str:
    """Excerpt text without markup."""
    return ' '.join(html.unescape(re.sub(r'<[^>]+>', '', s)).split())


def td_to_dt(td: str):
    try:
        return datetime.strptime(td, '%Y%m%d%H%M')
    except ValueError:
        return None


def fmt_td(td: str) -> str:
    t = td_to_dt(td)
    return t.strftime('%a %Y-%m-%d %H:%M') if t else td


def schedule_entries(days: int) -> list:
    """Schedule entries of the next 'days' days: one dict per instance, in Schedule order."""
    p = run(['./fzgraphhtml', '-I', '-r', '-N', 'all', '-D', str(days), '-x', '120', '-F', 'txt', '-e',
             '-o', 'STDOUT', '-E', os.devnull, '-q', '-T', LIST_TEMPLATE],
            capture_output=True, text=True, errors='replace')
    if p.returncode != 0:
        raise RuntimeError(f'fzgraphhtml returned {p.returncode}')
    entries = []
    for rec in p.stdout.split('@@R@@')[1:]:
        f = rec.rstrip('\n').split('@@F@@')
        if len(f) < 6 or not ID_RE.match(f[0].strip()):
            continue
        tdprop_raw = f[3]
        # The first instance of a repeating fixed/exact Node carries a skip button, later ones a '*'.
        repeats = ('*' in tdprop_raw) or ('action=skip' in tdprop_raw)
        tdprop = plain(re.sub(r'(?is)<button\b.*?</button>', '', tdprop_raw)).rstrip('*').strip()
        entries.append({'id': f[0].strip(), 'td': f[1].strip(), 'hrs': f[2].strip(), 'tdprop': tdprop,
                        'repeats': repeats, 'topic': f[4].strip(), 'excerpt': plain(f[5])})
    return entries


def read_history() -> list:
    recs = []
    try:
        with open(historyfile, 'r') as f:
            for line in f:
                try:
                    recs.append(json.loads(line))
                except ValueError:
                    pass
    except OSError:
        pass
    return recs


def skip_weeks(history: list, weeks: int) -> dict:
    """For each Node, in how many of the last 'weeks' weeks it was skipped with this form."""
    since = datetime.now() - timedelta(weeks=weeks)
    res = {}
    for r in history:
        if r.get('action') != 'skip' or not r.get('ok'):
            continue
        t = td_to_dt(r.get('t', ''))
        if t and t >= since:
            res.setdefault(r['id'], set()).add(t.isocalendar()[:2])
    return {k: len(v) for k, v in res.items()}


def node_link(nid: str, excerpt: str) -> str:
    return f'<a href="/cgi-bin/fzlink.py?id={nid}" target="_blank">{esc(excerpt)}</a>'


def node_age(nid: str) -> str:
    return nid[0:4]


def show_form(cfg: dict, days: int, msg: str):
    print(PAGE_HEAD % f'next {days} days')
    if msg:
        print(f'<p class="fail">{esc(msg)}</p>')
    try:
        entries = schedule_entries(days)
    except RuntimeError as e:
        print(f'<p class="fail">Could not read the Schedule: {esc(e)}</p>')
        print(PAGE_TAIL)
        return

    hide = set(cfg['hide'])
    skip_default, skip_offer = set(cfg['skip_default']), set(cfg['skip_offer'])
    move_tdprops = set(cfg['move_tdprops'])
    history = skip_weeks(read_history(), cfg['history_weeks'])

    # One row per Node: repeating Nodes may have several instances in the window.
    skips, moves, seen = [], [], {}
    for e in entries:
        nid = e['id']
        if nid in hide:
            continue
        if nid in seen:
            seen[nid]['count'] += 1
            continue
        e['count'] = 1
        seen[nid] = e
        if e['repeats']:
            listed = nid in skip_default or nid in skip_offer
            if (listed or (e['tdprop'] == 'fixed' and cfg['list_fixed_repeating'])
                    or (e['tdprop'] == 'exact' and cfg['list_exact_repeating'])):
                skips.append(e)
        elif e['tdprop'] in move_tdprops:
            moves.append(e)

    print('<form action="/cgi-bin/makeroom-cgi.py" method="post">')
    print('<input type="hidden" name="action" value="apply">')
    print(f'<input type="hidden" name="days" value="{days}">')

    print('<h2>Skip repeating Nodes</h2>')
    print('<p class="note">An approved skip moves the Node to its next regular occurrence '
          '(or further, with a larger number). Nodes in <code>skip_default</code> start out approved.</p>')
    if skips:
        print('<table class="mr"><tr><th>skip</th><th>next</th><th>prop</th><th>hrs</th><th>in window</th>'
              '<th>since</th><th>Node</th><th>skipped recently</th></tr>')
        for e in skips:
            nid = e['id']
            checked = ' checked' if nid in skip_default else ''
            recent = history.get(nid, 0)
            recent_str = f'{recent} of last {cfg["history_weeks"]} weeks' if recent else ''
            print(f'<tr><td><input type="checkbox" class="approve" name="skip_{nid}"{checked}> '
                  f'<input type="number" name="skipn_{nid}" value="1" min="1" max="{max(e["count"], 1) + 10}" style="width:3.5em"></td>'
                  f'<td>{esc(fmt_td(e["td"]))}</td><td>{esc(e["tdprop"])}</td><td>{esc(e["hrs"])}</td>'
                  f'<td>{e["count"]}&times;</td><td>{node_age(nid)}</td>'
                  f'<td>{node_link(nid, e["excerpt"])} <span class="muted">[{esc(e["topic"])}]</span></td>'
                  f'<td class="note">{esc(recent_str)}</td></tr>')
        print('</table>')
    else:
        print('<p class="muted">No repeating Nodes to propose.</p>')

    print('<h2>Move VTD Nodes to later</h2>')
    print(f'<p class="note">Proposed: the same weekday and time {cfg["move_days"]} days later. '
          'These start out declined. Change the date before approving if you like.</p>')
    if moves:
        print('<table class="mr"><tr><th>move</th><th>now</th><th>to</th><th>hrs</th><th>Node</th></tr>')
        for e in moves:
            nid = e['id']
            t = td_to_dt(e['td'])
            new = (t + timedelta(days=cfg['move_days'])).strftime('%Y-%m-%dT%H:%M') if t else ''
            print(f'<tr><td><input type="checkbox" class="approve" name="move_{nid}"></td>'
                  f'<td>{esc(fmt_td(e["td"]))}</td>'
                  f'<td><input type="datetime-local" name="newtd_{nid}" value="{new}"></td>'
                  f'<td>{esc(e["hrs"])}</td>'
                  f'<td>{node_link(nid, e["excerpt"])} <span class="muted">[{esc(e["topic"])}]</span></td></tr>')
        print('</table>')
    else:
        print('<p class="muted">No VTD Nodes to propose.</p>')

    print('<p><label><input type="checkbox" name="dryrun"> Dry run (show the commands, change nothing)</label> '
          '<input type="submit" value="Carry out approved"></p>')
    print('</form>')
    print(PAGE_TAIL)


def apply_choices(cfg: dict, days: int):
    print(PAGE_HEAD % 'results')
    dryrun = bool(form.getvalue('dryrun'))
    actions = []
    for key in form.keys():
        m = re.fullmatch(r'(skip|move)_(\d{14}\.\d+)', key)
        if not m:
            continue
        kind, nid = m.groups()
        if kind == 'skip':
            try:
                n = int(form.getvalue(f'skipn_{nid}', '1'))
            except ValueError:
                n = 0
            if n < 1:
                actions.append((kind, nid, '', f'invalid skip number for {nid}'))
                continue
            actions.append((kind, nid, str(n), ''))
        else:
            val = form.getvalue(f'newtd_{nid}', '')
            try:
                td = datetime.strptime(val, '%Y-%m-%dT%H:%M').strftime('%Y%m%d%H%M')
            except ValueError:
                actions.append((kind, nid, '', f'invalid new target date for {nid}: {val}'))
                continue
            actions.append((kind, nid, td, ''))

    if not actions:
        print('<p>Nothing was approved.</p>')
    now = datetime.now().strftime('%Y%m%d%H%M')
    print('<table class="mr"><tr><th>Node</th><th>action</th><th>result</th></tr>')
    for kind, nid, val, err in actions:
        what = f'skip {val}' if kind == 'skip' else (f'move to {fmt_td(val)}' if val else 'move')
        link = f'<a href="/cgi-bin/fzlink.py?id={nid}" target="_blank">{nid}</a>'
        if err:
            print(f'<tr><td>{link}</td><td>{esc(what)}</td><td class="fail">{esc(err)}</td></tr>')
            continue
        api = f'/fz/graph/nodes/{nid}?skip={val}' if kind == 'skip' else f'/fz/graph/nodes/{nid}?targetdate={val}'
        cmd = ['./fzgraph', '-E', 'STDOUT', '-C', api]
        if dryrun:
            print(f'<tr><td>{link}</td><td>{esc(what)}</td><td><code>{esc(" ".join(cmd))}</code></td></tr>')
            continue
        p = run(cmd, capture_output=True, text=True, errors='replace')
        ok = p.returncode == 0
        out = (p.stdout + p.stderr).strip()
        result = '<span class="success">done</span>' if ok else f'<span class="fail">error {p.returncode}</span> {esc(out[-300:])}'
        print(f'<tr><td>{link}</td><td>{esc(what)}</td><td>{result}</td></tr>')
        try:
            with open(historyfile, 'a') as f:
                f.write(json.dumps({'t': now, 'id': nid, 'action': kind, 'value': val, 'ok': ok}) + '\n')
        except OSError as e:
            print(f'<tr><td colspan="3" class="fail">Could not record in {esc(historyfile)}: {esc(e)}</td></tr>')
    print('</table>')
    if dryrun:
        print('<p>Dry run: nothing was changed.</p>')
    print(f'<p>[<a href="/cgi-bin/makeroom-cgi.py?days={days}">Back to the form</a>] '
          '[<a href="/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20" target="_blank">Schedule</a>]</p>')
    print(PAGE_TAIL)


if __name__ == '__main__':
    if form.getvalue('help'):
        print(interface_options_help)
        sys.exit(0)
    cfg, msg = load_config()
    try:
        days = int(form.getvalue('days', cfg['days']))
    except ValueError:
        days = cfg['days']
    days = max(1, min(days, 31))
    if form.getvalue('action') == 'apply':
        apply_choices(cfg, days)
    else:
        show_form(cfg, days, msg)
    sys.exit(0)
