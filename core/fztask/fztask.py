#!/usr/bin/python3
#
# fztask.py
#
# Randal A. Koene, 20201125
#
# Task control. See Readme.md for more information.

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
#fztaskconfigdir = fzuserbase+'/config/fztask.py'
#fztaskconfig = fztaskconfigdir+'/config.json'

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

version = "0.1.0-0.1"

# local defaults
#config['something'] = 'some/kind/of/default'

# replace local defaults with values from ~/.formalizer/config/fztask.py/config.json
#try:
#    with open(fztaskconfig) as f:
#        config += json.load(f)
#
#except FileNotFoundError:
#    print('Configuration files for fztask missing. Continuing with defaults.\n')


def parse_options():
    theepilog = ('See the Readme.md in the fztask.py source directory for more information.\n')

    parser = argparse.ArgumentParser(description='{{ brief_title }}.',epilog=theepilog)
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    parser.add_argument('-T', '--emulate', dest='T_emulate', help='emulate time')

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

    if args.T_emulate:
        print(f'\nEmulated Time: {args.T_emulate}\n')
    else:
        print('\nUsing actual time.\n')

    #choice = input('Is this correct? (y/N) \n')
    #if (choice != 'y'):
    #    print('Ok. You can try again with different command arguments.\n')
    #    exit(0)

    return args


def make_log_entry():
    retcode = try_subprocess_check_output("logentry", 'logentry_res')
    if (retcode != 0):
        print(f'Attempt to make Log entry failed.')
        sys.exit(retcode)


#def a_function_that_calls_subprocess(some_arg, resstore):
#    retcode = try_subprocess_check_output(f"someprogram -A '{some_arg}'", resstore)
#    if (retcode != 0):
#        print(f'Attempt to do something failed.')
#        exit(retcode)
#
#
#def a_function_that_spawns_a_call_in_pseudo_TTY(some_arg):
#    retcode = pty.spawn(['someprogram','-A', some_arg])
#    return retcode


def new_or_close_chunk():
    choice = input('[S]tart a Log chunk for the next task, or merely [c]lose the chunk? ')
    if (choice != 'c'):
        choice = 'S'
        print('Starting the next task chunk...')
    else:
        print('Closing Log chunk without immediately starting another...')

    return choice


def close_chunk():
    retcode = try_subprocess_check_output("fzlog -C", 'fzlog_res')
    if (retcode != 0):
        print(f'Attempt to close Log chunk failed.')
        sys.exit(retcode)


def get_updated_shortlist():
    retcode = try_subprocess_check_output(f"fzgraphhtml -u -L 'shortlist' -F node -e -q", 'shortlistnode')
    if (retcode != 0):
        print(f'Attempt to get "shortlist" Named Node List node data failed.')
        exit(retcode)
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'shortlist' -F desc -x 60 -e -q", 'shortlistdesc')
    if (retcode != 0):
        print(f'Attempt to get "shortlist" Named Node List description data failed.')
        exit(retcode)


def browse_for_Node():
    print('Use the browser to select a node.')
    retcode = pty.spawn(['w3m','http://localhost/select.html'])
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'selected' -F node -N 1 -e -q",'selected')
    if (retcode != 0):
        print(f'Attempt to get selected Node failed.')
        exit(retcode)
    print(f'Selected: {results["selected"]}')
    if results['selected']:
        return results['selected'][0:16]
    else:
        return ''


def select_Node_for_Log_chunk():
    get_updated_shortlist()
    shortlist_nodes = results['shortlistnode']
    shortlist_desc = results['shortlistdesc']
    print('Short-list of Nodes for the New Log Chunk:')
    shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
    for (number, line) in enumerate(shortlist_vec):
        print(f' {number}: {line}')

    choice = input('[0-9] from shortlist, or [?] to browse: ')
    if (choice == '?'):
        node = browse_for_Node()
    else:
        if ((choice >= '0') & (choice <= '9')):
                node = shortlist_nodes.splitlines()[int(choice)]
        else:
            node = '' # default
    if node:
        node = node.decode()
        print(f'Log chunk will belong to Node {node}.')
    else:
        print(f'We cannot make a new Log chunk without a Node.')
    return node


def next_chunk():
    # Closing the previous chunk is automatically done as part of this in fzlog.
    node = select_Node_for_Log_chunk()
    if not node:
        print('Attempt to select Node for new Log chunk failed.')
        sys.exit(1)

    retcode = try_subprocess_check_output(f"fzlog -c {node}", 'fzlog_res')
    if (retcode != 0):
        print(f'Attempt to close Log chunk failed.')
        sys.exit(retcode)

    print(f'Opened new Log chunk for Node {node}.')


def set_chunk_timer_and_alert():
    print('SETTING TIMER AND ALERT NOT YET IMPLEMENTED!')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    fztask_long_id = "Control:Task" + f" v{version} (core v{core_version})"

    print(fztask_long_id+"\n")

    args = parse_options()

    make_log_entry()

    chunkchoice = new_or_close_chunk()

    if (chunkchoice =='c'):
        close_chunk()
    else:
        next_chunk()
        set_chunk_timer_and_alert()
    # ** close_chunk() and next_chunk() could both return the new completion ratio of the
    # ** Node that owns the previous chunk (or at least a true/false whether completion >= 1.0)
    # ** and that could be used to check with the caller whether the Node really should
    # ** be considered completed. If not, then there is an opportunity to change the
    # ** time required or to set a guess for the actual completion ratio.

sys.exit(0)
