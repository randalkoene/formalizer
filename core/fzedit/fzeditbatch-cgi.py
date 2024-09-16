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

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='%s'" />
</html>
'''
searchresultsNNL = 'fzgraphsearch_cgi'

def Call_Error(msg:str, details=""):
    if details != "":
        details = '<p><pre>'+details+'</pre>'
    print(RESULTPAGE % ('fail', f'<b>Error: {msg}.'+details))
    sys.exit(0)

def show_updated_NNL(listname:str):
    redirecturl = '/cgi-bin/fzgraphhtml-cgi.py?srclist='+listname
    print(REDIRECT % redirecturl)
    sys.exit(0)

def clear_NNL(listname: str) -> bool:
    clearcmd = f"./fzgraph -q -E STDOUT -L delete -l '{listname}'"
    res = try_call_command(clearcmd, return_result=True)
    if res == "":
        return True
    Call_Error('Result of '+clearcmd, res)
    return False

def fill_NNL(listname:str, nodesstr:str) -> bool:
    fillcmd = f"./fzgraph -q -E STDOUT -L add -l '{listname}' -S '{nodesstr}'"
    res = try_call_command(fillcmd, return_result=True)
    if res == "":
        return True
    Call_Error('Result of '+clearcmd, res)
    return False

def get_node_data(nodestr:str, varname:str)->str:
    datacmd = f"./fzgraph -q -m -o STDOUT -C /fz/graph/nodes/{nodestr}/{varname}.raw"
    res = try_call_command(datacmd, return_result=True)
    return res

def get_nodes_CSV(nodelist:list)->str:
    nodes_csv = ''
    for node in nodelist:
        nodes_csv += node + ','
    nodes_csv = nodes_csv[0:len(nodes_csv)-1]
    return nodes_csv

def get_nodes_to_print(nodelist:list)->str:
    nodes_str = ''
    for node in nodelist:
        nodes_str += node + '<br>'
    return nodes_str

def batch_modify_unspecified_internal(nodelist:list):
    for node_id in nodelist:
        thecmd = f"./fzedit -q -E STDOUT -M {node_id} -p unspecified"
        if not try_call_command(thecmd):
            Call_Error('During TD property modification, this command failed: '+thecmd)

def batch_modify_unspecified():
    nodes = form.getvalue('nodes')
    nodes = nodes.split(',')

    batch_modify_unspecified_internal(nodes)

    print(RESULTPAGE % ('success', '<b>Modified %d TD properties of %d Nodes.</b>' % (len(nodes), len(nodes))))

def batch_modify_uniqueTD_internal(nodelist:list):
    # Collect target dates
    node_td_pairs = []
    for node in nodelist:
        node_td = get_node_data(node, 'targetdate')
        if node_td == '':
            Call_Error('Failed to get target date of Node '+node)
        node_td_pairs.append( (node, node_td) )

    node_td_pairs = sorted(node_td_pairs, key=lambda node_td_pair: node_td_pair[1])

    # Create unique target dates
    from datetime import datetime, timedelta
    earliest_td = datetime.strptime(node_td_pairs[0][1], "%Y%m%d%H%M")
    node_new_td_pairs = []
    for i in range(len(node_td_pairs)):
        new_td = earliest_td + timedelta(0, 60*(10*i))
        node_new_td_pairs.append( (node, new_td.strftime("%Y%m%d%H%M")) )

    # Modify target dates

    test_str = ''
    for i in range(len(node_td_pairs)):
        node, node_td = node_td_pairs[i]
        new_td = node_new_td_pairs[i][1]
        test_str += node+' '+node_td+' '+new_td+'<br>'
    print(RESULTPAGE % ('success', test_str))
    sys.exit(0)

def batch_modify_nodes():
    nodelist = []

    for formarg in form:
        if 'chkbx_' in formarg:
            if form.getvalue(formarg) == 'on':
                nodelist.append(formarg[6:])

    do_filter = form.getvalue('filter') == 'on'
    do_unspecified = form.getvalue('unspecified') == 'on'
    do_uniqueTD = form.getvalue('uniqueTD') == 'on'

    if do_filter:
        clear_NNL(searchresultsNNL)
        nodes_csv = get_nodes_CSV(nodelist)
        fill_NNL(searchresultsNNL, nodes_csv)
        show_updated_NNL(searchresultsNNL)

    if do_unspecified:
        batch_modify_unspecified_internal(nodelist)
        show_updated_NNL(searchresultsNNL)

    if do_uniqueTD:
        batch_modify_uniqueTD_internal(nodelist)
        show_updated_NNL(searchresultsNNL)

    print(RESULTPAGE % ('fail', get_nodes_to_print(nodelist)))

if __name__ == '__main__':
    action = form.getvalue('action')

    if (action=='batchmodify'):
        batch_modify_nodes()
        sys.exit(0)

    if action == 'targetdates':
        batch_modify_targetdates()
        sys.exit(0)

    if action == 'td_unspecified':
        batch_modify_unspecified()
        sys.exit(0)

    sys.exit(0)
