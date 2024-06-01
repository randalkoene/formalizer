#!/usr/bin/python3
#
# Randal A. Koene, 20201209
#
# CGI script to forward Node editing data to fzedit.
#
# This handler is currently also able to handle new Node specification, and
# will call fzgraph when the Node ID is "new" or "NEW". For a very similar
# implementation aimed specifically at adding new Nodes, please see fzgraph-cgi.py.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from time import strftime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

print('Content-type:text/html\n\n');

# Create instance of FieldStorage 
form = cgi.FieldStorage()

RESULTPAGE = '''<html>
<head>
<meta charset="utf-8">
<link rel="stylesheet" href="/fz.css">
<title>fz: Batch Edit</title>
</head>
<body onload="do_if_opened_by_script('Keep Page','Go to Topics','/cgi-bin/fzgraphhtml-cgi.py?topics=?');">
<style type="text/css">
.chktop {
    background-color: #B0C4F5;
}
</style>
<p class="%s">%s</p>
<hr>
<button id="closing_countdown" class="button button1" onclick="Keep_or_Close_Page('closing_countdown');">Keep Page</button>
<script type="text/javascript" src="/fzclosing_window.js"></script>
</body>
</html>
'''

def try_call_command(thecmd: str, return_result=False):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        if return_result:
            return result
        #print('<!-- begin: call output --><pre>')
        #print(result)
        #print('<!-- end  : call output --></pre>')
        return True

    except Exception as ex:                
        #print(ex)
        #f = StringIO()
        #print_exc(file=f)
        #a = f.getvalue().splitlines()
        #for line in a:
        #    print(line)
        return False

def batch_modify_targetdates():
    nodes = form.getvalue('nodes')
    tds = form.getvalue('tds')
    nodes = nodes.split(',')
    tds = tds.split(',')
    commands = '<!--\n'

    if len(nodes) != len(tds):
        print(RESULTPAGE % ('fail', '<b>Number of nodes must equal number of target dates for batch target dates modification.</b>'))
        return

    for i in range(len(nodes)):
        node_id = nodes[i]
        targetdate = tds[i]
        thecmd = f"./fzedit -q -E STDOUT -M {node_id} -t {targetdate}"
        commands += thecmd+'\n'
        if not try_call_command(thecmd):
            print(RESULTPAGE % ('fail', '<b>During target dates modification, this command failed: '+thecmd+'</b'))
            return
    commands += '-->\n'

    print(RESULTPAGE % ('success', '<b>Modified %d target dates of %d Nodes.</b>' % (len(tds), len(nodes))))

if __name__ == '__main__':
    action = form.getvalue('action')

    if action == 'targetdates':
        batch_modify_targetdates()
        sys.exit(0)

    sys.exit(0)
