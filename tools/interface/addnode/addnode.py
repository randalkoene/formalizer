#!/usr/bin/python3
#
# addnode.py
#
# Randal A. Koene, 20201126
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
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)

config['logcmdcalls'] = False

# core components
import Graphpostgres
import coreversion

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
    theepilog = ('See the Readme.md in the addnode.py source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Add a Node on the command line interactively.',epilog=theepilog)
    parser.add_argument('-S', '--simulate', dest='simualte', action="store_true", help='simulated call to fzgraph (no Node is created)')
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


def addnode_ansi():
    print(u'\u001b[38;5;$33m', end='')


def alert_ansi():
    print(u'\u001b[31m', end='')


def exit_error(retcode, errormessage):
    if (retcode != 0):
        alert_ansi()
        print('\n'+errormessage+'\n')
        addnode_ansi()
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


def is_float(n):
    try:
        float(n)
    except ValueError:
        return False
    return True


def collect_description():
    t_str = TimeStamp(datetime.now())
    descriptionfile = '/tmp/addnode-'+t_str+'.html'
    retcode = try_subprocess_check_output(f"{config['editor']} {descriptionfile}", '')
    exit_error(retcode, 'Attempt to edit description file failed.')
    return descriptionfile


treq_info = """
TIME REQUIRED
  An accurate estimate of time required with effective work practices
  aids proper planning, scheduling and decision making (target dates,
  parallel work, priorities, delegating, collaborating).
  (Log#200701210430.1)

  Hint A: Perhaps the time that was required for a completed similar
          task can help provide the estimate.

  Hint B: To curtail the risks of perfectionism, try to determine the
          normal standards for this (type of) task. What condition is
          considered done, and how long does that normally take?
          If necessary, consider hypothesis testing: try out a state
          of completion and see how it goes.
          (RTT S.P.2, Cause 2, solution D and solution E)

"""

def collect_time_required():
    print(treq_info)
    treq = -1.0
    while (float(treq) < 0):
        treq = input("Estimated 'time required' (in hours): ")
        if (not is_float(treq)):
            print('Time required must be a decimal number of hours in [0, inf), e.g. 1.0.')
            treq = -1.0
    return treq


val_info = """
VALUATION
"""

def collect_valuation():
    print(val_info)
    val = -1.0
    while (float(val) < 0):
        val = input("Estimated 'valuation' (0.0<x<1.0,x=1.0,x>1.0): ")
        if (not is_float(val)):
            print('Valuation must be a decimal number in (-inf, inf), e.g. 3.0.')
            val = -1.0
    return val


milestone_info = """
Hint: If this is a major task or part of a project with multiple
      tasks then, when possible, please involve another person in
      some way and ensure there is an agreement where something
      is expected from you.
      (RTT S.P.2, Sensation 5, solution A)

ASSOCIATED MILESTONE STEP EARNING (optional)
"""
def collect_milestone():
    print(milestone_info)
    milestone = ''
    while not milestone:
        milestone = input("Milestone identifier (n = not STEPEARNING): ")
    if (milestone == 'n'):
        milestone = ''
    return milestone


def collect_node_data():
    print('Interactive data collection for new Node.\n')
    descriptionfile = collect_description()
    print(f'Node description cached in {descriptionfile}.\n')
    treq = collect_time_required()
    val = collect_valuation()
    milestone = collect_milestone()


    fzgraphcmd = f'fzgraph -f {descriptionfile} -H {treq} -a {val}'
    if config['verbose']:
        fzgraphcmd += ' -V'
    return fzgraphcmd


def simulate_make_node(fzgraphcmd: str):
    print('Simulated call to fzgraph:')
    print('  '+fzgraphcmd)
    print('')


def make_node(fzgraphcmd: str):
    retcode = try_subprocess_check_output(fzgraphcmd, 'fzgraph_res')
    if (retcode != 0):
        exit_error(retcode, 'Call to fzgraph failed.')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    addnode_long_id = "Interface:Node:Add" + f" v{version} (core v{core_version})"

    print(addnode_long_id+"\n")

    args = parse_options()

    print('This tool is under construction.')
    args.simulate=True
    print('During this time, the tool is in simulated dry-run mode, where it will print the')
    print('fzgraph command that would be created following interactive data collaction.')

    fzgraphcmd = collect_node_data()

    if args.simulate:
        simulate_make_node(fzgraphcmd)
    else:
        make_node(fzgraphcmd)

    print('Completed addnode.')

sys.exit(0)
