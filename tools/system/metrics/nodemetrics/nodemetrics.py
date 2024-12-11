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
from datetime import datetime, timedelta
import statistics

TOPICSTATSFILE='/var/www/webdata/formalizer/topic_stats.json'

CHUNKMINS=20
CHUNKSPERHOUR=60/CHUNKMINS

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

THREESAMPLES_TOPIC_HEADER_HTML='''<tr><th></th><th colspan="2">%s</th><th colspan="2">%s</th><th colspan="2">%s</th><th>Nonlinear Mean</th><th>Nonlinear Median</th></tr>
'''

THREESAMPLES_TOPIC_DATA_HTML='''<tr><td%s>%s</td><td>Mean: %.2f</td><td%s>Median: %.2f</td><td>Mean: %.2f</td><td%s>Median: %.2f</td><td>Mean: %.2f</td><td%s>Median: %.2f</td><td>Mean: %.2f</td><td%s>Median: %.2f</td></tr>
'''

THREESAMPLES_NODES_SUBSET_METRICS_PAGE='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Nodes Three Sample Subsets Metrics</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>

<h1>Nodes Three Sample Subsets Metrics</h1>

<p>
Legend:<br>
Yellow means missing data.<br>
Red means significant differences between medians.
Magenta means extremely large median.
</p>

<p>
Note that extremely large medians point to Log data that needs fixing.
</p>

Statistics by Topic:
<table><tbody>
%s
%s
</tbody></table>

