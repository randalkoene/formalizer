#!/usr/bin/python3
#
# Randal A. Koene, 20210326
#
# This CGI handler provides a web interface to fzlog.

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

# Create instance of FieldStorage 
form = cgi.FieldStorage()

help = form.getvalue('help')
action = form.getvalue('action')
node = form.getvalue('node')
T_emulated = form.getvalue('T')
if (T_emulated == 'actual'): # 'actual' and '' have the same effect
    T_emulated = ''
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
<link rel="stylesheet" href="/fz.css">
<head>
<title>fzlog-cgi API</title>
</head>
<body>
<h1>fzlog-cgi API</h1>

<p>
Operations:
<ul>
<li><code>action=open</code>: Open a new Log chunk.
<li><code>action=close</code>: Close Log chunk (if open).
<li><code>action=reopen</code>: Reopen Log chunk (if closed).
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

<h3>Open a new Log chunk</h3>

<p>
<code>fzlog-cgi.py?action=open&amp;node=NODE_ID[&amp;T=EMULATED_TIME][&amp;verbosity=0|1|2]</code><br>
Open a new Log chunk that belongs to Node NODE_ID after closing the most recent Log chunk if it is open.
If provided, an EMULATED_TIME can be applied.
</p>

<h3>Close Log chunk</h3>

<p>
<code>fzlog-cgi.py?action=close[&amp;T=EMULATED_TIME][&amp;verbosity=0|1|2]</code><br>
Close the most recent Log chunk if it is open. If provided, an EMULATED_TIME can be applied.
</p>

<h3>Reopen Log chunk</h3>

<p>
<code>fzlog-cgi.py?action=reopen[&amp;verbosity=0|1|2]</code><br>
Reopen the most recent Log chunk if it is closed.
</p>

</body>
</html>
'''

openpagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: open Log chunk</title>
</head>
<body>
<h3>fz: open Log chunk</h3>

'''

openpagetail_success = '''<p><b>New Log chunk opened (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

openpagetail_failure = '''<p><b>ERROR: Unable to open new Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

closepagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: close Log chunk</title>
</head>
<body>
<h3>fz: close Log chunk</h3>

'''

closepagetail_success = '''<p><b>Log chunk closed (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

closepagetail_failure = '''<p><b>ERROR: Unable to close Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

reopenpagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: reopen Log chunk</title>
</head>
<body>
<h3>fz: reopen Log chunk</h3>

'''

reopenpagetail_success = '''<p><b>Log chunk reopened (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

reopenpagetail_failure = '''<p><b>ERROR: Unable to reopen Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

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

def extra_cmd_args(T_emulated: str, verbosity = 1) -> str:
    extra = ''
    if T_emulated:
        extra += ' -t '+T_emulated
    if verbosity < 1:
        extra += ' -q'
    else:
        if verbosity > 1:
            extra += ' -V'
    return extra

def get_selected_Node(verbosity = 1) -> str:
    retcode = try_subprocess_check_output(f"./fzgraphhtml -E STDOUT -W STDOUT -L 'selected' -F node -N 1 -e -q",'selected', verbosity)
    if (retcode != 0) and (verbosity > 0):
        print(f'Attempt to get selected Node failed.')
        return ''
    if (retcode == 0):
        node = (results['selected'][0:16]).decode()
        if (verbosity > 1):
            print(f'Selected: {node}')
        if results['selected']:
            return results['selected'][0:16]
        else:
            return ''
    else:
        return ''

def open_new_Log_chunk(node: str, T_emulated: str, verbosity = 1) -> bool:
    print(openpagehead)
    if not node:
        node = get_selected_Node(verbosity)
        if not node:
            print(openpagetail_failure)
            return False
    thecmd = './fzlog -E STDOUT -W STDOUT -c ' + node + extra_cmd_args(T_emulated, verbosity)
    retcode = try_subprocess_check_output(thecmd, 'open_chunk', verbosity)
    if (retcode == 0):
        print(openpagetail_success)
    else:
        print(openpagetail_failure)
    return (retcode == 0)

def close_Log_chunk(T_emulated: str, verbosity = 1) -> bool:
    print(closepagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -C' + extra_cmd_args(T_emulated, verbosity)
    retcode = try_subprocess_check_output(thecmd, 'close_chunk', verbosity)
    if (retcode == 0):
        print(closepagetail_success)
    else:
        print(closepagetail_failure)
    return (retcode == 0)

def reopen_Log_chunk(verbosity = 1) -> bool:
    print(reopenpagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -R' + extra_cmd_args('', verbosity)
    retcode = try_subprocess_check_output(thecmd, 'reopen_chunk', verbosity)
    if (retcode == 0):
        print(reopenpagetail_success)
    else:
        print(reopenpagetail_failure)
    return (retcode == 0)

def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)
    if (action == 'open'):
        res = open_new_Log_chunk(node, T_emulated, verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
    if (action == 'close'):
        res = close_Log_chunk(T_emulated, verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
    if (action == 'reopen'):
        res = reopen_Log_chunk(verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
