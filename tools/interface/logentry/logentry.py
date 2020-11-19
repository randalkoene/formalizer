#!/usr/bin/python3
#
# logentry.py
#
# Randal A. Koene, 20201119
#
# Command line interface to fzlog. See Readme.md for more information.

# std
import os
import sys
import json
import argparse
import subprocess

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'
# *** can add logentryconfigdir and logentryconfig here

# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring):
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
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

version = "0.1.0-0.1"

# local defaults
config['contenttmpfile'] = '/tmp/logentry.html'
config['editor'] = 'emacs'

# *** here you could replace local defaults with values from ~/.formalizer/config/logentry/config.json



def parse_options():
    theepilog = ('See the Readme.md in the logentry source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Make a Log entry.',epilog=theepilog)
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


def make_content_file():
    emptystr = ''
    with open(config['contenttmpfile','w']) as f:
        f.write(emptystr)


def edit_content_file():
    retcode = try_subprocess_check_output(f"{config['editor']} {config['contenttmpfile']}")
    if (retcode != 0):
        print(f'Attempt to edit content file failed.')
        exit(retcode)
    
    with open(config['contenttmpfile'],'r') as f:
        entrycontent = f.read()
    
    return entrycontent


def entry_belongs_to_same_or_other_Node():
    # *** get short-list from Named Node Lists
    # *** show options, including default, short-list and browse
    # *** get and act on choice
    print('NOT YET IMPLEMENTED!')
    node = ''
    return node


def send_to_fzlog(node, entrycontent):
    # *** if default then send to fzlog without Node specification
    # *** otherwise, send to fzlog with Node specification
    print('NOT YET IMPLEMENTED!')


def transition_dil2al_polldaemon_request(node, entrycontent):
    # *** if this is being used during the transition then do this
    # *** put makenote request with logentry data and possible Node ID where dil2al-polldaemon.sh will find it
    print('NOT YET IMPLEMENTED!')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    logentry_long_id = f"Formalizer:Interface:CommandLine:LogEntry v{version} (core v{core_version})"

    print(logentry_long_id+"\n")

    args = parse_options()

    make_content_file()

    entrycontent = edit_content_file()
    if len(entrycontent)<1:
        print('Empty Log entry.')
        exit(1)

    node = entry_belongs_to_same_or_other_Node()

    send_to_fzlog(node, entrycontent)

    transition_dil2al_polldaemon_request(node, entrycontent)

exit(0)
