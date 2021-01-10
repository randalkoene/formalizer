#! /usr/bin/python3
#
# graph-resident.py
#
# Randal A. Koene, 20210110
#
# Helper script to test if the Graph is memory-resident.

# std
import os
import sys
#import json
import argparse
import subprocess

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzconfigdir = fzuserbase + '/config'

# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring):
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if args.verbose:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
        if args.verbose:
            print(res)
        return 0

def parse_options():
    theepilog = ('\n')

    parser = argparse.ArgumentParser(description='Check if the Graph is memory-resident.',epilog=theepilog)
    #parser.add_argument('-i', '--inspector', dest='inspector', help='specify inspector program (default=less)')
    parser.add_argument('-T', '--text', dest='TFoutput', action="store_true", help='True/False text output to STDOUT')
    parser.add_argument('-V', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')

    return parser.parse_args()

if __name__ == '__main__':
    global args
    args = parse_options()

    thecmd="fzgraph -q -C /fz/status"
    retcode = try_subprocess_check_output(thecmd)
    if (retcode != 0):
        if args.TFoutput:
            print('False')
        sys.exit(retcode)
    
    if args.TFoutput:
        print('True')
    sys.exit(0)
