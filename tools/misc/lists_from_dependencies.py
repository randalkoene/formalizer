#!/usr/bin/python3
#
# lists_from_dependencies.py
#
# Randal A. Koene, 20250824
#
# Go through the dependencies of a Node and collect all the
# list contents under a specific list-header string.
#
# E.g. this is used to find all things Randal has to attend
# to noted in CCF Team Member Nodes.
# There is a button for this on the CCF Team board (see Top page).

import os
import sys
import json
import traceback
from io import StringIO
from traceback import print_exc
import subprocess
import argparse

parser = argparse.ArgumentParser(description='Collect lists below a specified string from dependencies of node.')
parser.add_argument('-n', dest='node', type=str, help='Node for which to search dependencies')
parser.add_argument('-s', dest='header', type=str, help='String above list')
parser.add_argument('-d', dest='depth', type=int, default=1, help='Depth number of dependency levels to parse.')
parser.add_argument('-tagopen', type=str, help='Optional opening tag string for identifiers to collect')
parser.add_argument('-tagclose', type=str, help='Optional closing tag string for identifiers to collect')
parser.add_argument('-F', dest='outformat', type=str, help='Output format: json (default), html')
parser.add_argument('-nocgi', dest='nocgi', action="store_true", help='force command line mode')
parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
args = parser.parse_args()

use_cgi_path = False
if args.nocgi:
    running_as_cgi = False
    use_cgi_path = 'GATEWAY_INTERFACE' in os.environ
else:
    running_as_cgi = 'GATEWAY_INTERFACE' in os.environ

