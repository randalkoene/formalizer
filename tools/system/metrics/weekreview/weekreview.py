#!/usr/bin/python3
#
# Randal A. Koene, 20240607
#
# This CGI handler displays Week Goal metrics.
#
# This is now also usable to display Threads metrics.
#
# Call this scrip as follows:
#   /cgi-bin/weekreview.py?nnl=week_main_goals&invested=3.0,2.3,4.1&remaining=1.0,5.0,0.0

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
import subprocess
import plotly.graph_objects as go
import plotly.io as pxio
#import pandas as pd

#webdata_path = "/var/www/webdata/formalizer"
#weekreview_path = webdata_path+'/weekreview_plot.html'

cgivars = [
    'nodes',
    'invested',
    'remaining',
    'nnl',
]

form = cgi.FieldStorage()

cgivalues = {}

results = {}

def try_subprocess_check_output(thecmdstring: str, resstore: str, verbosity=1) -> int:
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

def get_nodes_from_nnl(nnlstr:str)->str:
    retcode = try_subprocess_check_output(f"./fzgraphhtml -q -e -F node -o STDOUT -L "+nnlstr,'nodes')
    if (retcode != 0):
        return ''
    nodes = (results['nodes']).decode()
    nodes = nodes.split('\n')
    nodesstr = ''
    for i in range(len(nodes)-1):
        if i!=0:
            nodesstr += ','
        nodesstr += nodes[i]
    return nodesstr

# Three bars each:
# 1. Percentage progress.
# 2. Time invested.
# 3. Time remaining.
def three_bars_per_goal(cgivalues:dict)->str:
    nodes = cgivalues['nodes'].split(',')
    invested = [ float(v) for v in cgivalues['invested'].split(',') ]
    remaining = [ float(v) for v in cgivalues['remaining'].split(',') ]

    fig = go.Figure()

    # fig.add_trace(go.Bar(
    #     y=nodes,
    #     x=progress,
    #     name='Progress',
    #     marker_color='crimson',
    #     orientation='h'
    # ))

    fig.add_trace(go.Bar(
        y=nodes,
        x=invested,
        name='Invested',
        marker_color='lightsalmon',
        orientation='h'
    ))

    fig.add_trace(go.Bar(
        y=nodes,
        x=remaining,
        name='Remaining',
        marker_color='indianred',
        orientation='h'
    ))
    fig.update_layout(title="Invested and Remaining per Goal", xaxis_title="hours", width=1000)
    return pxio.to_html(fig, full_html=False)

def stacked_per_goal(cgivalues:dict)->str:
    nodes = cgivalues['nodes'].split(',')
    invested = [ float(v) for v in cgivalues['invested'].split(',') ]
    remaining = [ float(v) for v in cgivalues['remaining'].split(',') ]

    fig = go.Figure(
        data=[
            go.Bar(
                name="Invested",
                x=invested,
                y=nodes,
                offsetgroup=0,
                orientation='h'
            ),
            go.Bar(
                name="Remaining",
                x=remaining,
                y=nodes,
                offsetgroup=0,
                base=invested,
                orientation='h'
            )
        ],
        layout=go.Layout(
            title="Invested and Remaining per Goal",
            xaxis_title="hours",
            width=1000,
        )
    )
    return pxio.to_html(fig, full_html=False)

def stacked_percentage_per_goal(cgivalues:dict)->str:
    nodes = cgivalues['nodes'].split(',')
    invested = [ float(v) for v in cgivalues['invested'].split(',') ]
    remaining = [ float(v) for v in cgivalues['remaining'].split(',') ]
    totals = [ invested[i]+remaining[i] for i in range(len(invested)) ]
    pct_invested = [ 100*invested[i]/totals[i] for i in range(len(invested)) ]
    pct_remaining = [ 100*remaining[i]/totals[i] for i in range(len(remaining)) ]

    fig = go.Figure(
        data=[
            go.Bar(
                name="Invested",
                x=pct_invested,
                y=nodes,
                offsetgroup=0,
                orientation='h'
            ),
            go.Bar(
                name="Remaining",
                x=pct_remaining,
                y=nodes,
                offsetgroup=0,
                base=pct_invested,
                orientation='h'
            )
        ],
        layout=go.Layout(
            title="Percentage Invested and Remaining per Goal",
            xaxis_title="percent",
            width=1000,
        )
    )
    return pxio.to_html(fig, full_html=False)

