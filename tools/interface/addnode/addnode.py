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
    parser.add_argument('-S', '--simulate', dest='simulate', action="store_true", help='simulated call to fzgraph (no Node is created)')
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
    print(u'\u001b[34mBlue', end='')
    #print(u'\u001b[38;5;$33m', end='')


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


def collect_step_category():
    #print(milestone_info)
    stepcategory = ''
    while not stepcategory:
        stepcategory = input("Step category: ")
    return stepcategory


tdproperty_info = """
TARGETDATE PROPERTY

  u = unspecified
  v = variable
  i = inherit
  f = fixed
  e = exact
"""

tdproperty_label = {
    'u' : 'unspecified',
    'v' : 'variable',
    'i' : 'inherit',
    'f' : 'fixed',
    'e' : 'exact'
}


def collect_tdproperty():
    tdproperty_char = '_'
    while (not (tdproperty_char in ['u', 'v', 'i', 'f', 'e'])):
        print(tdproperty_info)
        tdproperty_char = input('Target date property: ')
    return tdproperty_label[tdproperty_char]

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

targetdate_info = """
TARGETDATE
"""

def collect_targetdate():
    print(targetdate_info)
    targetdate = ''
    while not targetdate:
        targetdate = input("Target date and time (YYYYmmddHHMM / TODAY): ")
        if (not ((targetdate=='TODAY') or is_TimeStamp(targetdate))):
            print('Target date needs to be a proper date and time stamp (e.g. 202101120813).')
            targetdate = ''
    return targetdate


def add_milestone_step_to_description(descriptionfile, milestone, stepcategory):
    try:
        with open(descriptionfile, 'a') as f:
            f.write(f'\n<p>\n@STEPEARNING:{milestone}:{stepcategory}@\n</p>\n')
    except FileNotFoundError:
        print(f'The description file {descriptionfile} appears to be missing.\n')
        exit(1)


tdpattern_info = """
TARGETDATE REPEAT PATTERN

  n = no repeats
  d = daily
  D = workdays
  w = weekly
  b = biweekly (fortnighly)
  m = monthly
  E = monthly specified as end-of-month offset
  y = yearly
"""

tdpattern_label = {
    'n' : 'nonperiodic',
    'd' : 'daily',
    'D' : 'workdays',
    'w' : 'weekly',
    'b' : 'biweekly',
    'm' : 'monthly',
    'E' : 'endofmonthoffset',
    'y' : 'yearly'
}


def collect_repeat_pattern():
    tdpattern_char = '_'
    while (not (tdpattern_char in ['n', 'd', 'D', 'w', 'b', 'm', 'E', 'y'])):
        print(tdpattern_info)
        tdpattern_char = input('Target date repeat pattern: ')
    return tdpattern_label[tdpattern_char]


def collect_every(pattern):
    #print(every_info)
    every = -1
    while (int(every) <= 0):
        every = input(f"Repeat {pattern} every N instances: ")
        if (not is_int(every)):
            print('The multiplier must be a positive integer number.')
            every = -1
    return every


def collect_span(pattern):
    #print(every_info)
    span = -1
    while (int(span) < 0):
        span = input(f"Repeat {pattern} M times (0 == inf, always): ")
        if (not is_int(span)):
            print('The span must be an integer number.')
            span = -1
    return span


def get_topics():
    thecmd="fzgraphhtml -t '?' -F desc -q -e"
    retcode = try_subprocess_check_output(thecmd, 'topics')
    exit_error(retcode, 'Attempt to get topics failed.')  


topics_info = """
TOPICS
"""

def collect_topics():
    get_topics()
    global topics_info
    topics_info += results['topics'].decode()
    topics = ''
    while not topics:
        print(topics_info)
        topics = input('Topics (comma separated): ')
    print('Beware: Test to see if the topics exist has not yet been implemented!')
    return topics


superiors_and_dependencies_info = """
SUPERIORS AND DEPENDENCIES

  Superiors and Dependencies are included from the 'superiors' and
  'dependencies' Named Node Lists.

  If you have not aleady made such Lists then you can do so now.

"""


def browse_for_Nodes(targetmsg):
    thecmd = config['localbrowser'] + ' http://localhost/select.html'
    retcode = try_subprocess_check_output(thecmd, 'browsed')
    exit_error(retcode, f'Attempt to browse for {targetmsg} failed.')


def collect_superiors_and_dependencies():
    print(superiors_and_dependencies_info)
    browse_for_Nodes('superiors and dependencies')


def collect_node_data():
    print('Interactive data collection for new Node.\n')
    descriptionfile = collect_description()
    print(f'Node description cached in {descriptionfile}.\n')
    treq = collect_time_required()
    val = collect_valuation()
    milestone = collect_milestone()
    if milestone:
        stepcategory = collect_step_category()
        add_milestone_step_to_description(descriptionfile, milestone, stepcategory)
    else:
        stepcategory = ''
    tdproperty = collect_tdproperty()
    if (tdproperty in ['variable', 'fixed', 'exact']):
        targetdate = collect_targetdate()
    else:
        targetdate = '-1'
    addto_fzgraphcmd = ''
    repeat_pattern = "nonperiodic"
    if (tdproperty in ['fixed', 'exact']):
        repeat_pattern = collect_repeat_pattern()
        if (repeat_pattern != "nonperiodic"):
            every = collect_every(repeat_pattern)
            span = collect_span(repeat_pattern)
            addto_fzgraphcmd = f" -r '{repeat_pattern}' -e '{every}' -s '{span}'"
    topics = collect_topics()
    collect_superiors_and_dependencies()

    fzgraphcmd = f"fzgraph -M node -f '{descriptionfile}' -H '{treq}' -a '{val}' -p '{tdproperty}' -t '{targetdate}' -g '{topics}'"+addto_fzgraphcmd
    if config['verbose']:
        fzgraphcmd += ' -V'
    return fzgraphcmd


def simulate_make_node(fzgraphcmd: str):
    print('Simulated call to fzgraph:')
    print('  '+fzgraphcmd)
    print('')
    return '999912312359'


def make_node(fzgraphcmd: str):
    print('EXTRA CHECK - here is the command:')
    print(f'fzgraphcmd = {fzgraphcmd}')
    keywait = input('Press ENTER to continue...')
    retcode = try_subprocess_check_output(fzgraphcmd, 'fzgraph_res')
    exit_error(retcode, 'Call to fzgraph failed.')
    fzgraph_res = results['fzgraph_res'].decode()
    print('HERE IS WHAT fzgraph REPORTED BACK:')
    print(fzgraph_res)
    keywait = input('Press ENTER to continue...')
    newnodepos = fzgraph_res.find('New Node: ')
    if (newnodepos > -1):
        node_id = results['fzgraph_res'][newnodepos+10:newnodepos+26]
    else:
        exit_error(1, "Node Node ID not found.")
    return node_id


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    addnode_long_id = "Interface:Node:Add" + f" v{version} (core v{core_version})"

    print(addnode_long_id+"\n")

    args = parse_options()

    fzgraphcmd = collect_node_data()

    if args.simulate:
        node_id = simulate_make_node(fzgraphcmd)
    else:
        node_id = make_node(fzgraphcmd)

    print(f'Completed addnode. Created Node {node_id}.')

    sys.exit(0)
