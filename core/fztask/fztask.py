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
config['transition'] = 'true'

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


def update_schedule():
    print('UPDATING SCHEDULE NOT YET IMPLEMENTED!')
    print('Presently, the shortlist is simply using the target date sorted list of')
    print('incomplete Nodes. There may well be a more desirable arrangement of Nodes')
    print('to suggest.')


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
    if (retcode != 0):
        print('Attempt to get Node topic failed.')
        exit(retcode)
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
    customtemplate = '{{ completion }} {{ required }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={config['customtemplate']}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'compreq')
    if (retcode != 0):
        print('Attempt to get Node completion and required failed.')
        exit(retcode)
    results['completion'] = (results['compreq'].split()[0]).decode()
    results['required'] = (results['compreq'].split()[1]).decode()


def transition_dil2al_request(node):
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
    set_DIL_entry_preset(node)
    thecmd = "dil2al -C -u no -p 'noaskALDILref'"
    retcode = try_subprocess_check_output(thecmd, 'dil2al_chunk')
    if (retcode != 0):
        print('Call to dil2al -C failed.')
        exit(retcode)
    get_completion_required(node)
    completion = results['completion']
    required = results['required']
    thecmd = f"w3m '/cgi-bin/dil2al?dil2al=MEi&DILID=20070113232521.1&required={required}&completion={completion}'"
    retcode = try_subprocess_check_output(thecmd, 'dil2al_compreq')
    if (retcode != 0):
        print('GET call to dil2al failed.')
        exit(retcode)


def set_chunk_timer_and_alert():
    print('SETTING TIMER AND ALERT NOT YET IMPLEMENTED!')
    print('Right now, you will have to manually remember to switch to the next')
    print('chunk after a reasonable amount of time.')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    fztask_long_id = "Control:Task" + f" v{version} (core v{core_version})"

    print(fztask_long_id+"\n")

    args = parse_options()

    make_log_entry()

    chunkchoice = new_or_close_chunk()

    # Note that in this process, where a schedule updated in accordance with time
    # added to the most recent Node's completion ratio affects the potential choice
    # of Node for the next Log chunk, we cannot make use of the built-in chunk
    # closing that is available through `fzlog -c <node-id>`. Instead, we must
    # close the chunk first, then update the schedule, and use the resulting
    # information for an informed choice.
    close_chunk()

    update_schedule()

    node = ''
    if (chunkchoice !='c'):
        node = next_chunk()
        set_chunk_timer_and_alert()
    # ** close_chunk() and next_chunk() could both return the new completion ratio of the
    # ** Node that owns the previous chunk (or at least a true/false whether completion >= 1.0)
    # ** and that could be used to check with the caller whether the Node really should
    # ** be considered completed. If not, then there is an opportunity to change the
    # ** time required or to set a guess for the actual completion ratio.

    if config['transition'] == 'true':
        transition_dil2al_request(node)

    print('BECAUSE THERE IS NO TIMER-ALERT YET, PAUSING HERE AS A REMINDER.')
    print('Some choices:')
    print('- Put a sleep timer here, an alert-call at the start of __main__, and')
    print('  embed content of __main__ in a while-loop. In this case, this program,')
    print('  fztask, operates as a memory-resident, daemonized task server.')
    print('- Set a cron-job (at-job).')
    print('- Launch or use a a separate daemonized task-timer.')
    print('- Do none of that and simply exit here.')
    print('These options can be made a configuration option.\n')
    pausekey = input('Enter any string to exit...')

sys.exit(0)
