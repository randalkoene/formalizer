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
import time


# ----- begin: Common variables and functions (probably put into a library module) -----

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'
#results = {}

# We need this everywhere to run various shell commands.
#def try_subprocess_check_output(thecmdstring, resstore):
#    if config['verbose']:
#        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
#    if config['logcmdcalls']:
#        with open(config['cmdlog'],'a') as f:
#            f.write(thecmdstring+'\n')
#    try:
#        res = subprocess.check_output(thecmdstring, shell=True)
#
#    except subprocess.CalledProcessError as cpe:
#        if config['logcmderrors']:
#            with open(config['cmderrlog'],'a') as f:
#                f.write(f'Subprocess call ({thecmdstring}) caused exception.\n')
#                f.write(f'Error output: {cpe.output.decode()}\n')
#                f.write(f'Error code  : {cpe.returncode}\n')
#                if (cpe.returncode>0):
#                    f.write('Formalizer error: '+error.exit_status_code[cpe.returncode]+'\n')
#        if config['verbose']:
#            print('Subprocess call caused exception.')
#            print('Error output: ',cpe.output.decode())
#            print('Error code  : ',cpe.returncode)
#            if (cpe.returncode>0):
#                print('Formalizer error: ', error.exit_status_code[cpe.returncode])
#        return cpe.returncode
#
#    else:
#        if resstore:
#            results[resstore] = res
#        if config['verbose']:
#            print('Result of subprocess call:', flush=True)
#            print(res.decode(), flush=True)
#        return 0

#def pause_key(action_str, pausehere = True):
#    if pausehere:
#        pausekey = input(f'\nEnter any string to {action_str}...')
#    else:
#        pausekey = '_'
#    return pausekey


#def exit_error(retcode, errormessage, ask_exit = False):
#    if (retcode != 0):
#        print(f'\n{ANSI_alert}'+errormessage+f'{ANSI_nrm}\n')
#        if ask_exit:
#            exitorcontinue = input(f'\n[{ANSI_rd}E{ANSI_nrm}]xit or attempt to [{ANSI_gn}c{ANSI_nrm}]ontinue? ')
#            if (exitorcontinue == 'c'):
#                print('\nAttempting to continue...\n')
#            else:
#                print('\nExiting.\n')
#                sys.exit(retcode)
#        else:
#            exitenter = pause_key('exit')
#            sys.exit(retcode)


#def browse_for_Node():
#    print('Use the browser to select a node.')
#    #retcode = pty.spawn([config['localbrowser'],'http://localhost/select.html'])
#    thecmd = config['localbrowser'] + ' http://localhost/select.html'
#    retcode = try_subprocess_check_output(thecmd, 'browsed')
#    exit_error(retcode, f'Attempt to browse for Node selection failed.{cmderrorreviewstr}', True)
#    if (retcode == 0):
#        retcode = try_subprocess_check_output(f"fzgraphhtml -L 'selected' -F node -N 1 -e -q",'selected')
#        exit_error(retcode, f'Attempt to get selected Node failed.{cmderrorreviewstr}', True)
#        if (retcode == 0):
#            node = (results['selected'][0:16]).decode()
#            print(f'Selected: {node}')
#            if results['selected']:
#                return results['selected'][0:16]
#            else:
#                return ''
#        else:
#            return ''
#    else:
#        return ''


def selected_Node_description(excerpt_len = 0):
    thecmd = "fzgraphhtml -L 'selected' -F desc -N 1 -e -q"
    if (excerpt_len > 0):
        thecmd += f' -x {excerpt_len}'
    retcode = try_subprocess_check_output(thecmd,'selected_desc', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get description of selected Node failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        res_selected_desc = results['selected_desc']
        selected_desc_vec = res_selected_desc.decode().split("@@@")
        return selected_desc_vec[0]
    else:
        return ''


# Handle the case where even fzsetup.py does not have a configuration file yet.
try:
    with open(fzsetupconfig) as f:
        config = json.load(f)

except FileNotFoundError:
    print('Configuration files missing. Please run fzsetup.py first!\n')
    exit(1)

else:
    #config['transition'] = 'false' # reading this from config/fzsetup.py/config.json now
    assert len(config) > 0

# Enable us to import standardized Formalizer Python components.
fzcorelibdir = config['sourceroot'] + '/core/lib'
fzcoreincludedir = config['sourceroot'] + '/core/include'
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)

