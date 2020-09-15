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
    'verbose' : False,
    'cgiuser' : 'www-data'
}

flow_control = {
    'create_database' : False,
    'create_schema' : False,
    'create_tables' : False,
    'make_fzuser_role' : False,
    'make_binaries' : False,
    'reset_environment' : False,
    'reset_graph' : False,
    'reset_log' : False,
    'reset_metrics' : False,
    'reset_guide' : False
}

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

def database_exists(dbname):
    try:
        res = subprocess.check_output('psql -c "select 1" -d '+dbname, shell=True, stderr=subprocess.DEVNULL) # this is your command
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
            print(f'Database { dbname } exists.')
        else:
            print(f'Database { dbname } does not exist.')
    
    return dbexists


def create_database(dbname):
    dbexists = database_exists(dbname)
    if not dbexists:
        print(f'Creating database {dbname}')
        retcode = try_subprocess_check_output(f"createdb {dbname}")
        if (retcode != 0):
            print('Unable to create the database')
            exit(retcode)

        else:
            print('Database created.')

    else:
        print(f'Database {dbname} already exists')


def create_schema(cmdargs):
    print(f'Creating schema {cmdargs.schemaname} if it does not already exist')
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'CREATE SCHEMA IF NOT EXISTS {cmdargs.schemaname};'")
    if (retcode != 0):
        print('Unable to create the schema')
        exit(retcode)
    else:
        print(f'The schema {cmdargs.schemaname} in database {cmdargs.dbname} should now exist.')      


def create_tables(cmdargs):
    # This should create all the tables fresh if they don't exist yet.
    print('***This is just a test to see if the PQ table layouts were imported correctly:')
    print(Graphpostgres.pq_topiclayout) # That worked!
    print('\n***Implement actual table creation here!\n')


def grant_fzuser_access(cmdargs,beverbose):
    cgiuser = config['cgiuser']
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA \"{cmdargs.schemaname}\" TO \"{cgiuser}\";'")
    if (retcode != 0):
        print(f'Unable to give access permissions to {cgiuser}.')
        return retcode
    if beverbose:
        print(f'The {cgiuser} has access permissions in schema {cmdargs.schemaname} in database {cmdargs.dbname}.')
    return 0


def make_fzuser_role(cmdargs,beverbose):
    fzuser = f"fz{cmdargs.schemaname}"
    cgiuser = config['cgiuser']
    print(f'Creating {fzuser} role if it does not already exist.')
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'CREATE ROLE \"{cgiuser}\";'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'CREATE ROLE \"{fzuser}\";'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'GRANT \"{fzuser}\" TO \"{cmdargs.schemaname}\";'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'GRANT \"{fzuser}\" TO \"{cgiuser}\";'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'ALTER SCHEMA \"{cmdargs.schemaname}\" OWNER TO \"{fzuser}\";'")
    retcode += grant_fzuser_access(cmdargs,False)
    if (retcode != 0):
        print(f'Creating {cgiuser} and {fzuser} roles, and giving ownership and access permissions failed.')
        exit(retcode)
    if beverbose:
        print(f'The {fzuser} role owns schema {cmdargs.schemaname} in database {cmdargs.dbname}, and  {cgiuser} and {fzuser} roles have access permissions to schema {cmdargs.schemaname}.')


"""
Calls `make` in the main Formalizer source directory, which ultimately
results in Formalizer core and tools executables being available through
symbolic links from ~/.formalizer/bin.
"""
def make_binaries_available():
    print('Calling `make` in Foramlizer source root to creaet ~/.formalizer/bin')
    retcode = try_subprocess_check_output('cd ~/src/formalizer && make')
    if (retcode != 0):
        print('Unable to call Formalizer root `make`')
        exit(retcode)   
    else:
        print('Formalizer executables made available at ~/.formalizer/bin')
        print('Please update your PATH to include ~/.formalizer/bin!')

    
def set_All_flowcontrol():
    flow_control['create_database']=True
    flow_control['create_schema']=True
    flow_control['create_tables']=True
    flow_control['make_fzuser_role']=True
    flow_control['make_binaries']=True    


