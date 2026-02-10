#!/usr/bin/python3
#
# team_doc.py
#
# Randal A. Koene, 20260114
#
# Use this to modify the selection of Team members involved with
# (dependencies of) a Node. See the "Add Team Members" button on
# a Node Edit page.

import os
import sys
import json
import traceback
from io import StringIO
from traceback import print_exc
import subprocess

try:
    import cgitb; cgitb.enable()
except:
    pass
import cgi

running_as_cgi = 'GATEWAY_INTERFACE' in os.environ

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

ERROR_PAGE = '''<html>
<head>
<title>Team SubGroup - Error</title>
</head>
<body>
<h1>Team SubGroup - Error</h1>
<p>
<b>%s</b>
</p>
</body>
</html>
'''

def show_error_page(msg:str):
    print(ERROR_PAGE % msg)
    sys.exit(0)

def get_team_data(node):
    if running_as_cgi:
        thecmd = f'./nodeboard -G -n {node} -Z -T -B 79 -J -o STDOUT -q'
    else:
        thecmd = f'nodeboard -G -n {node} -Z -T -B 79 -J -o STDOUT -q'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        show_error_page(f'{thecmd} caused error: '+res)
    data = json.loads(res)
    return data

def get_node_data(node)->dict:
    if running_as_cgi:
        thecmd = f'./fzgraphhtml -q -o STDOUT -F json -n {node}'
    else:
        thecmd = f'fzgraphhtml -q -o STDOUT -F json -n {node}'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        show_error_page(f'{thecmd} caused error: '+res)
    try:
        data = json.loads(res)
        return data
    except Exception as e:
        show_error_page(f'Error: {str(e)}')

HTML_PAGE = '''<html>
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
<meta http-equiv="Pragma" content="no-cache" />
<meta http-equiv="Expires" content="0" />
<link rel="icon" href="/favicon-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Select Subgroup from CCF Team</title>
<style>
.team_container {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  justify-content: flex-start;
}
.team_member {
  border-radius: 20px;
  box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2);
  transition: 0.3s;
  width: 20em;
  height: 3em;
  text-align: left;
  //overflow: scroll;
  border: 1px solid #333;
  display: flex; /* Centers the text within the item */
  color:#000!important;
  background-color:var(--w3-lightgrey)!important;
  justify-content: left;
  align-items: center;
  box-sizing: border-box;
  padding-left: 1em;
  padding-right: 1em;
}
.team_member:hover {
  box-shadow: 0 8px 16px 0 rgba(0,0,0,0.4);
}
.team_member > table {
  width: 100%%;
}
.team_member > table, tr, td {
  background-color:var(--w3-lightgrey)!important;
}
</style>
</head>
<body>
<script type="module" src="/fzuistate.js"></script>
<h1>Select Subgroup from CCF Team</h1>
Number of team members listed: %s
<form action="/cgi-bin/team_subgroup.py" method="post">
<input type="hidden" name="node" value="%s">
<div class="team_container">
%s
</div>
<p>
<input type="submit" name="action" value="update">
</form>
</body>
</html>
'''

