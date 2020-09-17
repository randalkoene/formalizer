#!/usr/bin/python3
#
# fzinfo.py
#
# Randal A. Koene, 20200917
#
# Information about the installed Formalizer environment. See README.md for more.

import os
import sys
sys.path.append(os.getenv('HOME')+'/src/formalizer/core/lib')
sys.path.append(os.getenv('HOME')+'/src/formalizer/core/include')

import argparse
import subprocess

import coreversion

version = "0.1.0-0.1"

config = {
    'verbose' : False,
    'sourceroot': '~/src/formalizer' 
}

flow_control = {
    'compile_all' : False,
    'clean_all' : False,
    'compile_lib' : False,
    'clean_lib' : False,
    'build_dil2graph' : False,
    'build_graph2dil' : False,
    'build_fzloghtml' : False
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
Cleans and compiles all components needed for `graph2dil`. That includes:
- `core` libraries
- `graph2dil`
"""
def build_graph2dil():
    print('Building graph2dil from scratch (graph2dil, core).\n')
    print('  Cleaning and compiling core library. This can take 2-3 minutes.')
    retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/core/lib && make clean && make')
    if (retcode != 0):
        print('Unable to clean and make core libraries.')
        exit(retcode)   
    else:
        print('  Cleaning and compiling graph2dil. This can take a minute.')
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/tools/conversion/graph2dil && make clean && make')
        if (retcode != 0):
            print('Unable to clean and make graph2dil.')
            exit(retcode)
        else:
            print('Build done.')
            exit(0)


"""
Cleans and compiles all components needed for `fzloghtml`. That includes:
- `core` libraries
- `fzloghtml`
"""
def build_fzloghtml():
    print('Building fzlogheml from scratch (fzloghtml, core).\n')
    print('  Cleaning and compiling core library. This can take 2-3 minutes.')
    retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/core/lib && make clean && make')
    if (retcode != 0):
        print('Unable to clean and make core libraries.')
        exit(retcode)   
    else:
        print('  Cleaning and compiling fzloghtml. This can take a minute.')
        retcode = try_subprocess_check_output('cd '+config['sourceroot']+'/tools/interface/fzloghtml && make clean && make')
        if (retcode != 0):
            print('Unable to clean and make fzloghtml.')
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


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Dev:Build v{version} (core v{core_version})"

    print(server_long_id+"\n")

    parser = argparse.ArgumentParser(description='Useful shortcuts for frequent build targets.')
    parser.add_argument('-M', '--MakeAll', dest='makeall', action="store_true", help='compile all build targets')
    parser.add_argument('-C', '--CleanAll', dest='cleanall', action="store_true", help='clean all build targets')
    parser.add_argument('-L', '--MakeLib', dest='makelib', action="store_true", help='compile library objects')
    parser.add_argument('-l', '--CleanLib', dest='cleanlib', action="store_true", help='clean library objects')
    parser.add_argument('-D', '--dil2graph', dest='dil2graph', action='store_true', help='Build dil2graph from scratch')
    parser.add_argument('-G', '--graph2dil', dest='graph2dil', action='store_true', help='Build graph2dil from scratch')
    parser.add_argument('-H', '--fzloghtml', dest='fzloghtml', action='store_true', help='Build fzloghtml from scratch')


    args = parser.parse_args()

    if args.makeall:
        flow_control['compile_all'] = True
    if args.cleanall:
        flow_control['clean_all'] = True
    if args.makelib:
        flow_control['compile_lib'] = True
    if args.cleanlib:
        flow_control['clean_lib'] = True
    if args.dil2graph:
        flow_control['build_dil2graph'] = True
    if args.graph2dil:
        flow_control['build_graph2dil'] = True
    if args.fzloghtml:
        flow_control['build_fzloghtml'] = True

    if flow_control['build_dil2graph']:
        build_dil2graph()
    if flow_control['compile_lib']:
        compile_lib()
    if flow_control['clean_lib']:
        clean_lib()
    if flow_control['build_graph2dil']:
        build_graph2dil()
    if flow_control['build_fzloghtml']:
        build_fzloghtml()

    print('Some options have not been implemented yet.\n')

    exit(0)
