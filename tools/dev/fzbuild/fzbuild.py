#!/usr/bin/python3
#
# fzbuild.py
#
# Randal A. Koene, 20200916
#
# Helpful script with shortcuts for frequent build targets.

import os
import sys
import argparse
import subprocess
import json

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

compile_dirs = {
    "lib" : "/core/lib",
    "fzedit" : "/core/fzedit",
    "fzgraph" : "/core/fzgraph",
    "fzgraphsearch" : "/core/fzgraphsearch",
    "fzguide.system" : "/core/fzguide.system",
    "fzlog" : "/core/fzlog",
    "fzquerypq" : "/core/fzquerypq",
    "fzserverpq" : "/core/fzserverpq",
    "fzupdate" : "/core/fzupdate",
    "dil2graph" : "/tools/conversion/dil2graph",
    "graph2dil" : "/tools/conversion/graph2dil",
    "boilerplate" : "/tools/dev/boilerplate",
    "fzdashboard" : "/tools/interface/fzdashboard",
    "fzgraphhtml" : "/tools/interface/fzgraphhtml",
    "fzloghtml" : "/tools/interface/fzloghtml",
    "fzlogmap" : "/tools/interface/fzlogmap",
    "fzlogtime" : "/tools/interface/fzlogtime",
    "fzserver-info" : "/tools/interface/fzserver-info",
    "nodeboard" : "/tools/interface/nodeboard"
}

flow_control = {
    'make_docs' : False,
    'compile_all' : False,
    'clean_all' : False,
    'compile_lib' : False,
    'clean_lib' : False,
    'build_boilerplate' : False,
    'build_dil2graph' : False,
    'build_graph2dil' : False,
    'build_fzedit' : False,
    'build_fzgraph' : False,
    'build_fzgraphsearch' : False,
    'build_fzguide.system' : False,
    'build_fzlog' : False,
    'build_fzquerypq' : False,
    'build_fzserverpq' : False,
    'build_fzupdate' : False,
    'build_fzloghtml' : False,
    'build_fzlogmap' : False,
    'build_fzlogtime' : False,
    'build_fzdashboard' : False,
    'build_fzgraphhtml' : False,
    'build_fzserver-info': False,
    'build_nodeboard' : False,
}

def try_subprocess_check_output(thecmdstring):
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
        if config['verbose']:
            print(res)
        return 0


"""
Update Doxygen documentation.
"""
def make_docs():
    print('Updating Doxygen documentation.\n')
    doxybuildcmd = 'cd ' + config['sourceroot'] + ' && make doxygen'
    retcode = try_subprocess_check_output(doxybuildcmd)
    if (retcode != 0):
        print('Unable to remake Doxygen documentation.')
        exit(retcode)
    
    print('Doxygen documentation rebuilt.\n')
    doxycopy = input('Copy Doxygen documentation to web root for web interface to docs? (y/N) ')
    if (doxycopy == 'y'):
        doxycopycmd = 'rm -rf ' + config["wwwdoxyroot"] + ' && cp -r ' + config["doxyroot"] + ' ' + config["wwwdoxyroot"]
        retcode = try_subprocess_check_output(doxycopycmd)
        if (retcode != 0):
            print(f'Unable to copy Doxygen documentation to {config["wwwdoxyroot"]}.')
            exit(retcode)
    
    print('Done.\n\n')
    exit(0)


"""
Cleans and compiles all components needed for `dil2graph`. That includes:
- `core` libraries
- `graph2dil` (for the optional `log2tl` component)
- `dil2graph`
"""
def build_dil2graph():
    print('Building dil2graph from scratch (dil2graph, core, log2tl).\n')
    print('  Cleaning and compiling core library. This can take 2-3 minutes.')
    retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/core/lib && make clean && make')
    if (retcode != 0):
        print('Unable to clean and make core libraries.')
        exit(retcode)   
    else:
        print('  Cleaning and compiling graph2dil (for the log2tl component). This can take a minute.')
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/tools/conversion/graph2dil && make clean && make')
        if (retcode != 0):
            print('Unable to clean and make graph2dil (for log2tl component).')
            exit(retcode)
        else:
            print('  Cleaning and compiling dil2graph. This can take a minute.')
            retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/tools/conversion/dil2graph && make clean && make')
            if (retcode != 0):
                print('Unable to clean and make dil2graph.')
                exit(retcode)
            else:
                print('Build done.')
                exit(0)


