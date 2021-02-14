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
fztasklockfile = fzuserbase+'/.fztask.lock'

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
from Logaccess import *
from TimeStamp import *
from tcpclient import get_server_address
from proclock import *

ANSI_sel = '\u001b[38;5;33m'
ANSI_upd = '\u001b[38;5;148m'

config['cmderrorreviewstr'] = ''
if config['logcmderrors']:
    cmderrlogstr = config['cmderrlog']
    config['cmderrorreviewstr'] = f'\nYou may review the error(s) in: {ANSI_yb}{cmderrlogstr}{ANSI_nrm}'

version = "0.1.0-0.1"

# local defaults
config['customtemplate'] = '/tmp/customtemplate'
config['addpause'] = False
config['chunkminutes'] = 20
config['exact_Node_intervals'] = True
#config['cmdlog'] = '/tmp/fztask-cmdcalls.log' -- this is now in fzsetup.py/config.json
#config['logcmdcalls'] = False -- this is now in fzsetup.py/config.json
#config['cmderrlog'] = '/tmp/cmdcalls-errors.log' -- this is now in fzsetup.py/config.json
#config['logcmderrors'] = False -- this is now in fzsetup.py/config.json

last_node = ''
last_T_close = ''
cycle_updated = False

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

# tools components
import logentry as le
le.config['verbose'] = config['verbose']


def unlock_it(lockfile: str, config: dict):
    if os.path.exists(lockfile):
        os.remove(lockfile)
    else:
        if config['verbose']:
            print(f'{ANSI_warn}Lock file {lockfile} to be removed did not exist.{ANSI_nrm}')


def lock_it(lockfile: str):
    if os.path.exists(lockfile):
        print(f'{ANSI_alert}Lock file {lockfile} found!{ANSI_nrm}')
        goaheadanyway = input(f'{ANSI_sel}Another instance of fztask may be running. Go ahead anyway? ({No_yes(ANSI_sel)}) ')
        if (goaheadanyway != 'y'):
            print('Exiting.')
            sys.exit(1)
        else:
            os.remove(lockfile)
    try:
        with open(lockfile,'w') as f:
            f.write(NowTimeStamp())
    except IOError:
        exit_error(1, f'Unable to write lock file {lockfile}.')


def parse_options():
    theepilog = ('See the Readme.md in the fztask.py source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Formalizer task control.',epilog=theepilog)
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

    #choice = input('Is this correct? (y/N) \n')
    #if (choice != 'y'):
    #    print('Ok. You can try again with different command arguments.\n')
    #    exit(0)

    return args


