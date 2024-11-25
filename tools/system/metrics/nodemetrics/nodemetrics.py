#!/usr/bin/env python3
# score.py
# Copyright 2023 Randal A. Koene
# License TBD
#
# Display Node specific metrics.
#
# This can be launched as a CGI script.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from io import StringIO
from subprocess import Popen, PIPE
import traceback
import json

import plotly.express as px
import plotly.io as pxio

from fzmodbase import *
from TimeStamp import NowTimeStamp

form = cgi.FieldStorage()

node = form.getvalue("node")
subset = form.getvalue("subset")
help = form.getvalue("help")

print("Content-type: text/html\n")

def try_command_call(thecmd, printhere = True) -> str:
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        error = child_stderr.read()
        child_stderr.close()
        if error:
            if len(error)>0:
                print(error)
        if printhere:
            print(result)
            return ''
        else:
            return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''

NODE_METRICS_PAGE='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Node Metric</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<h1>Node Metrics</h1>

<table><tbody>
<tr><td>Node:</td><td>%s</td></tr>
<tr><td>Number of Log chunks:</td><td>%s</td></tr>
<tr><td>Total hours:</td><td>%s</td></tr>
</tbody></table>

%s

</body>
</html>
'''

def show_node_metrics(node:str):
    thecmd = "./fzlogmap -m %s -F json -o STDOUT -q" % node
    json_data = try_command_call(thecmd, printhere=False)
    data = json.loads(json_data)
    num_chunks = data["node"]["num_chunks"]
    total_hrs = data["node"]["total_hrs"]
    chunks_data = data["node"]["chunks_data"]
    chunks_t_open = [ x[0][0:4]+'-'+x[0][4:6]+'-'+x[0][6:8]+' '+x[0][8:10]+':'+x[0][10:12]+':00' for x in chunks_data]
    chunks_mins = [ int(x[2]) for x in chunks_data ]
    #fig = px.scatter(x=chunks_t_open, y=chunks_mins)
    fig = px.histogram(x=chunks_t_open, y=chunks_mins, nbins=len(chunks_t_open))
    plot_content = pxio.to_html(fig, full_html=False)
    print(NODE_METRICS_PAGE % (node, str(num_chunks), str(total_hrs), plot_content))

NODES_SUBSET_METRICS_PAGE='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Nodes Subset Metrics</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<p>
Not yet displaying, graphing or using the day_sec data of collected Nodes metrics.
Doing so would be needed to identify Nodes that used to be repeated and that should
also be excluded when using the Nodes metrics to improve required time predictions.
</p>

<p>
Number of Nodes shown: %s
</p>

<table></tbody>
%s
</tbody></table>

</body>
</html>
'''

NODE_DATA_HTML='''<tr><td>ID: %s</td><td>Hours: %.2f</td></tr>
'''

def show_nodes_subset_metrics():
    norepeats = form.getvalue("norepeats")
    completed = form.getvalue("completed")
    nonzero = form.getvalue("nonzero")

    alloflog = form.getvalue('alloflog')
    startfrom = form.getvalue('startfrom')
    endbefore  = form.getvalue('endbefore')
    if alloflog:
        startfrom = "199001010000"
        endbefore = NowTimeStamp()

    if startfrom:
        startfrom_arg = " -1 "+startfrom
    else:
        startfrom_arg = ""
    if endbefore:
        endbefore_arg = " -2 "+endbefore
    else:
        endbefore_arg = ""
    if norepeats:
        norepeats_arg = " -A"
    else:
        norepeats_arg = ""
    if completed:
        completed_arg = " -b 1.0 -B 999.0"
    else:
        completed_arg = ""
    if nonzero:
        nonzero_arg = " -Z"
    else:
        nonzero_arg = ""

    thecmd = f"./fzlogmap -F json -q -o STDOUT{startfrom_arg}{endbefore_arg}{norepeats_arg}{completed_arg}{nonzero_arg}"
    json_data = try_command_call(thecmd, printhere=False)
    data = json.loads(json_data)

    num_nodes_shown = len(data)

    nodes_subset_html = ''
    for node in data:
        totsec = float(data[node]['tot_sec'])
        nodes_subset_html += NODE_DATA_HTML % (node, float(totsec/3600))

    print(NODES_SUBSET_METRICS_PAGE % (str(num_nodes_shown), nodes_subset_html))


UNRECOGNIZEDARGS='''<h1>Error</h1>
<p>
<b>Unrecognized CGI arguments.</b>
</p>
'''

HELP='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Node Metrics</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

%s

<h1>nodemetrics.py API</h1>

<p>
Main modes:
<ul>
<li><code>node</code>: Collect and show metrics for one Node
<li><code>subset</code>: Collect and show stats and metrics for a subset of Nodes
<li><code>norepeats</code>: Non-repeated Nodes only
<li><code>completed</code>: Completed Nodes only
<li>By default: Show this help
</ul>
</p>

</body>
</html>
'''

if __name__ == '__main__':
    if node:
        show_node_metrics(node)
        sys.exit(0)
    if subset:
        show_nodes_subset_metrics()
        sys.exit(0)

    if help:
        print(HELP % "")
    else:
        print(HELP % UNRECOGNIZEDARGS)
sys.exit(0)
