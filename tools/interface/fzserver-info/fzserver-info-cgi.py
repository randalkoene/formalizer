#!/usr/bin/python3
#
# Randal A. Koene, 20201013
#
# This CGI handler provides a near-verbatim equivalent access to fzserver-info via web form.

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

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

action = form.getvalue('action')
if not action:
    action = 'graphinfo'

print("Content-type:text/html\n\n")

def try_command_call(thecmd:str)->tuple:
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        return True, result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return False, ''

def show_graph_info():
    thecmd = "./fzserver-info -q -G -F html -E STDOUT"
    success, result = try_command_call(thecmd)
    if success:
        print(result)

BALANCE='''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">

<title>fz: Nodes Balance</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>

<h3>fz: Nodes Balance</h3>

<table><tbody>
<tr><td>Open Nodes</td><td>%s</td></tr>
<tr><td>Completed Nodes</td><td>%s</td></tr>
</tbody></table>

<hr>

<p>[<a href="/index.html">fz: Top</a>]</p>

</body>
</html>
'''

def show_nodes_balance():
    thecmd = "./fzserver-info -q -B -E STDOUT"
    success, result = try_command_call(thecmd)
    if success:
        balance = result.split(' ')
        print(BALANCE % tuple(balance))

if __name__ == '__main__':

    if action == 'nodesbalance':
        show_nodes_balance()
    else:
        show_graph_info()
