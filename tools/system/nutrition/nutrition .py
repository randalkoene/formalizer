#!/usr/bin/python3
#
# nutrition.py
#
# Randal A. Koene, 20210127
#
# Calculate nutrition for daily consumption.

import os
import sys
import json

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
    assert len(config) > 0

# Enable us to import standardized Formalizer Python components.
fzcorelibdir = config['sourceroot'] + '/core/lib'
fzcoreincludedir = config['sourceroot'] + '/core/include'
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)

import coreversion
from ansicolorcodes import *
from fzcmdcalls import *

version = "0.1.0-0.1"

if __name__ == '__main__':

    core_version = coreversion.coreversion()
    logentry_long_id = f"Formalizer:System:Health:Nutrition v{version} (core v{core_version})"

    print(logentry_long_id+"\n")

    #args = parse_options()

    print(f'{ANSI_alert}Nothing here yet.{ANSI_nrm}')

    sys.exit(0)
