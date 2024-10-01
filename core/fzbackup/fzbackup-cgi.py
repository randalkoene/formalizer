#!/usr/bin/python3
#
# Randal A. Koene, 20210526
#
# This CGI handler provides a web interface to fzbackup.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
sys.stderr = sys.stdout
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
import subprocess

# *** The user should probably actually be retrieved from a cookie or something
#     once logged in.
FZUSER = "randalk"
FORMALIZERSTATEDIR = f"/home/{FZUSER}/.formalizer"
FZBACKUPSDIR = FORMALIZERSTATEDIR+"/archive/postgres"
WEBDATAPATH = "/var/www/webdata/formalizer"

# Create instance of FieldStorage 
form = cgi.FieldStorage()

help = form.getvalue('help')
action = form.getvalue('action')
verbositystr = form.getvalue('verbosity')
if verbositystr:
    try:
        verbosity = int(verbositystr)
    except:
        verbosity = 1
else:
    verbosity = 1

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
<script type="text/javascript" src="/fzuistate.js"></script>

<h1>fzbackup-cgi API</h1>

<p>
Operations:
<ul>
<li><code>action=show</code>: Show database backups.
</ul>
</p>

<h3>Verbosity</h3>

<p>
In each of the modes, the verbosity level can be specified. The levels are:
</p>
<ul>
<li><code>verbosity=0</code>: Quiet mode.</li>
<li><code>verbosity=1</code>: Regular verbosity (default)</li>
<li><code>verbosity=2</code>: Very verbose.</li>
</ul>

<h3>Show database backups</h3>

<p>
<code>fzbackup-cgi.py?action=show[&amp;verbosity=0|1|2]</code><br>
Show the current state of Formalizer database backups.
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
<title>fz: backup</title>
</head>
<body>
<h3>fz: backup</h3>

'''

pagetail = '''
<hr>
<button class="button button1" onclick="window.open('/formalizer/development/backup_to_github.html','_blank');">Backups to GitHub</button>
<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''


results = {}

def try_subprocess_check_output(thecmdstring: str, resstore: str, verbosity: 1) -> int:
    if verbosity > 1:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if verbosity > 0:
            print('Subprocess call caused exception.')
            print('Error output: ',cpe.output.decode())
            print('Error code  : ',cpe.returncode)
            if (cpe.returncode>0):
                print(f'Formalizer error return code: {cpe.returncode}')
        return cpe.returncode
    else:
        if resstore:
            results[resstore] = res
        if verbosity > 1:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

def extra_cmd_args(verbosity = 1) -> str:
    extra = ''
    if verbosity < 1:
        extra += ' -q'
    else:
        if verbosity > 1:
            extra += ' -V'
    return extra


def error_message(msg: str, e):
    print(f'<p class="fail"><b>ERROR: {msg}: {str(e)}</b></p>\n')


def show_backup_state(verbosity = 1) -> bool:
    print(pagehead)
    try:
        _, _, filenames = next(os.walk(FZBACKUPSDIR), (None, None, []))
        print('<table class="blueTable"><tbody>')
        for fname in sorted(filenames):
            print(f'<tr><td>{fname}</td></tr>')
        print('</tbody></table>')
    except Exception as e:
        error_message(f"Unable to display contents of directory '{FZBACKUPSDIR}'", e)

    #thecmd = './fzbackup -E STDOUT -W STDOUT -c ' + extra_cmd_args(verbosity)
    #retcode = try_subprocess_check_output(thecmd, 'backup', verbosity)
    #if (retcode == 0):
        #render_node_with_history(node)
        #print(pagetail_success)
    #else:
    #    print(pagetail_failure)
    #return (retcode == 0)

    print(pagetail)
    return True


def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)
    if (action == 'show'):
        res = show_backup_state(verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)

    show_interface_options()
    sys.exit(0)