"""
Reset the Formalizer environment. After confirmation, this will delete the
existing schema (and possibly a few other things) to restore a fresh
environment. The function asks if the user would like to run the -A option
immediately to set up the fresh environment.
"""
def reset_environment(cmdargs):
    print('This is the RESET function. It DELETES the existing Formalizer database schema.\n')
    confirmation = input('Are you absolutely sure you want to proceed? (y/N) ')
    if (confirmation != 'y'):
        print('That is always the safer option...\n')
        exit(0)
    
    print(f'Removing the {cmdargs.schemaname} schema from the {cmdargs.dbname} database.')
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP SCHEMA \"{cmdargs.schemaname}\" CASCADE;'")
    if (retcode != 0):
        print(f'Attempt to remove schema failed.')
        exit(retcode)
    
    print(f'The schema has been removed.\n')
    choice = input('Would you like to immediately initialize a fresh Formalizer environment (-A)? (Y/n) ')
    if (choice == 'n'):
        print('Re-initializing skipped. Done.\n')
        exit(0)

    set_All_flowcontrol()
    print('Proceeding to re-initalization.\n')


def reset_graph(cmdargs):
    print('This is the Graph Reset function. It DELETES Formalizer Graph tables.\n')
    confirmation = input('Are you absolutely sure you want to proceed? (y/N) ')
    if (confirmation != 'y'):
        print('That is always the safer option...\n')
        exit(0)
    
    print(f'Removing the Graph tables and types in {cmdargs.schemaname} schema from the {cmdargs.dbname} database.')
    nodestable = '"'+cmdargs.schemaname + '".nodes'
    edgestable = '"'+cmdargs.schemaname + '".edges'
    topicstable = '"'+cmdargs.schemaname + '".topics'
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {edgestable} CASCADE;'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {nodestable} CASCADE;'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {topicstable} CASCADE;'")
    tdpropertytype = '"'+cmdargs.schemaname + '".td_property'
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TYPE IF EXISTS {tdpropertytype};'")
    tdpatterntype = '"'+cmdargs.schemaname + '".td_pattern'
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TYPE IF EXISTS {tdpatterntype};'")
    if (retcode != 0):
        print(f'Attempt to remove tables failed.')
        exit(retcode)
    
    print(f'The tables have been removed. Done.\n')
    exit(0)

def reset_log(cmdargs):
    print('This is the Log Reset function. It DELETES Formalizer Log tables.\n')
    confirmation = input('Are you absolutely sure you want to proceed? (y/N) ')
    if (confirmation != 'y'):
        print('That is always the safer option...\n')
        exit(0)
    
    print(f'Removing the Log tables in {cmdargs.schemaname} schema from the {cmdargs.dbname} database.')
    chunkstable = '"'+cmdargs.schemaname + '".chunks'
    entriestable = '"'+cmdargs.schemaname + '".entries'
    breakpointstable = '"'+cmdargs.schemaname + '".breakpoints'
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {breakpointstable} CASCADE;'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {chunkstable} CASCADE;'")
    retcode += try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {entriestable} CASCADE;'")
    if (retcode != 0):
        print(f'Attempt to remove tables failed.')
        exit(retcode)
    
    print(f'The tables have been removed. Done.\n')
    exit(0)

def reset_metrics(cmdargs):
    print('This is the Metrics Reset function. It DELETES Formalizer Metrics tables.\n')
    confirmation = input('Are you absolutely sure you want to proceed? (y/N) ')
    if (confirmation != 'y'):
        print('That is always the safer option...\n')
        exit(0)
    
    print(f'Removing the Metrics tables in {cmdargs.schemaname} schema from the {cmdargs.dbname} database.')
    print('\nTHIS IS JUST A STUB.\n')
    retcode = 1
    # retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {metricstable} CASCADE;'")
    if (retcode != 0):
        print(f'Attempt to remove tables failed.')
        exit(retcode)
    
    print(f'The tables have been removed. Done.\n')
    exit(0)

