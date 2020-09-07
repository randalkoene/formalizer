#!/usr/bin/python3
#
# fzsetup.py
#
# Randal A. Koene, 20200905
#
# The `fzsetup` utility is the authoritative method to prepare or refresh a Formalizer environment.

import os
import sys
sys.path.append(os.getenv('HOME')+'/src/formalizer/core/lib')
sys.path.append(os.getenv('HOME')+'/src/formalizer/core/include')

import argparse
import subprocess

import Graphpostgres
import coreversion

version = "0.1.0-0.1"

config = {
    'verbose' : False
}

flow_control = {
    'create_database' : False,
    'create_schema' : False,
    'create_tables' : False,
    'make_fzuser_role' : False,
    'make_binaries' : False
}

def database_exists(dbname):
    try:
        res = subprocess.check_output('psql -c "select 1" -d '+dbname, shell=True) # this is your command
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        dbexists = False
    else:
        if config['verbose']:
            print(res)
        dbexists = True

    if config['verbose']:
        if dbexists:
            print(f'Database { args.dbname } exists.')
        else:
            print(f'Database { args.dbname } does not exist.')
    
    return dbexists


def create_database(dbname):
    dbexists = database_exists(args.dbname)
    if not dbexists:
        print('Creating database {dbname}')
        try:
            res = subprocess.check_output('echo "DUMMY-CALL. Should be: psql -c ..."', shell=True)
        except subprocess.CalledProcessError as cpe:
            if config['verbose']:
                print('Error output: ',cpe.output)
                print('Error code  : ',cpe.returncode)
            print('Unable to create the database')
            exit(cpe.returncode)
        else:
            if config['verbose']:
                print(res)
            print('Database created.')
    else:
        print('Database {dbname} already exists')


def create_schema(cmdargs):
    print('Creating schema {cmdargs.schemaname} if it does not already exist')
    try:
        res = subprocess.check_output('echo "DUMMY-CALL. Should be: psql -c ..."', shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        print('Unable to create the schema')
        exit(cpe.returncode)   
    else:
        if config['verbose']:
            print(res)
        print('Scheme {cmdargs.schemaname} exists.')         


def create_tables(cmdargs):
    # This should create all the tables fresh if they don't exist yet.
    print('***This is just a test to see if the PQ table layouts were imported correctly:')
    print(Graphpostgres.pq_topiclayout) # That worked!
    pass


def make_fzuser_role(cmdargs):
    pass

"""
Calls `make` in the main Formalizer source directory, which ultimately
results in Formalizer core and tools executables being available through
symbolic links from ~/.formalizer/bin.
"""
def make_binaries_available():
    print('Calling `make` in Foramlizer source root to creaet ~/.formalizer/bin')
    try:
        res = subprocess.check_output('cd ~/src/formalizer && make', shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        print('Unable to call Formalizer root make')
        exit(cpe.returncode)   
    else:
        if config['verbose']:
            print(res)
        print('Formalizer executables made available at ~/.formalizer/bin')
        print('Please update your PATH to include ~/.formalizer/bin!')


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Setup v{version} (core v{core_version})"

    print(server_long_id)

    parser = argparse.ArgumentParser(description='Setup or refresh a Formalizer environment.')
    parser.add_argument('-A', '--All', dest='doall', action="store_true", help='do all setup steps, ensure environment is ready')
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    parser.add_argument('-m', '--makebins', dest='makebins', action='store_true', help='make Formalizer binaries available')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = os.getenv('USER')
    if args.verbose:
        config['verbose'] = True
    if args.doall:
        flow_control['create_database']=True
        flow_control['create_schema']=True
        flow_control['create_tables']=True
        flow_control['make_fzuser_role']=True
        flow_control['make_binaries']=True

    print('Formalizer:Setup 1.0.0-0.1 (core v2.0.0-0.1)\n')

    print('Working with the following targets:\n')
    print(f'  Formalizer Postgres database name   : {args.dbname}')
    print(f'  Formalizer user or group schema name: {args.schemaname}\n')

    choice = input('Is this correct? (y/N) \n')

    if (choice != 'y'):
        print('Ok. You can try again with different command arguments.\n')
        exit(0)

    if flow_control['create_database']:
        create_database(args.dbname)
    if flow_control['create_schema']:
        create_schema(args)
    if flow_control['create_tables']:
        create_tables(args)
    if flow_control['make_fzuser_role']:
        make_fzuser_role(args)
    if flow_control['make_binaries']:
        make_binaries_available()


    exit(0)
