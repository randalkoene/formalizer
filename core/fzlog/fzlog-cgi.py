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
from time import strftime, time
from datetime import datetime, timedelta
import re
import json
import traceback
import html
from io import StringIO
from traceback import print_exc
import subprocess

print("Content-type:text/html\n")

contentfilepath="/var/www/webdata/formalizer/fzlog-cgi.html"

chunkopen_signal_path='/var/www/webdata/formalizer/chunkopen.signal'

# Create instance of FieldStorage 
form = cgi.FieldStorage()

help = form.getvalue('help')
action = form.getvalue('action')
node = form.getvalue('node')
id = form.getvalue('id')
updatecheckbox = form.getvalue('updatecheckbox')
if updatecheckbox:
    action = 'updatecheckbox'
replacement_text = form.getvalue('text', '') # empty text is allowed, to empty an entry before deleting it
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
override_zerochunk_precaution = form.getvalue('overridezero') == 'on'

# The following should only show information that is safe to provide
# to those who have permission to connect to this CGI handler.
interface_options_help = '''
<html>
<head>
<title>fzlog-cgi API</title>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

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
<li><code>action=modifytimes</code>: Change start/end times of a chunk and its neighbours, in the given order (from the time strip on the Chunk Modify Page): <code>&amp;s1=CHUNK_ID:1|2:TIME[&amp;s2=...]</code> (1 = open time, 2 = close time; at most 4 steps), <code>&amp;final=CHUNK_ID</code> (the chunk's ID afterwards).
<li><code>action=delete</code>: Delete Log entry <code>id</code>. The entry must be empty (see 'editentry').
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

openpagehead = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/tooltip.css">
<title>fz: open Log chunk</title>
</head>
<body>
<h3>fz: open Log chunk</h3>

'''

openpagetail_success = '''
<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

openpagetail_failure = '''<p class="fail"><b>ERROR: Unable to open new Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

closepagehead = '''<html>
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

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

closepagetail_failure = '''<p class="fail"><b>ERROR: Unable to close Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

reopenpagehead = '''<html>
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

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

reopenpagetail_failure = '''<p class="fail"><b>ERROR: Unable to reopen Log chunk (<a href="/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END">link</a>).</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

editentrypagehead = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/copiedalert.css">
<link rel="stylesheet" href="/htmltemplatestocopy.css">
<title>fz: Edit Log entry</title>
</head>
<body>
<h3>fz: Edit Log entry</h3>
'''

editentryform_start = '''<form action="/cgi-bin/fzlog-cgi.py" method="post">
<br>
<textarea id="entrytext" rows="15" cols="100" name="text">'''

editentryform_middle = '''</textarea><br>
<p>Associated with Node: <input type="text" name="node" value="'''

editentryform_end = f'''"> [<a href="/cgi-bin/logcopytemplates.py" target="_blank">Log Copy Templates</a>] </p>
<input type="hidden" name="action" value="replaceentry">
<input type="hidden" name="id" value="{id}">
<p>
Replace entry <input type="submit" name="replaceentry" value="Text" /> or <input type="submit" name="replaceentry" value="Text and Node" />.
</p>
</form>
<button onclick="markdown2HTML()">Markdown2HTML</button>
'''

editentrypagetail_success = '''<div id="after_cptplt"></div>
<div id="templates_html" style="display:none;">
<button class="button2" onclick="copyInnerHTMLToClipboard('chkbx_tpl');">copy</button> <span id="chkbx_tpl"><input type="checkbox" ></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnkblank_tpl');">copy</button> <span id="lnkblank_tpl"><a href="URL" class="docpopupfunc" target="_blank">target</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnklocaldoc_tpl');">copy</button> <span id="lnklocaldoc_tpl"><a href="@FZSERVER@/doc/" target="_blank">~/doc/path</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnklocalwww_tpl');">copy</button> <span id="lnklocalwww_tpl"><a href="@FZSERVER@/" target="_blank">/var/www/html/path</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('img_tpl');">copy</button> <span id="img_tpl"><img class="hoverdelayfunc" src="/favicon.png" width=32px height=32px></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('acttype_tpl');">copy</button> <span id="acttype_tpl">@ACTTYPE=A100@</span>
<p></p>
</div>
<script type="text/javascript" src="/htmltemplatestocopy.js"></script>
<script>const htmltemplates = new htmlTemplates('after_cptplt', 'cptplt', 'templates', 'templates_html');</script>
<script type="text/javascript" src="/copiedalert.js"></script>
<script type="text/javascript" src="/copyinnerhtml.js"></script>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
<script type="text/javascript" src="/delayedpopup.js"></script>
<script>
pastePopupLink('docpopupfunc', 'entrytext');
// Sends content through md2html.
function markdown2HTML() {
    const content_ref = document.getElementById("entrytext");
    const content = content_ref.value;
    const cgiUrl = '/cgi-bin/md2html-cgi.py';
    const formData = new URLSearchParams();
    formData.append('markdown_text', content);
    fetch(cgiUrl, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: formData,
    }).then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.text(); // or .json() if your script returns JSON
    }).then(data => {
        content_ref.value = data;
        // console.log('Server response:', data);
        // console.log('Reloading page...');
        // window.location.reload();
    }).catch(error => {
        console.error('There was a problem with the fetch operation:', error);
        alert('An error occurred. Please check the console for details.');
    });
}
</script>
</body>
</html>
'''

