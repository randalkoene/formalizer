#!/usr/bin/env python3
#
# See schemas in the database and erase old backup schemas from
# within the database.

import subprocess
import shlex
import os
import json
import argparse
from datetime import datetime

this_user = os.environ['USER']
user_home = os.environ['HOME']

run_result = None
error_groups = None

#default_database = 'formalizer'
#default_schema = this_user

def check_permitted_errors(stderr_str: str, chkstr:str):
    global error_groups
    other_messages = []
    err_messages = stderr_str.split('\n')
    permitted = 0
    for message in err_messages:
        if message.find(chkstr) >= 0:
            permitted += 1
        else:
            message = message.strip()
            other_messages.append(message)
    error_groups = {
        'permitted': permitted,
        'other_num': len(other_messages),
        'other': '\n'.join(other_messages),
    }

def run_command(
    args,
    command,
    quiet_stdout_stderr=False,
    chkstr=' already ',
    return_stdout=False):
    """
    Runs a command.
    """
    global run_result
    if isinstance(command, list):
        command_str = ' '.join(command)
    else:
        command_str = command
        command = shlex.split(f"{command}")
    try:
        if not quiet_stdout_stderr:
            print(f"Running: {command_str}")
        run_result = subprocess.run(command, capture_output=True, text=True, check=True)
        if not quiet_stdout_stderr:
            print("WITHIN-CALL STDOUT:")
            print(run_result.stdout)
        if run_result.stderr:
            check_permitted_errors(run_result.stderr, chkstr)
            if not quiet_stdout_stderr:
                print("WITHIN-CALL STDERR:")
                print(error_groups['other'])
            if error_groups['permitted'] > 0:
                print('Permitted errors: %d' % error_groups['permitted'])
            return False
        if return_stdout:
            return run_result.stdout
        return run_result.returncode == 0

    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        if e.stdout:
            print("CALL STDOUT:")
            print(e.stdout)
        if e.stderr:
            print("CALL STDERR:")
            print(e.stderr)
    except FileNotFoundError:
        print(f"Error: Command '{command_str.split()[0]}' not found. Make sure it's in your PATH.")
    return False

def delete_schema(args):
    if not run_command(args, f"psql -d {args.database} -c 'DROP SCHEMA {args.delete} CASCADE;'"):
        print('Error in delete_schema.')
        exit(1)

def show_schemas(args):
    if not run_command(args, f'psql -d {args.database} -c "\dn"'):
        print('Error in show_schemas.')
        exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Show or delete database schemas.')
    parser.add_argument('-d', dest='database', type=str, default='formalizer', help='Specified database (default: formalizer).')
    parser.add_argument('-x', dest='delete', type=str, help='Expunge the specified schema from the database.')
    args = parser.parse_args()

    if args.delete:
        delete_schema(args)
    else:
        show_schemas(args)

    exit(0)
