#!/usr/bin/python3
#
# fzpackage.py
#
# Randal A. Koene, 20220519
#
# Package builder for executable Formalizer environment.

# *** WARNING: This is wildly out of date.
#              Also, we should really aim to genereate a .deb.

import os
import sys
import argparse
import subprocess
import json
import shlex
from datetime import datetime

version = "0.1.0-0.1"

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'

try:
    with open(fzsetupconfig) as f:
        config = json.load(f)

except FileNotFoundError:
    print('Unable to load fundamental standard configuration data.\nPlease run `fzsetup -l` first to self-initialize and check the results.\n')
    exit(1)

# Enable us to import standardized Formalizer Python components.
fzcorelibdir = config['sourceroot'] + '/core/lib'
fzcoreincludedir = config['sourceroot'] + '/core/include'
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)

# core components
import coreversion

timestamp=datetime.now().strftime("%Y%m%d%H%M")

packagedir = './.package/formalizer'
homedir = os.path.expanduser('~')
formalizerbasedir = homedir+'/src/formalizer'
package = './.package/formalizer-bin-'+timestamp+'.tar.gz'

"""
List of component programs:
--------------------------

fzedit
fzgraph
fzgraphsearch
fzguide.system
fzlog
fzquerypq
fzserverpq
fzupdate
fzserver-info
nodeboard
boilerplate
dil2graph
graph2dil
fzloghtml
fzlogmap
fzlogtime
fzdashboard
fzgraphhtml

addnode
categories_a2c-NNLs-init.sh
categories_hourly-NNLs-init.sh
dil2al-polldaemon.sh
earlywiz
frequent-init.sh
fzbackup
fzbackup-mirror-to-github.sh
fzbuild
fzcatchup
fzdaily.sh
fzgraphhtml-term.sh
fzlist_backups.sh
fzloghtml-term.sh
fzlog-mostrecent.sh
fzlogtime-term.sh
fzrestore.sh
fzsetup
fztask
fztask-server
fztask-serverd.sh
get_log_entry.sh
graph2dil-diff.sh
graph2dil-integrity
graph-resident
graph-topics.sh
index-term.sh
logentry
panes-term.sh
requestmanual
sysmet-extract-show.sh
sysmet-extract-term.sh
v1xv2x-refresh.sh

The Makefile describes which executables need to be available and which CGI scripts are needed.
"""

