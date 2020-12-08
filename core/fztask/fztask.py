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

# core components
import Graphpostgres
import coreversion
import error

version = "0.1.0-0.1"

# local defaults
# config['transition'] = 'true' # reading this from config/fzsetup.py/config.json now
config['customtemplate'] = '/tmp/customtemplate'
config['addpause'] = False
config['cmdlog'] = '/tmp/fztask-cmdcalls.log'
config['logcmdcalls'] = False

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
    print(u'\u001b[32m', end='')


def Node_selection_ansi():
    print(u'\u001b[38;5;$33m', end='')


def alert_ansi():
    print(u'\u001b[31m', end='')


def pause_key(action_str, pausehere = True):
    if pausehere:
        pausekey = input(f'\nEnter any string to {action_str}...')
    else:
        pausekey = '_'
    return pausekey


def exit_error(retcode, errormessage):
    if (retcode != 0):
        alert_ansi()
        print('\n'+errormessage+'\n')
        fztask_ansi()
        exitenter = pause_key('exit')
        sys.exit(retcode)


def make_log_entry():
    print('Launching logentry...')
    #thecmd = 'logentry'
    #if config['verbose']:
    #    thecmd += ' -v'
    #retcode = try_subprocess_check_output(thecmd, 'logentry_res')
    thecmd = ['logentry']
    if config['verbose']:
        thecmd = ['logentry','-v']
    retcode = pty.spawn(thecmd)
    print('Back from logentry.')
    if not os.WIFEXITED(retcode):
        exit_error(os.WEXITSTATUS(retcode), 'Attempt to make Log entry failed.')


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
    print(u'\u001b[32m', end='')
    choice = input('\n[S]tart a Log chunk for the next task, or merely [c]lose the chunk? ')
    if (choice != 'c'):
        choice = 'S'
        print('Starting the next task chunk...')
    else:
        print('Closing Log chunk without immediately starting another...')

    return choice


def close_chunk(args):
    thecmd = 'fzlog -C'
    if args.T_emulate:
        thecmd += ' -t ' + args.T_emulate
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res')
    exit_error(retcode,'Attempt to close Log chunk failed.')


def get_updated_shortlist():
    retcode = try_subprocess_check_output(f"fzgraphhtml -u -L 'shortlist' -F node -e -q", 'shortlistnode')
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List node data failed.')
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'shortlist' -F desc -x 60 -e -q", 'shortlistdesc')
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List description data failed.')


def browse_for_Node():
    print('Use the browser to select a node.')
    #retcode = pty.spawn([config['localbrowser'],'http://localhost/select.html'])
    thecmd = config['localbrowser'] + ' http://localhost/select.html'
    retcode = try_subprocess_check_output(thecmd, 'browsed')
    exit_error(retcode, 'Attempt to browse for Node selection failed.')
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'selected' -F node -N 1 -e -q",'selected')
    exit_error(retcode, 'Attempt to get selected Node failed.')
    print(f'Selected: {results["selected"]}')
    if results['selected']:
        return results['selected'][0:16]
    else:
        return ''


def select_Node_for_Log_chunk():
    get_updated_shortlist()
    shortlist_nodes = results['shortlistnode']
    shortlist_desc = results['shortlistdesc']
    Node_selection_ansi()
    print('\nShort-list of Nodes for the New Log Chunk:')
    shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
    for (number, line) in enumerate(shortlist_vec):
        print(f' {number}: {line}')

    choice = input('Use:\n- [0-9] from shortlist, or\n- [?] to browse: ')
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
    fztask_ansi()
    return node


def update_schedule(args):
    alert_ansi()
    print('\nUPDATING SCHEDULE NOT YET IMPLEMENTED!')
    print('Presently, the shortlist is simply using the target date sorted list of')
    print('incomplete Nodes. There may well be a more desirable arrangement of Nodes')
    print('to suggest.\n')
    fztask_ansi()


def next_chunk(args):
    # Closing the previous chunk is automatically done as part of this in fzlog.
    pause_key('select next Node',config['addpause'])
    node = select_Node_for_Log_chunk()
    if not node:
        exit_error(1, 'Attempt to select Node for new Log chunk failed.')

    pause_key('open new chunk',config['addpause'])
    thecmd = 'fzlog -c ' + node
    if args.T_emulate:
        thecmd += ' -t ' + args.T_emulate
    retcode = try_subprocess_check_output(thecmd, 'fzlog_res')
    exit_error(retcode, 'Attempt to open new Log chunk failed.')

    print(f'Opened new Log chunk for Node {node}.')
    return node


