#!/usr/bin/python3
#
# fzuistate.py
#
# Randal A. Koene, 20210308
#
# {{ brief_title }}. See Readme.md for more information.

# std
import os
import sys
import json
import argparse
import subprocess
import re
# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import cgi
sys.stderr = sys.stdout
import traceback
from io import StringIO
from traceback import print_exc


# Standardized expectations.
# userhome = os.getenv('HOME')
# fzuserbase = userhome + '/.formalizer'
# fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
# fzsetupconfig = fzsetupconfigdir+'/config.json'
#fzuistateconfigdir = fzuserbase+'/config/fzuistate.py'
#fzuistateconfig = fzuistateconfigdir+'/config.json'

webdata_path = "/var/www/webdata/formalizer"
uistatefile = webdata_path+'/fzuistate.json'

config = {
    'verbose' : False
}
results = {}

ui_state = {
    #'darkmode' : False,
    'darkmode' : 0,
    'something' : 100
}

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
# try:
#     with open(fzsetupconfig) as f:
#         config = json.load(f)

# except FileNotFoundError:
#     print('Configuration files missing. Please run fzsetup.py first!\n')
#     exit(1)

# else:
#     assert len(config) > 0

# Enable us to import standardized Formalizer Python components.
# fzcorelibdir = config['sourceroot'] + '/core/lib'
# fzcoreincludedir = config['sourceroot'] + '/core/include'
# sys.path.append(fzcorelibdir)
# sys.path.append(fzcoreincludedir)

# core components
# import Graphpostgres
# import coreversion

version = "0.1.0-0.1"

# local defaults
#config['something'] = 'some/kind/of/default'

# replace local defaults with values from ~/.formalizer/config/fzuistate.py/config.json
#try:
#    with open(fzuistateconfig) as f:
#        config += json.load(f)
#
#except FileNotFoundError:
#    print('Configuration files for fzuistate missing. Continuing with defaults.\n')


# def parse_options():
#     theepilog = ('See the Readme.md in the fzuistate.py source directory for more information.\n')

#     parser = argparse.ArgumentParser(description='{{ brief_title }}.',epilog=theepilog)
#     parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
#     parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
#     parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')

#     args = parser.parse_args()

#     if not args.dbname:
#         args.dbname = 'formalizer'
#     if not args.schemaname:
#         args.schemaname = os.getenv('USER')
#     if args.verbose:
#         config['verbose'] = True

#     print('Working with the following targets:\n')
#     print(f'  Formalizer Postgres database name   : {args.dbname}')
#     print(f'  Formalizer user or group schema name: {args.schemaname}\n')

#     #choice = input('Is this correct? (y/N) \n')
#     #if (choice != 'y'):
#     #    print('Ok. You can try again with different command arguments.\n')
#     #    exit(0)

#     return args


# def some_function():
#     pass


# def a_function_that_calls_subprocess(some_arg, resstore):
#     retcode = try_subprocess_check_output(f"someprogram -A '{some_arg}'", resstore)
#     if (retcode != 0):
#         print(f'Attempt to do something failed.')
#         exit(retcode)


# def a_function_that_spawns_a_call_in_pseudo_TTY(some_arg):
#     retcode = pty.spawn(['someprogram','-A', some_arg])
#     return retcode


# def a_function_that_requests_input():
#     shortlist_desc = results['shortlistdesc']
#     shortlist_vec = [s for s in shortlist_desc.decode().splitlines() if s.strip()]
#     for (number, line) in enumerate(shortlist_vec):
#         print(f' {number}: {line}')

#     choice = input('[D]efault same Node as chunk, or [0-9] from shortlist, or [?] browse? ')
#     if (choice == 'd'):
#         node = '' # default

#     return node

def simple_ok_response():
    print('Content-Type: text/plain\n\n')
    print('OK')


def simple_error_response():
    print('Content-Type: text/plain\n\n')
    print('ERROR')


def attempt_write_ui_state(uistate: dict) -> bool:
    try:
        with open(uistatefile, 'w') as f:
            json.dump(uistate, f)
        return True
    except:
        return False


def attempt_send_ui_state(uistate: dict):
    print('Content-Type: text/plain\n\n')
    print(json.dumps(uistate))


def handle_ui_state_update():
    form = cgi.FieldStorage()

    # in fzuistate_twoonly.js: darkmode expects '0' or '1'
    # in fzuistate.js: darkmode expects an integer between 0 and nummodes-1 (see fzuistate.js).
    darkmode = form.getvalue('darkmode')
    if darkmode:
        ui_state['darkmode'] = int(darkmode) # bool(int(darkmode))
        if (attempt_write_ui_state(ui_state)):
            simple_ok_response()
        else:
            simple_error_response()

    else:
        # We interpret this as a request for stored state
        attempt_send_ui_state(ui_state)


def load_stored_ui_state():
    try:
        with open(uistatefile, 'r') as f:
            stored_state = json.load(f)
        ui_state.update(stored_state)
    except:
        # just work with the defaults
        pass


if __name__ == '__main__':

    # core_version = coreversion.coreversion()
    # fzuistate_long_id = "Interface:UI:State" + f" v{version} (core v{core_version})"

    # print(fzuistate_long_id+"\n")

    # args = parse_options()

    load_stored_ui_state()

    handle_ui_state_update()

sys.exit(0)
