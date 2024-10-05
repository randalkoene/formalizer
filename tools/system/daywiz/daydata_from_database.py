#!/usr/bin/env python3
# daydata_from_database.py
# Copyright 2024 Randal A. Koene
# License TBD

# Note that this is merely used check the retrieval function to
# get one day's data from the database.

import sys
import traceback
from subprocess import Popen, PIPE
from traceback import print_exc
from io import StringIO
import json

def try_command_call(thecmd:str):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        if error:
        	print(error)
        	return None
        return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return None

def pretty(d, indent=0):
	for key, value in d.items():
		print('\t' * indent + str(key))
		if isinstance(value, dict):
			pretty(value, indent+1)
		else:
			print('\t' * (indent+1) + str(value))

def get_day_data_from_database(daydate:str):
	if (len(daydate) == 8):
		daydate += '0000'
	if (len(daydate) != 12):
		print('Unrecognized date.')
		sys.exit(1)

	thecmd = "fzmetricspq -q -R -i "+daydate+" -F json -o STDOUT -w true -n true -e true -a true -m true -c true"
	datastr = try_command_call(thecmd)
	try:
		data = json.loads(datastr)
		if not isinstance(data, dict) or len(data)==0:
			print(str(data))
		else:
			pretty(data)
	except:
		print(str(datastr))

if __name__ == '__main__':
	from sys import argv
	if len(argv) > 1:
		daydate = argv[1]
		get_day_data_from_database(daydate)
	sys.exit(0)