# ----- end: Common variables and functions (probably put into a library module) -----

# ----- begin: Local variables and functions -----

fztaskconfigdir = fzuserbase+'/config/fztask.py'
fztaskconfig = fztaskconfigdir+'/config.json'

# Enable import of logentry
logentrydir = config['sourceroot'] + '/tools/interface/logentry'
sys.path.append(logentrydir)

# core components
import Graphpostgres
import coreversion
from error import *
from ansicolorcodes import *
from fzcmdcalls import *
from Graphaccess import *

ANSI_sel = '\u001b[38;5;33m'

config['cmderrorreviewstr'] = ''
if config['logcmderrors']:
    cmderrlogstr = config['cmderrlog']
    config['cmderrorreviewstr'] = f'\nYou may review the error(s) in: {ANSI_yb}{cmderrlogstr}{ANSI_nrm}'

version = "0.1.0-0.1"

# local defaults
config['customtemplate'] = '/tmp/customtemplate'
config['addpause'] = False
#config['cmdlog'] = '/tmp/fztask-cmdcalls.log' -- this is now in fzsetup.py/config.json
#config['logcmdcalls'] = False -- this is now in fzsetup.py/config.json
#config['cmderrlog'] = '/tmp/cmdcalls-errors.log' -- this is now in fzsetup.py/config.json
#config['logcmderrors'] = False -- this is now in fzsetup.py/config.json

# Potentially replace defaults with values from fztask config file
try:
    with open(fztaskconfig) as f:
        fztaskconfig = json.load(f)
        if (len(fztaskconfig)>0):
            config.update(fztaskconfig)

except FileNotFoundError:
    if config['verbose']:
        print('No fztask-specific configuration file found. Using defaults.\n')

# Some things to do depending on configuration settings
if config['transition']:
    from fztask_transition import transition_dil2al_request


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
        print('Verbose mode.', flush=True)

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


def fztask_ansi():
    print(f'{ANSI_nrm}', end='')


def Node_selection_ansi():
    print(u'\u001b[38;5;33m', end='')


def alert_ansi():
    print(f'{ANSI_alert}', end='')


def make_log_entry():
    import logentry as le
    le.config['verbose'] = config['verbose']
    le.logentry_ansi()
    le.make_log_entry()


def new_or_close_chunk():
    ANSI_sel = '\u001b[38;5;99m'
    print(ANSI_sel)
    choice = input(f'\n[{ANSI_gn}S{ANSI_sel}]tart a Log chunk for the next task, or merely [{ANSI_rd}c{ANSI_sel}]lose the chunk? ')
    if (choice != 'c'):
        choice = 'S'
        print(f'{ANSI_gn}Starting{ANSI_sel} the next task chunk...')
    else:
        print(f'{ANSI_rd}Closing{ANSI_sel} Log chunk without immediately starting another...')

    return choice


def close_chunk(args):
    thecmd = 'fzlog -C'
    if args.T_emulate:
        thecmd += ' -t ' + args.T_emulate
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res', config)
    exit_error(retcode,'Attempt to close Log chunk failed.')


def get_updated_shortlist():
    retcode = try_subprocess_check_output(f"fzgraphhtml -u -L 'shortlist' -F node -e -q", 'shortlistnode', config)
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List node data failed.', True)
    if (retcode != 0):
        return False
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'shortlist' -F desc -x 60 -e -q", 'shortlistdesc', config)
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List description data failed.', True)
    if (retcode != 0):
        return False
    return True


class ShortList:
    def __init__(self):
        self.nodes = ''
        self.desc = ''
        self.vec = [ ]
        self.gotshortlist = get_updated_shortlist()
        if self.gotshortlist:
            self.nodes = results['shortlistnode']
            self.desc = results['shortlistdesc']
            #self.vec = [s for s in self.desc.decode().splitlines() if s.strip()]
            self.vec = [s for s in self.desc.decode().split("@@@") if s.strip()]
            self.size = len(self.vec)
    def show(self):
        print(f'\n{ANSI_sel}Short-list of Nodes for the {ANSI_wt}New Log Chunk{ANSI_sel}:')
        if self.gotshortlist:
            pattern = re.compile('[\W_]+')
            for (number, line) in enumerate(self.vec):
                printableline = pattern.sub(' ',line)
                print(f' {number}: {printableline}')

