#!/usr/bin/python3
#
# logentry-form.py
#
# Randal A. Koene, 20210304
#
# Generate Log entry forms for stand-alone or embedded use. This can generate the
# initial form, as well as its updates as Node is selected, etc.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os, stat
sys.stderr = sys.stdout
import time
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

cgiwritabledir = "/var/www/webdata/formalizer/"

config = {}
config['verbose'] = True
config['logcmdcalls'] = False

results = {}

pagehead = """Content-type:text/html

<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log Entry (fzlog)</title>
</head>
<body>
"""

redirect_head = """Content-type:text/html

<html>
<head>
<meta http-equiv="refresh" content="3;url=/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END" />
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Log Entry (fzlog)</title>
</head>
<body>
"""

pagetail = """<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
"""

selectnodehtml = """
Choose a Node and add it to the <b>select</b> List:
<ul>
<li><a href="/select.html" target="_blank">FZ: Selection Entry Points</a></li>
</ul>
<input type="hidden" name="showrecent" value="on">
<form action="/cgi-bin/logentry-form.py" method="post">
Make Log entry for <input type="submit" name="makeentry" value="Selected Node" />.
</form>
"""

# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring, resstore):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    print('<pre>')
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        p = Popen(thecmdstring,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        if resstore:
            results[resstore] = result
        child_stdout.close()
        if result and config['verbose']:
            print(result)
        print('</pre>')
        return 0

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        print('</pre>')
        return 1


def get_Node_from_selected_NNL() -> str:
    retcode = try_subprocess_check_output(f"./fzgraphhtml -o STDOUT -E STDOUT -L 'selected' -F node -N 1 -e -q",'selected')
    if (retcode != 0):
        return ''
    if results['selected']:
        node = results['selected'][0:16]
        # print(f'Selected: {node}')
        if (len(node) != 16):
            return ''
        else:
            return node # do not use decode() here, it is already a string
    else:
        return ''


def send_to_fzlog(logentrytmpfile: str, node = None, printhead = True):
    thecmd=f"./fzlog -e -E STDOUT -d formalizer -s randalk -f {logentrytmpfile}"
    #thecmd='./fzlog -h -V -E STDOUT -W STDOUT'
    # *** Probably add this as in logentry.py: node = check_same_as_chunk(node)
    if node:
        thecmd += f" -n {node}"

    if printhead:
        if showrecent:
            print(redirect_head)
        else:
            print(pagehead)
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res')
    if (retcode != 0):
        print('<p class="fail"><b>Attempt to add Log entry via fzlog failed.</b></p>')
    else:
        print('<p class="success"><b>Entry added to Log.</b></p>')
    if showrecent:
        print('<p><b>Redirecting to most recent Log in 3 seconds...</b></p>')
    print(pagetail)


def send_to_fzlog_with_selected_Node(logentrytmpfile: str):
    print(pagehead)
    node = get_Node_from_selected_NNL()
    if not node:
        print("<p class=\"fail\"><b>Unable to retrieve target Node from NNL 'selected'.</b></p>")
        print(selectnodehtml)
        print(pagetail)
    
    send_to_fzlog(logentrytmpfile, node, False)


def select_Node():
    print(pagehead)
    print(selectnodehtml)
    print(pagetail)


def missing_option():
    print(pagehead)
    print('<p class="fail"><b>No makeentry option was specified.</b></p>')
    print(pagetail)
    sys.exit(0)


def missing_entry_text():
    print(pagehead)
    print('<p class="fail"><b>No Log entry text submitted.</b></p>')
    print(pagetail)
    sys.exit(0)


def unknown_option():
    print(pagehead)
    print('<p class="fail"><b>Unknown make entry option.</b></p>')
    print(pagetail)
    sys.exit(0)


if __name__ == '__main__':
    form = cgi.FieldStorage()
    makeentry_option = form.getvalue("makeentry")
    showrecent = form.getvalue("showrecent")
    entrytext = form.getvalue("entrytext")

    if not makeentry_option:
        missing_option()

    logentrytextfile = cgiwritabledir+"logentry-text.html"
    if (makeentry_option == "Selected Node"):
        send_to_fzlog_with_selected_Node(logentrytextfile)

    else:
        if not entrytext:
            missing_entry_text()

        try:
            os.remove(logentrytextfile)
        except OSError:
            pass
        # making sure the files are group writable
        with open(os.open(logentrytextfile, os.O_CREAT | os.O_WRONLY, 0o664),"w") as f:
            f.write(entrytext)
        os.chmod(logentrytextfile,stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IROTH )

        if (makeentry_option == "Log Chunk Node"):
            send_to_fzlog(logentrytextfile)
        else:
            if (makeentry_option == "Other Node"):
                select_Node()
            else:
                unknown_option()

    sys.exit(0)
