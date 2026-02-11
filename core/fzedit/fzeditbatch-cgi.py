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
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Batch Edit</title>
</head>
<body onload="do_if_opened_by_script('Keep Page','Go to Topics','/cgi-bin/fzgraphhtml-cgi.py?topics=?');">
<script type="module" src="/fzuistate.js"></script>
<p class="%s">%s</p>
<hr>
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

TESTFROMJSONDATA='''<html>
<head>
<meta charset="utf-8">
<title>Test From JSON Data</title>
</head>
<body>
<pre>
Nodes:
%s
Target dates:
%s
</pre>
</body>
</html>
'''

def debug_mark(mark:str):
    try:
        with open('/tmp/fzeditbatch-cgi.debug', 'w') as f:
            f.write(mark)
    except:
        pass

def get_from_jsondata(jsondata:str)->tuple:
    from json import loads
    data = loads(jsondata)
    nodes = []
    tds = []
    for dataitem in data:
        tds.append(dataitem[0])
        nodes.append(dataitem[1])
    #print(TESTFROMJSONDATA % (str(nodes), str(tds)))
    return (nodes, tds)

def batch_modify_targetdates():
    jsondata = form.getvalue('jsondata')
    if jsondata:
        nodes, tds = get_from_jsondata(jsondata)
    else:
        nodes = form.getvalue('nodes')
        tds = form.getvalue('tds')
        nodes = nodes.split(',')
        tds = tds.split(',')
    commands = '<!--\n'

    debug_mark('entry')

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

    debug_mark('processed')

    print(RESULTPAGE % ('success', '<b>Modified %d target dates of %d Nodes.</b>' % (len(tds), len(nodes))))

    debug_mark('responded')

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
    redirecturl = '/cgi-bin/fzgraphhtml-cgi.py?srclist='+listname+'&sort_by=targetdate'
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

def set_node_targetdate(nodestr:str, new_td:str):
    datacmd = f"./fzgraph -q -C /fz/graph/nodes/{nodestr}?targetdate={new_td}"
    res = try_call_command(datacmd, return_result=False)
    if not res:
        Call_Error('Failed to modify target date for '+nodestr+' to '+new_td)

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
        node = node_td_pairs[i][0]
        new_td = earliest_td + timedelta(0, 60*(10*i))
        node_new_td_pairs.append( (node, new_td.strftime("%Y%m%d%H%M")) )

    # Modify target dates
    for node, new_td in node_new_td_pairs:
        set_node_targetdate(node, new_td)

    # test_str = ''
    # for i in range(len(node_td_pairs)):
    #     node = node_new_td_pairs[i][0]
    #     node_td = node_td_pairs[i][1]
    #     new_td = node_new_td_pairs[i][1]
    #     test_str += node+' '+node_td+' '+new_td+'<br>'

    # test_str += '<p>'

    # for node, new_td in node_new_td_pairs:
    #     set_node_targetdate(node, new_td)
    #     node_td = get_node_data(node, 'targetdate')
    #     test_str += node+' '+node_td+'<br>'

    # print(RESULTPAGE % ('success', test_str))
    # sys.exit(0)

def batch_modify_nodes():
    nodelist = []

    for formarg in form:
        if 'chkbx_' in formarg:
            if form.getvalue(formarg) == 'on':
                nodelist.append(formarg[6:])

    batchmodify = form.getvalue('batchmodify')

    if batchmodify=='filter':
        clear_NNL(searchresultsNNL)
        nodes_csv = get_nodes_CSV(nodelist)
        fill_NNL(searchresultsNNL, nodes_csv)
        show_updated_NNL(searchresultsNNL)

    elif batchmodify=='unspecified':
        batch_modify_unspecified_internal(nodelist)
        show_updated_NNL(searchresultsNNL)

    elif batchmodify=='uniqueTD':
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