def select_Node_for_Log_chunk():
    ANSI_Yes_no = f'{ANSI_gn}Y{ANSI_sel}/{ANSI_rd}n{ANSI_sel}'
    node = '' # none selected
    shortlist = ShortList()

    while not node:
        shortlist.show()
        print(f'{ANSI_sel}Use:')
        if (shortlist.size > 0):
            print(f'- [{ANSI_gn}0-{shortlist.size - 1}{ANSI_sel}] from shortlist, or')
        choice = input(f'- [{ANSI_gn}?{ANSI_sel}] to browse: ')
        if (choice == '?'):
            node = browse_for_Node(config)
            chosen_desc = selected_Node_description(60)
        else:
            if ((int(choice) >= 0) & (int(choice) < shortlist.size)):
                node = shortlist.nodes.splitlines()[int(choice)]
                chosen_desc = shortlist.vec[int(choice)]

        if node:
            node = node.decode()
            print(f'Log chunk will belong to Node {node}:')
            print(f'  {ANSI_wt}{chosen_desc}{ANSI_nrm}')
            if config['confirmchunknode']:
                iscorrectnext = input(f'Is that correct? ({ANSI_Yes_no}) ')
                if (iscorrectnext == 'n'):
                    node = ''

    fztask_ansi()
    return node


def update_schedule(args):
    ANSI_upd = '\u001b[38;5;148m'
    ANSI_Yes_no = f'{ANSI_gn}Y{ANSI_upd}/{ANSI_rd}n{ANSI_upd}'
    cmderrorreviewstr = config['cmderrorreviewstr']
    addtocmd = ''
    if args.T_emulate:
        addtocmd += ' -t '+args.T_emulate
    if config['verbose']:
        addtocmd += ' -V'
    print(f'{ANSI_upd}SCHEDULE UPDATES')

    # repeating Nodes
    skippassedrepeats = input(f'  Skip {ANSI_wt}passed repeating{ANSI_upd} Nodes? ({ANSI_Yes_no}) ')
    if (skippassedrepeats != 'n'):
        thecmd = 'fzupdate -q -E STDOUT -r'+addtocmd
        retcode = try_subprocess_check_output(thecmd, 'passedrepeatsskip', config)
        exit_error(retcode, f'Attempt to skip passed repeating Nodes failed.{cmderrorreviewstr}', True)

    # variable target dates
    varupdate = input(f'  Update {ANSI_wt}variable{ANSI_upd} target date Nodes? ({ANSI_Yes_no}) ')
    if (varupdate != 'n'):
        thecmd = 'fzupdate -q -E STDOUT -u'+addtocmd
        retcode = try_subprocess_check_output(thecmd, 'varupdate', config)
        exit_error(retcode, f'Attempt to update variable target date Nodes failed.{cmderrorreviewstr}', True)

    print('')


def next_chunk(args):
    # Closing the previous chunk is automatically done as part of this in fzlog.
    pause_key('select next Node',config['addpause'])
    node = select_Node_for_Log_chunk()
    if not node:
        exit_error(1, 'Attempt to select Node for new Log chunk failed.', True)
    else:
        pause_key('open new chunk',config['addpause'])
        thecmd = 'fzlog -c ' + node
        if args.T_emulate:
            thecmd += ' -t ' + args.T_emulate
        if config['verbose']:
            thecmd += ' -V'
        retcode = try_subprocess_check_output(thecmd, 'fzlog_res', config)
        cmderrorreviewstr = config['cmderrorreviewstr']
        exit_error(retcode, f'Attempt to open new Log chunk failed.{cmderrorreviewstr}', True)
        if (retcode == 0):
            print(f'Opened new Log chunk for Node {node}.')
        else:
            node = ''
    return node


