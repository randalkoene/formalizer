#!/usr/bin/python3
#
# today_btf.py
#
# Randal A. Koene, 20260123
#
# Quick feedback about time dedicated by BTF category.

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
        errorstr += f'Subprocess call: {str(cpe.cmd)}\n'+errorstr+'\n'
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
        if isinstance(res, bytes):
            return 0, res.decode()
        return 0, res

def get_BTF_data(prefix:str):
    thecmd = f'{prefix}fzloghtml -q -o STDOUT -F json -j'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        return f'Error: Failed to execute {thecmd}', None
    try:
        data = json.loads(res)
        return '', data
    except Exception as e:
        return f'Error: {str(e)}', None

def output_json_work(btfseconds:dict):
    if 'w' in btfseconds:
        workhours = btfseconds['w']/3600.0
    else:
        workhours = 0.0
    print('WORK: %.2f' % workhours)

def output_json_allbtf(btfseconds:dict):
    print('JSON allbtf not implemented yet')

def output_html_work(btfseconds:dict):
    print('HTML WORK not implemented yet')

def output_html_allbtf(btfseconds:dict):
    print('HTML allbtf not implemented yet')

def process_common(data:dict, outformat:str, allbtf, verbose):
    if not isinstance(data, dict):
        print('Error data is not a dict')
        sys.exit(0)
    if 'chunks' not in data:
        print('Error no chunks in data')
        sys.exit(0)

    btfseconds = {}

    for chunk in data['chunks']:
        category = chunk['category']
        if category not in btfseconds:
            btfseconds[category] = 0.0
        btfseconds[category] += chunk['seconds']
    
    if allbtf:
        if outformat=='html':
            output_html_allbtf(btfseconds)
        else:
            output_json_allbtf(btfseconds)
    else:
        if outformat=='html':
            output_html_work(btfseconds)
        else:
            output_json_work(btfseconds)

def process_as_cgi():
    print('Content-type:text/html\n')

    form = cgi.FieldStorage()
    outformat = form.getvalue('outformat')
    if not outformat:
        outformat = "json"
    allbtf = form.getvalue('allbtf')
    verbose = form.getvalue('verbose')

    error, data = get_BTF_data(prefix='./')
    if data is None:
        print(error)
        sys.exit(0)

    process_common(data, outformat, allbtf, verbose)

def process_as_cmdline():
    import argparse

    parser = argparse.ArgumentParser(description='Time in BTF categories.')
    parser.add_argument('-F', dest='outformat', type=str, default='json', help='Output format: json (default)')
    parser.add_argument('-A', dest='allbtf', action="store_true", help='Include all BTF categories (default: WORK only)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    args = parser.parse_args()

    error, data = get_BTF_data(prefix='')
    if data is None:
        print(error)
        sys.exit(0)

    process_common(data, args.outformat, args.allbtf, args.verbose)


if __name__ == '__main__':

    if running_as_cgi:
        process_as_cgi()
    else:
        process_as_cmdline()
    sys.exit(0)
