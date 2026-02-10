#!/usr/bin/python3
#
# team_activities.py
#
# Randal A. Koene, 20260114
#
# Use this to collect activities information for a team member.

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
targetdate_relevance_days = 30
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

def extract_ccf_skill_application(content:str)->str:
    ccfstart, ccfend = find_from_to(content, 'Main CCF Skill Application:', '\n')
    if ccfstart is None:
        return ''
    return content[ccfstart+len('Main CCF Skill Application:'):ccfend]

def extract_current_activities(content:str)->list:
    actstart, actend = find_from_to(content, '<b>Current activities</b>:', '<UL>')
    if actstart is None:
        return []
    actlistend = content.find('</UL>', actend)
    if actlistend<0:
        return []
    actlist = content[actend:actlistend].split('<LI>')
    activities = [activity.strip() for activity in actlist if activity.strip()!='']
    return activities

def get_superiors(node_data:dict)->list:
    if 'superiors' not in node_data:
        return []
    try:
        superiors = node_data['superiors'].strip().split(' ')
    except:
        return []
    return superiors

def get_dependencies(node_data:dict)->list:
    if 'dependencies' not in node_data:
        return []
    try:
        dependencies = node_data['dependencies'].strip().split(' ')
    except:
        return []
    return dependencies

ACTIVITIES_PAGE = '''<html>
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
<meta http-equiv="Pragma" content="no-cache" />
<meta http-equiv="Expires" content="0" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Team Member Activities</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>
<h1>Team Member Activities</h1>
<p>
Skills applied at CCF: %s
</p>
<p>
Current activities: %s
</p>
<p>
Involved in superiors due: %s
</p>
</body>
</html>
'''

TEAM_ACTIVITIES_PAGE = '''<html>
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
<meta http-equiv="Pragma" content="no-cache" />
<meta http-equiv="Expires" content="0" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Team Activities</title>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>
<h1>Team Activities</h1>
<p>
<pre>
%s
</pre>
</p>
</body>
</html>
'''

def td_beyond(td:str)->bool:
    target_date = datetime.strptime(td, "%Y%m%d%H%M")
    now = datetime.now()
    delta = target_date - now
    days_remaining = delta.days
    return days_remaining > targetdate_relevance_days

def collect_member_activities(node:str)->dict:
    node_data = get_node_data(node)
    # Collect CCF skills application
    ccfskills_str = extract_ccf_skill_application(node_data['node-text']).strip()
    # Collect current activities
    current_activities_list = extract_current_activities(node_data['node-text'])
    # Collect superiors (that are not complete, not zero required time, and possibly limited in targetdate)
    superiors_list = get_superiors(node_data)
    relevant_superiors = []
    for superior in superiors_list:
        try:
            superior_data = get_node_data(superior)
            if float(superior_data['completion'])<0 or float(superior_data['completion'])>=1.0:
                continue
            if float(superior_data['required'])<=0.0:
                continue
            if td_beyond(superior_data['targetdate']):
                continue
        except Exception as e:
            show_error(f'Error during superiors processing in team_activities.py: {str(e)}')
        relevant_superiors.append(superior)
    collected_activities = {
        'ccfskills': ccfskills_str,
        'activities': current_activities_list,
        'superiors': relevant_superiors,
    }
    return collected_activities

def process_common(node:str, team:str):
    global json_output
    if node=='all':
        team_node_data = get_node_data(team)
        team_members = get_dependencies(team_node_data)
        team_activities = {}
        for member in team_members:
            member_activities = collect_member_activities(member)
            team_activities[member] = member_activities
        json_output = team_activities
        if outformat=='html':
            print(TEAM_ACTIVITIES_PAGE % json.dumps(json_output, indent=4))
        else:
            print(json.dumps(json_output))
    else:
        collected_activities = collect_member_activities(node)
        if outformat=='html':
            print(ACTIVITIES_PAGE % (
                collected_activities['ccfskills'],
                ', '.join(collected_activities['activities']),
                ', '.join(collected_activities['superiors'])))
        else:
            json_output = collected_activities
            print(json.dumps(json_output))

def process_as_cgi():
    global outformat
    global targetdate_relevance_days
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
        show_error('Missing "node" argument.')
    tdays_relevant = form.getvalue('tdays_relevant')
    if tdays_relevant:
        try:
            targetdate_relevance_days = int(tdays_relevant)
        except:
            targetdate_relevance_days = 30
    team = form.getvalue('team')
    if not team:
        team = "20250822084723.1"

    process_common(node, team)

def process_as_cmdline():
    global outformat
    global targetdate_relevance_days
    import argparse

    parser = argparse.ArgumentParser(description='Collect CCF Team activities information.')
    parser.add_argument('-n', dest='node', type=str, help='Team Node to collect activities information about ("all" means for the whole team)')
    parser.add_argument('-s', dest='team', type=str, default="20250822084723.1", help='Superior Node in which to find Team members available (default: 20250822084723.1)')
    parser.add_argument('-T', dest='tdays_relevant', type=int, default=30, help='Days to target date considered relevant (default: 30)')
    parser.add_argument('-F', dest='outformat', type=str, default='json', help='Output format: json (default)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    outformat = args.outformat
    targetdate_relevance_days = args.tdays_relevant

    process_common(args.node, args.team)


if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
