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


#def init_screen():
#    curses.start_color() # load colors
#    curses.use_default_colors()
#    curses.noecho()      # do not echo text
#    curses.cbreak()      # do not wait for "enter"
#    curses.mousemask(curses.ALL_MOUSE_EVENTS)
#
#    # Hide cursor, if terminal AND curse supports it
#    if hasattr(curses, 'curs_set'):
#        try:
#            curses.curs_set(0)
#        except:
#            pass


# def run_curses_tty(thecmdstring):
#     stdscr = curses.initscr()
#     init_screen()
#     stdscr.keypad(1)
#     # Prepare screen for interactive command
#     curses.savetty()
#     curses.nocbreak()
#     curses.echo()
#     curses.endwin()

#     # Run command
#     pty.spawn(thecmdstring)

#     # Restore screen
#     init_screen()
#     curses.resetty()


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
config['contenttmpfile'] = '/tmp/logentry.html'
config['customtemplate'] = '/tmp/customtemplate'
config['cmdlog'] = '/tmp/logentry-cmdcalls.log'
config['logcmdcalls'] = True
# config['editor'] = 'emacs' # reading this from config/fzsetup.py/config.json now
# config['transition'] = 'true' # reading this from config/fzsetup.py/config.json now

# replace local defaults with values from ~/.formalizer/config/logentry/config.json
#try:
#    with open(logentryconfig) as f:
#        config += json.load(f)
#
#except FileNotFoundError:
#    print('Configuration files for logentry missing. Continuing with defaults.\n')


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
    print(u'\u001b[38;5;$208m', end='')


def Node_selection_ansi():
    print(u'\u001b[38;5;$33m', end='')


def alert_ansi():
    print(u'\u001b[31m', end='')


def exit_error(retcode, errormessage):
    if (retcode != 0):
        alert_ansi()
        print('\n'+errormessage+'\n')
        logentry_ansi()
        exitenter = input('Press ENTER to exit...')
        sys.exit(retcode)


def make_content_file():
    emptystr = ''
    with open(config['contenttmpfile'],'w') as f:
        f.write(emptystr)


def edit_content_file():
    retcode = try_subprocess_check_output(f"{config['editor']} {config['contenttmpfile']}", '')
    exit_error(retcode, 'Attempt to edit content file failed.')
    
    print('Reading edited content file...')
    with open(config['contenttmpfile'],'r') as f:
        entrycontent = f.read()
    
    return entrycontent


def get_from_Named_Node_Lists(list_name, output_format, resstore):
    retcode = try_subprocess_check_output(f"fzgraphhtml -L '{list_name}' -F {output_format} -x 60 -N 5 -e -q",resstore)
    exit_error(retcode, 'Attempt to get Named Node List data failed.')


def get_updated_shortlist():
    print('Getting updated shortlist...')
    retcode = try_subprocess_check_output(f"fzgraphhtml -u -L 'shortlist' -F node -e -q", 'shortlistnode')
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List node data failed.')
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'shortlist' -F desc -x 60 -e -q", 'shortlistdesc')
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List description data failed.')


def get_from_Incomplete(output_format, resstore):
    retcode = try_subprocess_check_output(f"fzgraphhtml -I -F {output_format} -x 60 -N 5 -e -q",resstore)
    exit_error(retcode, 'Attempt to get Incomplete Nodes data failed.')


def browse_for_Node():
    print('Use the browser to select a node.')
    #retcode = try_subprocess_check_output(f"urxvt -e {config['localbrowser']} http://localhost/index.html",'')
    #if (retcode != 0):
    #    print(f'Attempt to browse for Node failed.')
    #    exit(retcode)
    #run_curses_tty([config['localbrowser'],'http://localhost/index.html']) 
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


def entry_belongs_to_same_or_other_Node():
    get_updated_shortlist()
    shortlist_nodes = results['shortlistnode']
    shortlist_desc = results['shortlistdesc']
    Node_selection_ansi()
    print('\nShort-list of Nodes for this Log Entry:')
    #filtered = filter(lambda x: not re.match(r'^\s*$', x), shortlist_desc.decode().splitlines())
    #print(str(filtered))
    #shortlist_prettyprint = os.linesep.join([s for s in shortlist_desc.decode().splitlines() if s.strip()])
    #print(shortlist_prettyprint)
    shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
    for (number, line) in enumerate(shortlist_vec):
        print(f' {number}: {line}')

    choice = input('\nUse:\n- [d]efault, same Node as chunk, or\n- [0-9] from shortlist, or\n- [?] browse? ')
    if (choice == '?'):
        node = browse_for_Node()
    else:
        if ((choice >= '0') & (choice <= '9')):
                node = shortlist_nodes.splitlines()[int(choice)]
        else:
            node = '' # default
    if node:
        node = node.decode()
        print(f'Log entry belongs to Node {node}.')
    else:
        print(f'Log entry belongs to the same Node as the Log chunk.')
    logentry_ansi()
    return node


def send_to_fzlog(node):
    thecmd=f"fzlog -e -f {config['contenttmpfile']}"
    if node:
        thecmd += f" -n {node}"
    retcode = try_subprocess_check_output(thecmd, '')
    exit_error(retcode, 'Attempt to add Log entry via fzlog failed.')
    print('Entry added to Log.')


def get_main_topic(node):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
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
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(dilpreset+'\n')


def transition_dil2al_polldaemon_request(node):
    if node:
        set_DIL_entry_preset(node)
    else:
        if os.path.exists(userhome+'/.dil2al-DILidpreset'):
            os.remove(userhome+'/.dil2al-DILidpreset')
    thecmd=f"dil2al -m{config['contenttmpfile']} -p 'noaskALDILref' -p 'noalwaysopenineditor'"
    retcode = try_subprocess_check_output(thecmd, 'dil2al')
    exit_error(retcode, 'Call to dil2al -m failed.')
    #retcode = pty.spawn(['dil2al',f"-m{config['contenttmpfile']}","-p'noaskALDILref'","-p'noalwaysopenineditor'"])
    #if not os.WIFEXITED(retcode):
    #    exit_error(os.WEXITSTATUS(retcode), 'Call to dil2al -m failed.')
    print('Log entry synchronized to Formalizer 1.x files.')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    logentry_long_id = f"Formalizer:Interface:CommandLine:LogEntry v{version} (core v{core_version})"

    logentry_ansi()

    print(logentry_long_id+"\n")

    args = parse_options()

    make_content_file()

    entrycontent = edit_content_file()
    if len(entrycontent)<1:
        exit_error(1, 'Empty Log entry.')

    node = entry_belongs_to_same_or_other_Node()

    send_to_fzlog(node)

    if config['transition']:
        transition_dil2al_polldaemon_request(node)

    if args.waitexit:
        print('Done.', end='', flush=True)
        for i in range(int(args.waitexit)):
            time.sleep(1)
            print('.', end='', flush=True)
        print('')


sys.exit(0)
