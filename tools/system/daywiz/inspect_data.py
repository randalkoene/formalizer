#!/usr/bin/env python3
# score.py
# Copyright 2026 Randal A. Koene
# License TBD
#
# Inspect data format and more for wiztable and other related data stored
# in the Formalizer database.

# Import modules for CGI handling 
# try:
#     import cgitb; cgitb.enable()
# except:
#     pass
# import cgi
import sys, os
sys.stderr = sys.stdout
from io import StringIO
from subprocess import Popen, PIPE
import traceback

from datetime import datetime
from time import time
import json
from os.path import exists

def database_call(thecmd:str):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        # with open(debugdatabase, 'a') as f:
        #     f.write('Call: '+thecmd+'\n\n')
        #     f.write(result+'\n\n')
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        if error:
            print(f'Error: {error}')
            sys.exit(0)
            # with open(error_file, 'w') as f:
            #     f.write(error)
            # return None
        return result

    except Exception as ex:
        with open(error_file, 'w') as e:              
            e.write(str(ex))
            f = StringIO()
            traceback.print_exc(file=f)
            a = f.getvalue().splitlines()
            for line in a:
                e.write(line)
        return None

def get_data_from_database()->dict:
    thecmd = "fzmetricspq -q -d formalizer -s randalk -E STDOUT -R -i all -F json -o STDOUT -w true -n true -e true -a true -m true -c true"
    datastr = database_call(thecmd)
    try:
        data = json.loads(datastr)
        if not isinstance(data, dict) or len(data)==0:
            return {}
        else:
            return data
    except:
        return {}

def print_structure(data, indent=''):
    if isinstance(data, dict):
        print(f'{indent}{{')
        # Only iterate over a limited number of items if desired
        for key, value in data.items():
            print(f"{indent}  '{key}': ", end="")
            print_structure(value, indent + '  ')
            # Optional: break after the first few keys if the dict is very large
            # if counter > some_limit:
            #     print(f"{indent}  '...'")
            #     break
        print(f'{indent}}}')
    elif isinstance(data, list):
        print(f'{indent}[')
        if len(data) > 0:
            print_structure(data[0], indent + '  ')
            if len(data) > 1:
                print(f"{indent}  ...")
        print(f'{indent}]')
    else:
        # For non-container types, print the type name
        print(type(data).__name__)

def show_data_format():
    data = get_data_from_database()
    print(f'Top level data categories:\n  {list(data.keys())}')
    days = list(data["wiztable"].keys())
    print(f'Day keys in "wiztable" dict:\n  {days[0]} to {days[-1]}')
    print(f'The "wiztable" of each day contains a list of data [ID, timestamp, value].')
    print(f'Values are strings and can be "checked" or not for checkboxes, or a string representation of a numerical value.')
    print(f'The "wiztable" IDs of data["wiztable"][list(data["wiztable"].keys())[-1]] are:')
    IDs = []
    for entry in data['wiztable'][days[-1]]:
        IDs.append(entry[0])
    print(f'  {IDs}')
    #print_structure(data['wiztable'][days[-1]])

if __name__ == '__main__':
    show_data_format()