editentrypagetail_failure = '''<p class="fail"><b>ERROR: Unable to edit Log entry.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

replaceentrypagehead = '''<html>
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

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

replaceentrypagetail_failure = '''<p class="fail"><b>ERROR: Unable to replace Log entry content.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

deleteentrypagehead = '''<html>
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

<script type="module" src="/fzuistate.js"></script>
</body>
</html>
'''

deleteentrypagetail_failure = '''<p class="fail"><b>ERROR: Unable to delete Log entry content.</b></p>

<pre>
%s
</ptr>

<hr>
[<a href="/index.html">fz: Top</a>]

<script type="module" src="/fzuistate.js"></script>
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
                results['stderr'] += errstr
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
    print(f'\n<br>\n<p>\n<button class="button button2" onclick="location.href=\'/cgi-bin/fzlink.py?id={node}&alt=histfull#END\';">full history</button>\n</p>\n<br>\n')

PRECAUTIONFAIL = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Zero Chunk Length Precaution Failed</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Zero Chunk Length Precaution Failed</h3>

<p class="fail"><b>ERROR: Failed to carry out zero chunk length precaution check.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

PRECAUTIONPAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Zero Chunk Length Precaution</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Zero Chunk Length Precaution</h3>

<p><b>Warning: Log chunk has zero-minutes length. Confirm to proceed:</b> <button class="button button1" onclick="window.open('%s', '_self');">Confirm</button></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

NODEPRECAUTIONPAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Same Node Precaution</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Same Node Precaution</h3>

<p><b>Warning: Log chunk has same Node as requested. Confirm to proceed:</b> <button class="button button1" onclick="window.open('%s', '_self');">Confirm</button></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

# When a new Log chunk is opened and the most recent chunk is already CLOSED, a new chunk
# that would open up to this many minutes after that close time is opened AT the close time,
# so that no gap appears in the Log (as fztask on the command line does). A larger gap is
# probably a deliberate break, so then a confirmation page offers both choices.
# (When the most recent chunk is still open, fzlog -c closes it and opens the new chunk at
# the same time, so there is never a gap and this check does nothing.)
ATTACH_TO_CLOSE_LIMIT_MINUTES = 5

GAPPRECAUTIONPAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log Gap Precaution</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Log Gap Precaution</h3>

<p>The most recent Log chunk was closed at <b>%s</b>, %s minutes before the new chunk would open (%s).</p>

<p><button class="button button1" onclick="window.open('%s', '_self');">Attach: open the new chunk at %s</button> (no gap)</p>
<p><button class="button button2" onclick="window.open('%s', '_self');">Leave the gap: open the new chunk at %s</button></p>

<p>To do neither, close this page.</p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

