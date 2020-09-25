#!/usr/bin/python3
#
# fzbackup.py
#
# Randal A. Koene, 20200924
#
# The `fzbackup` utility is the authoritative method to back-up a Formalizer environment.

# std
import os
import sys
import json
import argparse
import subprocess
from datetime import datetime

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzsetupconfigdir = fzuserbase+'/config/fzsetup.py'
fzsetupconfig = fzsetupconfigdir+'/config.json'

try:
    with open(fzsetupconfig) as f:
        config = json.load(f)

except FileNotFoundError:
    print('Unable to load fundamental standard configuration data.\nPlease run `fzsetup -l` first to self-initialize and check the results.\n')
    exit(1)

# Enable us to import standardized Formalizer Python components.
fzcorelibdir = config['sourceroot'] + '/core/lib'
fzcoreincludedir = config['sourceroot'] + '/core/include'
sys.path.append(fzcorelibdir)
sys.path.append(fzcoreincludedir)

# core components
import coreversion

fzbackupconfigdir = fzuserbase+'/config/fzbackup.py'
fzbackupconfig = fzbackupconfigdir+'/config.json'


# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring):
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
        if config['verbose']:
            print(res)
        return 0


# Handle the case where fzbackup.py does not have a configuration file yet.
try:
    with open(fzbackupconfig) as f:
        backupconfig = json.load(f)

except FileNotFoundError:
    print('Creating configuration file for fzbackup.py.\n')
    backupconfig = {
        'backuproot' : userhome+'/.archive/postgres/'
    }
    retcode = try_subprocess_check_output(f'mkdir -p {fzbackupconfigdir}')
    if (retcode != 0):
        print(f'Unable to create the config directory {fzbackupconfigdir}')
        exit(retcode)
    with open(fzbackupconfig,'w') as cfgfile:
        json.dump(backupconfig, cfgfile, indent = 4, sort_keys=True)

else:
    assert len(backupconfig) > 0

version = "0.1.0-0.1"

flow_control = {
    'make_backup' : True
}


def parse_options():
    theepilog = ('This version of fzbackup assumes that the current user has all the necessary\n'
                'permissions to carry out a pg_dump of all tables in the database.\n\n')

    parser = argparse.ArgumentParser(description='Make a backup of a Formalizer environment.',epilog=theepilog)
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = os.getenv('USER')
    if args.verbose:
        config['verbose'] = True


    if config['verbose']:
        print('Working with the following targets:\n')
        print(f'  Formalizer Postgres database name   : {args.dbname}')
        print(f'  Formalizer user or group schema name: {args.schemaname}\n')

        choice = input('Is this correct? (y/N) \n')

        if (choice != 'y'):
            print('Ok. You can try again with different command arguments.\n')
            exit(0)

    return args

def make_backup():
    # make the backup target backup directory if necessary
    retcode = try_subprocess_check_output(f'mkdir -p {backupconfig["backuproot"]}')
    if (retcode != 0):
        print(f'Unable to create the config directory {backupconfig["backuproot"]}')
        exit(retcode)
    
    # create the backup name
    now = datetime.now()
    timestamp = now.strftime('%Y%m%d%H%M')
    backuppath = backupconfig["backuproot"]+"formalizer-postgres-backup-"+timestamp+".gz"

    # do the pg_dump to the designated file
    retcode = try_subprocess_check_output(f'pg_dump {args.dbname} | gzip > {backuppath}')
    if (retcode != 0):
        print(f'Unable to backup the {args.dbname} database to {backuppath}')
        exit(retcode)

    print(f'Formalizer database ({args.dbname}) backed up to {backuppath}')
    exit(0)
    


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Backup v{version} (core v{core_version})"

    print(server_long_id+"\n")

    args = parse_options()

    #if flow_control['reset_environment']:
    #    reset_environment(args)

    make_backup()


    #print('Note: Some options have not been fully implemented yet.')


    exit(0)