core = [
    'core/fzedit/fzedit',
    'core/fzbackup/fzbackup-mirror-to-github.sh',
    'core/fzbackup/fzbackup.py',
    'core/fzgraph/fzgraph',
    'core/fzgraphsearch/fzgraphsearch',
    'core/fzguide.system/fzguide.system',
    'core/fzinfo/fzinfo.py',
    'core/fzbackup/fzlist_backups.sh',
    'core/fzlog/fzlog',
    'core/fzquerypq/fzquerypq',
    'core/fzbackup/fzrestore.sh',
    'core/fzserverpq/fzserverpq',
    'core/fzserverpq/fzserverpqd.sh',
    'core/fzsetup/fzsetup.py',
    'core/fztask/fztask.py',
    'core/fztask-server/fztask-serverd.sh',
    'core/fztask-server/fztask-server.py',
    'core/fzupdate/fzupdate',
]
tools = [
    'tools/interface/addnode/addnode.py',
    'tools/dev/boilerplate/boilerplate',
    'tools/compat/categories_a2c-NNLs-init.sh',
    'tools/compat/categories_hourly-NNLs-init.sh',
    'tools/compat/dil2al-polldaemon.sh',
    'tools/conversion/dil2graph/dil2graph',
    'tools/system/earlywiz/earlywiz.py',
    'tools/compat/frequent-init.sh',
    'tools/dev/fzbuild/fzbuild.py',
    'tools/interface/fzcatchup/fzcatchup.py',
    'tools/glue/fzdaily.sh',
    'tools/interface/fzdashboard/fzdashboard',
    'tools/interface/fzgraphhtml/fzgraphhtml',
    'tools/interface/fzgraphhtml/fzgraphhtml-term.sh',
    'tools/interface/fzloghtml/fzloghtml',
    'tools/interface/fzloghtml/fzloghtml-term.sh',
    'tools/interface/fzlogmap/interface/fzloghtml/fzloghtml-cgi.py',
    'tools/interface/fzloghtml/fzlog-mostrecent.sh',
    'tools/interface/fzlogtime/fzlogtime',
    'tools/interface/fzlogtime/fzlogtime-term.sh',
    'tools/interface/fzserver-info/fzserver-info',
    'tools/interface/fzloghtml/get_log_entry.sh',
    'tools/conversion/graph2dil/graph2dil',
    'tools/conversion/graph2dil/graph2dil-diff.sh',
    'tools/conversion/graph2dil/graph2dil-integrity.py',
    'tools/glue/graph-resident.py',
    'tools/glue/graph-topics.sh',
    'tools/system/top/index-term.sh',
    'tools/interface/logentry/logentry.py',
    'tools/interface/nodeboard/nodeboard',
    'tools/interface/panes/panes-term.sh',
    'tools/system/requestmanual/requestmanual.py',
    'tools/system/metrics/sysmet-extract/sysmet-extract-show.sh',
    'tools/system/metrics/sysmet-extract/sysmet-extract-term.sh',
    'tools/compat/v1xv2x-refresh.sh',
    'tools/dev/installer.py',
]
cgi = [
    'core/include/ansicolorcodes.py',
    'tools/system/metrics/sysmet-extract/categories_a2c.json',
    'core/include/error.py',
    'core/fzbackup/fzbackup-cgi.py',
    'core/include/fzcmdcalls.py',
    'core/fzedit/fzedit-cgi.py',
    'core/fzgraph/fzgraph-cgi.py',
    'tools/interface/fzgraphhtml/fzgraphhtml-cgi.py',
    'core/fzgraphsearch/fzgraphsearch-cgi.py',
    'core/fzguide.system/fzguide.system-cgi.py',
    'tools/interface/fzlink/fzlink.py',
    'core/fzlog/fzlog-cgi.py',
    'tools/interface/fzloghtml/fzloghtml-cgi.py',
    'tools/interface/fzlogtime/fzlogtime.cgi',
    'core/include/fzmodbase.py',
    'core/fzquerypq/fzquerypq-cgi.py',
    'tools/interface/fzserver-info/fzserver-info-cgi.py',
    'core/fztask/fztask-cgi.py',
    'tools/interface/fzuistate/fzuistate.py',
    'core/fzupdate/fzupdate-cgi.py',
    'core/include/Graphaccess.py',
    'tools/interface/logentry-form/logentry-form.py',
    'core/include/proclock.py',
    'tools/system/metrics/sysmet-extract/sysmet-extract-cgi.py',
    'core/include/tcpclient.py',
    'core/include/TimeStamp.py',
]
cgi_copy = [
    'earlywiz.py',
    'fzdashboard',
    'fzedit',
    'fzgraph',
    'fzgraphhtml',
    'fzgraphsearch',
    'fzguide.system',
    'fzlog',
    'fzloghtml',
    'fzlogmap',
    'fzlogtime',
    'fzquerypq',
    'fzserver-info',
    'fztask.py',
    'fzupdate',
    'get_log_entry.sh',
]

def run_interactive(cmd_str: str) ->int:
    try:
        exitstatus = subprocess.check_call(shlex.split("bash -c '"+cmd_str+"'"))
        return exitstatus
    except Exception as e:
        print('Command `%s` caused exception: %s' % (cmd_str, str(e)))
        return -1

def components_compilation(clear_and_recompile: bool):
    if clear_and_recompile:
        print('Clearing to recompile from scratch...')
        if run_interactive('fzbuild -C') != 0:
            print('fzbuild -C failed.')
            exit(1)
    print('Ensuring that all components are compiled...')
    if run_interactive('fzbuild -M') != 0:
        print('fzbuild -M failed.')
        exit(1)

def collect_components():
    # Make packaged directory.
    if run_interactive('mkdir -p '+packagedir) != 0:
        print('Unable to make package directory.')
        exit(1)
    if run_interactive('mkdir -p '+packagedir+'/cgi-bin') != 0:
        print('Unable to make package directory.')
        exit(1)
    # Copy components.
    for core_component in core:
        core_program = formalizerbasedir+'/'+core_component
        if run_interactive('cp -f '+core_program+' '+packagedir+'/') != 0:
            print('Unable to copy '+core_program)
            exit(1)
    for tool_component in tools:
        tool_program = formalizerbasedir+'/'+tool_component
        if run_interactive('cp -f '+tool_program+' '+packagedir+'/') != 0:
            print('Unable to copy '+tool_program)
            exit(1)
    for cgi_component in cgi:
        cgi_program = formalizerbasedir+'/'+cgi_component
        if run_interactive('cp -f '+cgi_program+' '+packagedir+'/') != 0:
            print('Unable to copy '+cgi_program)
            exit(1)
    # Create package.
    if run_interactive('tar zcvf '+package+' '+packagedir) != 0:
        print('Creating package failed.')
        exit(1)
    print('Package created at '+package)

if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Dev:Package v{version} (core v{core_version})"

    print(server_long_id+"\n")

    components_compilation(input('Clear and recompile from scratch? (y/N) ') == 'y')

    collect_components()

    exit(0)
