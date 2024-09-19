#!/usr/bin/python3
#
# Randal A. Koene, 20210326
#
# This CGI handler provides a web interface to fzlog.

# Notes:
# 1. When 'action=open' calls 'open_new_Log_chunk()', and all goes well,
#    that function also updates the time stamp in the signal file.
#    That signal file should be read by the logautoupdate.js script that
#    is embedded in the Log page, which automatically refreshes the
#    most recent Log shown there. Whether that script is working can be
#    seen by checking the time stamps indicated at the bottom of the Log
#    page.

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

contentfilepath="/var/www/webdata/formalizer/fzlog-cgi.html"

chunkopen_signal_path='/var/www/webdata/formalizer/chunkopen.signal'

# Create instance of FieldStorage 
form = cgi.FieldStorage()

help = form.getvalue('help')
action = form.getvalue('action')
node = form.getvalue('node')
id = form.getvalue('id')
replacement_text = form.getvalue('text')
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
override_precautions = form.getvalue('override') == 'on' # Can be implemented as a checkbox.

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
<li><code>action=editentry</code>: Edit Log entry. (Leads to 'replaceentry'.)
<li><code>action=replaceentry</code>: Replace Log entry. (Comes from 'editentry'.)
<li><code>action=generic</code>: Make generic fzlog call.
<li><code>action=modify</code>: Generate Chunk Modify Page.
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

<h3>Edit Log entry</h3>

<p>
<code>fzlog-cgi.py?action=editentry&id=ENTRY_ID[&amp;verbosity=0|1|2]</code><br>
Edit the Log entry with ID specified by ENTRY_ID.
</p>

<h3>Replace Log entry</h3>

<p>
<code>fzlog-cgi.py</code> with POST arguments:
<ul>
<li><code>action=replaceentry</code>
<li><code>id=ENTRY_ID</code>
<li><code>text=REPLACEMENT_TEXT</code>
<li>(Optional) <code>node=NODE_ID</code>
<li>(Optional) <code>verbosity=0|1|2</code>
</ul>
Edit the Log entry with ID specified by ENTRY_ID. If a <code>NODE_ID</code>
is also provided then the Node associated with the Log entry is also set.
</p>

</body>
</html>
'''

openpagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: open Log chunk</title>
</head>
<body>
<h3>fz: open Log chunk</h3>

'''

openpagetail_success = '''
<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

openpagetail_failure = '''<p class="fail"><b>ERROR: Unable to open new Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

closepagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: close Log chunk</title>
</head>
<body>
<h3>fz: close Log chunk</h3>