def stacked_total_percentages(cgivalues:dict)->str:
    nodes = cgivalues['nodes'].split(',')
    totinvested = sum([ float(v) for v in cgivalues['invested'].split(',') ])
    totremaining = sum([ float(v) for v in cgivalues['remaining'].split(',') ])
    tottotal = totinvested + totremaining
    pct_totinvested = 100*totinvested/tottotal
    pct_totremaining = 100*totremaining/tottotal

    fig = go.Figure(
        data=[
            go.Bar(
                name="Invested",
                x=[ pct_totinvested ],
                y=[ 'goals' ],
                offsetgroup=0,
                orientation='h'
            ),
            go.Bar(
                name="Remaining",
                x=[ pct_totremaining ],
                y=[ 'goals' ],
                offsetgroup=0,
                base=[ pct_totinvested ],
                orientation='h'
            )
        ],
        layout=go.Layout(
            title="Total Percentage Invested and Remaining",
            xaxis_title="percent",
            width=2000,
            height=200,
        )
    )
    return pxio.to_html(fig, full_html=False)

HTML_HEAD='''<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="application/xml+xhtml; charset=UTF-8"/>
<link rel="stylesheet" href="/fz.css">
<title>Plotting Progress on %s</title>
</head>
<body style="color:white; background-color:black">
'''

HTML_TAIL='''
</body>
</html>
'''

WEEKGOALS='''
<table>
<tr>
<td colspan=2 style="width:2000px;height:200px">%s</td>
</tr>
<tr>
<td style="width:1000px">%s</td>
<td style="width:1000px">%s</td>
</tr>
<tr>
<td style="width:1000px">%s</td>
<td>%s</td>
</tr>
</table>
'''

def get_nodes_links(cgivalues:dict)->str:
    retcode = try_subprocess_check_output("./fzgraphhtml -q -L "+cgivalues['nnl']+" -F desc -o STDOUT -e",'nodedesc')
    if (retcode != 0):
        return ''
    descriptions = (results['nodedesc']).decode()
    descriptions = descriptions.split('@@@')

    nodes = cgivalues['nodes'].split(',')
    node_links_str = ''
    for i in range(len(nodes)):
        node = nodes[i]
        desc = descriptions[i]
        node_link = '<a style="color:yellow;" href="/cgi-bin/fzlink.py?id=%s&amp;alt=histfull" target="_blank">%s</a> ' % (node,node)
        node_links_str = node_link+desc+'<br>\n'+node_links_str
    return node_links_str

#<td style="width:100%%">%s</td>

def plot_week_goals_progress(cgivalues:dict):
    threebar_content = three_bars_per_goal(cgivalues)
    stackedpergoal = stacked_per_goal(cgivalues)
    stackedpercentages = stacked_percentage_per_goal(cgivalues)
    stackedtotalpercentages = stacked_total_percentages(cgivalues)

    print(HTML_HEAD % cgivalues['nnl'])
    print(WEEKGOALS % (stackedtotalpercentages, stackedpergoal, stackedpercentages, threebar_content, get_nodes_links(cgivalues)))
    print(HTML_TAIL)

if __name__ == '__main__':
    for cgivar in cgivars:
        cgivalues[cgivar] = form.getvalue(cgivar)

    if cgivalues['nnl']:
        cgivalues['nodes'] = get_nodes_from_nnl(cgivalues['nnl'])
    else:
        cgivalues['nnl'] = 'week_main_goals'

    plot_week_goals_progress(cgivalues)
    sys.exit(0)
