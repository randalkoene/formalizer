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
    'show_executables' : False
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


def print_in_columns(thelist):
    if len(thelist) % 2 != 0:
        thelist.append(" ")

    splitidx = int(len(thelist)/2)
    thelist1 = thelist[0:splitidx]
    thelist2 = thelist[splitidx:]

    for item1, item2 in zip(thelist1,thelist2):
        print('    {0:25} {1}'.format(item1, item2))


def show_executables():
    import executables

    print('The following Formalizer executable components are available:\n')

    #print(*executables.executables, sep='\n')
    print_in_columns(list(executables.executables))

    print('\nFor information about individual components do one of these:\n'
    '  - Call the executable with the "-h" option.\n'
    '  - Read the corresponding section in the Formalizer documentation.\n\n'
    'Note that this list does not include the following types of executables:\n'
    '  - Executables that are exclusively callable as CGI handlers.\n'
    '  - Executables provided exclusively for (temporary) active cross-\n'
    '    compatible operation (from the formalizer/tools/compat sources).\n'
    '\n')
    exit(0)

def parse_options():
    parser = argparse.ArgumentParser(description='Information about the installed Formalizer environment.')
    parser.add_argument('-E', '--Executables', dest='executables', action="store_true", help='show executables provided')

    args = parser.parse_args()

    if args.executables:
        flow_control['show_executables'] = True


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Dev:Build v{version} (core v{core_version})"

    print(server_long_id+"\n")

    parse_options()

    if flow_control['show_executables']:
        show_executables()


    print('Some options have not been implemented yet.\n')

    exit(0)
