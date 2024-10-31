# Copyright 2021 Randal A. Koene
# License TBD

"""
This header file declares functions for process detection and lock files.

"""

import os
from ansicolorcodes import *
from TimeStamp import NowTimeStamp

def unlock_it(lockfile: str, config: dict):
    if os.path.exists(lockfile):
        os.remove(lockfile)
    else:
        if config['verbose']:
            print(f'{ANSI_warn}Lock file {lockfile} to be removed did not exist.{ANSI_nrm}')


def lock_it(lockfile: str):
    ANSI_sel = '\u001b[38;5;33m'
    if os.path.exists(lockfile):
        print(f'{ANSI_alert}Lock file {lockfile} found!{ANSI_nrm}')
        goaheadanyway = input(f'{ANSI_sel}Another instance of fztask may be running. Go ahead anyway? ({No_yes(ANSI_sel)}) ')
        if (goaheadanyway != 'y'):
            print('Exiting.')
            sys.exit(1)
        else:
            # check again, because while waiting for input you may have closed the other instance and removed the lock already
            # unless there was no other instance and the lock was a remnant
            if os.path.exists(lockfile):
                os.remove(lockfile)
    try:
        with open(lockfile,'w') as f:
            f.write(NowTimeStamp())
    except IOError:
        print(f'\n{ANSI_alert}Unable to write lock file {lockfile}.{ANSI_nrm}\n')
        exitenter = input('\nEnter any string to exit...')
        sys.exit(1)

def called_as_cgi()->bool:
    return 'GATEWAY_INTERFACE' in list(os.environ.keys())

def make_runstamp()->str:
    from time import strftime
    return strftime("%Y%m%d%H%M%S")

# Use this to prevent a CGI script from being rerun if its output
# page is reloaded.
# @param form the cgi.FormStorage object.
# @param runstampfile Something like '/tmp/.thisscript.stamp'.
# @param blocked_func The function to run if the gate is blocked.
class ReloadGate:
    def __init__(self, form, runstampfile:str, blocked_func):
        from sys import exit
        self.gate_passed = False
        # Get the stored previous runstamp (if there is one) for this script.
        self.stamperror = ''
        try:
            with open(runstampfile, 'r') as f:
                self.storedstamp = f.read()
        except Exception as e:
            self.stamperror += 'Unable to load stored stamp: %s\n' % str(e)
            self.storedstamp = ''

        # Get the runstamp for this script as provided when called.
        self.runstamp = form.getvalue("runstamp")

        # Compare runstamps to see if the one provided is newer.
        if self.storedstamp != '':
            if int(self.storedstamp) >= int(self.runstamp):
                blocked_func(self)
                exit(0)

        # Store runstamp.
        try:
            with open(runstampfile, 'w') as f:
                f.write(self.runstamp)
        except Exception as e:
            self.stamperror += 'Unable to store stamp: %s\n' % str(e)

        self.gate_passed = True
