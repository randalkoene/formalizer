#!/usr/bin/python3
#
# Randal A. Koene, 20210526
#
# This CGI handler provides a web interface to fzbackup: the Backups page
# (Database: Backups on the FZ Admin page).
#
# 20261127 (Plan B package 4, Node 20261102153503.1): shows local and
# off-site backups with the retention rule that keeps each one, and offers
# Verify, Fetch (from off-site) and Restore for each backup.
#
# - Verify and Fetch run fzbackup-verify.py and database-from-offsite.py as
#   the Formalizer user, through fzserverpq (CGIbg_run_as_user). Both must be
#   listed in predefined_CGIbg in the fzserverpq config.
# - Restore does not change anything from the web page: it explains the steps
#   and the undo, and shows the fzbackup-restore.sh command to run in a
#   terminal (fzserverpq has to be stopped and started again for a restore).
# - Off-site information comes from the status file that fzbackup-offsite.py
#   writes after every run, because the web server cannot read the restic
#   password.

# Import modules for CGI handling
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, re, json, html
sys.stderr = sys.stdout
import importlib.util
from time import sleep
from datetime import datetime

# *** The user should probably actually be retrieved from a cookie or something
#     once logged in.
FZUSER = "randalk"
FORMALIZERSTATEDIR = f"/home/{FZUSER}/.formalizer"
FZBACKUPSDIR = FORMALIZERSTATEDIR+"/archive/postgres"
WEBDATAPATH = "/var/www/webdata/formalizer"
OFFSITE_STATUS = "/var/www/html/formalizer/fzbackup-offsite-status.json"

BACKUP_NAME_RE = re.compile(r'^formalizer-postgres-backup-(\d{12})\.gz$')

# Background jobs run as the Formalizer user: program, output file, signal file.
JOBS = {
    'verify': ('fzbackup-verify.py', 'fzbackup-verify'),
    'fetch': ('database-from-offsite.py', 'database-from-offsite'),
}
JOB_TIMEOUT_S = 4*60

# Create instance of FieldStorage
form = cgi.FieldStorage()

help = form.getvalue('help')
action = form.getvalue('action')
backupname = form.getvalue('f')

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<head>
<title>fzbackup-cgi API</title>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h1>fzbackup-cgi API</h1>

<p>
Operations:
<ul>
<li><code>action=show</code>: Show database backups, local and off-site, with Verify, Fetch and Restore.
<li><code>action=verify&amp;f=NAME</code>: Restore backup NAME into a scratch database and compare it with the live database.
<li><code>action=fetch&amp;f=NAME</code>: Fetch backup NAME from the off-site copy into the local backup directory.
</ul>
</p>

</body>
</html>
'''

pagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: %s</title>
<style>
.fail { color: #d03030; font-weight: bold; }
.ok { color: #30a030; font-weight: bold; }
.note { opacity: 0.75; }
.blueTable td { vertical-align: middle; }
.blueTable td.num { text-align: right; }
dialog { max-width: 52em; background: var(--tr-bg-2, #f4f4f4); color: var(--tr-fg, #000); border: 1px solid #888; }
dialog pre { white-space: pre-wrap; padding: 0.5em; border: 1px solid #888; }
</style>
</head>
<body>
<h3>fz: %s</h3>
'''

