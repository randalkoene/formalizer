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


# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'
# *** can add logentryconfigdir and logentryconfig here

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

version = "0.1.0-0.1"

# local defaults
config['contenttmpfile'] = '/tmp/logentry.html'
config['editor'] = 'emacs'

# *** here you could replace local defaults with values from ~/.formalizer/config/logentry/config.json



def parse_options():
    theepilog = ('See the Readme.md in the logentry source directory for more information.\n')

    parser = argparse.ArgumentParser(description='Make a Log entry.',epilog=theepilog)
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


def make_content_file():
    emptystr = ''
    with open(config['contenttmpfile'],'w') as f:
        f.write(emptystr)


def edit_content_file():
    retcode = try_subprocess_check_output(f"{config['editor']} {config['contenttmpfile']}", '')
    if (retcode != 0):
        print(f'Attempt to edit content file failed.')
        exit(retcode)
    
    with open(config['contenttmpfile'],'r') as f:
        entrycontent = f.read()
    
    return entrycontent


def get_from_Named_Node_Lists(list_name, output_format, resstore):
    retcode = try_subprocess_check_output(f"fzgraphhtml -L '{list_name}' -F {output_format} -x 60 -N 5 -e -q",resstore)
    if (retcode != 0):
        print(f'Attempt to get Named Node List data failed.')
        exit(retcode)

def get_updated_shortlist():
    retcode = try_subprocess_check_output(f"fzgraphhtml -u -L 'shortlist' -F node -e -q", 'shortlistnode')
    if (retcode != 0):
        print(f'Attempt to get "shortlist" Named Node List node data failed.')
        exit(retcode)
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'shortlist' -F desc -x 60 -e -q", 'shortlistdesc')
    if (retcode != 0):
        print(f'Attempt to get "shortlist" Named Node List description data failed.')
        exit(retcode)

def get_from_Incomplete(output_format, resstore):
    retcode = try_subprocess_check_output(f"fzgraphhtml -I -F {output_format} -x 60 -N 5 -e -q",resstore)
    if (retcode != 0):
        print(f'Attempt to get Incomplete Nodes data failed.')
        exit(retcode)


def browse_for_Node():
    print('Use the browser to select a node.')
    #retcode = try_subprocess_check_output(f"urxvt -e w3m http://localhost/index.html",'')
    #if (retcode != 0):
    #    print(f'Attempt to browse for Node failed.')
    #    exit(retcode)
    #run_curses_tty(['w3m','http://localhost/index.html'])
    retcode = try_subprocess_check_output(f"fzgraph -L delete -l 'selected' -q",'') # *** we can get rid of this when features are enabled
    if (retcode != 0):
        print(f'Attempt to clear "selected" Named Node List failed.')
        exit(retcode)    
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


def entry_belongs_to_same_or_other_Node():
    get_updated_shortlist()
    shortlist_nodes = results['shortlistnode']
    shortlist_desc = results['shortlistdesc']
    print('Short-list of Nodes for this Log Entry:')
    #filtered = filter(lambda x: not re.match(r'^\s*$', x), shortlist_desc.decode().splitlines())
    #print(str(filtered))
    #shortlist_prettyprint = os.linesep.join([s for s in shortlist_desc.decode().splitlines() if s.strip()])
    #print(shortlist_prettyprint)
    shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
    for (number, line) in enumerate(shortlist_vec):
        print(f' {number}: {line}')

    choice = input('[D]efault same Node as chunk, or [0-9] from shortlist, or [?] browse? ')
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
    return node


def send_to_fzlog(node):
    thecmd=f"fzlog -e -f {config['contenttmpfile']}"
    if node:
        thecmd += f" -n {node}"
    retcode = try_subprocess_check_output(thecmd, '')
    if (retcode != 0):
        print(f'Attempt to add Log entry via fzlog failed.')
        exit(retcode)
    print('Entry added to Log.')


def transition_dil2al_polldaemon_request(node, entrycontent):
    # *** if this is being used during the transition then do this
    # *** put makenote request with logentry data and possible Node ID where dil2al-polldaemon.sh will find it
    print('NOT YET IMPLEMENTED!')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    logentry_long_id = f"Formalizer:Interface:CommandLine:LogEntry v{version} (core v{core_version})"

    print(logentry_long_id+"\n")

    args = parse_options()

    make_content_file()

    entrycontent = edit_content_file()
    if len(entrycontent)<1:
        print('Empty Log entry.')
        exit(1)

    node = entry_belongs_to_same_or_other_Node()

    send_to_fzlog(node)

    exit(0) # remove this

    transition_dil2al_polldaemon_request(node, entrycontent)

exit(0)