</body>
</html>
'''

def collect_subset_Log_sample(norepeats, completed, nonzero, startfrom, endbefore, make_nodes_subset_html=True)->tuple:
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

        if make_nodes_subset_html:
            nodes_subset_html += NODE_DATA_HTML % (node, node, topic, hours_str, diff_days_str)

    topicstats = {
        'LOGMAP': { 'startfrom': startfrom, 'endbefore': endbefore, 'norepeats': str(norepeats), 'completed': str(completed), 'nonzero': str(nonzero) },
    }
    topic_keys = sorted(list(topics.keys()))
    for topic in topic_keys:
        median = statistics.median(topics[topic])
        mean = statistics.mean(topics[topic])
        topicstats[topic] = { 'mean': mean, 'median': median }

    return topics, nodes_subset_html, num_nodes_shown, topicstats

def sample_mean_median(topic:str, topicstats:dict)->list:
    if topic in topicstats:
        return [ topicstats[topic]['mean'], topicstats[topic]['median'] ]
    else:
        return [ -1, -1 ]

def sample_header(sample_interval:dict, num_nodes:int)->str:
    return sample_interval['d_startfrom'].strftime("%Y%m%d%H%M")+'->'+sample_interval['d_endbefore'].strftime("%Y%m%d%H%M")+' (%d Nodes)' % num_nodes

def display_topic_data_with_styles(topic_data:list)->list:
    display_with_style = [ topic_data[0] ]
    for i in range(3):
        offset = (i*2)+1
        display_with_style.append(topic_data[offset+0]) # mean
        if topic_data[offset+1]<0:
            display_with_style.append(' style="background-color:#3f3f3f"') # negative median style
        else:
            display_with_style.append('') # valid median style
        display_with_style.append(topic_data[offset+1]) # median
    return display_with_style

def samples_topic_style(topic_data:list)->str:
    # Style for significant median differences
    if topic_data[2]>=0 or topic_data[4]>=0 or topic_data[6]>=0:
        medians = []
        for median in [ topic_data[2], topic_data[4], topic_data[6] ]:
            if median>=0:
                medians.append(median)
        if len(medians)>=2:
            if statistics.stdev(medians)>0.75:
                return ' style="color:#ff0000"'
    # Style for missing data
    if topic_data[2]<0 or topic_data[4]<0 or topic_data[6]<0:
        return ' style="color:#ffff00"'
    # Default style
    return ""

def nonlinear_samples_mean(topic_data:list)->float:
    weights = [ 1.0, 0.5, 0.25 ]
    means = [ topic_data[1], topic_data[3], topic_data[5] ]
    weighted = [ means[i]*weights[i] for i in range(len(means)) ]
    totweights = 0.0
    nonlinear_mean = 0.0
    for i in range(len(means)):
        if means[i]>0:
            totweights += weights[i]
            nonlinear_mean += weighted[i]
    if totweights <= 0.0:
        return -1.0
    return nonlinear_mean / totweights

def nonlinear_samples_median(topic_data:list)->float:
    weights = [ 1.0, 0.5, 0.25 ]
    medians = [ topic_data[2], topic_data[4], topic_data[6] ]
    weighted = [ medians[i]*weights[i] for i in range(len(medians)) ]
    totweights = 0.0
    nonlinear_median = 0.0
    for i in range(len(medians)):
        if medians[i]>0:
            totweights += weights[i]
            nonlinear_median += weighted[i]
    if totweights <= 0.0:
        return -1.0
    return nonlinear_median / totweights

def nonlinear_median_style(nonlinear_median:float)->str:
    if nonlinear_median < 0.0:
        return ' style="color:#ffff00"'
    if nonlinear_median > 24.0:
        return ' style="color:#ff00ff"'
    else:
        return ""

def round_to_nearest_chunks_multiple(hrs:float)->float:
    return int(100*round(hrs*CHUNKSPERHOUR)/CHUNKSPERHOUR)/100

def round_stats_to_nearest_chunks_multiple(topicstats:dict)->dict:
    rounded_topicstats = {}
    for topic in topicstats:
        rounded_topicstats[topic] = topicstats[topic]
        if 'mean' in rounded_topicstats[topic]:
            rounded_topicstats[topic]['mean'] = round_to_nearest_chunks_multiple(rounded_topicstats[topic]['mean'])
        if 'median' in rounded_topicstats[topic]:
            rounded_topicstats[topic]['median'] = round_to_nearest_chunks_multiple(rounded_topicstats[topic]['median'])
    return rounded_topicstats

def save_subset_stats(topicstats:dict):
    rounded_topicstats = round_stats_to_nearest_chunks_multiple(topicstats)
    try:
        with open(TOPICSTATSFILE, "w") as f:
            json.dump(topicstats, f)
    except:
        pass

def show_nodes_subset_metrics():
    norepeats = form.getvalue("norepeats")
    completed = form.getvalue("completed")
    nonzero = form.getvalue("nonzero")

    threesamples = form.getvalue("threesamples")
    alloflog = form.getvalue('alloflog')
    startfrom = form.getvalue('startfrom')
    endbefore  = form.getvalue('endbefore')
    if alloflog:
        startfrom = "199001010000"
        endbefore = NowTimeStamp()

    if threesamples:
        now = datetime.now()
        start_one_year_ago = now - timedelta(days=365)
        start_date2020 = datetime(2020, 1, 1)
        start_alloflog = datetime(1990, 1, 1)

        sample_intervals = [
            { 'd_startfrom': start_one_year_ago, 'd_endbefore': now },
            { 'd_startfrom': start_date2020, 'd_endbefore': now },
            { 'd_startfrom': start_alloflog, 'd_endbefore': now },
        ]

        sample_num_nodes = []
        sample_topics = []
        sample_topicstats = []
        for sample_interval in sample_intervals:
            startfrom = sample_interval['d_startfrom'].strftime("%Y%m%d%H%M")
            endbefore = sample_interval['d_endbefore'].strftime("%Y%m%d%H%M")
            topics, nodes_subset_html, num_nodes_shown, topicstats = collect_subset_Log_sample(norepeats, completed, nonzero, startfrom, endbefore, make_nodes_subset_html=False)

            sample_num_nodes.append(num_nodes_shown)
            sample_topics.append(topics)
            sample_topicstats.append(topicstats)

        topics_str = ''
        suggestions_topicstats = {}
        all_topic_keys = sorted(list(sample_topics[-1]))
        for topic in all_topic_keys:
            topic_data = [topic]+sample_mean_median(topic, sample_topicstats[0])+sample_mean_median(topic, sample_topicstats[1])+sample_mean_median(topic, sample_topicstats[2])
            display_topic_data = display_topic_data_with_styles(topic_data)
            topic_style = samples_topic_style(topic_data)
            nonlinear_mean = nonlinear_samples_mean(topic_data)
            nonlinear_median = nonlinear_samples_median(topic_data)
            nlmedian_style = nonlinear_median_style(nonlinear_median)
            topic_data_tuple = tuple([topic_style]+display_topic_data+[nonlinear_mean]+[nlmedian_style]+[nonlinear_median]) # (s sfsffsffsf f s f)
            topics_str += THREESAMPLES_TOPIC_DATA_HTML % topic_data_tuple
            suggestions_topicstats[topic] = { 'mean': nonlinear_mean, 'median': nonlinear_median }

        save_subset_stats(suggestions_topicstats)

        topics_head_str = THREESAMPLES_TOPIC_HEADER_HTML % (sample_header(sample_intervals[0], sample_num_nodes[0]), sample_header(sample_intervals[1], sample_num_nodes[1]), sample_header(sample_intervals[2], sample_num_nodes[2]))
        print(THREESAMPLES_NODES_SUBSET_METRICS_PAGE % (topics_head_str, topics_str))

    else:
        topics, nodes_subset_html, num_nodes_shown, topicstats = collect_subset_Log_sample(norepeats, completed, nonzero, startfrom, endbefore)

        topics_str = ''
        topic_keys = sorted(list(topics.keys()))
        for topic in topic_keys:
            topics_str += TOPIC_DATA_HTML % (topic, topicstats[topic]['mean'], topicstats[topic]['median'])

        datetime_object1 = datetime.strptime(startfrom, '%Y%m%d%H%M')
        datetime_object2 = datetime.strptime(endbefore, '%Y%m%d%H%M')
        log_seconds = (datetime_object2 - datetime_object1).total_seconds()
        log_days = int(log_seconds/86400)

        save_subset_stats(topicstats)

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