TEAM_MEMBER_HTML = '''<div id="%s" class="team_member">
<table><tbody><tr>
<td style="width:3em;"><input type="checkbox" name="%s" %s></td><td><a href="/cgi-bin/fzlink.py?id=%s" target="_blank">%s</a></td>
</tr></tbody></table>
</div>
'''

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='%s'" />
</html>
'''

def find_from_to(content:str, fromtag:str, totag:str, search_from:int=0)->tuple:
    start = content.find(fromtag, search_from)
    if start<0:
        return None, None
    end = content.find(totag, start+len(fromtag))
    if end<0:
        return None, None
    return start, end+len(totag)

def extract_name(node_text:str)->str:
    h3start, h3end = find_from_to(node_text, '<h3>', '</h3>')
    if h3start is None:
        return 'unknown'
    return node_text[h3start+len('<h3>'):h3end-len('</h3>')]

def get_dependencies(node_data:dict)->list:
    if 'dependencies' not in node_data:
        return []
    try:
        dependencies = node_data['dependencies'].strip().split(' ')
    except:
        return []
    return dependencies

def is_checked(node_id:str, superior_data:dict)->str:
    if node_id in get_dependencies(superior_data):
        return 'checked'
    return ''

def extract_team_member(team_node:dict, node_data:dict)->str:
    return TEAM_MEMBER_HTML % (
        team_node['node-id'],
        team_node['node-id'],
        is_checked(team_node['node-id'], node_data),
        team_node['node-id'],
        extract_name(team_node['node-text']))

def valid_dependency_id(dependency_id:str)->bool:
    if not dependency_id:
        return False
    if dependency_id == '':
        return False
    if len(dependency_id) < 16:
        return False
    if dependency_id[14] != '.':
        return False
    try:
        s = float(dependency_id)
        return True
    except:
        return False

def get_team_alphabetic(team_superior)->list:
    team_data = get_team_data(team_superior)
    columns = sorted(team_data['the-columns'], key=lambda item: item['node-text'])
    subgroup = [team_node for team_node in columns if team_node['node-text'].find('MILESTONE:')<0]
    return subgroup

TEST_PAGE='''<html>
<body>
<p>
Intended subgroup: %s
<p>
Node dependencies: %s
<p>
Current subgroup: %s
<p>
Remove: %s
<p>
Add: %s
</body>
</html>
'''

def add_dependency(node_id:str, dependency_id:str)->tuple:
    thecmd= f'./fzgraph -q -o STDOUT -E STDOUT -C "/fz/graph/nodes/{node_id}/dependencies/add?{dependency_id}="'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        return False, f'{thecmd} caused error: '+res
    return True, res

def remove_dependency(node_id:str, dependency_id:str)->tuple:
    thecmd= f'./fzgraph -q -o STDOUT -E STDOUT -C "/fz/graph/nodes/{node_id}/dependencies/remove?{dependency_id}="'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        return False, f'{thecmd} caused error: '+res
    return True, res

def update_node_dependencies(node:str, team_superior:str, form):
    intended_subgroup = []
    # Collect submitted
    for field_name in form.keys():
        if valid_dependency_id(field_name):
            field_value = form.getvalue(field_name)
            if field_value:
                if field_value=='on':
                    intended_subgroup.append(field_name)
    # Collect Node's current Team Member dependencies
    team = get_team_alphabetic(team_superior)
    node_data = get_node_data(node)
    node_dependencies = get_dependencies(node_data)
    current_subgroup = [team_node['node-id'] for team_node in team if team_node['node-id'] in node_dependencies]
    # Identify dependencies to add or remove
    intended = set(intended_subgroup)
    current = set(current_subgroup)
    remove = list(current - intended)
    add = list(intended - current)
    # Modify
    if len(add)>0:
        for dependency_id in add:
            success, resstr = add_dependency(node, dependency_id)
            if not success:
                show_error_page(resstr)
    if len(remove)>0:
        for dependency_id in remove:
            success, resstr = remove_dependency(node, dependency_id)
            if not success:
                show_error_page(resstr)
    redirect_url = '/cgi-bin/fzgraphhtml-cgi.py?edit='+node
    print(REDIRECT % redirect_url)
    #print(TEST_PAGE % (', '.join(intended_subgroup), ', '.join(node_dependencies), ', '.join(current_subgroup), ', '.join(remove), ', '.join(add)))

def show_selected_subgroup(node:str, team_superior:str):
    node_data = get_node_data(node)
    subgroup = get_team_alphabetic(team_superior)
    team_html = ''
    for team_node in subgroup:
        team_html += extract_team_member(team_node, node_data)
    print(HTML_PAGE % (
        str(len(subgroup)),
        node,
        team_html))

def process_as_cgi():
    print('Content-type:text/html\n')

    form = cgi.FieldStorage()
    action = form.getvalue('action')
    if not action:
        action = 'select'
    team_superior = form.getvalue('team_superior')
    if not team_superior:
        team_superior = "20250822084723.1"
    node = form.getvalue('node')
    if not node:
        show_error_page('Missing "node" argument.')

    if action=='update':
        update_node_dependencies(node, team_superior, form)
    else:
        show_selected_subgroup(node, team_superior)

def process_as_cmdline():
    import argparse

    parser = argparse.ArgumentParser(description='Modify selected Team Members subgroup.')
    parser.add_argument('-s', dest='team_superior', type=str, default="20250822084723.1", help='Superior Node in which to find Team members available (default: 20250822084723.1)')
    parser.add_argument('-n', dest='node', type=str, help='Node for which to modify Team Members involved')
    parser.add_argument('-F', dest='outformat', type=str, default='html', help='Output format: html (default)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    print('This is meant to be run as a CGI script.')

if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
