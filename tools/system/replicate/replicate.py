#!/usr/bin/env python3
# replicate.py
# Copyright 2025 Randal A. Koene
# License TBD
#
# Install and/or restore complete backed-up system environment.

# +----------------------------------------------------------+
# + IMPORTANT:                                               +
# +                                                          +
# + This script must be copied to the mountable external     +
# + drive that contains the mountable environment backup.    +
# + Copy the script to the new system to start installation. +
# + This is necessary in case scp copying of the script is   +
# + not possible, such as then the original system is not    +
# + runnable.                                                +
# +----------------------------------------------------------+

import subprocess
import shlex
import os
import json
import argparse
from datetime import datetime

this_user = os.environ['USER']
user_home = os.environ['HOME']
state_file = user_home+'/.replicate_state.json'
stdout_file = user_home+'/.replicate_quiet_stdout'
stderr_file = user_home+'/.replicate_quiet_stderr'

T_start = None
Step = None

userdata_mounted = False

state = {
    'T': [],    # intervals during which replication activity has been taking place
    'step': '', # the last step that completed successfully, used for optional resume
    'exit': '', # previous exit condition
}

run_result = None
error_groups = None

def check_deletes_failed(stderr_str: str):
    global error_groups
    other_messages = []
    err_messages = stderr_str.split('\n')
    deletes_failed = 0
    for message in err_messages:
        if message.find('rsync: [sender] sender failed to remove') >= 0:
            deletes_failed += 1
        else:
            other_messages.append(message)
    error_groups = {
        'deletes': deletes_failed,
        'other_num': size(other_messages),
        'other': '\n'.join(other_messages),
    }

def run_command(args, command, quiet_stdout_stderr=False)->bool:
    """
    Runs a command.
    """
    global state
    global run_result
    if isinstance(command, list):
        command_str = ' '.join(command)
    else:
        command_str = command
        command = shlex.split(f"{command}")
    try:
        if not quiet_stdout_stderr:
            print(f"Running: {command_str}")
        if args.simulate:
            print('(** Simulating in step [%s])' % state['step'])
            if not args.failafter:
                return True
            return state['step'] != args.failafter
        run_result = subprocess.run(command, capture_output=True, text=True, check=True)
        if not quiet_stdout_stderr:
            print("STDOUT:")
            print(run_result.stdout)
        else:
            if len(run_result.stdout) > 0:
                with open(stdout_file, 'a') as f:
                    f.write(run_result.stdout+'\n')
        if run_result.stderr:
            check_deletes_failed(run_result.stderr)
            if not quiet_stdout_stderr:
                print("STDERR:")
                print(error_groups['other'])
            else:
                if len(error_groups['other_num']) > 0:
                    with open(stderr_file, 'a') as f:
                        f.write(error_groups['other']+'\n')
            if error_groups['deletes'] > 0:
                print('Rsync deletes failed: %d' % error_groups['deletes'])
            return False
        return run_result.returncode == 0

    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        if e.stdout:
            print("STDOUT:")
            print(e.stdout)
        if e.stderr:
            print("STDERR:")
            print(e.stderr)
    except FileNotFoundError:
        print(f"Error: Command '{command_str.split()[0]}' not found. Make sure it's in your PATH.")
    return False

def run_sudo_command(args, command, quiet_stdout_stderr=False)->bool:
    """
    Runs a sudo command, prompting for password once if necessary.
    """
    global state
    global run_result
    if isinstance(command, list):
        command_str = ' '.join(command)
    else:
        command_str = command
        command = shlex.split(f"{command}")
    try:
        # Refresh sudo timestamp to cache password
        # This will prompt for password if needed, allowing subsequent sudo commands to run without re-entry
        subprocess.run(["sudo", "-v"], check=True)

        # Execute the actual sudo command
        if not quiet_stdout_stderr:
            print(f"Running: sudo {command_str}")
        if args.simulate:
            print('(** Simulating in step [%s])' % state['step'])
            if not args.failafter:
                return True
            return state['step'] != args.failafter
        run_result = subprocess.run(['sudo']+command, capture_output=True, text=True, check=True)
        if not quiet_stdout_stderr:
            print("STDOUT:")
            print(run_result.stdout)
        else:
            if len(run_result.stdout) > 0:
                with open(stdout_file, 'a') as f:
                    f.write(run_result.stdout+'\n')
        if run_result.stderr:
            if not quiet_stdout_stderr:
                print("STDERR:")
                print(run_result.stderr)
            else:
                if len(run_result.stderr) > 0:
                    with open(stderr_file, 'a') as f:
                        f.write(run_result.stderr+'\n')
            return False
        return run_result.returncode == 0

    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {e}")
        if e.stdout:
            print("STDOUT:")
            print(e.stdout)
        if e.stderr:
            print("STDERR:")
            print(e.stderr)
    except FileNotFoundError:
        print(f"Error: Command '{command_str.split()[0]}' not found. Make sure it's in your PATH.")
    return False

