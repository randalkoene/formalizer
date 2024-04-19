#!/usr/bin/python3
#
# Randal A. Koene, 20240419
#
# This script is a first / temporary version of a helper script to carry out
# full environment backup (beyond the regular Formalizer database backup).
#
# The capabilities here may be merged with fzbackup once ready.
#
# This script should be usable from the command line and through CGI.

# Import modules for CGI handling 
import sys, os
sys.stderr = sys.stdout
from time import strftime
from datetime import datetime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

config = {
    'verbose': False,
    'logcmdcalls': False,
    'cmdlog': '/var/www/webdata/formalizer/backup-environment.log',

    'external': '/media/randalk/ExtremeSSD/randalk-ext4-mountable-20240131',
    'mountpoint': '/media/randalk/ExtremeSSD/mount-point',
    'rsyncdirs': [
        '/home/randalk',
        '/var/www',
        '/usr/lib/cgi-bin',
    ],
    'dryrun': False,
}

results = {}

running_as_cgi = False
if 'REQUEST_METHOD' in os.environ :
    running_as_cgi = True
    try:
        import cgitb; cgitb.enable()
    except:
        pass
    print("Content-type:text/html\n\n")
else :
    running_as_cgi = False
    print('This script is running locally.')

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

def cgi_full_backup():
    print('Not yet implemented.')
    sys.exit(0)

FROMDIRS_LINE='''  %s
'''

SETTINGS='''Settings:

External drive mountable partition path:
  %s

Mountable partition mount point:
  %s

Rsync from directories:
%s

Dryrun: %s
'''

def show_settings():
    fromdirs=''
    for dirname in config['rsyncdirs']:
        fromdirs += FROMDIRS_LINE % dirname
    print(SETTINGS % ( config['external'], config['mountpoint'], fromdirs, str(config['dryrun']) ))

MENU='''
Options:

  x = Exit
  b = Proceed with backup
  e = Change external drive mountable partition path
  m = Change mountable partition mount point
  d = Toggle dry-run mode
%s

Choice? '''

def show_menu():
    fromdirs=''
    numkey=1
    for dirname in config['rsyncdirs']:
        fromdirs += '  '+str(numkey)+' = Modify '+dirname+'\n'
        numkey += 1
    print(MENU % fromdirs)

def do_full_backup():
    res = os.system('sudo mount -t ext4 -o loop %s %s' % ( config['external'], config['mountpoint'] ))
    if res != 0:
        print('Mounting failed.')
        sys.exit(res)
    os.system('ls -a -l %s' % config['mountpoint'])
    if config['dryrun']:
        dryrun_arg = '--dry-run '
    else:
        dryrun_arg = ''
    for dirname in config['rsyncdirs']:
        thecmd = 'rsync %s --delete --force -avhP ' % dryrun_arg
        thecmd += dirname
        thecmd += ' %s/' % config['mountpoint']
        os.system(thecmd)
    do_unmount = input('Finished rsyncing. Unmount? (Y/n) ')
    if do_unmount != 'n':
        os.system('sudo umount %s' % config['mountpoint'])

def full_backup():
    while True:
        show_settings()
        show_menu()
        choice = input()[0]
        if choice=='x':
            sys.exit(0)
        elif choice=='e':
            mpath = input('Mountable path: ')
            if mpath != '':
                config['external'] = mpath
        elif choice=='m':
            mpath = input('Mount point: ')
            if mpath != '':
                config['mountpoint'] = mpath
        elif choice=='d':
            config['dryrun'] = not config['dryrun']
        elif choice=='b':
            do_full_backup()
        else:
            if choice >= '1' and choice <= '9':
                i = int(choice) - int('1')
                if (i>=0) and (i<len(config['rsyncdirs'])):
                    rpath = input('Change to: ')
                    if rpath != '':
                        config['rsyncdirs'][i] = rpath

if __name__ == '__main__':
    backupmethod = 'unknown'
    if running_as_cgi:
        import cgi
        form = cgi.FieldStorage()
        backupmethod = form.getvalue("backup")
    else:
        import argparse
        parser = argparse.ArgumentParser(description="Backup the (full) environment")
        parser.add_argument("-full", action='store_true', help="Make a full environment backup")
        args = parser.parse_args()
        if args.full:
            backupmethod = 'full'

    if backupmethod == 'full':
        if running_as_cgi:
            cgi_full_backup()
        else:
            full_backup()

    print('Unrecognized backup method.')
    sys.exit(0)
