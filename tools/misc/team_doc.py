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

def get_team_data(args):
    if running_as_cgi:
        thecmd = ''
    else:
        thecmd = f'nodeboard -G -n {args.node} -Z -T -B 79 -J -o STDOUT -q'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        print(f'{thecmd} caused error: '+res)
        return
    data = json.loads(res)
    return data

HTML_PAGE = '''<html>
<body>
%s
</body>
</html>
'''

def process_as_cgi():
    print('Content-type:text/html\n')

def process_as_cmdline():
    import argparse

    parser = argparse.ArgumentParser(description='Extract team state.')
    parser.add_argument('-n', dest='node', type=str, default="20250822084723.1", help='Node for which to search dependencies (default: 20250822084723.1)')
    parser.add_argument('-F', dest='outformat', type=str, default='html', help='Output format: html (default)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    data = get_team_data(args)
    if args.outformat == "html":
        team_html = ''
        for team_node in data['the-columns']:
            node_html = team_node['node-text']
            team_html += node_html + '\n<p>\n'
        print(HTML_PAGE % team_html)
    else:
        print(data)

if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