def reset_guide(cmdargs):
    print('This is the Guide Reset function. It DELETES Formalizer Guide tables.\n')
    confirmation = input('Are you absolutely sure you want to proceed? (y/N) ')
    if (confirmation != 'y'):
        print('That is always the safer option...\n')
        exit(0)
    
    print(f'Removing the Guide tables in {cmdargs.schemaname} schema from the {cmdargs.dbname} database.')
    systemguidetable = '"'+cmdargs.schemaname + '".guide_system'
    retcode = try_subprocess_check_output(f"psql -d {cmdargs.dbname} -c 'DROP TABLE IF EXISTS {systemguidetable} CASCADE;'")
    if (retcode != 0):
        print(f'Attempt to remove tables failed.')
        exit(retcode)

    print(f'The tables have been removed. Done.\n')
    exit(0)


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Setup v{version} (core v{core_version})"

    print(server_long_id+"\n")

    parser = argparse.ArgumentParser(description='Setup or refresh a Formalizer environment.',epilog='Note that the schema name is also used for the fz(schema) role.\n\nReset "all" will delete the existing schema and then suggest -A.')
    parser.add_argument('-A', '--All', dest='doall', action="store_true", help='do all setup steps, ensure environment is ready')
    parser.add_argument('-1', '--One', metavar='setupaction', help='specify a step to do: database, schema, tables, fzuser, binaries')
    parser.add_argument('-d', '--database', dest='dbname', help='specify database name (default: formalizer)')
    parser.add_argument('-s', '--schema', dest='schemaname', help='specify schema name (default: $USER)')
    parser.add_argument('-p', '--permissions', dest='permissions', action='store_true', help=f'give access permissions to {config["cgiuser"]}')
    parser.add_argument('-m', '--makebins', dest='makebins', action='store_true', help='make Formalizer binaries available')
    parser.add_argument('-v', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')
    parser.add_argument('-R', '--reset', dest='reset', help='reset: all, graph, log, metrics, guide')

    args = parser.parse_args()

    if not args.dbname:
        args.dbname = 'formalizer'
    if not args.schemaname:
        args.schemaname = os.getenv('USER')
    if args.verbose:
        config['verbose'] = True
    if args.doall:
        set_All_flowcontrol()
    if args.One:
        if (args.One == "database"):
            flow_control['create_database']=True
        if (args.One == "schema"):
            flow_control['create_schema']=True
        if (args.One == "tables"):
            flow_control['create_tables']=True
        if (args.One == "fzuser"):
            flow_control['make_fzuser_role']=True
        if (args.One == "binaries"):
            flow_control['make_binaries']=True
    if args.reset:
        if (args.reset == "all"):
            flow_control['reset_environment']=True
        if (args.reset == "graph"):
            flow_control['reset_graph']=True
        if (args.reset == "log"):
            flow_control['reset_log']=True
        if (args.reset == "metrics"):
            flow_control['reset_metrics']=True
        if (args.reset == "guide"):
            flow_control['reset_guide']=True


    print('Working with the following targets:\n')
    print(f'  Formalizer Postgres database name   : {args.dbname}')
    print(f'  Formalizer user or group schema name: {args.schemaname}\n')

    choice = input('Is this correct? (y/N) \n')

    if (choice != 'y'):
        print('Ok. You can try again with different command arguments.\n')
        exit(0)

    if flow_control['reset_environment']:
        reset_environment(args)
    if flow_control['reset_graph']:
        reset_graph(args)
    if flow_control['reset_log']:
        reset_log(args)
    if flow_control['reset_metrics']:
        reset_metrics(args)
    if flow_control['reset_guide']:
        reset_guide(args)

    if flow_control['create_database']:
        create_database(args.dbname)
    if flow_control['create_schema']:
        create_schema(args)
    if flow_control['create_tables']:
        create_tables(args)
    if flow_control['make_fzuser_role']:
        make_fzuser_role(args,True)
    if flow_control['make_binaries']:
        make_binaries_available()


    exit(0)
