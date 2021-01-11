#! /usr/bin/python3
#
# graph2dil-integrity.py
#
# Randal A. Koene, 20210110
#
# Carry out integrity tests by compariing regenerated Formalizer 1.x files with
# original Formalizer 1.x files.

# std
import os
import sys
import json
import argparse
import subprocess
import pty

# Standardized expectations.
userhome = os.getenv('HOME')
fzuserbase = userhome + '/.formalizer'
fzconfigdir = fzuserbase + '/config'
graph2dilconfigdir = fzconfigdir+'/graph2dil'
graph2dilconfig = graph2dilconfigdir+'/config.json'
DILfilesdir = userhome + '/doc/html/lists'


# We need this everywhere to run various shell commands.
def try_subprocess_check_output(thecmdstring):
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if args.verbose:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
        if args.verbose:
            print(res)
        return 0


def pty_process(thecmd):
    # Making this a drop-in replacement for try_subprocess by converting command string
    # and by returning an equivalent return status.
    splitcmd = thecmd.split()
    retcode = pty.spawn(splitcmd)
    #print('Back from logentry.')
    if not os.WIFEXITED(retcode):
        print(f'Spawned pty for {thecmd} terminated abnormally')
        sys.exit(os.WEXITSTATUS(retcode))
    return os.WEXITSTATUS(retcode)


def get_graph2dil_config():
    global config
    try:
        with open(graph2dilconfig) as f:
            config = json.load(f)

    except FileNotFoundError:
        config = {
            "DILTLdirectory" : "/var/www/html/formalizer/graph2dil"
        }
        print(f'No file {graph2dilconfig}, using default:\n\tDILTLdirectory = {config["DILTLdirectory"]}\n')


def parse_options():
    theepilog = ('\n')

    parser = argparse.ArgumentParser(description='Test the integrity of graph2dil conversion by comparing generated HTML files\nwith original Formalizer 1.x HTML files.',epilog=theepilog)
    parser.add_argument('-i', '--inspector', dest='inspector', help='specify inspector program (default=less)')
    parser.add_argument('-G', '--graph', dest='graph', action="store_true", help='Compare original and regenerated Graph')
    parser.add_argument('-L', '--log', dest='log', action="store_true", help='Compare original and regenerated Log')
    parser.add_argument('-V', '--verbose', dest='verbose', action="store_true", help='turn on verbose mode')

    return parser.parse_args()


def compare_Graph_original_and_regenerated():
    run_graph2dil = input('Run graph2dil on Graph (no skips this step)? (Y/n) ')
    if (run_graph2dil != 'n'):
        thecmd = 'graph2dil -D'
        if args.verbose:
            thecmd += ' -V'
        retcode = pty_process(thecmd)
        #retcode = try_subprocess_check_output(thecmd)
        if (retcode != 0):
            print('graph2dil failed.')
            sys.exit(retcode)
    
    run_diff = input('Ready to run graph2dil-diff.sh (no exits)? (Y/n) ')
    if (run_diff == 'n'):
        print('Stopping.')
        sys.exit(0)
    
    thecmd = f'graph2dil-diff.sh -G {DILfilesdir} {config["DILTLdirectory"]}/lists'
    #retcode = try_subprocess_check_output(thecmd)
    retcode = pty_process(thecmd)
    if (retcode != 0):
        print('graph2dil-diff.sh failed.')
        sys.exit(retcode)

    run_inspector = input('Ready to inspect diff file (no exits)? (Y/n) ')
    if (run_inspector == 'n'):
        print('Stopping.')
        sys.exit(0)
    
    thecmd = 'less /tmp/graph2dil.diff'
    if args.inspector:
        thecmd = f'{args.inspector} /tmp/graph2dil.diff'
    #retcode = try_subprocess_check_output(thecmd)
    retcode = pty_process(thecmd)
    if (retcode != 0):
        print('Inspector failed.')
        sys.exit(retcode)


def compare_Log_original_and_regenerated():
    run_graph2dil = input('Run graph2dil on Log (no skips this step)? (Y/n) ')
    if (run_graph2dil != 'n'):
        thecmd = 'graph2dil -L'
        if args.verbose:
            thecmd += ' -V'
        retcode = pty_process(thecmd)
        #retcode = try_subprocess_check_output(thecmd)
        if (retcode != 0):
            print('graph2dil failed.')
            sys.exit(retcode)
    
    run_diff = input('Ready to run graph2dil-diff.sh (no exits)? (Y/n) ')
    if (run_diff == 'n'):
        print('Stopping.')
        sys.exit(0)
    
    thecmd = f'graph2dil-diff.sh -L {DILfilesdir} {config["DILTLdirectory"]}/lists'
    #retcode = try_subprocess_check_output(thecmd)
    retcode = pty_process(thecmd)
    if (retcode != 0):
        print('graph2dil-diff.sh failed.')
        sys.exit(retcode)

    run_inspector = input('Ready to inspect diff file (no exits)? (Y/n) ')
    if (run_inspector == 'n'):
        print('Stopping.')
        sys.exit(0)
    
    thecmd = 'less /tmp/graph2dil.diff'
    if args.inspector:
        thecmd = f'{args.inspector} /tmp/graph2dil.diff'
    #retcode = try_subprocess_check_output(thecmd)
    retcode = pty_process(thecmd)
    if (retcode != 0):
        print('Inspector failed.')
        sys.exit(retcode)


if __name__ == '__main__':
    get_graph2dil_config()
    global args
    args = parse_options()
    
    retcode = try_subprocess_check_output('graph-resident')
    if (retcode != 0):
        print('Missing memory-resident Graph. Please start \'fzserverpq\' to retry.')
        sys.exit(retcode)

    if args.graph:
        compare_Graph_original_and_regenerated()

    if args.log:
        compare_Log_original_and_regenerated()

    print('Done')
    sys.exit(0)
