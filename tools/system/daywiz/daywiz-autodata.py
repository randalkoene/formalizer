#!/usr/bin/python3
#
# Randal A. Koene, 20240406
#
# This script is called with fzserverpq user permissions.
# For this reason, this script must be included on the
# predefined_CGIbg parameter in the config.json file of fzserverpq.

datafile = '/var/www/webdata/formalizer/daywiz_autodata.json'

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from datetime import datetime
import traceback
from io import StringIO
from traceback import print_exc
import subprocess

import json
import plotly.express as px
import plotly.io as pxio
import pandas as pd
import argparse

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


def extract_data(line_id, data_str, everything_after=False)->str:
    if isinstance(data_str, bytes):
        data_str = data_str.decode()
    data_start = data_str.find(line_id)
    if data_start < 0:
        return ''
    data_start += len(line_id)
    if everything_after:
        return data_str[data_start:]
    data_end = data_str.find('\n', data_start)
    if data_end < 0:
        return data_str[data_start:]
    return data_str[data_start:data_end]

def get_email_data(email_data_str:str)->dict:
    fromstart = email_data_str.find('From: ')
    if fromstart<0:
        return None
    fromstart += len('From: ')
    fromend = email_data_str.find('\n', fromstart)
    if fromend<0:
        return None
    details = {}
    details['from'] = email_data_str[fromstart:fromend]
    subjectstart = email_data_str.find('Subj: ', fromend)
    if subjectstart<0:
        return None
    subjectstart += len('Subj: ')
    subjectend = email_data_str.find('\n', subjectstart)
    if subjectend<0:
        return None
    details['subject'] = email_data_str[subjectstart:subjectend]
    return details

def extract_email_details(data_str)->list:
    if isinstance(data_str, bytes):
        data_str = data_str.decode()
    # Are email details included?
    emailspos = data_str.find('Emails from')
    if emailspos < 0:
        return None
    # Identify separated emails and collect details into list
    startpos = data_str.find('\n', emailspos)
    if startpos < 0:
        return None
    startpos += 1
    email_details = []
    while True:
        nextpos = data_str.find('-----', startpos)
        if nextpos < 0:
            break;
        email_data = get_email_data(data_str[startpos:nextpos])
        if email_data:
            email_details.append(email_data)
        startpos = data_str.find('\n', nextpos)
        if startpos < 0:
            break;
        startpos += 1
    return email_details

def parse_calendar_events(data_str)->dict:
    if isinstance(data_str, bytes):
        data_str = data_str.decode()
    return json.loads(data_str)

def parse_chunks_open_checkboxes(data_str)->dict:
    if isinstance(data_str, bytes):
        data_str = data_str.decode()
    return json.loads(data_str)

def main_cmdline():
    data = {}

    # Emails information
    thecmd = f'''email-info.py -s'''
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        print(f'{thecmd} caused error: '+res)
        return
    unread_emails = extract_data('Unread emails:', res)
    data['unread_emails'] = unread_emails
    email_details = extract_email_details(res)
    if email_details:
        data['email_details'] = email_details

    # Calendar information
    thecmd = f'''calendar-info.py'''
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        print(f'{thecmd} caused error: '+res)
        return
    calendar_events_str = extract_data('----\n', res, everything_after=True)
    calendar_events = parse_calendar_events(calendar_events_str)
    data['calendar_events'] = calendar_events

    # Chunks with open checkboxes
    thecmd = f"""fzloghtml -q -o STDOUT -N -1 20240401 -2 now -F json -z -x '[<]input type="checkbox"[ ]*[>]'"""
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        print(f'{thecmd} caused error: '+res)
        return
    chunks_open_checkboxes = parse_chunks_open_checkboxes(res)
    data['chunks_open_checkboxes'] = chunks_open_checkboxes['chunks_rendered']

    print(json.dumps(data))

def main_cgi():
    form = cgi.FieldStorage()

    cgioutput = form.getvalue('cgioutput')
    earliest = form.getvalue('earliest')
    latest = form.getvalue('latest')
    workselfwork = form.getvalue('workselfwork')

if __name__ == "__main__":
    if running_as_cgi:
        main_cgi()
    else:
        main_cmdline()
