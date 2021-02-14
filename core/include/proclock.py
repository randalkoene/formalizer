# Copyright 2021 Randal A. Koene
# License TBD

"""
This header file declares functions for process detection and lock files.

"""

import os
from ansicolorcodes import *
from error import exit_error
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
            os.remove(lockfile)
    try:
        with open(lockfile,'w') as f:
            f.write(NowTimeStamp())
    except IOError:
        exit_error(1, f'Unable to write lock file {lockfile}.')
