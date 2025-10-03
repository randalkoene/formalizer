#!/usr/bin/env python3
# replicate.py
# Copyright 2025 Randal A. Koene
# License TBD
#
# Install and/or restore complete backed-up system environment.

import subprocess
import shlex
import os
import json
import argparse
from datetime import datetime

this_user = os.environ['USER']
user_home = os.environ['HOME']
state_file = user_home+'/.replicate_state.json'

T_start = None
Step = None

userdata_mounted = False

state = {
    'T': [],    # intervals during which replication activity has been taking place
    'step': '', # the last step that completed successfully, used for optional resume
    'exit': '', # previous exit condition
}

def run_command(args, command)->bool:
    """
    Runs a command.
    """
    try:
        print(f"Running: {command}")
        if args.simulate:
            if not args.failafter:
                return True
            return state['step'] != args.failafter
        result = subprocess.run(shlex.split(f"{command}"), capture_output=True, text=True, check=True)
        print("STDOUT:")
        print(result.stdout)
        if result.stderr:
            print("STDERR:")
            print(result.stderr)
            return False
        return True

    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        if e.stdout:
            print("STDOUT:")
            print(e.stdout)
        if e.stderr:
            print("STDERR:")
            print(e.stderr)
    except FileNotFoundError:
        print(f"Error: Command '{command.split()[0]}' not found. Make sure it's in your PATH.")
    return False

def run_sudo_command(args, command)->bool:
    """
    Runs a sudo command, prompting for password once if necessary.
    """
    try:
        # Refresh sudo timestamp to cache password
        # This will prompt for password if needed, allowing subsequent sudo commands to run without re-entry
        subprocess.run(["sudo", "-v"], check=True)

        # Execute the actual sudo command
        print(f"Running: sudo {command}")
        if args.simulate:
            if not args.failafter:
                return True
            return state['step'] != args.failafter
        result = subprocess.run(shlex.split(f"sudo {command}"), capture_output=True, text=True, check=True)
        print("STDOUT:")
        print(result.stdout)
        if result.stderr:
            print("STDERR:")
            print(result.stderr)
            return False
        return True

    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        if e.stdout:
            print("STDOUT:")
            print(e.stdout)
        if e.stderr:
            print("STDERR:")
            print(e.stderr)
    except FileNotFoundError:
        print(f"Error: Command '{command.split()[0]}' not found. Make sure it's in your PATH.")
    return False

def sudo_commands_test(args):
    print("Attempting to run multiple sudo commands...")
    run_sudo_command(args, "ls -l /root")
    run_sudo_command(args, "apt update") # This command might require password if the sudo timestamp expires
    run_sudo_command(args, "touch /root/test_file.txt")
    run_sudo_command(args, "rm /root/test_file.txt")
    print("Finished running sudo commands.")

def ensure_user_data_mounted(args)->bool:
    if userdata_mounted: return True
    # mount user data
    if not run_sudo_command(args, f'mount -t {args.mounttype} -o loop {args.archive} {args.mountpoint}'):
        return False
    userdata_mounted = True
    return True

def ensure_user_data_unmounted(args)->bool:
    if not userdata_mounted: return True
    # unmount user data
    if not run_sudo_command(args, f'umount {args.mountpoint}'):
        return False
    userdata_mounted = False
    return True

def state_update(step_done:str):
    state['step'] = step_done

def did_step(step_done:str)->bool:
    return state['step'] == step_done

def do_exit(args, condition:str):
    if not ensure_user_data_unmounted(args):
        print(f'Warning: Failed to unmount {args.mountpoint}.')
    T_end = datetime.now()
    state['T'].append( (T_start.strftime('%Y%m%d%H%M%S'), T_end.strftime('%Y%m%d%H%M%S')) )
    state['exit'] = condition
    with open(state_file, 'w') as f:
        json.dump(state, f)
    print(f'\n{condition}.')
    exit(0)

def detect_already_installed(args):
    print('\nDetecting previous replications.\n')
    do_replicate = False
    # Does the replicate state file exist in ~/ and does it have valid data?
    try:
        with open(state_file, 'r') as f:
            state = json.load(f)
        if args.restart:
            state_update('')
            print('Force new replication.')
            do_replicate = True
        else:
            # Did the previous replicate run reach completion?
            if state['exit'] != 'Completed':
                do_replicate = True
                print('Resuming replication.')
            else:
                T_last = state['T'][-1][1]
                print(f'Last replicated at {T_last}.')
                print('Beware: Replicating may overwrite existing user system environment files.')
                response = input('Do you want to replicate anyway? (y/N) ')
                if response:
                    do_replicate = response == 'y' or response == 'Y'
    except:
        do_replicate = True
        print('New replication.')
    if not do_replicate: do_exit(args, 'Canceled')
    if did_step(''):
        state_update('detect_installed')