"""
Cleans and compiles all components needed for a program. That includes:
- `core` libraries
- program specific source files
"""
def build_program(programname: str, args):
    print(f'Building {programname} from scratch ({programname}, core).\n')
    if args.nolib:
        print('  Skipping library rebuilds.')
        retcode = 0
    else:
        print('  Cleaning and compiling core library. This can take 2-3 minutes.')
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/core/lib && make clean && make')
    if (retcode != 0):
        print('Unable to clean and make core libraries.')
        exit(retcode)   
    else:
        print(f'  Cleaning and compiling {programname}. This can take a minute.')
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+compile_dirs[programname]+' && make clean && make')
        if (retcode != 0):
            print(f'Unable to clean and make {programname}.')
            exit(retcode)
        else:
            print('Build done.')
            exit(0)


def compile_lib():
    print('Compiling core library.\n')
    retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/core/lib && make')
    if (retcode != 0):
        print('Unable to make core libraries.')
        exit(retcode)   
    else:
        print('Compiling done.')
        exit(0)


def clean_lib():
    print('Cleaning core library.\n')
    retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/core/lib && make clean')
    if (retcode != 0):
        print('Unable to clean core libraries.')
        exit(retcode)   
    else:
        print('Cleaning done.')
        exit(0)

def clean_all():
    print('Cleaning all compilation directories.\n')
    for compilable in compile_dirs:
        print('\t'+compilable)
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+compile_dirs[compilable]+' && make clean')
        if (retcode != 0):
            print(f'Unable to clean {compilable}.')
            exit(retcode)   

    print('Cleaning done.')
    exit(0)