def get_most_recent_chunk():
    global last_node
    thecmd = 'fzloghtml -R -o STDOUT -N -F raw -q'
    retcode = try_subprocess_check_output(thecmd, 'recentlog', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get most recent Log chunk data failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        results['recentlog'] = results['recentlog'].decode()
        chunk_data = results['recentlog'].split()
        last_node = chunk_data[2]
    else:
        chunk_data = []
    return chunk_data


def check_emulated_time():
    global last_T_close
    recent_chunk = get_most_recent_chunk()
    if args.T_emulate:
        if (is_TimeStamp(args.T_emulate)):
            if (is_Future(args.T_emulate)):
                exit_error(1, f'Emulated time should normally not be in the future: {args.T_emulate}.', True)
            else:
                #recent_chunk = get_most_recent_chunk()
                if (int(args.T_emulate) <= int(recent_chunk[0])):
                    exit_error(1, 'Emulated time should be later than most recent Log chunk open time.', True)
                if (recent_chunk[1] == 'CLOSED'):
                    print(f'Most recent Log chunk was alredy {ANSI_wt}closed{ANSI_nrm}.')
                    last_T_close = recent_chunk[5]
                    t_closed_diff = int(args.T_emulate) - int(recent_chunk[5])
                    if (t_closed_diff < 0):
                        exit_error(1, 'Emulated time should be equal or later most recent Log chunk close time.', True)
                    if (t_closed_diff > 1):
                        print(f'{ANSI_mg}Emulated time {ANSI_wt}{args.T_emulate}{ANSI_nrm}{ANSI_mg} > T_close {ANSI_wt}{recent_chunk[5]}{ANSI_nrm}{ANSI_mg} of most recent Log chunk.{ANSI_nrm}')
                        leavegap = input(f'{ANSI_bb}Your choices are: [{ANSI_gn}A{ANSI_bb}]ttach to close-time, make the [{ANSI_rd}g{ANSI_bb}]ap, or [{ANSI_yb}e{ANSI_bb}]xit?{ANSI_nrm} ')
                        if (leavegap == 'e'):
                            print('Exiting.')
                            sys.exit(0)
                        if (leavegap != 'A'):
                            print('Adjusting emulated time to closing time of most recent Log chunk.')
                            args.T_emulate = recent_chunk[5]
                        else:
                            print('Continuing with time-gap in Log record.')
                print(f'\nEmulated Time: {args.T_emulate}\n')
        else:
            exit_error(1, f'Emulated time has invalid time stamp: {args.T_emulate}.')
    else:
        print('\nUsing actual time.\n')


def fztask_ansi():
    print(f'{ANSI_nrm}', end='')


def Node_selection_ansi():
    print(u'\u001b[38;5;33m', end='')


def alert_ansi():
    print(f'{ANSI_alert}', end='')


def make_log_entry():
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
    global last_T_close
    thecmd = 'fzlog -C'
    if args.T_emulate:
        stamp_t_close = args.T_emulate
        thecmd += ' -t ' + args.T_emulate
    else:
        stamp_t_close = NowTimeStamp()
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res', config)
    exit_error(retcode,'Attempt to close Log chunk failed.')
    last_T_close = stamp_t_close


def select_Node_for_Log_chunk():
    node = '' # none selected
    shortlist = ShortList(f'\n{ANSI_sel}Short-list of Nodes for the {ANSI_wt}New Log Chunk{ANSI_sel}:', config)

    while not node:
        shortlist.show()
        print(f'{ANSI_sel}Use:')
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

        if (node == last_node):
            print(f'{ANSI_or}The most recent Log chunk belongs to the same Node.{ANSI_nrm}')
            confirmsame = input(f'{ANSI_sel}Is that intentional? ({No_yes(ANSI_sel)}) ')
            if (confirmsame != 'y'):
                node = ''
        if node:
            node = node.decode()
            print(f'{ANSI_sel}Log chunk will belong to Node {node}:')
            print(f'  {ANSI_wt}{chosen_desc}{ANSI_nrm}')
            if config['confirmchunknode']:
                iscorrectnext = input(f'{ANSI_sel}Is that correct? ({Yes_no(ANSI_sel)}) ')
                if (iscorrectnext == 'n'):
                    node = ''

    fztask_ansi()
    return node


def make_filter_passed_fixed(args):
    completionfilter = 'completion=[0.0-0.999]'
    hoursfilter = 'hours=[0.001-1000.0]'
    if args.T_emulate:
        targetdatesfilter = f'targetdate=[MIN-{args.T_emulate}]'
    else:
        targetdatesfilter = 'targetdate=[MIN-NOW]'
    tdpropertiesfilter = 'tdproperty=[fixed-exact]'
    return f'{completionfilter},{hoursfilter},{targetdatesfilter},{tdpropertiesfilter},repeats=false'


def manual_update_passed_fixed():
    print(f'\n{ANSI_upd}  Opening browser to list of incomplete non-repeating fixed/exact Nodes.{ANSI_nrm}')
    print(f'{ANSI_cy}  Please specify {ANSI_wt}future{ANSI_cy} target dates{ANSI_nrm} for any that should remain fixed/exact.')
    print(f'{ANSI_upd}  The others will be switched to variable target date type.{ANSI_nrm}')
    thecmd = config['localbrowser'] + ' http://localhost/cgi-bin/fzgraphhtml-cgi.py?srclist=passed_fixed'
    retcode = try_subprocess_check_output(thecmd, 'fixedmoved', config)
    exit_error(retcode, f'Attempt to browse passed fixed Nodes failed.', True)
    return retcode


def update_passed_fixed(args):
    global cycle_updated
    # clear passed_fixed NNL
    if not clear_NNL('passed_fixed', config):
        return 2
    # filter for passed fixed target date (possibly with T_emulate) Nodes and put them into the passed_fixed NNL
    filterstr = make_filter_passed_fixed(args)
    num = select_to_NNL(filterstr,'passed_fixed')
    if (num < 0):
        return 2
    if not num:
        print(f'  {ANSI_lt}No passed non-repeating fixed/exact Nodes.{ANSI_upd}')
        return 0
    
    # explain that there are passed fixed target date Nodes and ask to manually move those that should not become variable target date (open browser)
    updatepassedfixed = input(f'  {ANSI_upd}Update/convert {ANSI_yb}{num}{ANSI_nrm} {ANSI_wt}passed non-repeating fixed/exact{ANSI_upd} Nodes? ({Yes_no(ANSI_upd)}) ')
    if (updatepassedfixed == 'n'):
        return 0
    retcode = manual_update_passed_fixed()
    if (retcode != 0):
        return 2
    # clear passed fixed NNL again
    if not clear_NNL('passed_fixed', config):
        return 2
    # filter again for passed fixed target date Nodes and put them into the passed_fixed NNL (or use another NNL)
    remaining_num = select_to_NNL(filterstr,'passed_fixed')
    if (remaining_num < 0):
        return 2
    diff_num = num - remaining_num
    if (diff_num > 0):
        print(f'\n  {ANSI_cy}Thank you for manually updating the target dates of {ANSI_yb}{diff_num}{ANSI_nrm}{ANSI_cy} Nodes.')
        cycle_updated = True 
    if remaining_num:
        cycle_updated = True
        print(f'  {ANSI_cy}Switching {ANSI_yb}{remaining_num}{ANSI_nrm}{ANSI_cy} Nodes to {ANSI_wt}variable{ANSI_nrm}{ANSI_cy} target dates.\n')
        num_fixed_converted = edit_nodes_in_NNL('passed_fixed','tdproperty','variable')
        if (num_fixed_converted != num):
            exit_error(retcode, f'Attempt to convert fixed to variable target date Nodes failed.', True)
            if (retcode != 0):
                return 1
    return 0


def make_filter_skip_repeats(args):
    completionfilter = 'completion=[0.0-0.999]'
    hoursfilter = 'hours=[0.001-1000.0]'
    if args.T_emulate:
        targetdatesfilter = f'targetdate=[MIN-{args.T_emulate}]'
    else:
        targetdatesfilter = 'targetdate=[MIN-NOW]'
    return f'{completionfilter},{hoursfilter},{targetdatesfilter},repeats=true'


def inspect_passed_repeating():
    thecmd = config['localbrowser'] + ' http://localhost/cgi-bin/fzgraphhtml-cgi.py?srclist=skip_repeats'
    retcode = try_subprocess_check_output(thecmd, 'skiprepeatsinspected', config)
    exit_error(retcode, f'Attempt to browse passed repeating Nodes failed.', True)
    return retcode    


def skip_passed_repeats(args, addtocmd):
    global cycle_updated
    while True:
        # clear skip_repeats NNL
        if not clear_NNL('skip_repeats', config):
            return 2
        # filter for passed target date (possibly with T_emulate) repeating Nodes and put them into the skip_repeats NNLs
        filterstr = make_filter_skip_repeats(args)
        num = select_to_NNL(filterstr, 'skip_repeats')
        if (num<0):
            return 2
        if not num:
            print(f'  {ANSI_lt}No passed repeating Nodes.{ANSI_upd}')
            return 0
        
        # explain that there are passed fixed repeating Nodes and offer to skip, not, or inspect first
        ANSI_Yes_no_inspect = f'{ANSI_gn}Y{ANSI_upd}es/{ANSI_rd}n{ANSI_upd}o/[{ANSI_yb}i{ANSI_upd}]nspect'
        skippassedrepeats = input(f'  Skip {ANSI_yb}{num}{ANSI_nrm}{ANSI_cy} {ANSI_wt}passed repeating{ANSI_upd} Nodes? ({ANSI_Yes_no_inspect}) ')
        if (skippassedrepeats == 'n'):
            print(f'  {ANSI_lt}Not skipping.{ANSI_upd}')
            return 0
        if (skippassedrepeats != 'i'):
            cycle_updated = True
            print(f'  {ANSI_lt}Skipping {num} passed repeating Nodes.{ANSI_upd}')
            thecmd = 'fzupdate -q -E STDOUT -r'+addtocmd
            return try_subprocess_check_output(thecmd, 'passedrepeatsskip', config)
        retcode = inspect_passed_repeating()


def refresh_Next_Nodes():
    refresh_cmd = 'panes-term.sh -N'
    retcode = try_subprocess_check_output(refresh_cmd, 'refresh_panes', config)


def update_schedule(args):
    global cycle_updated
    print(f'\n{ANSI_upd}SCHEDULE UPDATES{ANSI_nrm}')
    cycle_updated = False
    cmderrorreviewstr = config['cmderrorreviewstr']
    addtocmd = ''
    if args.T_emulate:
        print(f'  {ANSI_lt}Operating in {ANSI_wt}Emulated Time (T_emulate = {args.T_emulate}).{ANSI_nrm}')
        if config['recommend_noupdate_ifTemulated']:
            print(f'  {ANSI_alert}Current configuration recommends NOT to update while in emulated time{ANSI_nrm}.')
            doitanyway = input(f'  {ANSI_upd}Update anyway? ({No_yes(ANSI_upd)}) ')
            if (doitanyway != 'y'):
                return
        addtocmd += ' -t '+args.T_emulate
        print(f'    {ANSI_upd}Chosen updates will be carried out with \' -t {args.T_emulate}\'.')
    else:
        print(f'  {ANSI_lt}Operating in {ANSI_wt}Real Time{ANSI_nrm}.')
    if config['verbose']:
        addtocmd += ' -V'

    # passed non-repeating fixed and exact target date Nodes
    retcode = update_passed_fixed(args)
    exit_error(retcode, f'Attempt to Update/convert passed non-repeating fixed/exact Nodes failed.', True)

    # repeating Nodes
    retcode = skip_passed_repeats(args, addtocmd)
    exit_error(retcode, f'Attempt to skip passed repeating Nodes failed.{cmderrorreviewstr}', True)

    # variable target date Nodes (can be worth doing even if none have been passed)
    varupdate = input(f'  {ANSI_upd}Update {ANSI_wt}variable{ANSI_upd} target date Nodes? ({Yes_no(ANSI_upd)}) ')
    if (varupdate != 'n'):
        print(f'  {ANSI_lt}Updating variable target date Nodes.{ANSI_nrm}')
        thecmd = 'fzupdate -q -E STDOUT -u'+addtocmd
        retcode = try_subprocess_check_output(thecmd, 'varupdate', config)
        exit_error(retcode, f'Attempt to update variable target date Nodes failed.{cmderrorreviewstr}', True)
        cycle_updated = True
    else:
        print(f'  {ANSI_lt}Not updating.{ANSI_nrm}.')

    if cycle_updated:
        refresh_Next_Nodes()
    print('')


def refresh_panes():
    refresh_cmd = 'panes-term.sh -R'
    retcode = try_subprocess_check_output(refresh_cmd, 'refresh_panes', config)


def open_chunk(node, args):
    pause_key('open new chunk',config['addpause'])
    thecmd = 'fzlog -c ' + node
    if args.T_emulate:
        thecmd += ' -t ' + args.T_emulate
    else:
        # this was added to prevent a gap due to time taken while selecting (see FIX THIS list item in https://trello.com/c/I2f2kvmc)
        thecmd += ' -t ' + last_T_close
    if config['verbose']:
        thecmd += ' -V'
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to open new Log chunk failed.{cmderrorreviewstr}', True)
    return retcode


def next_chunk(args):
    # Closing the previous chunk is automatically done as part of this in fzlog.
    pause_key('select next Node',config['addpause'])
    node = select_Node_for_Log_chunk()
    if not node:
        exit_error(1, 'Attempt to select Node for new Log chunk failed.', True)
    else:
        retcode = open_chunk(node, args)
        if (retcode == 0):
            print(f'Opened new Log chunk for Node {node}.')
        else:
            node = ''
    return node


def get_completion_required(node):
    # *** This function is presently not yet being used, but see the note further
    #     below about testing completion >= 1.0.
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


def simple_emulated_time_check(T_candidate, args):
    if not is_TimeStamp(T_candidate):
        return False
    if is_Future(T_candidate):
        return False
    if (int(T_candidate) <= int(last_T_close)):
        return False
    args.T_emulate = T_candidate
    return True


def chunk_interval_alert():
    print(f'{ANSI_or}Chunk time passed. Calling formalizer-alert.sh.{ANSI_nrm}')
    thecmd = 'formalizer-alert.sh'
    retcode = try_subprocess_check_output(thecmd, 'alert', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Call to formalizer-alert.sh failed.{cmderrorreviewstr}', True)
    fztask_ansi()
    if (retcode != 0):
        return 'r'
    return 'N'


def chunk_interval_interrupted(args):
    print(f'\n{ANSI_alert}Chunk timer interrupted.{ANSI_nrm} {ANSI_pu}Options:\n')
    print(f'  - {ANSI_pu}[{ANSI_gn}N{ANSI_nrm}{ANSI_pu}]ew chunk at actual current time.')
    print(f'  - {ANSI_pu}Never mind, [{ANSI_mg}r{ANSI_nrm}{ANSI_pu}]esume the present chunk.')
    print(f'  - {ANSI_pu}New chunk at a specified [{ANSI_yb}e{ANSI_nrm}{ANSI_pu}]mulated time.')
    print(f'  - {ANSI_pu}E[{ANSI_rd}x{ANSI_nrm}{ANSI_pu}]it.{ANSI_nrm}')
    proceed_choice = input(f'\n{ANSI_sel}Your choice? ')
    if (proceed_choice == 'x'):
        print(f'{ANSI_lt}Exiting.')
        sys.exit(0)
    if (proceed_choice == 'e'):
        valid_T_emulate = False
        while not valid_T_emulate:
            T_candidate = input(f'\n{ANSI_sel}New emulated time (YYYYmmddHHMM):{ANSI_nrm} ')
            valid_T_emulate = simple_emulated_time_check(T_candidate, args)
    return proceed_choice


def set_interval_duration(node):
    if not config['exact_Node_intervals']:
        return 60*config['chunkminutes']
    tdproperty, required_mins = get_node_data(node, 'tdproperty, required', config)
    #print(f'--DEBUG: tdproperty={tdproperty}, required={required}')
    #tdproperty='debug'
    if (tdproperty == 'exact'):
        return 60*int(required_mins)
    else:
        return 60*config['chunkminutes']
    

def set_chunk_timer_and_alert(args, node):
    # It looks like I can just run the same formalizer-alert.sh that dil2al was using.
    interval_seconds = set_interval_duration(node)
    proceed_choice = 'r'
    while (proceed_choice == 'r'):
        print(f'{ANSI_lt}Setting chunk duration: {ANSI_yb}{int(interval_seconds/60)}{ANSI_nrm}{ANSI_lt} mins. Chunk starts now.')
        try:
            time.sleep(interval_seconds)
            proceed_choice = chunk_interval_alert()

        except KeyboardInterrupt:
            proceed_choice = chunk_interval_interrupted(args)
        interval_seconds = 60*config['chunkminutes'] # resume for only 20 minutes (this could be improved)


def task_control(args):
    make_log_entry()

    chunkchoice = new_or_close_chunk()

    # Note that in this process, where a schedule updated in accordance with time
    # added to the most recent Node's completion ratio affects the potential choice
    # of Node for the next Log chunk, we cannot make use of the built-in chunk
    # closing that is available through `fzlog -c <node-id>`. Instead, we must
    # close the chunk first, then update the schedule, and use the resulting
    # information for an informed choice.
    pause_key('close current chunk',config['addpause'])
    close_chunk(args) # from here-on last_T_close contains the t_close of the preceding chunk

    pause_key('update schedule',config['addpause'])
    update_schedule(args)

    node = ''
    if (chunkchoice == 'c'):
        refresh_panes()
        return chunkchoice

    pause_key('start next chunk',config['addpause'])
    node = next_chunk(args)
    refresh_panes() # after opening the next chunk
    if not node:
        print(f'{ANSI_alert}We have no next Node, so we behave as if "close chunk" was chosen.{ANSI_nrm}')
        return 'c'

    # ** close_chunk() and next_chunk() could both return the new completion ratio of the
    # ** Node that owns the previous chunk (or at least a true/false whether completion >= 1.0)
    # ** and that could be used to check with the caller whether the Node really should
    # ** be considered completed. If not, then there is an opportunity to change the
    # ** time required or to set a guess for the actual completion ratio.

    if config['transition']:
        pause_key('synchronize back to Formalizer 1.x',config['addpause'])
        transition_dil2al_request(node, args)

    # remove any T_emulate as we proceed through the next time interval
    args.T_emulate = None
    last_node = node
    pause_key('start the chunk timer',config['addpause'])
    set_chunk_timer_and_alert(args, node)
    return chunkchoice

# ----- end: Local variables and functions -----

# *** Note:
# *** - fztask.py presently uses a while-loop to act like a daemonized task server.
# *** - One alternative is to set a cron-job.
# *** - Another alternative is to launch or use a separate deamonized task-timer.
# *** - Or simply exit after one run-through of the fztask steps.
# *** These options can be made a configuration option.

if __name__ == '__main__':

    core_version = coreversion.coreversion()
    fztask_long_id = "Control:Task" + f" v{version} (core v{core_version})"

    get_server_address(fzuserbase)

    fztask_ansi()

    print(fztask_long_id+"\n")

    lock_it(fztasklockfile)

    args = parse_options()

    check_emulated_time()

    chunkchoice = 'S'

    while (chunkchoice != 'c'):
        chunkchoice = task_control(args)

    print(f'\n{ANSI_nrm}fztask done.')

    unlock_it(fztasklockfile, config)

    pausekey = pause_key('exit')

    sys.exit(0)