def sudo_commands_test(args):
    print("Attempting to run multiple sudo commands...")
    run_sudo_command(args, "ls -l /root")
    run_sudo_command(args, "apt update") # This command might require password if the sudo timestamp expires
    run_sudo_command(args, "touch /root/test_file.txt")
    run_sudo_command(args, "rm /root/test_file.txt")
    print("Finished running sudo commands.")

def ensure_user_data_mounted(args)->bool:
    global userdata_mounted
    if userdata_mounted: return True
    # mount user data
    if not run_sudo_command(args, f'mount -t {args.mounttype} -o loop {args.archive} {args.mountpoint}'):
        return False
    run_command(args, 'df')
    if not run_command(args, f'mountpoint {args.mountpoint}'):
        do_exit(args, f'Archive failed to mount at {args.mountpoint}')
    userdata_mounted = True
    # test if user names are identical
    if not os.path.isdir(f'{args.mountpoint}/{this_user}'):
        print(f'=> User name may be different. Directory {args.mountpoint}/{this_user} not found.')
        do_exit(args, 'User name not in archive')
    return True

def ensure_user_data_unmounted(args)->bool:
    global userdata_mounted
    if not userdata_mounted: return True
    # unmount user data
    if not run_sudo_command(args, f'umount {args.mountpoint}'):
        return False
    userdata_mounted = False
    return True

def state_update(step_done:str):
    global state
    state['step'] = step_done
    print('\n=> Step done: %s' % str(step_done))

def did_step(step_done:str)->bool:
    global state
    return state['step'] == step_done

def do_exit(args, condition:str):
    global state
    if not ensure_user_data_unmounted(args):
        print(f'Warning: Failed to unmount {args.mountpoint}.')
    T_end = datetime.now()
    state['T'].append( (T_start.strftime('%Y%m%d%H%M%S'), T_end.strftime('%Y%m%d%H%M%S')) )
    state['exit'] = condition
    with open(state_file, 'w') as f:
        json.dump(state, f)
    print(f'\n{condition}.')
    exit(0)

def test_to_here(args):
    print('\nStopping early due to "test_to_here".')
    do_exit(args, 'Test to Here')

def detect_already_installed(args):
    global state
    print('\nDetecting previous replications.\n')
    do_replicate = False
    # Does the replicate state file exist in ~/ and does it have valid data?
    try:
        with open(state_file, 'r') as f:
            prev_state = json.load(f)
        if not isinstance(prev_state, dict):
            print('=> Invalid state file.')
            do_replicate = True
            print('=> New replication.')
        else:
            state = prev_state
            if args.restart:
                state_update('')
                print('=> Force new replication.')
                do_replicate = True
            else:
                # Did the previous replicate run reach completion?
                if state['exit'] != 'Completed':
                    print('Last run failed with: %s' % str(state['exit']))
                    do_replicate = True
                    print('=> Resuming replication from %s after completed step [%s]' % (state['T'][-1][1], state['step']))
                else:
                    T_last = state['T'][-1][1]
                    print(f'=> Last replicated at {T_last}.')
                    print('Beware: Replicating may overwrite existing user system environment files.')
                    response = input('Do you want to replicate anyway? (y/N) ')
                    if response:
                        do_replicate = response == 'y' or response == 'Y'
                        # *** Should this follow?
                        # state_update('')
                        # *** Or something else that causes sync without full copying?
    except:
        do_replicate = True
        print('=> New replication.')
    if not do_replicate: do_exit(args, 'Canceled')
    if did_step(''):
        state_update('detect_installed')

