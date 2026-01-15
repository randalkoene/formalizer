#!/usr/bin/python3
#
# team_clusters.py
#
# Randal A. Koene, 20260114
#
# Use this to collect CCF team clusters information.

import os
import sys
import json
import traceback
from io import StringIO
from traceback import print_exc
import subprocess
from datetime import datetime

try:
    import cgitb; cgitb.enable()
except:
    pass
import cgi

running_as_cgi = 'GATEWAY_INTERFACE' in os.environ

outformat = 'json'
json_output = {}

def show_error(msg:str):
    global json_output
    if outformat=='html':
        print(ERROR_PAGE % msg)
    else:
        json_output['error'] = msg
        print(json.dumps(json_output))

# Copied from fzcmdcalls.py to simplify using this as CGI script.
results = {}
def try_subprocess_check_output(
    thecmdstring:str,
    resstore:str=None,
    config:dict={
        'verbose': False,
        'logcmdcalls': False,
        'cmdlog': '',
        'logcmderrors': False,
        'cmderrlog': '',
    })->tuple:

    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        try:
            with open(config['cmdlog'],'a') as f:
                f.write(thecmdstring+'\n')
        except:
            pass
    results['thecmd'] = thecmdstring
    results['error'] = ''

    try:
        res = subprocess.check_output(thecmdstring, shell=True)

    except subprocess.CalledProcessError as cpe:
        errorstr = f'Error output: {str(cpe.output)}\nError code: {cpe.returncode}'
        results['error'] = errorstr
        errorstr = f'Subprocess call: {str(cpe.cmd)}\n'+errorstr+'\n'
        if config['logcmderrors']:
            try:
                with open(config['cmderrlog'],'a') as f:
                    f.write(errorstr)
            except:
                pass
        if config['verbose']:
            print(errorstr)
        return cpe.returncode, results['error']

    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print('Result of subprocess call:', flush=True)
            if isinstance(res, bytes):
                print(res.decode(), flush=True)
            else:
                print(res, flush=True)
        return 0, res

def get_node_data(node:str)->dict:
    if running_as_cgi:
        thecmd = f'./fzgraphhtml -q -o STDOUT -F json -n {node}'
    else:
        thecmd = f'fzgraphhtml -q -o STDOUT -F json -n {node}'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        show_error(f'{thecmd} caused error: '+res)
    try:
        data = json.loads(res)
        return data
    except Exception as e:
        show_error(f'Error: {str(e)}')

def find_from_to(content:str, fromtag:str, totag:str, search_from:int=0)->tuple:
    start = content.find(fromtag, search_from)
    if start<0:
        return None, None
    end = content.find(totag, start+len(fromtag))
    if end<0:
        return None, None
    return start, end+len(totag)

def get_dependencies(node_data:dict)->list:
    if 'dependencies' not in node_data:
        return []
    try:
        dependencies = node_data['dependencies'].strip().split(' ')
    except:
        return []
    return dependencies

CLUSTERS_PAGE = '''<html>
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
<meta http-equiv="Pragma" content="no-cache" />
<meta http-equiv="Expires" content="0" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Team Clusters</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h1>Team Clusters</h1>
<p>
<pre>
%s
</pre>
</p>
</body>
</html>
'''

def process_common(node:str, team:str):
    global json_output
    clusters_top_node_data = get_node_data(node)
    team_node_data = get_node_data(team)
    # top-down collection
    top_down = {}
    dependencies = get_dependencies(clusters_top_node_data)
    for dependency in dependencies:
        cluster_node_data = get_node_data(dependency)
        cluster_dependencies = get_dependencies(cluster_node_data)
        top_down[dependency] = cluster_dependencies
    # bottom-up collection
    bottom_up = {}
    team = get_dependencies(team_node_data)
    for member in team:
        bottom_up[member] = [cluster for cluster in top_down.keys() if member in top_down[cluster]]
    # output
    json_output = {
        'clusters': top_down,
        'team': bottom_up,
    }
    if outformat=='json':
        print(json.dumps(json_output))
    else:
        print(CLUSTERS_PAGE % json.dumps(json_output, indent=4))

def process_as_cgi():
    global outformat
    print('Content-type:text/html\n')

    form = cgi.FieldStorage()
    action = form.getvalue('action')
    if not action:
        action = 'collect'
    outformat = form.getvalue('outformat')
    if not outformat:
        outformat = 'json'
    node = form.getvalue('node')
    if not node:
        node = "20250822225247.1"
    team = form.getvalue('team')
    if not taem:
        team = "20250822084723.1"

    process_common(node, team)

def process_as_cmdline():
    global outformat
    import argparse

    parser = argparse.ArgumentParser(description='Collect CCF Team clusters information.')
    parser.add_argument('-n', dest='node', type=str, default="20250822225247.1", help='Clusters top Node (default: 20250822225247.1)')
    parser.add_argument('-s', dest='team', type=str, default="20250822084723.1", help='Superior Node in which to find Team members available (default: 20250822084723.1)')
    parser.add_argument('-F', dest='outformat', type=str, default='json', help='Output format: json (default)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    outformat = args.outformat

    process_common(args.node, args.team)


if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
