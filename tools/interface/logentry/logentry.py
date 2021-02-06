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
import re
#import curses
import pty
import time


# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'
# *** can add logentryconfigdir and logentryconfig here

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
from error import *
from standard import *
from ansicolorcodes import *
from fzcmdcalls import *
from Graphaccess import *
from Logaccess import *

ANSI_sel = '\u001b[38;5;33m'

if not 'cmderrorreviewstr' in config:
    config['cmderrorreviewstr'] = ''
    if config['logcmderrors']:
        cmderrlogstr = config['cmderrlog']
        config['cmderrorreviewstr'] = f'\nYou may review the error(s) in: {ANSI_yb}{cmderrlogstr}{ANSI_nrm}'

# ----- begin: Local variables and functions -----

logentryconfigdir = fzuserbase+'/config/logentry.py'
logentryconfig = logentryconfigdir+'/config.json'

version = "0.1.0-0.1"

# local defaults
config['logentrytmpfile'] = '/tmp/logentry.html'
config['customtemplate'] = '/tmp/customtemplate'
config['confirm_not_chunknode'] = True
# config['editor'] = 'emacs' # reading this from config/fzsetup.py/config.json now
# config['transition'] = 'true' # reading this from config/fzsetup.py/config.json now

# Potentially replace defaults with values from logentry config file
if not 'logentryconfig_checked' in config:
    try:
        config['logentryconfig_checked'] = True
        with open(logentryconfig) as f:
            logentryconfig = json.load(f)
            if (len(logentryconfig)>0):
                config.update(logentryconfig)

    except FileNotFoundError:
        if config['verbose']:
            print('No logentry-specific configuration file found. Using defaults.\n')


def parse_options():
    theepilog = ('See the Readme.md in the logentry source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Make a Log entry.',epilog=theepilog)
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    parser.add_argument('-w', '--waitexit', dest='waitexit', help='wait a number of seconds before exiting')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = os.getenv('USER')
    if args.verbose:
        config['verbose'] = True
        print('Verbose mode.', flush=True)

    print('Working with the following targets:\n')
    print(f'  Formalizer Postgres database name   : {args.dbname}')
    print(f'  Formalizer user or group schema name: {args.schemaname}\n')

    #choice = input('Is this correct? (y/N) \n')
    #if (choice != 'y'):
    #    print('Ok. You can try again with different command arguments.\n')
    #    exit(0)

    return args

def logentry_ansi():
    print(f'{ANSI_or}', end='')


def Node_selection_ansi():
    print(f'{ANSI_sel}', end='')


def alert_ansi():
    print(f'{ANSI_alert}', end='')


def make_content_file():
    emptystr = ''
    with open(config['logentrytmpfile'],'w') as f:
        f.write(emptystr)


def edit_content_file():
    retcode = try_subprocess_check_output(f"{config['editor']} {config['logentrytmpfile']}", '', config)
    exit_error(retcode, 'Attempt to edit content file failed.')
    
    print('Reading edited content file...')
    with open(config['logentrytmpfile'],'r') as f:
        entrycontent = f.read()
    
    return entrycontent


#def get_from_Named_Node_Lists(list_name, output_format, resstore):
#    retcode = try_subprocess_check_output(f"fzgraphhtml -L '{list_name}' -F {output_format} -x 60 -N 5 -e -q",resstore, config)
#    exit_error(retcode, 'Attempt to get Named Node List data failed.')


#def get_from_Incomplete(output_format, resstore):
#    retcode = try_subprocess_check_output(f"fzgraphhtml -I -F {output_format} -x 60 -N 5 -e -q",resstore, config)
#    exit_error(retcode, 'Attempt to get Incomplete Nodes data failed.')


#filtered = filter(lambda x: not re.match(r'^\s*$', x), shortlist_desc.decode().splitlines())
#print(str(filtered))
#shortlist_prettyprint = os.linesep.join([s for s in shortlist_desc.decode().splitlines() if s.strip()])
#print(shortlist_prettyprint)
#shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]