def copy_home(args):
    print('\nCopying user home data.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_home.')

    if not run_command(args, f'cp -r {args.mountpoint}/{this_user} {user_home}/{this_user}'): do_exit(args, 'Error during copy_home.')

    state_update('copy_home')

def check_undeleted_files(args, dirpath:str, filenames:list):
    print('Checking %d undeleted files...' % len(filenames))
    remaining = []
    # Form corresponding ~/ dirpath
    p = dirpath.find('/'+this_user)
    home_dirpath = dirpath[0:p]+dirpath[p+len('/'+this_user):]
    for fname in filenames:
        if not os.path.exists(home_dirpath+'/'+fname):
            remaining.append(fname)
        else:
            # Compare corresponding files
            if run_command(args, ['cmp', '-s', home_dirpath+'/'+fname, dirpath+'/'+fname], quiet_stdout_stderr=True):
                # Delete the identical file
                if not run_sudo_command(args, ['rm', '-f', dirpath+'/'+fname], quiet_stdout_stderr=True):
                    remaining.append(fname)
            else:
                remaining.append(fname)
            if error_groups:
                if 'other_num' in error_groups:
                    if len(error_groups['other_num']) > 0:
                        print('An error occurred. Check %s' % stderr_file)
    if len(remaining)==0:
        return None
    return remaining

def delete_empty_folders(args)->bool:
    print('\nDeleting empty folders of source copy...')
    total_deleted_count = 0
    errors = 0
    while True:
        directories_with_content = 0
        remaining_files = 0
        deleted_count = 0
        for dirpath, dirnames, filenames in os.walk(user_home+'/'+this_user, topdown=False):
            # Check if the current directory is empty (contains no files and no subdirectories)
            if not dirnames and not filenames:
                # Sudo here to also delete directories marked read only
                if run_sudo_command(args, ['rmdir', dirpath], quiet_stdout_stderr=True):
                    deleted_count += 1
                else:
                    errors += 1
            else:
                if filenames:
                    filenames = check_undeleted_files(args, dirpath, filenames)
                    if filenames:
                        remaining_files += len(filenames)
                    if not dirnames and not filenames:
                        if run_sudo_command(args, ['rmdir', dirpath], quiet_stdout_stderr=True):
                            deleted_count += 1
                        else:
                            errors += 1
                    else:
                        directories_with_content += 1
                else:
                    directories_with_content += 1
        total_deleted_count += deleted_count
        print('Iteration, deleted count: %d' % deleted_count)
        if deleted_count == 0:
            break;

    if total_deleted_count == 0:
        print("No empty directories found.")
    else:
        print(f"Successfully deleted {total_deleted_count} empty directories.")
    print(f'Directories with content in {user_home}/{this_user}: {directories_with_content}')
    if directories_with_content > 0:
        print('=> Note: You may want to delete these manually.')
    if errors > 0:
        print('Errors: %d' % errors)
        print(f'You can read the error output in {user_home}/{stderr_file}.')
    print(f'You can read suppressed standard output in {user_home}/{stdout_file}.')

    return errors == 0

def merge_home(args):
    print('\nMerging user home data.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during merge_home.')

    if not run_command(args, f'rsync -a --remove-source-files --ignore-existing {user_home}/{this_user}/ {user_home}/'):
        if error_groups['other_num'] > 0:
            do_exit(args, 'Error during merge_home.')

    if not delete_empty_folders(args): do_exit(args, 'Error during merge_home.')

    state_update('merge_home')

def copy_www_webdata(args):
    print('\nCopying /var/www/webdata data.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_www_webdata.')

    # Make /var/www
    if not run_sudo_command(args, 'mkdir /var/www'): do_exit(args, 'Error during copy_www_webdata.')

    # Copy webdata
    if not run_sudo_command(args, f'cp -r {args.mountpoint}/www/webdata /var/www/'): do_exit(args, 'Error during copy_www_webdata.')

    # Change owner to www-data
    if not run_sudo_command(args, 'chown -R www-data:www-data /var/www/webdata'): do_exit(args, 'Error during copy_www_webdata.')

    state_update('copy_www_webdata')

