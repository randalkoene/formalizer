#!/usr/bin/python3
#
# fzcatchup.py
#
# Randal A. Koene, 20210113
#
# Catch up the Log. See Readme.md for more information.

# std
import os
import sys
import json
import argparse
import subprocess
import re
import pty
from datetime import datetime


# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'
#addnodeconfigdir = fzuserbase+'/config/addnode.py'
#addnodeconfig = addnodeconfigdir+'/config.json'

results = {}

# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring, resstore):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Subprocess call caused exception.')
        if config['verbose']:
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
fzinterfacedir = config['sourceroot'] + '/tools/interface'
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)
sys.path.append(fzinterfacedir)

config['logcmdcalls'] = False

# core components
import Graphpostgres
import coreversion
from addnode import addnode

version = "0.1.0-0.1"

# local defaults
#config['something'] = 'some/kind/of/default'

# replace local defaults with values from ~/.formalizer/config/addnode.py/config.json
#try:
#    with open(addnodeconfig) as f:
#        config += json.load(f)
#
#except FileNotFoundError:
#    print('Configuration files for addnode missing. Continuing with defaults.\n')


def parse_options():
    theepilog = ('See the Readme.md in the fzcatchup source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Catch up the Log interactively.',epilog=theepilog)
    parser.add_argument('-S', '--simulate', dest='simulate', action="store_true", help='simulated calls to addnode and fzlog')
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


def lightgreen_ansi():
    print(u'\u001b[92mLight green', end='')


def lightgray_ansi():
    print(u'\u001b[37mLight gray', end='')


def alert_ansi():
    print(u'\u001b[31m', end='')


def exit_error(retcode, errormessage):
    if (retcode != 0):
        alert_ansi()
        print('\n'+errormessage+'\n')
        lightgray_ansi()
        exitenter = input('Press ENTER to exit...')
        sys.exit(retcode)


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


def TimeStamp(t):
    return t.strftime("%Y%m%d%H%M%S")


def is_int(n):
    try:
        int(n)
    except ValueError:
        return False
    return True

def is_float(n):
    try:
        float(n)
    except ValueError:
        return False
    return True

def is_TimeStamp(s):
    if not is_int(s):
        return False
    if (len(s) != 12):
        return False
    try:
        datetime.strptime(s, '%Y%m%d%H%M')
    except ValueError:
        return False
    return True


def browse_for_Node():
    print('Use the browser to select a node.')
    #retcode = pty.spawn([config['localbrowser'],'http://localhost/select.html'])
    thecmd = config['localbrowser'] + ' http://localhost/select.html'
    retcode = try_subprocess_check_output(thecmd, 'browsed')
    exit_error(retcode, 'Attempt to browse for Node selection failed.')
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'selected' -F node -N 1 -e -q",'selected')
    exit_error(retcode, 'Attempt to get selected Node failed.')
    #print(f'Selected: {results["selected"]}')
    if results['selected']:
        return results['selected'][0:16]
    else:
        return ''


def collect_entry_text():
    entryfile = '/tmp/fzcatchup-entry.html'
    try:
        os.remove(entryfile)
    except OSError:
        pass
    retcode = try_subprocess_check_output(f"{config['editor']} {entryfile}", '')
    exit_error(retcode, 'Attempt to edit entry text failed.')
    return entryfile


def add_log_entry_same_chunk(entryfile: str, args):
    thecmd = f'fzlog -e -f {entryfile}'
    if args.simulate:
        thecmd += ' -Q sim'
    if config['verbose']:
        thecmd += ' -V'
    else:
        thecmd += ' -q'
    if args.simulate:
        print(f'Simulating, with thecmd = {thecmd}')
    else:
        retcode = try_subprocess_check_output(thecmd, '')
        exit_error(retcode, 'Attempt to make Log entry failed.')


