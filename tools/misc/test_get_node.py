#!/usr/bin/python3
#
# test_get_node.py
#
# Randal A. Koene, 20250823
#
# Tests accessing Node data from Python.

import os
import sys
import json
#import argparse

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
#from ansicolorcodes import *
from fzcmdcalls import *
#from Logaccess import *
from Graphaccess import get_node_data

version = "0.1.0-0.1"

if __name__ == '__main__':

    core_version = coreversion.coreversion()
    _long_id = "Test:GetNode" + f" v{version} (core v{core_version})"

    print(_long_id+"\n")

    #args = parse_options()

    node_id = input('Node ID: ')

    data = get_node_data(node_id)

    dependencies = data['dependencies'].split(' ')

    print('Top Node: %s' % data['Node ID'])

    print('Dependencies:')
    for dep in dependencies:
    	if (len(dep)==16):
    		dep_data = get_node_data(dep)
    		print('  %s' % dep_data['Node ID'])

    sys.exit(0)