def copy_www_html(args):
    print('\nCopying /var/www/html data.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_www_html.')

    # Copy html
    if not run_sudo_command(args, f'cp -r {args.mountpoint}/www/html /var/www/'): do_exit(args, 'Error during copy_www_html.')

    # Change owner to user
    if not run_sudo_command(args, f'chown -R {this_user}:{this_user} /var/www/html'): do_exit(args, 'Error during copy_www_html.')

    state_update('copy_www_html')

def copy_cgibin(args):
    print('\nCopying /usr/lib/cgi-bin.\n')
    if not ensure_user_data_mounted(args): do_exit(args, 'Error during copy_cgibin.')

    # Copy cgi-bin
    if not run_sudo_command(args, f'cp -r {args.mountpoint}/cgi-bin /usr/lib/'): do_exit(args, 'Error during copy_cgibin.')

    # Make files in /usr/lib/cgi-bin executable
    run_sudo_command(args, f'chmod -R a+x /usr/lib/cgi-bin')

    state_update('copy_cgibin')

def check_ssh()->bool:
    if run_command(args, 'systemctl is-active --quiet sshd'):
        print('SSH server is running and active.')
        return True
    print('SSH server is not running or not active.')
    return False

def ensure_ssh(args):
    print('\nEnsuring SSH server is working.\n')

    if check_ssh():
        state_update('ensure_ssh')
        return

    # Install and activate SSH server
    print('Installing OpenSSH server...')
    run_sudo_command(args, 'apt update -y')
    run_sudo_command(args, 'apt install openssh-server -y')

    print('Enabling and starting SSH service...')
    run_sudo_command(args, 'systemctl enable --now ssh')

    if check_ssh():
        state_update('ensure_ssh')
    else:
        do_exit(args, 'Error during ensure_ssh.')

def check_apache()->bool:
    if run_command(args, 'pgrep -x apache2'):
        print('Apache server is running.')
        return True
    print('Apache server is not running.')
    return False

def ensure_httpd(args):
    print('\nEnsuring Web server is running.\n')

    if check_apache():
        state_update('ensure_httpd')
        return

    # Install and activate Apache server
    print('Installing Apache server...')
    run_sudo_command(args, 'apt update -y')
    run_sudo_command(args, 'apt install apache2 -y')

    print('Enabling and starting Apache service...')
    run_sudo_command(args, 'systemctl enable --now apache2')

    # *** There may be more Apache set up at: https://trello.com/c/kztslGPJ

    if check_apache():
        state_update('ensure_httpd')
    else:
        do_exit(args, 'Error during ensure_httpd.')

def copy_profile(args):
    print('\nCopying .bashrc and .profile.\n')

    print('NOT YET IMPLEMENTED -- See Trello for dev details.')

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
    print('There is a list in ~/apt-installed.txt, but the list is too long. There should be a subset of much-used programs.')
    print('More detailed in Trello.')

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

    if os.path.exists(stdout_file):
        os.remove(stdout_file)
    if os.path.exists(stderr_file):
        os.remove(stderr_file)

    print("Replicate complete system environment with programs and user data.\n")
    T_start = datetime.now()
    detect_already_installed(args)

    if did_step('detect_installed'): copy_home(args)
    if did_step('copy_home'): merge_home(args)

    if did_step('merge_home'): copy_www_webdata(args)

    if did_step('copy_www_webdata'): copy_www_html(args)

    if did_step('copy_www_html'): copy_cgibin(args)

    if did_step('copy_cgibin'): ensure_ssh(args)

    if did_step('ensure_ssh'): ensure_httpd(args)

    if did_step('ensure_httpd'): copy_profile(args)

    test_to_here(args)

    if did_step('copy_profile'): setup_postgres(args)

    if did_step('setup_postgres'): restore_formalizer_database(args)

    if did_step('restore_formalizer_database'): test_formalizer(args)

    if did_step('test_formalizer'): install_programs(args)

    if did_step('install_programs'): setup_vpn(args)

    if did_step('setup_vpn'): setup_rsyncaccount(args)

    do_exit(args, "Completed")
