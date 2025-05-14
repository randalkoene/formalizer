#!/usr/bin/python3
#
# Randal A. Koene, 20250513
#
# This script parses HTML that contains Log chunks and adds selection checkboxes with
# Log chunk IDs to easily select a subset of Log chunks for further processing, e.g. to
# extract related data and carry out processing.
#
# This script is a receiver script that can be used when calling fzloghtml
# with the -S (select and process) option.
#
# This script was made with the help of DeepSeek.

import cgi
import cgitb
import sys
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

# Enable error reporting for debugging (remove in production)
cgitb.enable()

print("Content-Type: text/plain\n")

config = {
    'verbose': False,
    'logcmdcalls': False,
    'cmdlog': '/var/www/webdata/formalizer/selectchunks.log'
}

results = {}

def try_subprocess_check_output(thecmdstring, resstore):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
        print('<pre>')
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        p = Popen(thecmdstring,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        if resstore:
            results[resstore] = result
        child_stdout.close()
        if result and config['verbose']:
            print(result)
            print('</pre>')
        return 0

    except Exception as ex:
        print('<pre>')
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        print('</pre>')
        return 1

def main():
    try:
        # Parse the form data
        form = cgi.FieldStorage()
        
        # Initialize list to store select values
        select_values = []
        
        # Check if the form was submitted with POST
        if sys.stdin.isatty():
            print("Error: This script should be called with POST method.")
            return
        
        # Get all 'select' parameters (handles multiple values for the same name)
        if 'select' in form:
            # FieldStorage returns all values for a parameter as a list
            select_params = form.getlist('select')
            select_values.extend(select_params)
        
        # Print the collected values for demonstration
        #print("Collected 'select' values:")
        #for value in select_values:
        #    print(f"- {value}")
        
        csv_keys = ','.join(select_values)
        cmdstr = './fzlogdata -q -C '+csv_keys
        res = try_subprocess_check_output(cmdstr, 'data')
        if 'data' in results:
        	data_list = results['data'].split('\n')
        	days = {}
        	for chunk_data_str in data_list:
        		if len(chunk_data_str)>0:
        			chunk_data = chunk_data_str.split(':')
        			timestr = chunk_data[0]
        			minsstr = chunk_data[1]
        			daystr = timestr[0:8]
        			if daystr not in days:
        				days[daystr] = []
        			days[daystr].append(int(minsstr))
        	print('Selected Log chunks:\n')
        	print(str(select_values))
        	print('\nMinutes in Log chunks each day and their sum:\n')
        	for daystr in days:
        		print('%s: sum(%s) = %d mins' % (daystr, str(days[daystr]), sum(days[daystr])))
        	print('\nDays and sums of minutes:\n')
        	total_minutes = 0
        	for daystr in days:
        		sum_minutes = sum(days[daystr])
        		total_minutes += sum_minutes
        		print('%s, %d' % (daystr, sum_minutes))
        	print('\nTotal minutes:\n')
        	print('%d' % total_minutes)
        	print('\nTotal hours:\n')
        	print('%.2f' % (float(total_minutes)/60.0))
        else:
        	print('Missing data.')
        
    except Exception as e:
        print(f"An error occurred: {str(e)}")

if __name__ == "__main__":
    main()