def copy_home(args):
    print('\nCopying and merging user home data.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_home.')

    # Copy
    if not run_command(args, f'cp -r {args.mountpoint}/{this_user} {user_home}/{this_user}'): do_exit(args, 'Error during copy_home.')

    # Merge
    if not run_command(args, f'mv {user_home}/{this_user}/. {user_home}/'): do_exit(args, 'Error during copy_home.')

    state_update('copy_home')

def copy_www(args):
    print('\nCopying /var/www data.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_www.')

    # Make /var/www
    if not run_sudo_command(args, 'mkdir /var/www'): do_exit(args, 'Error during copy_www.')

    # Copy webdata
    if not run_sudo_command(args, f'cp -r {args.mountpoint}/www/webdata /var/www/'): do_exit(args, 'Error during copy_www.')

    # Change owner to www-data
    if not run_sudo_command(args, 'chown -R www-data:www-data /var/www/webdata'): do_exit(args, 'Error during copy_www.')

    # Copy html
    if not run_sudo_command(args, f'cp -r {args.mountpoint}/www/html /var/www/'): do_exit(args, 'Error during copy_www.')

    # Change owner to user
    if not run_sudo_command(args, f'chown -R {this_user}:{this_user} /var/www/html'): do_exit(args, 'Error during copy_www.')

    state_update('copy_www')

def copy_cgibin(args):
    print('\nCopying /usr/lib/cgi-bin.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_cgibin.')

    # Copy cgi-bin
    if not run_sudo_command(args, f'cp -r {args.mountpoint}/cgi-bin /usr/lib/'): do_exit(args, 'Error during copy_cgibin.')

    state_update('copy_cgibin')

def ensure_ssh(args):
    print('\nEnsuring SSH server is working.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('ensure_ssh')

def ensure_httpd(args):
    print('\nEnsuring Web server is running.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('ensure_httpd')

def copy_profile(args):
    print('\nCopying .bashrc and .profile.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('copy_profile')

def setup_postgres(args):
    print('\nSetting up postgres.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('setup_postgres')

def restore_formalizer_database(args):
    print('\nRestoring Formalizer database from backup.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('restore_formalizer_database')

def test_formalizer(args):
    print('\nTesting Formalizer by running fzsetup.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('test_formalizer')

def install_programs(args):
    print('\nInstalling designated programs.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('install_programs')

def setup_vpn(args):
    print('\nSetting up VPN.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('setup_vpn')

def setup_rsyncaccount(args):
    print('\nSetting up rsyncaccount.\n')

    print('NOT YET IMPLEMENTED.')

    state_update('setup_rsyncaccount')

# Example usage:
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Install and/or restore complete backed-up system environment.')
    parser.add_argument('archive', type=str, help='Path to a loop-mountable archive file.')
    parser.add_argument('mountpoint', type=str, help='Path to a mount-point.')
    parser.add_argument('-t', dest='mounttype', type=str, default='ext4', help='Loop-mountable type (default=ext4).')
    parser.add_argument('-R', '--restart', dest='restart', action="store_true", help='Force restart of replication from first step.')
    parser.add_argument('-S', '--simulate', dest='simulate', action="store_true", help='Simulate actions.')
    parser.add_argument('-F', '--failafter', dest='failafter', type=str, help='Simulate failure after specified step.')
    args = parser.parse_args()

    if args.failafter:
        args.simulate = True

    print("Replicate complete system environment with programs and user data.\n")
    T_start = datetime.now()
    detect_already_installed(args)

    if did_step('detect_installed'): copy_home(args)

    if did_step('copy_home'): copy_www(args)

    if did_step('copy_www'): copy_cgibin(args)

    if did_step('copy_cgibin'): ensure_ssh(args)

    if did_step('ensure_ssh'): ensure_httpd(args)

    if did_step('ensure_httpd'): copy_profile(args)

    if did_step('copy_profile'): setup_postgres(args)

    if did_step('setup_postgres'): restore_formalizer_database(args)

    if did_step('restore_formalizer_database'): test_formalizer(args)

    if did_step('test_formalizer'): install_programs(args)

    if did_step('install_programs'): setup_vpn(args)

    if did_step('setup_vpn'): setup_rsyncaccount(args)

    do_exit(args, "Completed")