TEST_HTML='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Lists extracted from dependencies</title>
</head>
<body>
<button>Test output %s</button>.
</body>
<script type="text/javascript" src="/fzuistate.js"></script>
</html>
'''

version = "0.1.0-0.1"

results = {}

# Copied from fzcmdcalls.py to simplify using this as CGI script.
def try_subprocess_check_output(thecmdstring, resstore, config: dict):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        res = subprocess.check_output(thecmdstring, shell=True)

    except subprocess.CalledProcessError as cpe:
        if config['logcmderrors']:
            with open(config['cmderrlog'],'a') as f:
                f.write(f'Subprocess call ({thecmdstring}) caused exception.\n')
                f.write(f'Error output: {cpe.output.decode()}\n')
                f.write(f'Error code  : {cpe.returncode}\n')
                if (cpe.returncode>0):
                    f.write('Formalizer error: '+error.exit_status_code[cpe.returncode]+'\n')
        if config['verbose']:
            print('Subprocess call caused exception.')
            print('Error output: ',cpe.output.decode())
            print('Error code  : ',cpe.returncode)
            if (cpe.returncode>0):
                print('Formalizer error: ', error.exit_status_code[cpe.returncode])
        return cpe.returncode

    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

# Copied from Graphaccess.py to simplify using this as CGI script.
def get_node_data(node: str, config:dict=None):
    if config is None:
        config= {
            'verbose': False,
            'logcmdcalls': False,
            'cmdlog': '',
            'logcmderrors': False,
            'cmderrlog': '',
        }
    if running_as_cgi or use_cgi_path:
        thecmd = f'./fzgraphhtml -n {node} -o STDOUT -F json -e -q'
    else:
        thecmd = f'fzgraphhtml -n {node} -o STDOUT -F json -e -q'
    retcode = try_subprocess_check_output(thecmd, 'nodedata', config)
    if (retcode != 0):
        return None
    else:
        from json import loads
        return loads(results['nodedata'])

forbidden_set = set('\r\n ')

def is_non_empty(input_string:str)->bool:
    input_set = set(input_string)
    return bool(input_set.difference(forbidden_set))

def extract_identifier(opentag:str, closetag:str, content:str)->str:
    open_pos = content.find(opentag)
    if open_pos < 0:
        return ''
    open_pos += len(opentag)
    close_pos = content.find(closetag, open_pos)
    if close_pos < 0:
        return ''
    return content[open_pos:close_pos]

def extract_list_data(header:str, content:str)->list:
    h_pos = content.find(header)
    if h_pos < 0:
        return None
    ls_pos = content.find('<UL>', h_pos)
    if ls_pos < 0:
        return None
    ls_pos += 4
    le_pos = content.find('</UL>', ls_pos)
    if le_pos < 0:
        return None
    extracted = content[ls_pos:le_pos]
    extracted_list = extracted.split('<LI>')
    return [ line for line in extracted_list if is_non_empty(line) ]

def to_dependencies(header:str, data:dict, depth:int, maxdepth:int, collection:list, opentag:str, closetag:str):
    try:
        dependencies = data['dependencies'].split(' ')
    except Exception as e:
        print('Error: Missing dependencies: '+str(e))
        sys.exit(1)

    for dep in dependencies:
        if len(dep)>=16:
            dep_data = get_node_data(dep)
            list_data = extract_list_data(header, dep_data['node-text'])
            if list_data:
                if opentag and closetag:
                    identifier = extract_identifier(opentag, closetag, dep_data['node-text'])
                    collection.append([ dep, identifier, list_data ])
                else:
                    collection.append([ dep, list_data ])
            if depth < maxdepth:
                to_dependencies(header, dep_data, depth+1, maxdepth, collection, opentag, closetag)

HTML_HEAD='''<html>
<head>
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Lists extracted from dependencies</title>
<style>
table, th, td {
  border: 1px solid gray;
  border-collapse: collapse;
  padding: 15px;
}
</style>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<ul>
'''

HTML_TAIL='''
</ul>
<script>
// Sends a data argument from a button to a server-side CGI script and reloads the page.
function sendArgumentToCGI(buttonElement) {
    const argument = buttonElement.getAttribute('data-argument');
    const cgiscript = buttonElement.getAttribute('cgi-arg');
    const cgiUrl = '/cgi-bin/'+cgiscript+'?data=' + encodeURIComponent(argument);
    fetch(cgiUrl)
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.text(); // or .json() if your script returns JSON
        })
        .then(data => {
            console.log('Server response:', data);
            console.log('Reloading page...');
            window.location.reload();
        })
        .catch(error => {
            console.error('There was a problem with the fetch operation:', error);
            alert('An error occurred. Please check the console for details.');
        });
}
</script>
</body>
</html>
'''

SECTION='''<li><b>%s</b> <a href="/cgi-bin/fzlink.py?id=%s" target="_blank">%s</a><br>
<table><tbody>
%s
</tbody></table>
<p>
'''

LINE='''<tr>
<td><button onclick="sendArgumentToCGI(this)" cgi-arg="selected_to_nth.py" data-argument="%s:%s">Associate selected node</button></td>
<td><button onclick="sendArgumentToCGI(this)" cgi-arg="done_nth.py" data-argument="%s:%s">Done</button></td>
<td><button onclick="sendArgumentToCGI(this)" cgi-arg="delete_nth.py" data-argument="%s:%s">Delete</button></td>
<td>%s</td>
</tr>
'''

def list_section_to_html(node_id:str, name:str, lines:list)->str:
    linesstr = ''
    for i in range(len(lines)):
        linesstr += LINE % (i, node_id, i, node_id, i, node_id, lines[i])
    htmlstr = SECTION % (name, node_id, node_id, linesstr)
    return htmlstr

def to_html(collection:list):
    htmlstr = HTML_HEAD
    for list_data in collection:
        if len(list_data)>2:
            node_id, name, lines = list_data
        else:
            name = ''
            node_id, lines = list_data
        htmlstr += list_section_to_html(node_id, name, lines)
    htmlstr += HTML_TAIL
    print(htmlstr)

def process_as_cgi():
    print('Content-type:text/html\n')

    # Import modules for CGI handling
    try:
        import cgitb; cgitb.enable()
    except:
        pass
    import cgi
    sys.stderr = sys.stdout

    # Create instance of FieldStorage 
    form = cgi.FieldStorage()

    node = form.getvalue('node')
    header = form.getvalue('header')
    depth = form.getvalue('depth')
    if not depth:
        depth = 1
    else:
        depth = int(depth)
    tagopen = form.getvalue('tagopen')
    tagclose = form.getvalue('tagclose')

    try:
        data = get_node_data(node)
    except Exception as e:
        print('Error: Invalid Node data returned: '+str(e))
        sys.exit(1)
    if not data:
        print(f'Error: Failed to retrieve data for Node [{node}].')
        sys.exit(1)

    collection = []
    to_dependencies(header, data, 1, depth, collection, tagopen, tagclose)

    to_html(collection)

def process_as_cmdline():

    if not args.node:
        print('Error: No node specified.')
        sys.exit(1)

    if len(args.node)<16:
        print('Error: Invalid node ID.')
        sys.exit(1)

    if not args.header:
        print('Error: Missing header string.')
        sys.exit(1)

    try:
        data = get_node_data(args.node)
    except Exception as e:
        print('Error: Invalid Node data returned: '+str(e))
        sys.exit(1)
    if not data:
        print(f'Error: Failed to retrieve data for Node [{args.node}].')
        sys.exit(1)

    collection = []
    to_dependencies(args.header, data, 1, args.depth, collection, args.tagopen, args.tagclose)

    if args.outformat == 'html':
        to_html(collection)
    else:
        print(json.dumps(collection))

if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
