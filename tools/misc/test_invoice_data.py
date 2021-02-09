#!/usr/bin/python3
#
# test_invoice_data.py
#
# Randal A. Koene, 20210208
#
# Quick and dirty test to extract time data from the Log, mapping to
# a consulting topic for invoicing.

import os
import sys
import json
import argparse

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'

# Handle the case where even fzsetup.py does not have a configuration file yet.
try:
    with open(fzsetupconfig) as f:
        config = json.load(f)

except FileNotFoundError:
    print('Configuration files missing. Please run fzsetup.py first!\n')
    exit(1)

else:
    assert len(config) > 0

# Enable us to import standardized Formalizer Python components.
fzcorelibdir = config['sourceroot'] + '/core/lib'
fzcoreincludedir = config['sourceroot'] + '/core/include'
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)

import coreversion
from ansicolorcodes import *
from fzcmdcalls import *
from Logaccess import *
from Graphaccess import *

version = "0.1.0-0.1"
config['customtemplate'] = '/tmp/customtemplate'
config['cmderrorreviewstr'] = ''
if config['logcmderrors']:
    cmderrlogstr = config['cmderrlog']
    config['cmderrorreviewstr'] = f'\nYou may review the error(s) in: {ANSI_yb}{cmderrlogstr}{ANSI_nrm}'



def parse_options():
    #theepilog = ('See the Readme.md in the fztask.py source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Invoice data.') #,epilog=theepilog)
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    parser.add_argument('-D', '--days', dest='numdays', help='number of days (default = 10)')
    parser.add_argument('-t', '--topic', dest='topic', help='topic tag to invoice (default = neurobotx)')
    parser.add_argument('-m', '--multiplier', dest='multiplier', help='multiplication factor (default = 1.0)')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = os.getenv('USER')
    if args.verbose:
        config['verbose'] = True
        print('Verbose mode.', flush=True)
    if not args.numdays:
        args.numdays = 10
    if not args.topic:
        args.topic = 'neurobotx'
    if not args.multiplier:
        args.multiplier = '1.0'

    print('Working with the following targets:\n')
    print(f'  Formalizer Postgres database name   : {args.dbname}')
    print(f'  Formalizer user or group schema name: {args.schemaname}\n')

    #choice = input('Is this correct? (y/N) \n')
    #if (choice != 'y'):
    #    print('Ok. You can try again with different command arguments.\n')
    #    exit(0)

    return args


def collect_data(numdays: int):
    if not get_Log_days_data(numdays, config):
        print('Unable to obtain data.')
        sys.exit(1)
    logdatavec = results['recentlogdata'].decode().split('\n')
    print(f'Number of days   : {numdays}')
    print(f'Number of entries: {len(logdatavec)}')
    return logdatavec


def filter_data(logdatavec, topictag):
    filtered = []
    for dataline in logdatavec:
        linedata = dataline.split()
        if (len(linedata) == 4):
            ttag = get_main_topic(linedata[3], config)
            if (ttag == topictag):
                filtered.append(linedata)
    print(f'Filtered entries: {len(filtered)}')
    return filtered


def get_date(yyyymmddhhmmstr: str):
    return yyyymmddhhmmstr[0:8]


def show_per_day(filtered, multiplier: float):
    datedata = {}
    for dateline in filtered:
        datestr = get_date(dateline[0])
        if (datestr in datedata):
            datedata[datestr] += int(dateline[2])
        else:
            datedata[datestr] = int(dateline[2])
    for key, val in datedata.items():
        print(f'{key} : {int(multiplier*val)} minutes = {multiplier*val/60:.3} hours')
    return datedata


def show_total(datedata, multiplier: float):
    total = 0
    for key, val in datedata.items():
        total += val
    print(f'Total: {int(multiplier*total)} minutes = {multiplier*total/60:.3} hours')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    fztask_long_id = "Test:LogData:Consulting" + f" v{version} (core v{core_version})"

    print(fztask_long_id+"\n")

    args = parse_options()

    logdatavec = collect_data(args.numdays)

    filtered = filter_data(logdatavec, args.topic)

    datedata = show_per_day(filtered, float(args.multiplier))

    show_total(datedata, float(args.multiplier))

    sys.exit(0)
