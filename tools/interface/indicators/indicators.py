#!/usr/bin/python3
#
# indicators.py
#
# Randal A. Koene, 20260105
#
# Common entry point for indicators checking and updating.
# This is used by ONOPEN/ONCLOSE calls within Nodes.
# See Readme.md for more information.

# std
import os
import sys
import json
import argparse
import subprocess
from datetime import datetime
#import re
#import pty


# Standardized expectations.
#userhome = os.getenv('HOME')
#fzuserbase = userhome + '/.formalizer'
#fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
#fzsetupconfig = fzsetupconfigdir+'/config.json'
#indicatorsconfigdir = fzuserbase+'/config/indicators.py'
#indicatorsconfig = indicatorsconfigdir+'/config.json'

data_path = '/var/www/html/formalizer/indicators.json'

data = {
    "WeekGoals_Overdue": {
            "state": False,
            "t_update": "202506010000",
        }
}

results = {}

# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring, resstore):
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print(res)
        return 0

# Handle the case where even fzsetup.py does not have a configuration file yet.
# try:
#     with open(fzsetupconfig) as f:
#         config = json.load(f)

# except FileNotFoundError:
#     print('Configuration files missing. Please run fzsetup.py first!\n')
#     exit(1)

# else:
#     assert len(config) > 0

# Enable us to import standardized Formalizer Python components.
# fzcorelibdir = config['sourceroot'] + '/core/lib'
# fzcoreincludedir = config['sourceroot'] + '/core/include'
# sys.path.append(fzcorelibdir)
# sys.path.append(fzcoreincludedir)

# core components
#import Graphpostgres
#import coreversion

version = "0.1.0-0.1"

# local defaults
#config['something'] = 'some/kind/of/default'

# replace local defaults with values from ~/.formalizer/config/indicators.py/config.json
#try:
#    with open(indicatorsconfig) as f:
#        config += json.load(f)
#
#except FileNotFoundError:
#    print('Configuration files for indicators missing. Continuing with defaults.\n')


def parse_options():
    theepilog = ('See the Readme.md in the indicators.py source directory for more information.\n')

    parser = argparse.ArgumentParser(description='{{ brief_title }}.',epilog=theepilog)
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: randalk)')
    parser.add_argument('-T', '--tests', dest='tests', action="store_true", help='Run all indicator tests')
    parser.add_argument('-W', '--weekgoals', dest='weekgoals', action="store_true", help='Weekly Main Goals has been updated')
    #parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = 'randalk'
    #if args.verbose:
    #    config['verbose'] = True

    print('Working with the following targets:\n')
    print(f'  Formalizer Postgres database name   : {args.dbname}')
    print(f'  Formalizer user or group schema name: {args.schemaname}\n')

    #choice = input('Is this correct? (y/N) \n')
    #if (choice != 'y'):
    #    print('Ok. You can try again with different command arguments.\n')
    #    exit(0)

    return args


def some_function():
    pass


# def a_function_that_calls_subprocess(some_arg, resstore):
#     retcode = try_subprocess_check_output(f"someprogram -A '{some_arg}'", resstore)
#     if (retcode != 0):
#         print(f'Attempt to do something failed.')
#         exit(retcode)


# def a_function_that_spawns_a_call_in_pseudo_TTY(some_arg):
#     retcode = pty.spawn(['someprogram','-A', some_arg])
#     return retcode


# def a_function_that_requests_input():
#     shortlist_desc = results['shortlistdesc']
#     shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
#     for (number, line) in enumerate(shortlist_vec):
#         print(f' {number}: {line}')

#     choice = input('[D]efault same Node as chunk, or [0-9] from shortlist, or [?] browse? ')
#     if (choice == 'd'):
#         node = '' # default

#     return node

def Read_DataFile():
    global data
    try:
        with open(data_path, 'r') as f:
            data = json.load(f)
    except:
        pass

def Update_DataFile():
    try:
        with open(data_path, 'w') as f:
            json.dump(data, f)
    except Exception as e:
        print('Update error: '+str(e))

# Called by ONCLOSE in 20240603105017.1.
def WeekGoals_updated():
    data['WeekGoals_Overdue']['state'] = False
    data['WeekGoals_Overdue']['t_update'] = datetime.now().strftime("%Y%m%d%H%M")
    Update_DataFile()

# Called periodically by overall indicators test.
# Data is checked frequently by Log page.
def WeekGoals_check():
    t_str = data['WeekGoals_Overdue']['t_update']
    t = datetime.strptime(t_str, "%Y%m%d%H%M")
    t_now = datetime.now()
    t_elapsed = t_now - t
    t_days = t_elapsed.total_seconds()/(24*60*60)
    data['WeekGoals_Overdue']['state'] = t_days > 7.0
    #data['WeekGoals_Overdue']['state'] = t_elapsed.total_seconds() > 60.0

def Indicators_Tests():
    WeekGoals_check()
    # *** Put other checks here...

    Update_DataFile()

def Ensure_DataFile_Exists_World_Writable():
    if not os.path.exists(data_path):
        Update_DataFile()
        os.chmod(data_path, 0o666) # world readable/writeable

if __name__ == '__main__':

    Ensure_DataFile_Exists_World_Writable()

    Read_DataFile()

    #core_version = coreversion.coreversion()
    indicators_long_id = "Interface:Indicators" + f" v{version}" # (core v{core_version})"

    print(indicators_long_id+"\n")

    args = parse_options()

    if args.tests:
        Indicators_Tests()
    elif args.weekgoals:
        WeekGoals_updated()

sys.exit(0)
