#!/usr/bin/python3
#
# fzbuild.py
#
# Randal A. Koene, 20200916
#
# Helpful script with shortcuts for frequent build targets.

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
    'build_di2graph' : False
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
    print('Building dil2graph from scratch (dil2graph, core, log2tl).')
    retcode = try_subprocess_check_output('cd '+config.sourceroot+'/core/lib && make clean && make')
    if (retcode != 0):
        print('Unable to clean and make core libraries.')
        exit(retcode)   
    else:
        retcode = try_subprocess_check_output('cd '+config.sourceroot+'/tools/conversion/graph2dil && make clean && make')
        if (retcode != 0):
            print('Unable to clean and make graph2dil (for log2tl component).')
            exit(retcode)
        else:
            retcode = try_subprocess_check_output('cd '+config.sourceroot+'/tools/conversion/dil2graph && make clean && make')
            if (retcode != 0):
                print('Unable to clean and make dil2graph.')
                exit(retcode)
            else:
                print('Build done.')
                exit(0)
    

if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Dev:Build v{version} (core v{core_version})"

    print(server_long_id+"\n")

    parser = argparse.ArgumentParser(description='Useful shortcuts for frequent build targets.')
    parser.add_argument('-M', '--MakeAll', dest='makeall', action="store_true", help='compile all build targets')
    parser.add_argument('-C', '--CleanAll', dest='cleanall', action="store_true", help='clean all build targets')
    parser.add_argument('-L', '--MakeLib', dest='makelib', help='compile library objects')
    parser.add_argument('-l', '--CleanLib', dest='cleanlib', help='clean library objects')
    parser.add_argument('-D', '--dil2graph', dest='dil2graph', action='store_true', help='Build dil2graph from scratch')

    args = parser.parse_args()

    if args.verbose:
        config['verbose'] = True
    if args.makeall:
        config['compile_all'] = True
    if args.cleanall:
        config['clean_all'] = True
    if args.makelib:
        config['compile_lib'] = True
    if args.cleanlib:
        config['clean_lib'] = True
    if args.dil2graph:
        config['build_dil2graph'] = True

    if flow_control['build_dil2graph']:
        build_dil2graph()

    print('Some options have not been implemented yet.\n')

    exit(0)
