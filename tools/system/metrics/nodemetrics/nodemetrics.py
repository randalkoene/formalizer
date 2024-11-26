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
from datetime import datetime
import statistics

TOPICSTATSFILE='/var/www/webdata/formalizer/topic_stats.json'

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

<h1>Nodes Subset Metrics</h1>

<p>
Data collected from Log from %s to %s, %s days.
</p>

Statistics by Topic:
<table><tbody>
%s
</tbody></table>

<p></p>

<p>
Number of Nodes shown: %s
</p>

Data by Node:
<table><tbody>
%s
</tbody></table>

</body>
</html>
'''

NODE_DATA_HTML='''<tr><td>ID: <a class="nnl" href="/cgi-bin/fzlink.py?id=%s" target="_blank">%s</a></td><td>%s</td><td>Hours: %s</td><td>Diff Days: %s</td></tr>
'''

TOPIC_DATA_HTML='''<tr><td>%s</td><td>Mean: %.2f</td><td>Median: %.2f</td></tr>
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

    topics = {}

    nodes_subset_html = ''
    for node in data:
        totsec = float(data[node]['tot_sec'])
        topic = data[node]['topic']

        diff_days_str = ''
        daysref = data[node]['days_sec']
        numdays = len(daysref)
        data[node]['days_diff'] = []
        probably_repeating_diffs = False
        for i in range(1, len(daysref)):
            datetime_object1 = datetime.strptime(daysref[i-1][0], '%Y%m%d')
            datetime_object2 = datetime.strptime(daysref[i][0], '%Y%m%d')
            diff_days = int((datetime_object2 - datetime_object1).total_seconds()/86400)
            if diff_days >= 7:
                probably_repeating_diffs = True
            data[node]['days_diff'].append(diff_days)
            diff_days_str += str(diff_days)+' '
        if len(data[node]['days_diff']) >= 5:
            probably_repeating_diffs = True

        t_smallest = 999999
        t_largest = 0
        probably_repeating_tdiff = False
        for i in range(len(daysref)):
            if daysref[i][1] < t_smallest:
                t_smallest = daysref[i][1]
            if daysref[i][1] > t_largest:
                t_largest = daysref[i][1]
        if (t_largest - t_smallest) < 1800 and numdays > 1:
            probably_repeating_tdiff = True

        if probably_repeating_diffs:
            diff_days_str += "[probably was repeating (diffs)]"
        if probably_repeating_tdiff:
            diff_days_str += "[probably was repeating (tdiff)]"
        if t_smallest != t_largest:
            diff_days_str += "(%.2f %.2f)" % (t_smallest, t_largest)

        hours = float(totsec/3600)
        if not probably_repeating_diffs and not probably_repeating_tdiff:
            hours_str = '<b>%.2f</b>' % hours
        else:
            hours_str = '%.2f' % hours
        if topic in topics:
            topics[topic].append(hours)
        else:
            topics[topic] = [ hours ]

        nodes_subset_html += NODE_DATA_HTML % (node, node, topic, hours_str, diff_days_str)

    topicstats = {}
    topics_str = ''
    for topic in topics:
        median = statistics.median(topics[topic])
        mean = statistics.mean(topics[topic])
        topicstats[topic] = { 'mean': mean, 'median': median }
        topics_str += TOPIC_DATA_HTML % (topic, median, mean)

    datetime_object1 = datetime.strptime(startfrom, '%Y%m%d%H%M')
    datetime_object2 = datetime.strptime(endbefore, '%Y%m%d%H%M')
    log_seconds = (datetime_object2 - datetime_object1).total_seconds()
    log_days = int(log_seconds/86400)

    try:
        with open(TOPICSTATSFILE, "w") as f:
            json.dump(topicstats, f)
    except:
        pass

    print(NODES_SUBSET_METRICS_PAGE % (startfrom, endbefore, str(log_days), topics_str, str(num_nodes_shown), nodes_subset_html))


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