def get_main_topic(node):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
    #     E.g. could call fzgraph -C or curl with the corresponding URL.
    customtemplate = '{{ topics }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    customtemplatefile = config['customtemplate']
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={customtemplatefile}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'topic', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get Node topic failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        topic = results['topic'].split()[0]
        topic = topic.decode()
    else:
        topic = ''
    return topic


def get_completion_required(node):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
    #     E.g. could call fzgraph -C or curl with the corresponding URL.
    customtemplate = '{{ comp }} {{ req_hrs }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    customtemplatefile = config['customtemplate']
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={customtemplatefile}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'compreq', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get Node completion and required failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        results['completion'] = (results['compreq'].split()[0]).decode()
        results['required'] = (results['compreq'].split()[1]).decode()
    else:
        results['completion'] = ''
        results['required'] = ''


def get_most_recent_task():
    thecmd = 'fzloghtml -R -o STDOUT -N -F raw -q'
    retcode = try_subprocess_check_output(thecmd, 'recentlog', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get most recent Log chunk data failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        recent_node = (results['recentlog'].split()[2]).decode()
    else:
        recent_node = ''
    return recent_node


def set_chunk_timer_and_alert():
    # It looks like I can just run the same formalizer-alert.sh that dil2al was using.
    print('Setting chunk duration: 20 mins. Chunk starts now.')
    time.sleep(1200)
    alert_ansi()
    print('Chunk time passed. Calling formalizer-alert.sh.')
    thecmd = 'formalizer-alert.sh'
    retcode = try_subprocess_check_output(thecmd, 'alert', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Call to formalizer-alert.sh failed.{cmderrorreviewstr}', True)
    fztask_ansi()


def task_control(args):
    # *** I have to let dil2al do its update things instead, due to the complex nature of
    #     target date updates through Superiors.
    #if config['transition']:
    #    recent_node = get_most_recent_task()

    make_log_entry()

    chunkchoice = new_or_close_chunk()

    pause_key('close current chunk',config['addpause'])

    # Note that in this process, where a schedule updated in accordance with time
    # added to the most recent Node's completion ratio affects the potential choice
    # of Node for the next Log chunk, we cannot make use of the built-in chunk
    # closing that is available through `fzlog -c <node-id>`. Instead, we must
    # close the chunk first, then update the schedule, and use the resulting
    # information for an informed choice.
    close_chunk(args)

    pause_key('update schedule',config['addpause'])

    update_schedule(args)

    node = ''
    if (chunkchoice !='c'):
        pause_key('start next chunk',config['addpause'])
        node = next_chunk(args)
        if node:
            # ** close_chunk() and next_chunk() could both return the new completion ratio of the
            # ** Node that owns the previous chunk (or at least a true/false whether completion >= 1.0)
            # ** and that could be used to check with the caller whether the Node really should
            # ** be considered completed. If not, then there is an opportunity to change the
            # ** time required or to set a guess for the actual completion ratio.
            if config['transition']:
                pause_key('synchronize back to Formalizer 1.x',config['addpause'])
                transition_dil2al_request(node, args)
                #transition_dil2al_request(recent_node, node)
            pause_key('start the chunk timer',config['addpause'])
            set_chunk_timer_and_alert()
        else:
            print(f'{ANSI_alert}We have no next Node, so we behave as if "close chunk" was chosen.{ANSI_nrm}')
            chunkchoice = 'c'

    return chunkchoice

# ----- end: Local variables and functions -----


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    fztask_long_id = "Control:Task" + f" v{version} (core v{core_version})"

    fztask_ansi()

    print(fztask_long_id+"\n")

    print('Note:')
    print('- fztask.py presently uses a while-loop to act like a daemonized task server.')
    print('- One alternative is to set a cron-job.')
    print('- Another alternative is to launch or use a separate deamonized task-timer.')
    print('- Or simply exit after one run-through of the fztask steps.')
    print('These options can be made a configuration option.\n')

    args = parse_options()

    chunkchoice = 'S'

    while (chunkchoice != 'c'):
        chunkchoice = task_control(args)
        args.T_emulate = None # needs to apply only when fztask is first called

    print('\nfztask done.')

    pausekey = pause_key('exit')

    sys.exit(0)