def entry_belongs_to_same_or_other_Node():
    shortlist = ShortList(f'\n{ANSI_sel}Short-list of Nodes for this Log Entry:', config)
    node = '?'

    while (node == '?'):
        shortlist.show()
        print(f'{ANSI_sel}Use:\n- [{ANSI_wb}d{ANSI_sel}]efault, same Node as chunk, or')
        if (shortlist.size > 0):
            print(f'- [{ANSI_gn}0-{shortlist.size - 1}{ANSI_sel}] from shortlist, or')
        choice = input(f'- [{ANSI_gn}?{ANSI_sel}] to browse: ')
        if (choice == '?'):
            node = browse_for_Node(config)
            chosen_desc = selected_Node_description(config, 60)
        else:
            if ((int(choice) >= 0) & (int(choice) < shortlist.size)):
                node = shortlist.nodes.splitlines()[int(choice)]
                chosen_desc = shortlist.vec[int(choice)]
            else:
                node = '' # default

        if node:
            node = node.decode()
            print(f'Log entry belongs to Node {node}:')
            print(f'  {ANSI_wt}{chosen_desc}{ANSI_nrm}')
            if config['confirm_not_chunknode']:
                confirmothernode = input(f'Confirmed? ({No_yes(ANSI_sel)}) ')
                if (confirmothernode != 'y'):
                    node = '?'
        else:
            print(f'Log entry belongs to the same Node as the Log chunk.')
    logentry_ansi()
    return node


def send_to_fzlog(node):
    thecmd=f"fzlog -e -f {config['logentrytmpfile']}"
    if node:
        thecmd += f" -n {node}"
    retcode = try_subprocess_check_output(thecmd, '', config)
    exit_error(retcode, 'Attempt to add Log entry via fzlog failed.')
    print('Entry added to Log.')


def get_main_topic(node):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
    customtemplate = '{{ topics }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={config['customtemplate']}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'topic', config)
    exit_error(retcode, 'Attempt to get Node topic failed.')
    topic = results['topic'].split()[0]
    topic = topic.decode()
    return topic


def set_DIL_entry_preset(node):
    topic = get_main_topic(node)
    dilpreset = f'{topic}.html#{node}:!'
    print(f'Specifying the DIL ID preset: {dilpreset}')
    with open(userhome+'/.dil2al-DILidpreset','w') as f:
        f.write(dilpreset)
    if config['logentry_logcmdcalls']:
        with open(config['logentry_cmdlog'],'a') as f:
            f.write(dilpreset+'\n')


def transition_dil2al_polldaemon_request(node):
    if node:
        set_DIL_entry_preset(node)
    else:
        if os.path.exists(userhome+'/.dil2al-DILidpreset'):
            os.remove(userhome+'/.dil2al-DILidpreset')
    thecmd=f"dil2al -m{config['logentrytmpfile']} -p 'noaskALDILref' -p 'noalwaysopenineditor'"
    retcode = try_subprocess_check_output(thecmd, 'dil2al', config)
    exit_error(retcode, 'Call to dil2al -m failed.')
    #retcode = pty.spawn(['dil2al',f"-m{config['logentrytmpfile']}","-p'noaskALDILref'","-p'noalwaysopenineditor'"])
    #if not os.WIFEXITED(retcode):
    #    exit_error(os.WEXITSTATUS(retcode), 'Call to dil2al -m failed.')
    print('Log entry synchronized to Formalizer 1.x files.')


# Call this function when logentry.py is imported into another script.
def make_log_entry():
    make_content_file()

    entrycontent = ""
    while len(entrycontent)<1:
        entrycontent = edit_content_file()
        if len(entrycontent)<1:
            choice = input('Empty Log entry. Skip making a Log entry? (y/N): ')
            if (choice == 'y'):
                print('Log entry skipped.')
                return

    node = entry_belongs_to_same_or_other_Node()

    send_to_fzlog(node)

    if config['transition']:
        transition_dil2al_polldaemon_request(node)



if __name__ == '__main__':

    core_version = coreversion.coreversion()
    logentry_long_id = f"Formalizer:Interface:CommandLine:LogEntry v{version} (core v{core_version})"

    logentry_ansi()

    print(logentry_long_id+"\n")

    args = parse_options()

    make_log_entry()

    wait_exit(wait_seconds = float(args.waitexit))