def get_most_recent_opentime():
    thecmd="fzloghtml -R -o STDOUT -N -F raw -q"
    retcode = try_subprocess_check_output(thecmd, 'Tprev')
    exit_error(retcode, 'Unable to get most recent chunk open time.')
    Tprev_str = results['Tprev'].decode()
    Tprev = Tprev_str[0:12]
    return Tprev


def get_chunk_end_stamp():
    Tprev = get_most_recent_opentime()
    Tstamp = ''
    while not Tstamp:
        print(f'\nOpening time stamp of most recent Log chunk is {Tprev}.')
        Tstamp = input("End date-and-time stamp of most recent Log chunk, which corresponds\nto the start time of this new chunk and entry (YYYYmmddHHMM): ")
        if (not is_TimeStamp(Tstamp)):
            print('It must be a proper date and time stamp (e.g. 202101120813).')
            Tstamp = ''
        else:
            Tdiff_minslike = int(Tstamp) - int(Tprev)
            if (Tdiff_minslike <= 0):
                print(f'The chunk closing date-time must be later than {Tprev}.')
                Tstamp = ''
            else:
                if (Tdiff_minslike > 15000):
                    diffok = input(f'\nThe previous chunk size from {Tprev} to {Tstamp} seems pretty big.\nAre you sure? (y/N) ')
                    if (diffok != 'y'):
                        print("Ok, let's try again.")
                        Tstamp = ''
    return Tstamp


def get_new_Node(args):
    if config['verbose']:
        print('\nCalling addnode...')
    fzgraphcmd = addnode.collect_node_data()
    if args.simulate:
        node_id = addnode.simulate_make_node(fzgraphcmd)
    else:
        node_id = addnode.make_node(fzgraphcmd)
    return node_id.decode()


def get_existing_Node():
    if config['verbose']:
        print('\nUsing \'selected\' Named Node List to pick Node...')
    node_id = browse_for_Node()
    return node_id.decode()


def new_log_chunk_and_entry(entryfile: str, T_chunkend: str, node_id: str, args):
    if config['verbose']:
        print(f'\nCreating new Log chunk for Node {node_id}...')
    thecmd = f'fzlog -t {T_chunkend} -c {node_id}'
    if args.simulate:
        thecmd += ' -Q sim'
    if config['verbose']:
        thecmd += ' -V'
    else:
        thecmd += ' -q'
    if args.simulate:
        print(f'Simulating, with thecmd = {thecmd}')
    else:
        retcode = try_subprocess_check_output(thecmd, '')
        exit_error(retcode, 'Attempt to make new Log chunk failed.')    
    if config['verbose']:
        print('\nAdding Log entry...')
    add_log_entry_same_chunk(entryfile, args)


def catchup(args):
    print('\nPlease copy or write the Log entry text for the next catch-up\nentry into the editor window, save and exit...\n')
    entryfile = collect_entry_text()
    newchunk = input('\nWill this belong to a new Log chunk? (Y/n) ')
    if (newchunk == 'n'):
        add_log_entry_same_chunk(entryfile, args)
    else:
        T_chunkend = get_chunk_end_stamp()
        existingnode = input('\nWill the Log chunk belong to a Node that already exists? (Y/n) ')
        if (existingnode == 'n'):
            node_id = get_new_Node(args)
        else:
            node_id = get_existing_Node()
        if config['verbose']:
            print('The Node ID: '+node_id)
        new_log_chunk_and_entry(entryfile, T_chunkend, node_id, args)


def iterate_catchup(args):
    while True:
        catchup(args)
        lightgreen_ansi()
        catchupmore = input('\nMore to catch up? (Y/n) ')
        lightgreen_ansi()
        if (catchupmore == 'n'):
            break


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    fzcatchup_long_id = "Interface:Log:Catchup" + f" v{version} (core v{core_version})"

    print(fzcatchup_long_id+"\n")

    args = parse_options()

    iterate_catchup(args)

    print('Completed fzcatchup.')

    sys.exit(0)