def compile_all():
    print('Compiling all compilables.\n')
    for compilable in compile_dirs:
        print('\t'+compilable)
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+compile_dirs[compilable]+' && make')
        if (retcode != 0):
            print(f'Unable to compile {compilable}.')
            exit(retcode)   

    print('Compiling done.')
    exit(0)


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Dev:Build v{version} (core v{core_version})"

    print(server_long_id+"\n")

    parser = argparse.ArgumentParser(description='Useful shortcuts for frequent build targets.')
    parser.add_argument('-d', '--Doxygen', dest='makedocs', action="store_true", help='update Doxygen documentation')
    parser.add_argument('-M', '--MakeAll', dest='makeall', action="store_true", help='compile all build targets')
    parser.add_argument('-C', '--CleanAll', dest='cleanall', action="store_true", help='clean all build targets')
    parser.add_argument('-L', '--MakeLib', dest='makelib', action="store_true", help='compile library objects')
    parser.add_argument('-l', '--CleanLib', dest='cleanlib', action="store_true", help='clean library objects')
    parser.add_argument('-n', '--no-lib', dest='nolib', action="store_true", help='skip library rebuilds')
    parser.add_argument('-B', '--boilerplate', dest='boilerplate', action='store_true', help='Build boilerplate from scratch')
    parser.add_argument('-D', '--dil2graph', dest='dil2graph', action='store_true', help='Build dil2graph from scratch')
    parser.add_argument('-g', '--graph2dil', dest='graph2dil', action='store_true', help='Build graph2dil from scratch')
    parser.add_argument('-H', '--fzloghtml', dest='fzloghtml', action='store_true', help='Build fzloghtml from scratch')
    parser.add_argument('-m', '--fzlogmap', dest='fzlogmap', action='store_true', help='Build fzlogmap from scratch')
    parser.add_argument('-T', '--fzlogtime', dest='fzlogtime', action='store_true', help='Build fzlogtime from scratch')
    parser.add_argument('-D', '--fzdashboard', dest='fzdashboard', action='store_true', help='Build fzdashboard from scratch')
    parser.add_argument('-G', '--fzgraphhtml', dest='fzgraphhtml', action='store_true', help='Build fzgraphhtml from scratch')
    parser.add_argument('-E', '--fzedit', dest='fzedit', action='store_true', help='Build fzedit from scratch')
    parser.add_argument('-p', '--fzgraph', dest='fzgraph', action='store_true', help='Build fzgraph from scratch')
    parser.add_argument('--fzgraphsearch', dest='fzgraphsearch', action='store_true', help='Build fzgraph from scratch')
    parser.add_argument('-s', '--fzguide_system', dest='fzguide_system', action='store_true', help='Build fzgraphsearch from scratch')
    parser.add_argument('-o', '--fzlog', dest='fzlog', action='store_true', help='Build fzlog from scratch')
    parser.add_argument('-q', '--fzquerypq', dest='fzquerypq', action='store_true', help='Build fzquerypq from scratch')
    parser.add_argument('-S', '--fzserverpq', dest='fzserverpq', action='store_true', help='Build fzserverpq from scratch')
    parser.add_argument('-U', '--fzupdate', dest='fzupdate', action='store_true', help='Build fzupdate from scratch')
    parser.add_argument('-I', '--fzserver_info', dest='fzserver_info', action='store_true', help='Build fzserver-info from scratch')
    parser.add_argument('-N', '--nodeboard', dest='nodeboard', action='store_true', help='Build nodeboard from scratch')

    args = parser.parse_args()

    if args.fzedit:
        flow_control['build_fzedit'] = True
    if args.fzgraph:
        flow_control['build_fzgraph'] = True
    if args.fzgraphsearcg:
        flow_control['build_fzgraphsearch'] = True
    if args.fzguide_system:
        flow_control['build_fzguide.system'] = True
    if args.fzlog:
        flow_control['build_fzlog'] = True
    if args.fzquerypq:
        flow_control['build_fzquerypq'] = True
    if args.fzserverpq:
        flow_control['build_fzserverpq'] = True
    if args.fzupdate:
        flow_control['build_fzupdate'] = True
    if args.fzserver_info:
        flow_control['build_fzserver-info'] = True
    if args.nodeboard:
        flow_control['build_nodeboard'] = True
    if args.makedocs:
        flow_control['make_docs'] = True
    if args.makeall:
        flow_control['compile_all'] = True
    if args.cleanall:
        flow_control['clean_all'] = True
    if args.makelib:
        flow_control['compile_lib'] = True
    if args.cleanlib:
        flow_control['clean_lib'] = True
    if args.boilerplate:
        flow_control['build_boilerplate'] = True
    if args.dil2graph:
        flow_control['build_dil2graph'] = True
    if args.graph2dil:
        flow_control['build_graph2dil'] = True
    if args.fzloghtml:
        flow_control['build_fzloghtml'] = True
    if args.fzlogmap:
        flow_control['build_fzlogmap'] = True
    if args.fzlogtime:
        flow_control['build_fzlogtime'] = True
    if args.fzdashboard:
        flow_control['build_fzdashboard'] = True
    if args.fzgraphhtml:
        flow_control['build_fzgraphhtml'] = True

    if flow_control['make_docs']:
        make_docs()
    if flow_control['compile_lib']:
        compile_lib()
    if flow_control['clean_lib']:
        clean_lib()
    if flow_control['clean_all']:
        clean_all()
    if flow_control['compile_all']:
        compile_all()
    if flow_control['build_boilerplate']:
        build_program('boilerplate', args)
    if flow_control['build_dil2graph']:
        build_program('dil2graph', args)
    if flow_control['build_graph2dil']:
        build_program('graph2dil', args)
    if flow_control['build_fzloghtml']:
        build_program('fzloghtml', args)
    if flow_control['build_fzlogmap']:
        build_program('fzlogmap', args)
    if flow_control['build_fzgraphhtml']:
        build_program('fzgraphhtml', args)
    if flow_control['build_fzdashboard']:
        build_program('fzdashboard', args)
    if flow_control['build_fzlogtime']:
        build_program('fzlogtime', args)
    if flow_control['build_fzedit']:
        build_program('fzedit', args)
    if flow_control['build_fzgraph']:
        build_program('fzgraph', args)
    if flow_control['build_fzguide.system']:
        build_program('fzguide.system', args)
    if flow_control['build_fzlog']:
        build_program('fzlog', args)
    if flow_control['build_fzquerypq']:
        build_program('fzquerypq', args)
    if flow_control['build_fzserverpq']:
        build_program('fzserverpq', args)
    if flow_control['build_fzupdate']:
        build_program('fzupdate', args)
    if flow_control['build_fzserver-info']:
        build_program('fzserver-info', args)
    if flow_control['build_nodeboard']:
        build_program('nodeboard', args)

    print('Some options may not have been implemented yet.\n')

    exit(0)
