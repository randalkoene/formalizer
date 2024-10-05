#!/usr/bin/env python3
# convert_to_database.py
# Copyright 2024 Randal A. Koene
# License TBD

# Note that this is merely used for conversion from the JSON data file
# the data in the Formalizer database.
# It is meant to be run from the command line.

import sys
import traceback
from subprocess import Popen, PIPE
from traceback import print_exc
from io import StringIO
import json

JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'

def try_command_call(thecmd:str):
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        print(result)
        if error:
        	print(error)

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)

def file_key_to_database_index(filekey:str)->str:
	return filekey[0:4]+filekey[5:7]+filekey[8:10]+'0000'

num_with_no_data = 0

def convert_data_to_database_data(data:dict, tablekey:str, key:str, dbindex:str):
	global num_with_no_data
	if key in data[tablekey]:
		dictdata = data[tablekey][key]
		#print(key+' --> '+str(dictdata))
		jsondata = json.dumps(dictdata)
		#print(dbindex+' --> '+jsondata)
	else:
		jsondata = ''
		#print(dbindex+' no data')
		num_with_no_data += 1
	with open('/dev/shm/'+tablekey+'.json', 'w') as f:
		f.write(jsondata)

def send_to_database(dbindex:str):
	thecmd = "fzmetricspq -q -S -i "+dbindex+" -w file:/dev/shm/wiztable.json -n file:/dev/shm/nutrition.json -e file:/dev/shm/exercise.json -a file:/dev/shm/accounts.json -m file:/dev/shm/milestones.json -c file:/dev/shm/comms.json"
	try_command_call(thecmd)

def convert_from_file():
	global num_with_no_data
	# Load from file
	with open(JSON_DATA_PATH, 'r') as f:
		data = json.load(f)
	print('Loaded from file.')
	print('Keys: '+str(list(data.keys())))
	for key in list(data.keys()):
		print('Number of entries in '+key+': '+str(len(data[key])))
	# The wiztable entries have the full list of date index keys
	all_keys = list(data['wiztable'].keys())
	for key in all_keys:
		dbindex = file_key_to_database_index(key)
		convert_data_to_database_data(data, 'wiztable', key, dbindex)
		convert_data_to_database_data(data, 'nutrition', key, dbindex)
		convert_data_to_database_data(data, 'exercise', key, dbindex)
		convert_data_to_database_data(data, 'accounts', key, dbindex)
		convert_data_to_database_data(data, 'milestones', key, dbindex)
		convert_data_to_database_data(data, 'comms', key, dbindex)
		send_to_database(dbindex)
	print('Number of entries with no data: '+str(num_with_no_data))

if __name__ == '__main__':
	convert_from_file()
	sys.exit(0)
