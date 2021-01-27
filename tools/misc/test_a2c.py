#!/usr/bin/python3
#
# test_a2c.py
#
# Randal A. Koene, 20210127
#
# Experimental script to test using Formalizer tools to obtain data needed for
# A2C metrics as typically used in daily System Metrics.

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

from fzcmdcalls import *

customtemplate = r'STR:{{ t_chunkopen }} {{ t_diff_mins }} {{ node_id }}\\n'
thecmd = f"fzloghtml -r -D 5 -N -T 'STR:{customtemplate}'"
retcode = try_subprocess_check_output(thecmd, 'logres', config)

