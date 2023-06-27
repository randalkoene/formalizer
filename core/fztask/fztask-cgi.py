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
    #retcode = try_subprocess_check_output(f"./fzgraphhtml -E STDOUT -W STDOUT -L 'selected' -F node -N 1 -e -q",'selected', verbosity)
    retcode = try_subprocess_check_output(f"./fzgraphhtml -E STDOUT -W /dev/null -L 'selected' -F node -N 1 -e -q",'selected', verbosity)
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
    #retcode = try_subprocess_check_output(f"./fzgraphhtml -E STDOUT -W STDOUT -L 'selected' -F html -N 1 -e -q",'selected_html', verbosity)
    retcode = try_subprocess_check_output(f"./fzgraphhtml -E STDOUT -W /dev/null -L 'selected' -F html -N 1 -e -q",'selected_html', verbosity)
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

def nonlocal_fztask_page():
    print('Content-type:text/html\n\n')
    new_GET_str = ''

    # Get data from fields
    is_static = form.getvalue('is_static')

    T_emulated = form.getvalue('T')
    if (T_emulated == '') or (T_emulated == 'actual'):
        T_emulated = None
    if T_emulated:
        new_GET_str += '&T='+T_emulated
        T_emulated_showhtml = T_emulated
    else:
        T_emulated_showhtml = "<b>actual time</b>"
    
    # Prepare state data
    chunkdatavec = get_most_recent_Log_chunk_info()
    t_open = chunkdatavec[0]
    open_or_closed = chunkdatavec[1]
    recent_id = chunkdatavec[2]
    num_entries = chunkdatavec[3]
    t_open_epoch = int(datetime.datetime.strptime(t_open, '%Y%m%d%H%M').timestamp())
    if not T_emulated:
        fzlog_T = ''
        t_epoch = int(datetime.datetime.now().timestamp())
        diff_minutes = int((t_epoch - t_open_epoch) / 60)
        diff_min_str = f'at least {diff_minutes}'
    else:
        fzlog_T = '&T='+T_emulated
        t_epoch = int(datetime.datetime.strptime(T_emulated, '%Y%m%d%H%M').timestamp())
        diff_minutes = int((t_epoch - t_open_epoch) / 60)
        diff_min_str = str(diff_minutes)
    if (open_or_closed == 'OPEN'):
        open_closed_str = f'<b>Opened</b> at {t_open}.'
    else:
        open_closed_str = f'<b>Closed.</b>'
    got_selected = False
    same_node = False
    selected_node_HTML = get_selected_Node_HTML()
    # Check if same Node (needs confirmation)
    if selected_node_HTML:
        idstr_start = selected_node_HTML.find('?id=')
        if (idstr_start >= 0):
            got_selected = True
            idstr_start += 4
            idstr_end = selected_node_HTML.find('"',idstr_start)
            if (idstr_start < 0):
                got_selected = False
            else:
                selected_id = selected_node_HTML[idstr_start:idstr_end]
                if (recent_id == selected_id):
                    same_node = True
    static_page_call = '/cgi-bin/fztask-cgi.py?is_static=on'+new_GET_str

    if is_static:
        fztask_webpage = ("<html>\n"
        "<head>\n"
        """<meta charset="utf-8" />\n"""
        """<link rel="icon" href="/favicon-32x32.png">\n"""
        """<link rel="stylesheet" href="/fz.css">\n"""
        """<link rel="stylesheet" href="/fzuistate.css">\n"""
        "<title>fz: Task</title>\n"
        "<style>\n"
        "td.stateinfo {\n"
        "    background-color: #b8bfff;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<!-- STATIC version -->"
        "<h1>fz: Task</h1>\n"
        "\n"
        f"<p>T = {T_emulated_showhtml} ({diff_min_str} minutes since Log chunk opening)</p>\n"
        "\n"
        "<table><tbody>\n"
        "<tr>\n"
        """<td>1. [<a href="/formalizer/logentry-form_fullpage.template.html" target="_blank">Make Log entry</a>]</td>\n"""
        """<td class="stateinfo">\n"""
        f'Log entries in this chunk: {num_entries}'
        f"""</td></tr>\n"""
        """<tr>\n"""
        f"""<td>2. [<a href="/cgi-bin/fzlog-cgi.py?action=close{fzlog_T}" target="_blank">Close Log chunk</a>]</td>\n"""
        """<td class="stateinfo">\n"""
        f'{open_closed_str}\n'
        """</td></tr>\n"""
        """<tr>\n""")

        if T_emulated:
            fztask_webpage += ("""<td>3. (Updating Schedule not recommended in Emulated Time.) [<a href="cgi-bin/fzgraphhtml-cgi.py" target="_blank">Update Anyway</a>]</td>\n""")
        else:
            fztask_webpage += ("""<td>3. [<a href="cgi-bin/fzgraphhtml-cgi.py" target="_blank">Update Schedule</a>]</td>\n""")

        fztask_webpage += ("""<td class="stateinfo"></td>\n"""
        """</tr>\n"""
        """<tr>\n"""
        """<td>4. [<a href="/select.html" target="_blank">Select Node for Next Log chunk</a>]</td>\n"""
        """<td class="stateinfo"><table><tbody>\n"""
        f'{selected_node_HTML}\n'
        f"""</tbody></table></td></tr>\n"""
        """<tr>\n""")

        if got_selected:
            if same_node:
                fztask_webpage += (f"""<td>5. [Open New Log chunk] confirm: [<a href="/cgi-bin/fzlog-cgi.py?action=open{fzlog_T}">Same Node - Open Log chunk</a>]</td>\n""")
            else:
                fztask_webpage += (f"""<td>5. [<a href="/cgi-bin/fzlog-cgi.py?action=open{fzlog_T}">Open New Log chunk</a>]</td>\n""")
        else:
            fztask_webpage += (f"""<td>5. [Open New Log chunk] (select a Node to activate)</td>\n""")

        fztask_webpage += ("""<td class="stateinfo"></td>\n"""
        """</tr>\n"""
        """</tbody></table>\n"""
        """\n"""
        """<hr>\n"""
        """[<a href="/index.html">fz: Top</a>]\n"""
        """\n"""
        """</body>\n"""
        """</html>\n""")

    else:
        fztask_webpage = ("<html>\n"
        "<head>\n"
        """<meta charset="utf-8" />\n"""
        f"""<noscript><meta http-equiv="refresh" content="0; url={static_page_call}" /></noscript>"""
        """<link rel="icon" href="/favicon-32x32.png">\n"""
        """<link rel="stylesheet" href="/fz.css">\n"""
        """<link rel="stylesheet" href="/fzuistate.css">\n"""
        "<title>fz: Task</title>\n"
        "<style>\n"
        "td.stateinfo {\n"
        "    background-color: #b8bfff;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>fz: Task</h1>\n"
        "\n"
        f"<p>T = {T_emulated_showhtml} ({diff_min_str} minutes since Log chunk opening)</p>\n"
        "\n"
        "<table><tbody>\n"
        "<tr>\n"
        """<td>1. <button class="button button1" onclick="window.open('/formalizer/logentry-form_fullpage.template.html','_blank');">Make Log Entry</button></td>\n"""
        """<td class="stateinfo">\n"""
        f'Log entries in this chunk: {num_entries}'
        f"""</td></tr>\n"""
        """<tr>\n"""
        f"""<td>2. <button class="button button2" onclick="window.open('/cgi-bin/fzlog-cgi.py?action=close{fzlog_T}','_blank');">Close Log chunk</button></td>\n"""
        """<td class="stateinfo">\n"""
        f'{open_closed_str}\n'
        """</td></tr>\n"""
        """<tr>\n""")

        if T_emulated:
            fztask_webpage += ("""<td>3. (Updating Schedule not recommended in Emulated Time.) <button class="button button1" onclick="window.open('cgi-bin/fzgraphhtml-cgi.py','_blank');">Update Anyway</button></td>\n""")
        else:
            fztask_webpage += ("""<td>3. <button class="button button1" onclick="window.open('cgi-bin/fzgraphhtml-cgi.py','_blank');">Update Schedule</button></td>\n""")

        fztask_webpage += ("""<td class="stateinfo"></td>\n"""
        """</tr>\n"""
        """<tr>\n"""
        """<td>4. <button class="button button2" onclick="window.open('/select.html','_blank');">Select Node for Next Log chunk</button></td>\n"""
        """<td class="stateinfo"><table><tbody>\n"""
        f'{selected_node_HTML}\n'
        f"""</tbody></table></td></tr>\n"""
        """<tr>\n""")

        if got_selected:
            if same_node:
                fztask_webpage += (f"""<td>5. <button class="button button_inactive">Open New Log chunk</button> confirm: <button class="button button2" onclick="window.open('/cgi-bin/fzlog-cgi.py?action=open{fzlog_T}','');">Same Node - Open Log chunk</button></td>\n""")
            else:
                fztask_webpage += (f"""<td>5. <button class="button button1" onclick="window.open('/cgi-bin/fzlog-cgi.py?action=open{fzlog_T}','');">Open New Log chunk</button></td>\n""")
        else:
            fztask_webpage += (f"""<td>5. <button class="button button1">Open New Log chunk</button> (select Node to activate)</td>\n""")

        fztask_webpage += ("""<td class="stateinfo"></td>\n"""
        """</tr>\n"""
        """</tbody></table>\n"""
        """\n"""
        """*** Missing behavior on this page: (1) There should be no need to press reload, changing the 'select' NNL to a different Node should cause state-change here. (2) Successful opening of new Log chunks should cause visible state change so that there is no question if that was already done (in addition to the Node history that pops up), perhaps even start count-down to closing page.\n"""
        """(3) Both the content of this page, and the LogTime mini day-display could be integrated in the Top page as part of tab areas or overlays that change."""
        """<hr>\n"""
        """[<a href="/index.html">fz: Top</a>]\n"""
        """\n"""
        """<script type="text/javascript" src="/fzuistate.js"></script>\n"""
        """</body>\n"""
        """</html>\n""")

    print(fztask_webpage)


def local_fztask_page():
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


if __name__ == '__main__':

    if (non_local == 'on'):
        nonlocal_fztask_page()
        sys.exit(0)

    ### Below is executed only when run locally (e.g. in w3m)
    local_fztask_page()
    sys.exit(0)