def get_main_topic(node):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
    #     E.g. could call fzgraph -C or curl with the corresponding URL.
    customtemplate = '{{ topics }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={config['customtemplate']}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'topic')
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


def get_completion_required(node):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
    #     E.g. could call fzgraph -C or curl with the corresponding URL.
    customtemplate = '{{ comp }} {{ req_hrs }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={config['customtemplate']}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'compreq')
    exit_error(retcode, 'Attempt to get Node completion and required failed.')
    results['completion'] = (results['compreq'].split()[0]).decode()
    results['required'] = (results['compreq'].split()[1]).decode()


def get_most_recent_task():
    thecmd = 'fzloghtml -R -o STDOUT -N -F raw -q'
    retcode = try_subprocess_check_output(thecmd, 'recentlog')
    exit_error(retcode, 'Attempt to get most recent Log chunk data failed.')
    recent_node = (results['recentlog'].split()[2]).decode()
    return recent_node


#def transition_dil2al_request(recent_node, node):
def transition_dil2al_request(node, args):
    # - set 'flagcmd' in dil2al/controller.cc:chunk_controller() such that no alert is called
    # - provide a command string to automatically answer confirmation() 'N' about making a note
    # - provide 'S' or 't' to start a new chunk or simply close the chunk, for auto_interactive()
    # - case 'S': decide_add_TL_chunk(): perhaps set timechunkoverstrategy=TCS_NEW and timechunkunderstrategy,
    #   then like DIL_entry selection in logentry
    # - case 't': stop_TL_chunk(): could modify alautoupdate if I want to prevent AL update and
    #   set completion ratios directly instead of automatically
    # - set 'isdaemon' false to prevent waiting in loop in schedule_controller()
    # - possibly also prevent an at-command from being created
    # *** If this is all too difficult, I can just call `dil2al -C` and let me figure it out manually.
    #     And note that `dil2al -u` sets alautoupdate to no, yes or ask. Note that `dil2al -C` does
    #     not appear to set a timer or at-command (in fact, it seems that the `at` program is not
    #     even installed on aether).
    print('For transition synchronization back to Formalizer 1.x:')
    set_DIL_entry_preset(node)
    print(f'  preset DIL entry selection to {node}')
    # thecmd = "urxvt -e dil2al -C -u no -p 'noaskALDILref' -p 'noshowflag'"
    # print(f'  calling `{thecmd}` for:\n  alautoupdate=no, no timer setting, and use the (preset) default as the DIL entry')
    thecmd = "urxvt -e dil2al -C -u yes -p 'noaskALDILref' -p 'noshowflag'"
    if args.T_emulate:
        thecmd += ' -T ' + args.T_emulate
    print(f'  calling `{thecmd}` for:\n  alautoupdate=yes, no timer setting, and use the (preset) default as the DIL entry')
    retcode = try_subprocess_check_output(thecmd, 'dil2al_chunk')
    exit_error(retcode, 'Call to dil2al -C failed.')

    # *** I can'd do the steps below and need to let dil2al do its thing instead, because of the
    #     complex nature of targetdate updates through links to Superiors.
    # get_completion_required(node)
    # completion = results['completion']
    # required = results['required']
    # print(f'  synchronizing {recent_node} completion={completion} and required={required}')
    # thecmd = f"urxvt -e w3m '/cgi-bin/dil2al?dil2al=MEi&DILID={recent_node}&required={required}&completion={completion}'"
    # retcode = try_subprocess_check_output(thecmd, 'dil2al_compreq')
    # exit_error(retcode, 'GET call to dil2al failed.')


def set_chunk_timer_and_alert():
    # It looks like I can just run the same formalizer-alert.sh that dil2al was using.
    print('Setting chunk duration: 20 mins. Chunk starts now.')
    time.sleep(1200)
    alert_ansi()
    print('Chunk time passed. Calling formalizer-alert.sh.')
    thecmd = 'formalizer-alert.sh'
    retcode = try_subprocess_check_output(thecmd, 'alert')
    exit_error(retcode, 'Call to formalizer-alert.sh failed.')
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

    return chunkchoice


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

    print('\nfztask done.')

    pausekey = pause_key('exit')

sys.exit(0)
