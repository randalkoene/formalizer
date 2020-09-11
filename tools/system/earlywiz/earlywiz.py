#!/usr/bin/env python3
# earlywiz.py
# Copyright 2020 Randal A. Koene
# License TBD

import os
import sys
sys.path.append(os.getenv('HOME')+'/src/formalizer/core/lib')
sys.path.append(os.getenv('HOME')+'/src/formalizer/core/include')
import coreversion

import subprocess

version = "0.1.0-0.1"

config = {
    'verbose' : False
}

def initialize():
    print("** CONFIG NOTE: This tool (and its components from core) still need a")
    print("*              standardized method of configuration. See Trello card")
    print("*              at https://trello.com/c/4B7x2kif.\n")

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:System:Update:AMwizard v{version} (core v{core_version})"

    print(server_long_id)

def call_guide(subsection,snippet_idx):
    try:
        res = subprocess.check_output(f'fzguide.system -q -R -A -U {subsection} -x {snippet_idx} -F txt', shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        exit(cpe.returncode)

    else:
        if config['verbose']:
            print(res)
        
    return res


def wakeup_shower_suggestion():
    snippet = call_guide('wakeup','1.0')
    print(snippet)
    stepdone = input("Please press ENTER...")


if __name__ == '__main__':

    initialize()

    wakeup_shower_suggestion()