def attach_to_close_decision(recent_chunk_raw:str, t_open:str) -> tuple:
    '''
    Decide when to open a new Log chunk, given the most recent chunk (one line of
    `fzloghtml -R -F raw`: chunk_id status node_id minor_id entry_id t_close) and
    the intended open time t_open (YYYYmmddHHMM).

    Returns (decision, t_close, gap_minutes), where decision is:
      'as_is'  : open at t_open (the most recent chunk is open, could not be read,
                 or there is no gap);
      'attach' : open at t_close instead (closed 1 to ATTACH_TO_CLOSE_LIMIT_MINUTES
                 minutes before t_open);
      'ask'    : closed longer before t_open, let the user choose.
    '''
    fields = recent_chunk_raw.split('\n')[0].split(' ')
    if (len(fields) < 6) or (fields[1] != 'CLOSED'):
        return ('as_is', '', 0)
    t_close = fields[5]
    try:
        d_close = datetime.strptime(t_close, "%Y%m%d%H%M")
        d_open = datetime.strptime(t_open, "%Y%m%d%H%M")
    except ValueError:
        return ('as_is', '', 0)
    if t_close <= fields[0]:
        return ('as_is', '', 0) # zero-length chunk: a new chunk cannot have the same ID (open time)
    gap_minutes = int((d_open - d_close).total_seconds() // 60)
    if gap_minutes <= 0:
        return ('as_is', t_close, gap_minutes) # fzlog itself rejects an open time before the close time
    if gap_minutes <= ATTACH_TO_CLOSE_LIMIT_MINUTES:
        return ('attach', t_close, gap_minutes)
    return ('ask', t_close, gap_minutes)

def readable_stamp(stamp:str) -> str:
    try:
        return datetime.strptime(stamp, "%Y%m%d%H%M").strftime("%Y-%m-%d %H:%M")
    except ValueError:
        return stamp

def gap_to_closed_chunk_check(node:str, T_emulated:str, verbosity:int, override_precautions:bool) -> str:
    '''
    See ATTACH_TO_CLOSE_LIMIT_MINUTES. Returns the emulated time to open the new chunk with
    (unchanged, or the close time of the most recent chunk), or shows the confirmation page
    and exits.
    '''
    if form.getvalue('gapok') == 'on': # the user chose to leave the gap
        return T_emulated
    if 'recent_chunk_raw' not in results: # not yet read by zero_time_Log_chunk_precaution()
        retcode = try_subprocess_check_output('./fzloghtml -E STDOUT -o STDOUT -q -R -F raw', 'recent_chunk_raw', verbosity, print_result=False)
        if retcode != 0:
            return T_emulated # keep the previous behavior if the most recent chunk cannot be read
    t_open = T_emulated if T_emulated else datetime.now().strftime("%Y%m%d%H%M")
    decision, t_close, gap_minutes = attach_to_close_decision(results['recent_chunk_raw'].decode(), t_open)
    if decision == 'attach':
        return t_close
    if decision == 'ask':
        attachcall = make_rerun_arguments(True, node, t_close, verbosity, override_precautions)
        gapcall = make_rerun_arguments(True, node, T_emulated, verbosity, override_precautions) + '&gapok=on'
        open_at_str = readable_stamp(T_emulated) if T_emulated else 'the time you click'
        print(GAPPRECAUTIONPAGE % (readable_stamp(t_close), str(gap_minutes), readable_stamp(t_open),
                                   attachcall, readable_stamp(t_close), gapcall, open_at_str))
        sys.exit(0)
    return T_emulated

def make_rerun_arguments(opennew:bool, node:str, T_emulated:str, verbosity:int, override_precautions:bool)->str:
    cgiarguments = ''
    if opennew:
        if node:
            cgiarguments += '&node='+node
    if T_emulated:
        cgiarguments += '&T='+T_emulated
    cgiarguments += '&verbosity='+str(verbosity)
    if override_precautions:
        cgiarguments += '&override=on'
    # The rerun is the confirmation, so it must skip the precaution checks
    # (zero-length chunk, same Node) that produced the confirmation page.
    cgiarguments += '&overridezero=on'
    if opennew:
        confirmedcall = '/cgi-bin/fzlog-cgi.py?action=open'+cgiarguments
    else:
        confirmedcall = '/cgi-bin/fzlog-cgi.py?action=close'+cgiarguments
    return confirmedcall

def zero_time_Log_chunk_precaution(opennew:bool, T_emulated:str, verbosity:int, override_precautions:bool, node:str):
    # If overriden then just carry on (return)
    if override_zerochunk_precaution:
        return

    # Check the number of minutes in the most recent Log chunk (if open)
    thecmd = './fzloghtml -E STDOUT -o STDOUT -q -R -F raw'
    retcode = try_subprocess_check_output(thecmd, 'recent_chunk_raw', verbosity)
    if (retcode != 0):
        print(PRECAUTIONFAIL)
        sys.exit(0)
    lines = results['recent_chunk_raw'].decode().split('\n')
    components = lines[0].split(' ')
    startstamp = components[0]
    nodestr = components[2]
    nodelen = len(nodestr)
    try:
        start_as_int = int(startstamp)
    except:
        startstamp = ''
    if startstamp == '' or nodelen != 16:
        print(PRECAUTIONFAIL)
        sys.exit(0)
    d_start = datetime.strptime(startstamp, "%Y%m%d%H%M")
    d_now = datetime.now()
    minutes = int((d_now - d_start).seconds/60)

    if minutes <= 0:
        # If zero then create a confirmation page that reruns fzlog-cgi.py with an override if confirmed
        confirmedcall = make_rerun_arguments(opennew, node, T_emulated, verbosity, override_precautions)
        print(PRECAUTIONPAGE % confirmedcall)
        sys.exit(0)

    if nodestr == node:
        # If the same Node then create a confirmation page that reruns fzlog-cgi.py with an override if confirmed
        confirmedcall = make_rerun_arguments(opennew, node, T_emulated, verbosity, override_precautions)
        print(NODEPRECAUTIONPAGE % confirmedcall)
        sys.exit(0)

# DEBUG_LONG_DELAY_INSTANCES_FILE='/var/www/webdata/formalizer/fzlog.debug'
# def DEBUG_LONG_DELAY_INSTANCES(mark:str):
#     t = int(time()*10)
#     if mark == 'mark-0':
#         try:
#             with open(DEBUG_LONG_DELAY_INSTANCES_FILE, 'w') as f:
#                 f.write(str(t)+'\n')
#         except:
#             pass
#     else:
#         try:
#             with open(DEBUG_LONG_DELAY_INSTANCES_FILE, 'a') as f:
#                 f.write(str(t)+'\n')
#         except:
#             pass

# This should open a new Log chunk, unless the current Log chunk already belongs to the same Node.
def open_new_Log_chunk(node: str, T_emulated: str, verbosity = 1, override_precautions=False) -> bool:
    #DEBUG_LONG_DELAY_INSTANCES('mark-0')
    zero_time_Log_chunk_precaution(opennew=True, T_emulated=T_emulated, verbosity=verbosity, override_precautions=override_precautions, node=node)
    T_emulated = gap_to_closed_chunk_check(node, T_emulated, verbosity, override_precautions)
    #DEBUG_LONG_DELAY_INSTANCES('mark-1')
    print(openpagehead)
    if not node:
        #DEBUG_LONG_DELAY_INSTANCES('mark-2')
        node = get_selected_Node(verbosity)
        if not node:
            print(openpagetail_failure)
            return False
    #DEBUG_LONG_DELAY_INSTANCES('mark-3')
    thecmd = './fzlog -E STDOUT -W STDOUT -c ' + node + extra_cmd_args(T_emulated, verbosity, override_precautions)
    retcode = try_subprocess_check_output(thecmd, 'open_chunk', verbosity)
    #DEBUG_LONG_DELAY_INSTANCES('mark-4')
    if (retcode == 0):
        render_node_with_history(node)
        print(openpagetail_success)
        update_chunk_signal()
    else:
        print(openpagetail_failure)
    #DEBUG_LONG_DELAY_INSTANCES('mark-5')
    return (retcode == 0)

def close_Log_chunk(T_emulated: str, verbosity = 1, override_precautions=False) -> bool:
    zero_time_Log_chunk_precaution(opennew=False, T_emulated=T_emulated, verbosity=verbosity, override_precautions=override_precautions, node=None)
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

def find_next_checkbox(entrytext:str, pos:int)->tuple:
    tag = '<input type="checkbox">'
    tagpos = entrytext.find(tag, pos)
    if tagpos < 0:
        return None, -1, -1
    eolpos = entrytext.find('\n', tagpos + len(tag))
    if eolpos < 0:
        eolpos = len(entrytext)
    checkboxtext = entrytext[tagpos+len(tag):eolpos]
    return checkboxtext, eolpos, tagpos+len(tag)

def collect_checkboxes(entrytext:str)->list:
    checkboxes = []
    pos = 0
    while True:
        next_checkbox, pos, text_pos = find_next_checkbox(entrytext, pos)
        if not next_checkbox:
            break
        checkboxes.append( [ next_checkbox, text_pos ] )
    return checkboxes

def make_checkboxes_list(checkboxes:list, id:str)->str:
    checkboxeslist_html = ''
    for i in range(len(checkboxes)):
        node_cell = '<td><input name="checkboxnode_%s_%s" type="text"></td>' % (str(id), str(checkboxes[i][1]))
        use_selected_cell = '''<td><button type="button" onclick="window.open('/cgi-bin/fzlog-cgi.py?updatecheckbox=use_selected&text_pos=%s&id=%s','_blank');">Use Selected</button></td>''' % (str(checkboxes[i][1]), str(id))
        text_cell = '<td>'+str(checkboxes[i][0])+'</td>'
        checkboxeslist_html += '<tr>'+node_cell+use_selected_cell+text_cell+'</tr>\n'
    return checkboxeslist_html

def edit_Log_entry(id: str, verbosity = 0) -> bool:
    print(editentrypagehead)
    thecmd = './fzloghtml -e '+id+' -N -o STDOUT -F raw -q'
    retcode = try_subprocess_check_output(thecmd, 'entry_text', verbosity)
    if (retcode == 0):
        entrynode, entrytext = extract_node_and_text(results['entry_text'].decode())
        checkboxes = collect_checkboxes(entrytext)
        checkboxeslist_html = make_checkboxes_list(checkboxes, id)
        print('<p>Editing Log entry: <b>'+id+'</b></p>')
        print(editentryform_start)
        # Escape '&', '<' and '>' so that the textarea shows the stored text exactly,
        # including entities like &lt; (a browser decodes entities inside a textarea).
        print(html.escape(entrytext, quote=False), end='')
        print(editentryform_middle)
        print(entrynode, end='')
        print(editentryform_end)
        print('<hr><form action="/cgi-bin/fzlog-cgi.py" method="post">')
        print('<table><tbody>\n'+checkboxeslist_html+'</tbody></table>')
        print('Update checkboxes <input type="submit" name="updatecheckbox" value="use_input" />.')
        print('</form>')
        print(editentrypagetail_success)
    else:
        print(editentrypagetail_failure)
    return (retcode == 0)

def replace_Log_entry(id: str, text: str, verbosity = 0, entry_node = None) -> bool:
    # entry_node, when given, is the Node the entry belongs to ('' for the chunk's Node).
    # Otherwise the Node comes from the form.
    set_node = node if entry_node is None else entry_node
    with open(contentfilepath, 'w') as f:
        f.write(text)
    print(replaceentrypagehead)
    thecmd = './fzlog -E STDOUT -W STDOUT -r ' + id + ' -f ' + contentfilepath + extra_cmd_args(T_emulated, verbosity)
    if set_node:
        thecmd += ' -n ' + set_node
    print('<!-- thecmd = '+thecmd+' -->')
    retcode = try_subprocess_check_output(thecmd, 'replace_log', verbosity)
    if (retcode == 0):
        print('<p>Replacement content: </p>')
        print(text)
        if set_node:
            print(f'<p>Replacement Node: {set_node}</p>')
        else:
            print('<p>Now associated with <b>the same Node</b> as the Log chunk.</p>')
        print(replaceentrypagetail_success)
    else:
        print(replaceentrypagetail_failure)
    return (retcode == 0)

def find_update_checkbox_id()->tuple:
    for field_name in form.keys():
        field_pos = field_name.find('checkboxnode_')
        if field_pos >= 0:
            field_value = form.getvalue(field_name)
            if field_value:
                id_end = field_name.find('_', field_pos+len('checkboxnode_'))
                id = field_name[field_pos+len('checkboxnode_'):id_end]
                text_pos = field_name[id_end+1:]
                return id, text_pos, field_value
    return None, None

def update_checkbox_entrytext(entrytext:str, text_pos:int, set_node:str)->str:
    if text_pos<1:
        return entrytext
    update_entrytext = entrytext[:text_pos-1]
    update_entrytext += 'checked> [ '
    update_entrytext += set_node
    update_entrytext += ' ]'
    update_entrytext += entrytext[text_pos:]
    return update_entrytext

def update_checkbox(id:str, updatecheckbox:str):
    text_pos = form.getvalue('text_pos')
    if updatecheckbox == 'use_selected':
        set_node = get_selected_Node(verbosity=verbosity)
        if set_node == '':
            print(replaceentrypagehead)
            print("No Node in 'selected' NNL for checkbox update.")
            print(replaceentrypagetail_failure)
            return
    if not id:
        id, text_pos, set_node = find_update_checkbox_id()
    if not id:
        print(replaceentrypagehead)
        print('No ID for checkbox update.')
        print(replaceentrypagetail_failure)
        return
    thecmd = './fzloghtml -e '+id+' -N -o STDOUT -F raw -q'
    retcode = try_subprocess_check_output(thecmd, 'entry_text', verbosity)
    if (retcode == 0):
        entrynode, entrytext = extract_node_and_text(results['entry_text'].decode())
        entrytext = update_checkbox_entrytext(entrytext, int(text_pos), set_node)
        #print(replaceentrypagehead)
        #print('Replace Log entry %s at position %s using Node %s.\n<p>' % (str(id), str(text_pos), str(set_node)))
        #print(entrytext)
        #print(replaceentrypagetail_success)
        # Keep the Node the entry belongs to. Without -n, fzlog -r would give it the chunk's Node.
        return replace_Log_entry(id=str(id), text=entrytext, verbosity=verbosity, entry_node=entrynode)
    else:
        print(replaceentrypagehead)
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

CHUNK_MODIFY_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/chunkstrip.css">
<title>fz: Log - Modify Chunk %s</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h1>fz: Log - Modify Chunk %s</h1>

<button class="button button2" onclick="copyInputValueToClipboard('chunk_id');">copy Chunk ID</button>
<input id="chunk_id" type="hidden" value="%s">
<script type="text/javascript" src="/copyinputvalue.js"></script>
<P>
Visit Log centered around <button class="button button1" onclick="window.open('/cgi-bin/fzloghtml-cgi.py?around=%s#%s','_blank');">%s</button>
<P>
@@CHUNKSTRIP@@

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
</body>
</html>
'''

def is_stamp(s) -> bool:
    '''True for a 12-digit Formalizer time stamp (YYYYmmddHHMM), e.g. a Log chunk ID.'''
    return isinstance(s, str) and (len(s) == 12) and s.isdigit()

def stamp_add_minutes(stamp:str, minutes:int) -> str:
    return (datetime.strptime(stamp, "%Y%m%d%H%M") + timedelta(minutes=minutes)).strftime("%Y%m%d%H%M")

def stamp_diff_minutes(a:str, b:str) -> int:
    return int((datetime.strptime(a, "%Y%m%d%H%M") - datetime.strptime(b, "%Y%m%d%H%M")).total_seconds() // 60)

def get_chunk_neighbourhood(chunk_id:str):
    '''
    The Log chunk with chunk_id and the chunks just before and after it, for the time
    strip (chunkstrip.js): {'now', 'prev', 'cur', 'next'}, each chunk {'open', 'close',
    'node', 'desc'} with close None while open. Returns None if this cannot be determined.

    Two fzloghtml calls: the chunk and the one after it, and the chunk and the one before
    it. Times are database times: fzloghtml shifts t_chunkopen/t_chunkclose by its
    timezone_offset_hours setting, so that shift is measured on t_chunkopen against the
    chunk ID and undone for t_chunkclose.
    '''
    template = 'STR:{{ chunk_id }} {{ t_chunkopen }} {{ t_chunkclose }} {{ node_id }}\n'
    pattern = re.compile(r'^([0-9]{12}) ([0-9]{12}) ([0-9]{12}|OPEN) ([0-9]{14}\.[0-9]+)$')
    chunks = {}
    for interval in (['-1', chunk_id, '-c', '2'], ['-2', chunk_id, '-c', '2', '-r']):
        try:
            res = subprocess.run(['./fzloghtml'] + interval + ['-T', template, '-o', 'STDOUT', '-N', '-q', '-E', 'STDOUT', '-W', 'STDOUT'],
                                 capture_output=True, timeout=30)
        except Exception:
            return None
        if res.returncode != 0:
            return None
        for line in res.stdout.decode(errors='replace').split('\n'):
            m = pattern.match(line.strip())
            if not m:
                continue
            cid, topen_shown, tclose_shown, nid = m.groups()
            shift = stamp_diff_minutes(topen_shown, cid)
            close = None if (tclose_shown == 'OPEN') else stamp_add_minutes(tclose_shown, -shift)
            chunks[cid] = {'open': cid, 'close': close, 'node': nid, 'desc': ''}
    if chunk_id not in chunks:
        return None
    ids = sorted(chunks)
    i = ids.index(chunk_id)
    return {
        'now': datetime.now().strftime("%Y%m%d%H%M"),
        'prev': chunks[ids[i-1]] if (i > 0) else None,
        'cur': chunks[chunk_id],
        'next': chunks[ids[i+1]] if (i+1 < len(ids)) else None,
    }

CHUNKSTRIP_SECTION = '''<h3>Drag the start or end of this chunk</h3>
<div id="chunkstrip"></div>
<script type="text/javascript" src="/chunkstrip.js"></script>
<script>
ChunkStrip(document.getElementById('chunkstrip'), @@DATA@@, (steps, d) => {
    let url = '/cgi-bin/fzlog-cgi.py?action=modifytimes';
    steps.forEach((s, i) => { url += '&s' + (i + 1) + '=' + s.id + ':' + s.flag + ':' + s.to; });
    url += '&final=' + ChunkTimes.fmtStamp(d.open);
    window.location.href = url;
});
</script>
<hr>
'''

def generate_chunk_modify_page():
    page = CHUNK_MODIFY_PAGE % (id, id, id, id, id, id, id, id, id)
    data = get_chunk_neighbourhood(id) if is_stamp(id) else None
    if data:
        strip = CHUNKSTRIP_SECTION.replace('@@DATA@@', json.dumps(data))
    else:
        strip = '<p>(The time strip is not available for this chunk: its times, or those of its neighbours, could not be read.)</p>\n'
    print(page.replace('@@CHUNKSTRIP@@', strip))

REFRESH_HISTORIES_BUTTON = '''<p>Node histories are rebuilt separately, so that changed chunks show on their Nodes' history pages again:
<button class="button button1" onclick="window.open('/cgi-bin/fzquerypq-cgi.py','_blank');">refresh Node histories</button></p>
'''

MODIFY_TIMES_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log - Chunk Times %s</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Log - Chunk Times %s</h3>
%s
<hr>
</body>
</html>
'''

def modify_chunk_times(form):
    '''
    Carries out the steps planned by the time strip (chunkstrip.js), in order, one fzlog
    call each: s1, s2, ... = CHUNK_ID:1|2:TIME (1 = open time, 2 = close time). fzlog checks
    its rules for every step. Stops at the first step that fails.
    '''
    steps = []
    for i in range(1, 5):
        v = form.getvalue(f's{i}')
        if not v:
            break
        parts = str(v).split(':')
        if (len(parts) != 3) or (not is_stamp(parts[0])) or (parts[1] not in ('1', '2')) or (not is_stamp(parts[2])):
            print(MODIFY_TIMES_PAGE % ('Not Changed', 'Not Changed', f'<p class="fail"><b>Invalid step {i}: {html.escape(str(v))}</b>. Nothing was changed.</p>'))
            return False
        steps.append(parts)
    final = form.getvalue('final')
    if (not steps) or (not is_stamp(final)):
        print(MODIFY_TIMES_PAGE % ('Not Changed', 'Not Changed', '<p class="fail"><b>Missing or invalid steps.</b> Nothing was changed.</p>'))
        return False

    body = '<ol>\n'
    all_ok = True
    open_changed = False
    for n, (cid, flag, to) in enumerate(steps, 1):
        what = ('start (open time, the chunk ID)' if flag == '1' else 'end (close time)')
        cmd = ['./fzlog', '-E', 'STDOUT', '-W', 'STDOUT', '-m', cid, '-' + flag, to]
        try:
            res = subprocess.run(cmd, capture_output=True, timeout=60)
            output = res.stdout.decode(errors='replace') + res.stderr.decode(errors='replace')
            ok = (res.returncode == 0)
        except Exception as e:
            output = str(e)
            ok = False
        body += (f'<li>Chunk {cid}: {what} to {to}: <b>{"done" if ok else "FAILED"}</b>'
                 f'<br><code>fzlog -m {cid} -{flag} {to}</code><pre>{html.escape(output.strip())}</pre></li>\n')
        if not ok:
            all_ok = False
            if n < len(steps):
                body += '<li>(The remaining steps were not carried out.)</li>\n'
            break
        if flag == '1':
            open_changed = True
    body += '</ol>\n'
    if all_ok:
        body = '<p>All steps were carried out.</p>\n' + body
        show_id = final
    else:
        body = ('<p class="fail"><b>A step failed.</b> The steps before it were carried out, and each of them left a valid Log, '
                'but the Log may now have a gap or a boundary that moved on one side only. Check the Log, '
                'and use the time strip or the forms on the modify page to finish or undo the change.</p>\n' + body)
        show_id = None
    links = ''
    if show_id:
        links += (f'<p>[<a href="/cgi-bin/fzloghtml-cgi.py?around={show_id}#{show_id}">Log around this chunk</a>] '
                  f'[<a href="/cgi-bin/fzlog-cgi.py?action=modify&id={show_id}">modify this chunk again</a>]</p>\n')
    else:
        links += f'<p>[<a href="/cgi-bin/fzloghtml-cgi.py?around={steps[0][0]}#{steps[0][0]}">Log around this change</a>]</p>\n'
    if open_changed:
        links += REFRESH_HISTORIES_BUTTON
    print(MODIFY_TIMES_PAGE % (('Changed' if all_ok else 'Partly Changed'), ('Changed' if all_ok else 'Partly Changed'), body + links))
    return all_ok

GENERIC_ERROR_PAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log - Generic Call Failed</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

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
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log - Generic Call Succeeded</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

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
        decoded_result = results['stdout'].decode()
    except:
        decoded_result = str(results['stdout'])
    if (retcode != 0):
        print(GENERIC_ERROR_PAGE % (thecmd, decoded_result, results['stderr']))
        sys.exit(0)

    # 3. Present the result.
    page = GENERIC_SUCCESS_PAGE % (thecmd, decoded_result)
    if 'Refresh Node histories' in decoded_result:
        page = page.replace('<hr>\n</body>', REFRESH_HISTORIES_BUTTON + '<hr>\n</body>')
    print(page)

INSERT_LOG_CHUNK_ERROR='''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Insert Log chunk error</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Insert Log chunk error</h3>

<p class="fail"><b>ERROR: Failed to insert Log chunk.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

def insert_Log_chunk_error():
    print(INSERT_LOG_CHUNK_ERROR)
    sys.exit(1)

INSERT_LOG_CHUNK='''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Insert Log chunk</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Insert Log chunk</h3>

<p class="success"><b>Log chunk inserted at %s for Node %s with close time stamp %s.</b></p>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

def get_t_chunkclose(chunk_id:str)->str:
    thecmd = f"./fzloghtml -1 {chunk_id} -2 {chunk_id} -T 'STR:{{{{ t_chunkopen }}}} {{{{ t_chunkclose }}}}\n' -o STDOUT -N -q -E STDOUT"
    retcode = try_subprocess_check_output(thecmd, 'stdout', print_result=False)
    if (retcode != 0):
        insert_Log_chunk_error()
    try:
        logdatavec = results['stdout'].decode().split('\n')
    except:
        insert_Log_chunk_error()
    if len(logdatavec) < 1:
        insert_Log_chunk_error()
    logline_datavec = logdatavec[0].split(' ')
    if len(logline_datavec) < 2:
        insert_Log_chunk_error()
    if logline_datavec[0] != chunk_id:
        insert_Log_chunk_error()
    return logline_datavec[1]

# Note that insert_Log_entry is handled through logentry-form.py.
# This is used to automatically create a gap and then use the fzlog '-I' option.
# Parameters:
# chunk_id: The Log chunk after which to insert the new one.
# t_newchunk: The start time stamp for the new Log chunk.
# node_id: The Node for the new Log chunk.
def insert_Log_chunk(chunk_id:str, t_newchunk:str, node_id:str):
    if node_id == 'selected':
        node_id = get_selected_Node(verbosity)
        if not node_id:
            insert_Log_chunk_error()
    # 1. Determine the Log chunk close time that needs to be shifted earlier to make a gap.
    t_chunkclose = get_t_chunkclose(chunk_id)

    # # 2. Shift earlier the close time of the existing Log chunk.
    if int(t_newchunk) > int(t_chunkclose):
        insert_Log_chunk_error()
    if int(t_newchunk) <= int(chunk_id):
        insert_Log_chunk_error()

    thecmd = './fzlog -m %s -2 %s -E STDOUT -W STDOUT' % (chunk_id, t_newchunk)
    retcode = try_subprocess_check_output(thecmd, 'stdout', print_result=False)
    if (retcode != 0):
        insert_Log_chunk_error()
    # 3. Insert the new Log chunk.
    thecmd = './fzlog -I %s -n %s -2 %s -E STDOUT -W STDOUT' % (t_newchunk, node_id, t_chunkclose)
    retcode = try_subprocess_check_output(thecmd, 'stdout', print_result=False)
    if (retcode != 0):
        insert_Log_chunk_error()
    print(INSERT_LOG_CHUNK % (t_newchunk, node_id, t_chunkclose))

INSERT_LOG_CHUNK_PAGE='''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Insert Log chunk</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Insert Log chunk</h3>

After Log chunk %s:

<form action="/cgi-bin/fzlog-cgi.py" method="GET">
<input type="hidden" name="action" value="insert">
<input type="hidden" name="id" value="%s">
At time stamp: <input type="text" name="T"><br>
For Node: <input type="text" name="node" value="selected"><br>
<input type="submit" value="Insert">
</form>

<hr>
[<a href="/index.html">fz: Top</a>]

</body>
</html>
'''

def insert_Log_chunk_page(chunk_id:str):
    print(INSERT_LOG_CHUNK_PAGE % (chunk_id, chunk_id))

def show_interface_options():
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
    if (action == 'modifytimes'):
        modify_chunk_times(form)
        sys.exit(0)
    if (action == 'insert'):
        insert_Log_chunk(id, T_emulated, node)
        sys.exit(0)
    if (action == 'insertchunkpage'):
        insert_Log_chunk_page(id)
        sys.exit(0)
    if (action == 'updatecheckbox'):
        update_checkbox(id, updatecheckbox)
        sys.exit(0)

    show_interface_options()
    sys.exit(0)
