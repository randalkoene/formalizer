#!/usr/bin/python3
#
# team_doc.py
#
# Randal A. Koene, 20260108
#
# Use this to get CCF Team Member information and generate a document.

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

def get_team_data(node):
    if running_as_cgi:
        thecmd = f'./nodeboard -G -n {node} -Z -T -B 79 -J -o STDOUT -q'
    else:
        thecmd = f'nodeboard -G -n {node} -Z -T -B 79 -J -o STDOUT -q'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        print(f'{thecmd} caused error: '+res)
        return
    data = json.loads(res)
    return data

HTML_PAGE = '''<html>
<head>
<title>CCF Team</title>
%s
</head>
<body>
<h1>CCF Team</h1>
Number of team members listed: %s
%s
</body>
</html>
'''

DETAILS_STYLE='''<style>
details > summary {
  padding: 4px;
  width: 100%%;
  background-color: #eeeeee;
  border: none;
  box-shadow: 1px 1px 2px #bbbbbb;
  cursor: pointer;
}
</style>
'''

TABLE_STYLE='''<style>
  table {
    /* Basic table styling */
    border-collapse: collapse; /* Merges borders for a cleaner look */
    width: 100%;
  }

  th, td {
    /* Basic cell styling */
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
  }

  th {
    /* Header specific styling */
    background-color: #04AA6D;
    color: white;
  }

  /* Target even-numbered rows in the table body */
  tbody tr:nth-child(even) {
    background-color: #f2f2f2; /* Light gray background for even rows */
  }
  
  /* Optional: Add a hover effect for better user experience */
  tbody tr:hover {
    background-color: #ddd;
  }
</style>
'''

def find_from_to(content:str, fromtag:str, totag:str, search_from:int=0)->tuple:
    start = content.find(fromtag, search_from)
    if start<0:
        return None, None
    end = content.find(totag, start+len(fromtag))
    if end<0:
        return None, None
    return start, end+len(totag)

def extract_shareable(rawcontent:str)->str:
    prunestart, pruneend = find_from_to(rawcontent, '<h4>CCF career path</h4>', '<b>Coordination notes</b>:')
    if prunestart is None:
        return rawcontent
    rawcontent = rawcontent[:prunestart]+rawcontent[pruneend:]
    prunestart = rawcontent.find('<h4>Immediate activity</h4>')
    if prunestart<0:
        return rawcontent
    return rawcontent[:prunestart]

DETAILS_SUMMARY='''<details>
  <summary>%s</summary>
  %s
</details>
'''

MINIMAL='''<tr><td>%s</td><td>%s</td><td>%s</td></tr>
'''

def extract_minimal(content:str)->str:
    shareable = extract_shareable(content)
    h3start, h3end = find_from_to(shareable, '<h3>', '</h3>')
    if h3start is None:
        return shareable
    name = shareable[h3start+len('<h3>'):h3end-len('</h3>')]
    expstart, expend = find_from_to(shareable, 'Experience Summary:', '\n', h3end)
    if expstart is None:
        return shareable
    experience = shareable[expstart+len('Experience Summary:'):expend]
    ccfstart, ccfend = find_from_to(shareable, 'Main CCF Skill Application:', '\n', expend)
    if ccfstart is None:
        return shareable
    ccfskill = shareable[ccfstart+len('Main CCF Skill Application:'):ccfend]
    return MINIMAL % ('<b>'+name+'</b>', experience, ccfskill)

def details_summary(content:str)->str:
    h3start, h3end = find_from_to(content, '<h3>', '</h3>')
    if h3start is None:
        return content
    summary = content[h3start+len('<h3>'):h3end-len('</h3>')]
    details = content[h3end:]
    return DETAILS_SUMMARY % (summary, details)

def process_common(node, outformat, shareable, minimal, verbose):
    data = get_team_data(node)
    if outformat == "html":
        num_listed = 0
        team_html = ''
        columns = sorted(data['the-columns'], key=lambda item: item['node-text'])
        for team_node in columns:
            if team_node['node-text'].find('MILESTONE:')<0:
                if minimal:
                    node_html = extract_minimal(team_node['node-text'])
                elif shareable:
                    node_html = details_summary(extract_shareable(team_node['node-text']))
                else:
                    node_html = details_summary(team_node['node-text'])
                team_html += node_html + '\n<p>\n'
                num_listed += 1
        if minimal:
            print(HTML_PAGE % (TABLE_STYLE, str(num_listed), '<table><tbody>\n'+team_html+'</tbody></table>\n'))
        else:
            print(HTML_PAGE % (DETAILS_STYLE, str(num_listed), team_html))
    else:
        print(data)

def process_as_cgi():
    print('Content-type:text/html\n')

    form = cgi.FieldStorage()
    node = form.getvalue('node')
    if not node:
        node="20250822084723.1"
    shareable = form.getvalue('shareable')
    minimal = form.getvalue('minimal')

    process_common(node, 'html', shareable, minimal, False)

def process_as_cmdline():
    import argparse

    parser = argparse.ArgumentParser(description='Extract team state.')
    parser.add_argument('-n', dest='node', type=str, default="20250822084723.1", help='Node for which to search dependencies (default: 20250822084723.1)')
    parser.add_argument('-F', dest='outformat', type=str, default='html', help='Output format: html (default)')
    parser.add_argument('-S', dest='shareable', action="store_true", help='Sharable information only')
    parser.add_argument('-M', dest='minimal', action="store_true", help='Minimal information')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    process_common(args.node, args.outformat, args.shareable, args.minimal, args.verbose)


if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
