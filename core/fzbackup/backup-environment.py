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
import sys, os, json
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

    'external': '/media/randalk/SSKSDDEXT4/randalk-ext4-mountable-20250603', # was: '/media/randalk/ExtremeSSD/randalk-ext4-mountable-20240131',
    'mountpoint': '/media/randalk/SSKSDDEXT4/mount-point', # was: '/media/randalk/ExtremeSSD/mount-point',
    'rsyncdirs': [
        '/home/randalk',
        '/var/www',
        '/usr/lib/cgi-bin',
    ],
    'dryrun': False,
    # Directories whose copy is compared file by file with checksums by the
    # verify step. (Counting files is done for all rsyncdirs.)
    'checksumdirs': [
        '/home/randalk/.formalizer',
        '/home/randalk/src/formalizer',
    ],
}

fzbackup_verify = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'fzbackup-verify.py')

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
  c = Create mountable partition (should not exist already)
  M = Make mount point
  d = Toggle dry-run mode
  v = Verify the copy on the external drive (file counts, checksums, database restore test)
%s

Choice? '''

def show_menu():
    fromdirs=''
    numkey=1
    for dirname in config['rsyncdirs']:
        fromdirs += '  '+str(numkey)+' = Modify '+dirname+'\n'
        numkey += 1
    print(MENU % fromdirs)

def mount_external():
    if os.path.ismount(config['mountpoint']):
        return
    res = os.system('sudo mount -t ext4 -o loop %s %s' % ( config['external'], config['mountpoint'] ))
    if res != 0:
        print('Mounting failed.')
        sys.exit(res)

def ask_unmount():
    do_unmount = input('Unmount? (Y/n) ')
    if do_unmount != 'n':
        os.system('sudo umount %s' % config['mountpoint'])

def copy_of(dirname: str) -> str:
    # rsync /home/randalk <mountpoint>/ puts the copy at <mountpoint>/randalk,
    # so a path below an rsyncdir maps to <mountpoint>/<basename(rsyncdir)>/<rest>.
    for rsyncdir in config['rsyncdirs']:
        if dirname == rsyncdir or dirname.startswith(rsyncdir.rstrip('/') + '/'):
            rest = os.path.relpath(dirname, rsyncdir)
            return os.path.normpath(os.path.join(config['mountpoint'], os.path.basename(rsyncdir.rstrip('/')), rest))
    return ''

def count_files(dirname: str) -> int:
    p = Popen(['find', dirname, '-type', 'f'], stdout=PIPE, stderr=PIPE)
    n = sum(1 for _ in p.stdout)
    p.wait()
    return n

def verify_copy() -> bool:
    """Check the copy on the mounted external drive. Returns True if no problems were found."""
    ok = True
    print('\n=== Verify: file counts ===')
    print('(Small differences are expected for files that changed since the rsync.)')
    for dirname in config['rsyncdirs']:
        copy = copy_of(dirname)
        if not os.path.isdir(copy):
            print(f'FAIL: no copy of {dirname} at {copy}')
            ok = False
            continue
        n_src = count_files(dirname)
        n_copy = count_files(copy)
        note = '' if n_src == n_copy else f'  (differs by {n_src - n_copy:+d})'
        print(f'  {dirname}: {n_src} files, copy: {n_copy} files{note}')

    print('\n=== Verify: checksums ===')
    for dirname in config['checksumdirs']:
        copy = copy_of(dirname)
        if not os.path.isdir(copy):
            print(f'FAIL: no copy of {dirname} at {copy}')
            ok = False
            continue
        # A dry run with --checksum lists every file whose content differs or is missing.
        p = Popen(['rsync', '-a', '--dry-run', '--checksum', '--itemize-changes', '--delete',
                   dirname + '/', copy + '/'], stdout=PIPE, stderr=PIPE, universal_newlines=True)
        out, err = p.communicate()
        changed = [l for l in out.splitlines() if l and not l.startswith('.d')]
        if p.returncode != 0:
            print(f'FAIL: checksum comparison of {dirname} failed: {err.strip()}')
            ok = False
        elif changed:
            print(f'  {dirname}: {len(changed)} file(s) differ from the copy, for example:')
            for line in changed[:10]:
                print('    ' + line)
        else:
            print(f'  {dirname}: all files match by checksum.')

    print('\n=== Verify: Formalizer database backup on the external drive ===')
    with open(os.path.expanduser('~/.formalizer/config/fzbackup.py/config.json')) as f:
        backuproot = json.load(f)['backuproot']
    copydir = copy_of(backuproot.rstrip('/'))
    backups = sorted(f for f in os.listdir(copydir) if f.endswith('.gz')) if os.path.isdir(copydir) else []
    if not backups:
        print(f'FAIL: no database backups found at {copydir}')
        return False
    newest = os.path.join(copydir, backups[-1])
    # The verify script restores this copy into a scratch database and compares it
    # with the live database. Exit code 2 only means the Formalizer changed since.
    res = os.system(f'{fzbackup_verify} "{newest}"')
    if os.waitstatus_to_exitcode(res) == 1:
        ok = False

    print('\nVerify result: ' + ('no problems found.' if ok else 'PROBLEMS FOUND, see above.'))
    return ok

def do_verify():
    mount_external()
    verify_copy()
    ask_unmount()

def do_full_backup():
    mount_external()
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
    print('Finished rsyncing.')
    if not config['dryrun']:
        do_check = input('Verify the copy now? (Y/n) ')
        if do_check != 'n':
            verify_copy()
    ask_unmount()

def create_mountable_partition():
    if not os.path.exists(config['mountpoint']):
        print('Error: The mount point must exist. Create it first.')
        sys.exit(1)
    if os.path.exists(config['external']):
        print('Error: Cannot create mountable partition, because file already exists at %s.' % config['external'])
        sys.exit(1)

    res = os.system('truncate -s 180G %s' % config['external'])
    if res != 0:
        print('Failed to create 180G file at %s.' % config['external'])
        sys.exit(res)
    os.system('ls -a -l %s' % config['external'])
    res = os.system('mkfs.ext4 %s' % config['external'])
    if res != 0:
        print('Failed to create Ext4 filesystem in file at %s.' % config['external'])
        sys.exit(res)
    res = os.system('sudo mount -t ext4 -o loop %s %s' % ( config['external'], config['mountpoint'] ))
    if res != 0:
        print('Mounting failed.')
        sys.exit(res)
    res = os.system('sudo chmod -R 777 %s' % config['mountpoint'])
    if res != 0:
        print('Failed to chmod -R 777 the mount point at %s.' % config['mountpoint'])
        sys.exit(res)
    os.system('ls -a -l %s' % config['mountpoint'])
    os.system('sudo umount %s' % config['mountpoint'])
    print('Ext4 filesystem created at %s.' % config['external'])

def make_mount_point():
    if os.path.exists(config['mountpoint']):
        print('Error: Unable to make mount point at %s, because path already exists.' % config['mountpoint'])
        sys.exit(1)

    res = os.system('mkdir %s' % config['mountpoint'])
    if res != 0:
        print('Failed to create mount point at %s.' % config['mountpoint'])
        sys.exit(res)
    print('Mount point created at %s.' % config['mountpoint'])

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
        elif choice=='v':
            do_verify()
        elif choice=='c':
            create_mountable_partition()
        elif choice=='M':
            make_mount_point()
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
        parser.add_argument("-verify", action='store_true', help="Verify the copy on the external drive")
        args = parser.parse_args()
        if args.full:
            backupmethod = 'full'
        if args.verify:
            do_verify()
            sys.exit(0)

    if backupmethod == 'full':
        if running_as_cgi:
            cgi_full_backup()
        else:
            full_backup()

    print('Unrecognized backup method.')
    sys.exit(0)
