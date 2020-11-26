#!/usr/bin/python3
#
# {{ this }}.py
#
# Randal A. Koene, {{ thedate }}
#
# {{ brief_title }}. See Readme.md for more information.

# std
import os
import sys
import json
import argparse
import subprocess
import re
import pty


# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'
#{{ this }}configdir = fzuserbase+'/config/{{ this }}.py'
#{{ this }}config = {{ this }}configdir+'/config.json'

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

# core components
import Graphpostgres
import coreversion

version = "{{ version }}"

# local defaults
#config['something'] = 'some/kind/of/default'

# replace local defaults with values from ~/.formalizer/config/{{ this }}.py/config.json
#try:
#    with open({{ this }}config) as f:
#        config += json.load(f)
#
#except FileNotFoundError:
#    print('Configuration files for {{ this }} missing. Continuing with defaults.\n')


def parse_options():
    theepilog = ('See the Readme.md in the {{ this }}.py source directory for more information.\n')

    parser = argparse.ArgumentParser(description='{{ brief_title }}.',epilog=theepilog)
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = os.getenv('USER')
    if args.verbose:
        config['verbose'] = True

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


def a_function_that_calls_subprocess(some_arg, resstore):
    retcode = try_subprocess_check_output(f"someprogram -A '{some_arg}'", resstore)
    if (retcode != 0):
        print(f'Attempt to do something failed.')
        exit(retcode)


def a_function_that_spawns_a_call_in_pseudo_TTY(some_arg):
    retcode = pty.spawn(['someprogram','-A', some_arg])
    return retcode


def a_function_that_requests_input():
    shortlist_desc = results['shortlistdesc']
    shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
    for (number, line) in enumerate(shortlist_vec):
        print(f' {number}: {line}')

    choice = input('[D]efault same Node as chunk, or [0-9] from shortlist, or [?] browse? ')
    if (choice == 'd'):
        node = '' # default

    return node


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    logentry_long_id = "{{ module_id }}" + f" v{version} (core v{core_version})"

    print(logentry_long_id+"\n")

    args = parse_options()

    some_function()

sys.exit(0)
