#!/usr/bin/python3
#
# team_state.py
#
# Randal A. Koene, 20250901
#
# Use dependencies list extractions to populate a template of team
# activities and support needed. This is useful for team reviews
# and meetings.
# There is a button for this on the CCF Team board (see Top page).

import os
import sys
import json
import traceback
from io import StringIO
from traceback import print_exc
import subprocess

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
<table><tbody>
'''

HTML_TAIL='''
</tbody></table>

<hr>
<p>[<a href="/index.html">fz: Top</a>]</p>

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

SECTION='''<tr><td colspan="%d"><b>%s</b> <a href="/cgi-bin/fzlink.py?id=%s" target="_blank">%s</a></td></tr>
%s
<p>
'''

LINE='''<tr>
<td><button class="tiny_button tiny_wider" onclick="sendArgumentToCGI(this)" cgi-arg="selected_to_nth.py" data-argument="%s:%s:%s">Associate selected node</button></td>
<td><button class="tiny_button tiny_wider" onclick="sendArgumentToCGI(this)" cgi-arg="done_nth.py" data-argument="%s:%s:%s">Done</button></td>
<td><button class="tiny_button tiny_wider" onclick="sendArgumentToCGI(this)" cgi-arg="delete_nth.py" data-argument="%s:%s:%s">Delete</button></td>
<td>%s</td>
</tr>
'''

NOLINES='''<tr>
<td></td>
</tr>
'''

HEADER='''<tr>
<td colspan="4">%s</td>
</tr>
'''

def list_subsection_to_html(node_id:str, header:str, lines:list)->str:
    linesstr = f'<table><tbody>\n'
    linesstr += HEADER % header
    if lines:
        for i in range(len(lines)):
            linesstr += LINE % (i, node_id, header, i, node_id, header, i, node_id, header, lines[i])
    else:
        linesstr += NOLINES
    htmlstr = linesstr + '</tbody></table>\n'
    return htmlstr

def subsections_section_to_html(node_id:str, name:str, subsection_tables:str, colspan:int)->str:
    htmlstr = '<tr>\n'+subsection_tables+'</tr>\n'
    return SECTION % (colspan, name, node_id, node_id, htmlstr)

def to_html(data:dict, headers:list):
    htmlstr = HTML_HEAD
    sorteddata = sorted(data.items())
    for sd in sorteddata:
        name, info = sd
        node_id = info['node']
        subsection_tables = ''
        for header in headers:
            if header in info:
                lines = info[header]
            else:
                lines = None
            subsection_tables += '<td>\n'+list_subsection_to_html(node_id, header, lines)+'</td>\n'
        htmlstr += subsections_section_to_html(node_id, name, subsection_tables, colspan=len(headers))
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
    headers = form.getvalue('headers')
    depth = form.getvalue('depth')
    if not depth:
        depth = 1
    else:
        depth = int(depth)
    tagopen = form.getvalue('tagopen')
    tagclose = form.getvalue('tagclose')

    IDtags = ''
    if tagopen:
        IDtags += f""" -tagopen '{tagopen}'"""
    if tagclose:
        IDtags += f""" -tagclose '{tagclose}'"""
    headers = headers.split(',')

    data = {}
    for header in headers:
        thecmd = f'''./lists_from_dependencies.py -nocgi -n {node} -s '{header}' -d {depth} -F json {IDtags}'''
        retcode, res = try_subprocess_check_output(thecmd)
        if retcode != 0:
            print(f'{thecmd} caused error: '+res)
            return
        header_data = json.loads(res)
        for d in header_data:
            dep, identifier, list_data = d
            if identifier not in data:
                data[identifier] = {}
            data[identifier]['node'] = dep
            data[identifier][header] = list_data

    to_html(data, headers)


def process_as_cmdline():
    import argparse

    parser = argparse.ArgumentParser(description='Extract team state.')
    parser.add_argument('-n', dest='node', type=str, help='Node for which to search dependencies')
    parser.add_argument('-s', dest='headers', type=str, help='Comma separated list of headers')
    parser.add_argument('-d', dest='depth', type=int, default=1, help='Depth number of dependency levels to parse.')
    parser.add_argument('-tagopen', type=str, help='Optional opening tag string for identifiers to collect')
    parser.add_argument('-tagclose', type=str, help='Optional closing tag string for identifiers to collect')
    parser.add_argument('-F', dest='outformat', type=str, default='json', help='Output format: json (default), html')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    if not args.node:
        print('Error: No node specified.')
        sys.exit(1)

    if len(args.node)<16:
        print('Error: Invalid node ID.')
        sys.exit(1)

    if not args.headers:
        print('Error: Missing header string.')
        sys.exit(1)

    IDtags = ''
    if args.tagopen:
        IDtags += f""" -tagopen '{args.tagopen}'"""
    if args.tagclose:
        IDtags += f""" -tagclose '{args.tagclose}'"""
    headers = args.headers.split(',')

    data = {}
    for header in headers:
        thecmd = f'''lists_from_dependencies.py -n {args.node} -s '{header}' -d {args.depth} -F json {IDtags}'''
        retcode, res = try_subprocess_check_output(thecmd)
        if retcode != 0:
            print(f'{thecmd} caused error: '+res)
            return
        header_data = json.loads(res)
        for d in header_data:
            dep, identifier, list_data = d
            if identifier not in data:
                data[identifier] = {}
            data[identifier]['node'] = dep
            data[identifier][header] = list_data

    if args.outformat == 'html':
        to_html(data, headers)
    else:
        print(json.dumps(data))


if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
