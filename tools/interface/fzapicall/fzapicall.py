#!/usr/bin/env python3
# fzapicall.py
# Copyright 2026 Randal A. Koene
# License TBD
#
# Forward an API call to the Formalizer API server
#
# This script expects the argument 'apicall'.
# The value of the argument should be a URI-encoded string that can be
# decoded and sent to `fzgraph -C`.
#
# E.g. Try http://localhost/cgi-bin/fzapicall?apicall=/fz/graph/namedlists/selected.raw
#
# If the API server data returned is detected to have an HTTP header then
# it is returned exactly as received, otherwise 'Content-type:text/plain'
# is prepended.

start_CGI_output = '''Content-type:text/plain
'''

# This helps spot errors by printing to the browser.
#print(start_CGI_output)

# Import modules for CGI handling 
try:
    import cgitb
    cgitb.enable()
except:
    pass
import sys, cgi
sys.stderr = sys.stdout
import subprocess
import shlex
from urllib.parse import unquote

form = cgi.FieldStorage() 
apicall = form.getvalue('apicall')


# Copied and modified from replicate.py.
# The command can be a list or a string.
# Returns (success:bool, proccall:bool, result_code:int, result_stdout:str, result_stderr:str).
def run_command(command)->tuple:
    """
    Runs a command.
    """
    # Make sure we have both the command tuple and command_str.
    if isinstance(command, list):
        command_str = ' '.join(command)
    else:
        command_str = command
        command = shlex.split(f"{command}")

    # Run the shell command and process the result.
    try:
        run_result = subprocess.run(command, capture_output=True, text=True, check=True)
        success = True
        result_stdout = run_result.stdout
        if run_result.stderr:
            result_stderr = run_result.stderr
            success = False
        else:
            result_stderr = ''
        result_code = run_result.returncode
        if result_code != 0:
            success = False
        return success, True, result_code, result_stdout, result_stderr

    except subprocess.CalledProcessError as e:
        return False, False, -1, str(e.stdout), str(e.stderr)
        print(f"Error executing command: {e}")
    except FileNotFoundError:
        return False, False, -2, '', f"Command '{str(command[0])}' not found."

def foward_api_call(apicall:str):
    # Decode apicall argument.
    decoded_apicall = unquote(apicall)
    #print(f'DECODED APICALL: {decoded_apicall}')
    #sys.exit(0)
    # Forward to Formalizer API server via `fzgraph -C`.
    thecmd = ['./fzgraph', '-q', '-o', 'STDOUT', '-m', '-C', decoded_apicall] # could add -m here
    success, proccall, result_code, result_stdout, result_stderr = run_command(thecmd)
    # Return textual standard output result as `text/plain`.
    if success:
        if isinstance(result_stdout, bytes):
            result_stdout = result_stdout.decode()
        if not isinstance(result_stdout, str):
            print('ERROR: Call output is not text.')
            return
        if len(result_stdout)>=5:
            if result_stdout[0:5] == 'HTTP/':
                print(result_stdout)
                return
        print(start_CGI_output)
        print(result_stdout)
        return
    if not proccall:
        print(f'ERROR: Calling fzgraph failed: {result_stderr}')
        return
    print(f'ERROR: Call returned error: {result_error}\nCode: {result_code}\nStdout: {result_stdout}')

if __name__ == '__main__':
    if apicall:
        foward_api_call(apicall)
    else:
        print('ERROR: Missing apicall argument.')
    sys.exit(0)