pagetail = '''
<hr>
[<a href="/index.html">fz: Top</a>]
<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''


def load_prune_module():
    """The retention rule of fzbackup-prune.py, copied to the cgi-bin directory."""
    here = os.path.dirname(os.path.realpath(__file__))
    spec = importlib.util.spec_from_file_location('fzbackup_prune', os.path.join(here, 'fzbackup-prune.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_offsite_status() -> dict:
    try:
        with open(OFFSITE_STATUS) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def human_size(n: int) -> str:
    return f'{n/1e6:.1f} MB'


def backup_time(name: str) -> datetime:
    return datetime.strptime(BACKUP_NAME_RE.match(name).group(1), '%Y%m%d%H%M')


def status_section(local: dict, status: dict):
    print('<h4>Status</h4>\n<ul>')
    if local:
        newest = max(local)
        print(f'<li>Local: {len(local)} backups in <code>{FZBACKUPSDIR}</code>, newest '
              f'{backup_time(newest):%Y-%m-%d %H:%M}.</li>')
    else:
        print(f'<li class="fail">Local: no backups found in <code>{FZBACKUPSDIR}</code>.</li>')

    if not status:
        print('<li class="fail">Off-site: no status yet. Run <code>fzbackup-offsite.py</code> once '
              '(see <code>core/fzbackup/README.md</code>).</li>')
    else:
        snaps = status.get('snapshots', [])
        newest_snap = snaps[-1]['time'] if snaps else 'none'
        if status.get('ok'):
            age_h = (datetime.now() - datetime.strptime(status['time'], '%Y-%m-%d %H:%M')).total_seconds() / 3600
            stale = f' <span class="fail">That is {age_h/24:.1f} days ago: is the daily cron run working?</span>' if age_h > 36 else ''
            print(f'<li>Off-site: <span class="ok">last run OK</span> at {status["time"]}{stale}. '
                  f'{len(snaps)} snapshots in <code>{html.escape(status.get("repository", ""))}</code>, newest {newest_snap}.</li>')
        else:
            last_ok = status.get('last_ok_time', 'unknown')
            print(f'<li class="fail">Off-site: last run FAILED at {status["time"]}: '
                  f'{html.escape(status.get("message", ""))}. Last successful run: {last_ok}.</li>')
            if status.get('auth_expired'):
                print('<li class="fail">The Google Drive sign-in has expired. Renew it in a terminal with '
                      '<code>rclone config reconnect gdrive:</code> and then run <code>fzbackup-offsite.py</code>.</li>')
    print('<li class="note">Retention, for local backups and off-site snapshots: the newest backup of each day '
          'for 14 days, of each week for 3 months, of each month forever, plus the 4 most recent.</li>')
    print('</ul>')


def backups_table(local: dict, status: dict):
    offsite = status.get('backups', {})
    names = sorted(set(local) | set(offsite), reverse=True)
    keep = {}
    try:
        prune = load_prune_module()
        local_sorted = sorted((backup_time(n), n) for n in local)
        keep = prune.decide(local_sorted, datetime.now(), 14, 92, 4)
    except Exception as e:
        print(f'<p class="fail">Could not apply the retention rule: {html.escape(str(e))}</p>')

    print('<h4>Backups</h4>')
    print('<table class="blueTable"><thead><tr><th>Made</th><th>Size</th><th>Local</th>'
          '<th>Off-site</th><th>Kept by</th><th>Actions</th></tr></thead><tbody>')
    for name in names:
        t = backup_time(name)
        size = local.get(name) or offsite.get(name, {}).get('size', 0)
        is_local = name in local
        snaps = offsite.get(name, {}).get('snapshots', [])
        if is_local:
            kept = keep.get(name, '<span class="fail">removed at next pruning</span>') if keep else ''
        else:
            kept = '<span class="note">only off-site</span>'
        actions = []
        if is_local:
            actions.append(f'<button class="button button1" onclick="window.open(\'/cgi-bin/fzbackup-cgi.py?action=verify&f={name}\',\'_blank\');">Verify</button>')
            actions.append(f'<button class="button button2" onclick="showRestore(\'{name}\');">Restore&hellip;</button>')
        elif snaps:
            actions.append(f'<button class="button button1" onclick="window.open(\'/cgi-bin/fzbackup-cgi.py?action=fetch&f={name}\',\'_blank\');">Fetch</button>')
        print(f'<tr><td title="{name}">{t:%Y-%m-%d %H:%M}</td><td class="num">{human_size(size)}</td>'
              f'<td>{"yes" if is_local else "-"}</td>'
              f'<td title="{", ".join(snaps)}">{("in " + str(len(snaps)) + " snapshot" + ("s" if len(snaps) != 1 else "")) if snaps else "-"}</td>'
              f'<td>{kept}</td><td>{" ".join(actions)}</td></tr>')
    print('</tbody></table>')


RESTORE_DIALOG = '''
<dialog id="restoreDialog">
<h3>Restore <span id="restoreName"></span>?</h3>
<p>This replaces the live Formalizer data (Graph, Log, Named Node Lists and the rest) with the
data in this backup. Changes made after the backup was made are no longer in the live data,
but nothing is deleted, and the restore can be undone.</p>
<p>Restoring needs fzserverpq to be stopped and started again, so it is done in a terminal.
Run this command there:</p>
<pre id="restoreCommand"></pre>
<button class="button button1" onclick="navigator.clipboard.writeText(document.getElementById('restoreCommand').textContent);">Copy command</button>
<p>It asks for confirmation, and then:</p>
<ol>
<li>makes a fresh backup (and off-site copy) of the current state;</li>
<li>verifies the chosen backup in a scratch database;</li>
<li>stops fzserverpq;</li>
<li>renames the current data to schema <code>%s_before_restore_TIMESTAMP</code> (nothing is deleted);</li>
<li>loads the backup and checks that the restored data matches it exactly;</li>
<li>tells you to start fzserverpq again with <code>fzserverpqd.sh</code>.</li>
</ol>
<p><b>To undo</b>, run the command it shows at the end:
<code>fzbackup-restore.sh --undo TIMESTAMP</code>. That puts the renamed data back (and keeps the
restored data aside). When you are sure, <code>fzbackup-restore.sh --drop-old TIMESTAMP</code>
deletes the old data.</p>
<button class="button button2" onclick="document.getElementById('restoreDialog').close();">Close</button>
</dialog>
<script>
function showRestore(name) {
    document.getElementById('restoreName').textContent = name;
    document.getElementById('restoreCommand').textContent = 'fzbackup-restore.sh ' + name;
    document.getElementById('restoreDialog').showModal();
}
</script>
'''


def show_backup_state() -> bool:
    print(pagehead % ('backups', 'Database backups'))
    local = {}
    try:
        for fname in os.listdir(FZBACKUPSDIR):
            if BACKUP_NAME_RE.match(fname):
                local[fname] = os.path.getsize(os.path.join(FZBACKUPSDIR, fname))
    except Exception as e:
        print(f'<p class="fail">Unable to read the directory {FZBACKUPSDIR}: {html.escape(str(e))}</p>')
    status = read_offsite_status()
    status_section(local, status)
    backups_table(local, status)
    print(RESTORE_DIALOG % FZUSER)
    print(pagetail)
    return True


def run_job(job: str, name: str) -> bool:
    """Run a predefined background job as the Formalizer user and show its output."""
    title = {'verify': 'Verify backup', 'fetch': 'Fetch backup'}[job]
    print(pagehead % (job, title))
    if not name or not BACKUP_NAME_RE.match(name):
        print('<p class="fail">Not a backup name.</p>' + pagetail)
        return False
    print(f'<p>{title} <code>{name}</code>. This can take a minute or two...</p>', flush=True)

    from tcpclient import serial_API_request
    program, basename = JOBS[job]
    signalfile = f'{WEBDATAPATH}/{basename}.signal'
    outfile = f'{WEBDATAPATH}/{basename}.out'
    try:
        os.remove(signalfile)
    except OSError:
        pass
    start = datetime.now().strftime('%Y%m%d%H%M%S')
    serial_API_request(f'CGIbg_run_as_user({program},f={name}&S={signalfile},{outfile})',
                       running_on_server=True, error_exit_pause=False)

    for _ in range(JOB_TIMEOUT_S):
        sleep(1)
        try:
            with open(signalfile) as f:
                if f.read().strip() >= start:
                    break
        except OSError:
            continue
    else:
        print(f'<p class="fail">No result after {JOB_TIMEOUT_S//60} minutes. Is {program} listed in '
              'predefined_CGIbg in the fzserverpq config, and is fzserverpq running? '
              f'Output so far may be in <code>{outfile}</code>.</p>' + pagetail)
        return False
    try:
        with open(outfile) as f:
            output = f.read()
    except OSError as e:
        output = f'Unable to read {outfile}: {e}'
    print(f'<pre>{html.escape(output)}</pre>')
    print('<p><a href="/cgi-bin/fzbackup-cgi.py?action=show">Back to the backups</a></p>' + pagetail)
    return True


def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)
    if action == 'show':
        sys.exit(0 if show_backup_state() else 1)
    if action in JOBS:
        sys.exit(0 if run_job(action, backupname) else 1)

    show_interface_options()
    sys.exit(0)