'''

closepagetail_success = '''<p class="success"><b>Log chunk closed (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

closepagetail_failure = '''<p class="fail"><b>ERROR: Unable to close Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

reopenpagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: reopen Log chunk</title>
</head>
<body>
<h3>fz: reopen Log chunk</h3>

'''

reopenpagetail_success = '''<p class="success"><b>Log chunk reopened (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

reopenpagetail_failure = '''<p class="fail"><b>ERROR: Unable to reopen Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

editentrypagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Edit Log entry</title>
</head>
<body>
<h3>fz: Edit Log entry</h3>
'''

editentryform_start = '''<form action="/cgi-bin/fzlog-cgi.py" method="post">
<br>
<textarea rows="15" cols="100" name="text">'''

editentryform_middle = '''</textarea><br>
<p>Associated with Node: <input type="text" name="node" value="'''

editentryform_end = f'''"> [<a href="/cgi-bin/logcopytemplates.py" target="_blank">Log Copy Templates</a>] </p>
<input type="hidden" name="action" value="replaceentry">
<input type="hidden" name="id" value="{id}">
<p>
Replace entry <input type="submit" name="replaceentry" value="Text" /> or <input type="submit" name="replaceentry" value="Text and Node" />.
</p>
</form>
'''

editentrypagetail_success = '''<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

editentrypagetail_failure = '''<p class="fail"><b>ERROR: Unable to edit Log entry.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

replaceentrypagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Replace Log entry</title>
</head>
<body>
<h3>fz: Replace Log entry</h3>
'''

replaceentrypagetail_success = '''<p class="success"><b>Log entry content replaced.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

replaceentrypagetail_failure = '''<p class="fail"><b>ERROR: Unable to replace Log entry content.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

deleteentrypagehead = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Delete Log entry</title>
</head>
<body>
<h3>fz: Delete Log entry</h3>
'''

deleteentrypagetail_success = '''<p class="success"><b>Log entry deleted.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

deleteentrypagetail_failure = '''<p class="fail"><b>ERROR: Unable to delete Log entry content.</b></p>

<pre>
%s
</ptr>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

results = {
    'stdout': '',
    'stderr': '',
}

# Store a new time stamp to signal active Log chunk change.
def update_chunk_signal():
    try:
        with open(chunkopen_signal_path, 'w') as f:
            f.write(strftime("%Y%m%d%H%M%S"))
    except Exception as e:
        print('Failed to write to signal file: '+str(e), file=sys.stderr)

def try_subprocess_check_output(thecmdstring: str, resstore: str, verbosity=1, print_result=True) -> int:
    if verbosity > 1 and print_result:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if verbosity > 0:
            errstr = 'Subprocess call caused exception.'
            try:
                errstr += 'Error output: ' + cpe.output.decode()
            except:
                pass
            errstr += 'Error code  : ' + str(cpe.returncode)
            if (cpe.returncode>0):
                errstr += f'Formalizer error return code: {cpe.returncode}'
            if print_result:
                print(errstr)
            else:
                result['stderr'] += errstr
        return cpe.returncode
    else:
        if resstore:
            results[resstore] = res
        if verbosity > 1 and print_result:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

def extra_cmd_args(T_emulated:str, verbosity=1, override_precautions=False) -> str:
    extra = ''
    if T_emulated:
        extra += ' -t '+T_emulated
    if verbosity < 1:
        extra += ' -q'
    else:
        if verbosity > 1:
            extra += ' -V'
    if override_precautions:
        extra += ' -O'

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

def render_node_with_history(node: str, verbosity = 0):
    nodedatacmd = "./fzgraphhtml -q -E STDOUT -o STDOUT -n "+node
    nodehistcmd = "./fzloghtml -q -d formalizer -s randalk -o STDOUT -E STDOUT -N -c 50 -n "+node
    thisscript = os.path.realpath(__file__)
    print(f'<!--(For dev reference, this script is at {thisscript}.) -->')

    print(f'\n<!-- Primary command: {nodedatacmd} -->\n')
    print('<br>\n<table><tbody>\n')
    retcode = try_subprocess_check_output(nodedatacmd, 'node_data', verbosity)
    if (retcode != 0):
        print('<tr><td><b>Unable to display Node data.</b></td></tr>')
    else:
        print(results['node_data'].decode())
    print("</tbody></table>")

    print(f'\n<!-- Secondary command: {nodehistcmd} -->\n')
    print('<br>\n<table><tbody>\n')
    print('<br>\n\n')
    retcode = try_subprocess_check_output(nodehistcmd, 'node_history', verbosity)
    if (retcode != 0):
        print('<tr><td><b>Unable to display Node history.</b></td></tr>')
    else:
        print(results['node_history'].decode())
    print("</tbody></table>")

    print(f'<noscript>\n<br>\n<p>\n[<a href="/cgi-bin/fzlink.py?id={node}&alt=histfull">full history</a>]\n</p>\n<br>\n</noscript>')
    print(f'\n<br>\n<p>\n<button class="button button2" onclick="location.href=\'/cgi-bin/fzlink.py?id={node}&alt=histfull\';">full history</button>\n</p>\n<br>\n')

def open_new_Log_chunk(node: str, T_emulated: str, verbosity = 1, override_precautions=False) -> bool:
    print(openpagehead)
    if not node:
        node = get_selected_Node(verbosity)
        if not node:
            print(openpagetail_failure)
            return False
    thecmd = './fzlog -E STDOUT -W STDOUT -c ' + node + extra_cmd_args(T_emulated, verbosity, override_precautions)
    retcode = try_subprocess_check_output(thecmd, 'open_chunk', verbosity)
    if (retcode == 0):
        render_node_with_history(node)
        print(openpagetail_success)
        update_chunk_signal()
    else:
        print(openpagetail_failure)
    return (retcode == 0)

def close_Log_chunk(T_emulated: str, verbosity = 1, override_precautions=False) -> bool:
    print(closepagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -C' + extra_cmd_args(T_emulated, verbosity, override_precautions)
    retcode = try_subprocess_check_output(thecmd, 'close_chunk', verbosity)
    if (retcode == 0):
        print(closepagetail_success)
        update_chunk_signal()
    else:
        print(closepagetail_failure)
    return (retcode == 0)

def reopen_Log_chunk(verbosity = 1) -> bool:
    print(reopenpagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -R' + extra_cmd_args('', verbosity)
    retcode = try_subprocess_check_output(thecmd, 'reopen_chunk', verbosity)
    if (retcode == 0):
        print(reopenpagetail_success)
        update_chunk_signal()
    else:
        print(reopenpagetail_failure)
    return (retcode == 0)

# def extract_node_and_text(getlogentryoutput: str) -> tuple:
#     firstlineend = getlogentryoutput.find('\n')
#     nodestr = getlogentryoutput[0:firstlineend]
#     textstr = getlogentryoutput[firstlineend+1:]
#     node = (nodestr[1:])[:-1]
#     return (node, textstr)

def extract_node_and_text(getlogentryoutput: str) -> tuple:
    firstlineend = getlogentryoutput.find('\n')
    topline = getlogentryoutput[0:firstlineend]
    toppieces = topline.split(':')
    nodestr = toppieces[2]
    secondlineend = getlogentryoutput.find('\n', firstlineend+1)
    lastlineend = getlogentryoutput.rfind('\n', 0, -1)
    textstr = getlogentryoutput[secondlineend+1:lastlineend+1]
    node = nodestr.strip()
    return (node, textstr)

def edit_Log_entry(id: str, verbosity = 0) -> bool:
    print(editentrypagehead)
    thecmd = './fzloghtml -e '+id+' -N -o STDOUT -F raw -q'
    retcode = try_subprocess_check_output(thecmd, 'entry_text', verbosity)
    if (retcode == 0):
        entrynode, entrytext = extract_node_and_text(results['entry_text'].decode())
        print('<p>Editing Log entry: <b>'+id+'</b></p>')
        print(editentryform_start)
        print(entrytext, end='')
        print(editentryform_middle)
        print(entrynode, end='')
        print(editentryform_end)
        print(editentrypagetail_success)
    else:
        print(editentrypagetail_failure)
    return (retcode == 0)

def replace_Log_entry(id: str, text: str, verbosity = 0) -> bool:
    with open(contentfilepath, 'w') as f:
        f.write(text)
    print(replaceentrypagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -r ' + id + ' -f ' + contentfilepath + extra_cmd_args(T_emulated, verbosity)
    if node:
        thecmd += ' -n ' + node
    print('<!-- thecmd = '+thecmd+' -->')
    retcode = try_subprocess_check_output(thecmd, 'replace_log', verbosity)
    if (retcode == 0):
        print('<p>Replacement content: </p>')
        print(text)
        if node:
            print(f'<p>Replacement Node: {node}</p>')
        else:
            print('<p>Now associated with <b>the same Node</b> as the Log chunk.</p>')
        print(replaceentrypagetail_success)
    else:
        print(replaceentrypagetail_failure)
    return (retcode == 0)

def delete_Log_entry(id: str, verbosity = 0)->bool:
    print(deleteentrypagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -D '+id+extra_cmd_args(T_emulated, verbosity)
    retcode = try_subprocess_check_output(thecmd, 'delete_log', verbosity)
    if (retcode == 0):
        print(deleteentrypagetail_success)
    else:
        print(deleteentrypagetail_failure % results['delete_log'].decode())
    return (retcode == 0)

CHUNK_MODIFY_PAGE = '''Content-type:text/html

<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log - Modify Chunk %s</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>

<h1>fz: Log - Modify Chunk %s</h1>

<form action="/cgi-bin/fzlog-cgi.py" method="GET">
Change Node to:<br>
<input type="hidden" name="action" value="generic">
<input type="hidden" name="m" value="%s">
<input type="hidden" name="E" value="STDOUT">
<input type="hidden" name="W" value="STDOUT">
<input type="text" name="n" size=20>
<input type="submit" value="Modify">
</form>
<P>
<form action="/cgi-bin/fzlog-cgi.py" method="GET">
Change Open timestamp to:<br>
<input type="hidden" name="action" value="generic">
<input type="hidden" name="m" value="%s">
<input type="hidden" name="E" value="STDOUT">
<input type="hidden" name="W" value="STDOUT">
<input type="text" name="1" size=20>
<input type="submit" value="Modify">
</form>
<P>
<form action="/cgi-bin/fzlog-cgi.py" method="GET">
Change Close timestamp to:<br>
<input type="hidden" name="action" value="generic">
<input type="hidden" name="m" value="%s">
<input type="hidden" name="E" value="STDOUT">
<input type="hidden" name="W" value="STDOUT">
<input type="text" name="2" size=20>
<input type="submit" value="Modify">
</form>

<P>
<b>Please note</b>: To change open or close time of a Log chunk, there must be room in the timeline. An adjacent Log chunk may need to be modified first.

<hr>
<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

def generate_chunk_modify_page():
    print(CHUNK_MODIFY_PAGE % (id, id, id, id, id))

GENERIC_ERROR_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Log - Generic Call Failed</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>

<b>Attempted command</b>:
<p>
<pre>
%s
</pre>

<p>
<b>Stdout</b>:
<p>
<pre>
%s
</pre>

<p>
<b>Stderr</b>:
<p>
<pre>
%s
</pre>

<hr>
</body>
</html>
'''

GENERIC_SUCCESS_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Log - Generic Call Succeeded</title>
</head>
<body>
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>

<b>Command</b>:
<p>
<pre>
%s
</pre>

<p>
<b>Stdout</b>:
<p>
<pre>
%s
</pre>

<hr>
</body>
</html>
'''

def generic_fzlog_call(form):
    print("Content-type:text/html\n\n")

    # 1. Collect all variables and their values and transform them to fzgraph arguments.
    cgi_keys = list(form.keys())
    argpairs = []
    for key_str in cgi_keys:
        argpairs.append( (key_str, form.getvalue(key_str)) )
    argstr = ''
    for argpair in argpairs:
        arg, argval = argpair
        if arg != 'action':
            if argval=='true':
                argstr += ' -%s' % arg
            else:
                argstr += " -%s '%s'" % (arg, argval)

    # 2. Carry out an fzlog call.
    thecmd = './fzlog '+argstr
    retcode = try_subprocess_check_output(thecmd, 'stdout', print_result=False)
    try:
        decoded_result = result['stdout'].decode()
    except:
        decoded_result = str(result['stdout'])
    if (retcode != 0):
        print(GENERIC_ERROR_PAGE % (thecmd, decoded_result, resultdict['stderr']))
        sys.exit(0)

    # 3. Present the result.
    print(GENERIC_SUCCESS_PAGE % (thecmd, decoded_result))

# Note that insert_Log_entry is handled through logentry-form.py.

def show_interface_options():
    print("Content-type:text/html\n\n")
    print(interface_options_help)


if __name__ == '__main__':
    if help:
        show_interface_options()
        sys.exit(0)

    if not action:
        show_interface_options()
        sys.exit(0)

    if (action == 'generic'):
        generic_fzlog_call(form)
        sys.exit(0)

    if (action == 'open'):
        res = open_new_Log_chunk(node, T_emulated, verbosity, override_precautions)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
    if (action == 'close'):
        res = close_Log_chunk(T_emulated, verbosity, override_precautions)
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
    if (action == 'editentry'):
        if not verbositystr:
            verbosity = 0
        res = edit_Log_entry(id, verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
    if (action == 'replaceentry'):
        if not verbositystr:
            verbosity = 0
        res = replace_Log_entry(id, replacement_text, verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
    if (action == 'delete'):
        if not verbositystr:
            verbosity = 0
        res = delete_Log_entry(id, verbosity)
        if res:
            sys.exit(0)
        else:
            sys.exit(1)
    if (action == 'modify'):
        generate_chunk_modify_page()
        sys.exit(0)

    show_interface_options()
    sys.exit(0)
