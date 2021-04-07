#!/usr/bin/python3
#
# Randal A. Koene, 20201125
#
# This CGI handler provides a web interface to fztask.py.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
sys.stderr = sys.stdout
from time import strftime
import datetime
import traceback
from io import StringIO
from traceback import print_exc
import subprocess

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

non_local = form.getvalue('n')

cmdoptions = ""

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
            return node
        else:
            return ''
    else:
        return ''

def get_selected_Node_HTML(verbosity = 1) -> str:
    retcode = try_subprocess_check_output(f"./fzgraphhtml -E STDOUT -W STDOUT -L 'selected' -F html -N 1 -e -q",'selected_html', verbosity)
    if (retcode != 0) and (verbosity > 0):
        print(f'Attempt to get selected Node data failed.')
        return ''
    if (retcode == 0):
        html = (results['selected_html']).decode()
        if results['selected_html']:
            return html
        else:
            return ''
    else:
        return ''

def get_most_recent_Log_chunk_info(verbosity = 1) -> list:
    retcode = try_subprocess_check_output("./fzloghtml -E STDOUT -W STDOUT -o STDOUT -q -R -F raw", 'recent_log', 0)
    if (retcode != 0) and (verbosity > 0):
        print(f'Attempt to get most recent Log chunk data failed.')
        return []
    if (retcode == 0):
        return results['recent_log'].decode().split(' ')
    else:
        return []

if (non_local == 'on'):
    chunkdatavec = get_most_recent_Log_chunk_info()
    t_open = chunkdatavec[0]
    open_or_closed = chunkdatavec[1]
    num_entries = chunkdatavec[3]
    t_open_epoch = int(datetime.datetime.strptime(t_open, '%Y%m%d%H%M').timestamp())
    # Get data from fields
    T_emulated = form.getvalue('T')
    if (T_emulated == '') or (T_emulated == 'actual'):
        fzlog_T = ''
        t_epoch = int(datetime.datetime.now().timestamp())
        diff_minutes = int((t_epoch - t_open_epoch) / 60)
        diff_min_str = f'at least {diff_minutes}'
    else:
        fzlog_T = '&T='+T_emulated
        t_epoch = int(datetime.datetime.strptime(T_emulated, '%Y%m%d%H%M').timestamp())
        diff_minutes = int((t_epoch - t_open_epoch) / 60)
        diff_min_str = str(diff_minutes)

fztask_webpage_head = f"""Content-type:text/html

<html>
<head>
<meta charset="utf-8" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Task</title>
<style>
td.stateinfo {{
    background-color: #b8bfff;
}}
</style>
</head>
<body>
<h1>fz: Task</h1>

<p>T = {T_emulated} ({diff_min_str} minutes since Log chunk opening)</p>

<table><tbody>
<tr>
<td>1. [<a href="/formalizer/logentry-form_fullpage.template.html" target="_blank">Make Log entry</a>]</td>
<td class="stateinfo">
"""

fztask_webpage_middle_1 = f"""</td></tr>
<tr>
<td>2. [<a href="/cgi-bin/fzlog-cgi.py?action=close{fzlog_T}" target="_blank">Close Log chunk</a>]</td>
<td class="stateinfo">
"""

fztask_webpage_middle_2 = f"""</td></tr>
<tr>
<td>3. [<a href="cgi-bin/fzgraphhtml-cgi.py" target="_blank">Update Schedule</a>]</td>
<td class="stateinfo"></td>
</tr>
<tr>
<td>4. [<a href="/select.html" target="_blank">Select Node for Next Log chunk</a>]</td>
<td class="stateinfo"><table><tbody>
"""

fztask_webpage_tail = f"""</tbody></table></td></tr>
<tr>
<td>5. [<a href="/cgi-bin/fzlog-cgi.py?action=open{fzlog_T}">Open New Log chunk</a>]</td>
<td class="stateinfo"></td>
</tr>
</tbody></table>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
"""

if (non_local == 'on'):
    print(fztask_webpage_head)
    print(f'Log entries in this chunk: {num_entries}')
    print(fztask_webpage_middle_1)
    if (open_or_closed == 'OPEN'):
        print(f'<b>Opened</b> at {t_open}.')
    else:
        print(f'<b>Closed.</b>')
    print(fztask_webpage_middle_2)
    print(get_selected_Node_HTML())
    print(fztask_webpage_tail)
    sys.exit(0)


### Below is executed only when run locally (e.g. in w3m)

#thecmd = "./fztask"+cmdoptions
#nohup env -u QUERY_STRING urxvt -rv -title "dil2al daemon" -geometry +$xhloc+$xvloc -fade 30 -e dil2al -T$emulatedtime -S &
thecmd = "nohup urxvt -rv -title 'fztask' -fn 'xft:Ubuntu Mono:pixelsize=14' -bd red -e fztask"+cmdoptions+" &"

# Let's delete QUERY_STRING from the environment now so that dil2al does not accidentally think it was
# called from a form if dil2al is called for synchronization back to Formalizer 1.x.
del os.environ['QUERY_STRING']


print("Content-type:text/html\n\n")

print("<html>")
print("<head>")
print("<title>fztask-cgi.py</title>")
print("</head>")
print("<body>")
print(f'\n<!-- Primary command: {thecmd} -->\n')
try:
    p = subprocess.Popen(thecmd,shell=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,close_fds=True, universal_newlines=True)
    (child_stdin,child_stdout) = (p.stdin, p.stdout)
    child_stdin.close()
    result = child_stdout.read()
    child_stdout.close()
    print(result)
    print('fztask call completed.')

except Exception as ex:                
    print(ex)
    f = StringIO()
    print_exc(file=f)
    a = f.getvalue().splitlines()
    for line in a:
        print(line)

print('</body></html>')
sys.exit(0)
